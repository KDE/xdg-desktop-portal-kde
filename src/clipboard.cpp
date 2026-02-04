/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2021 Méven Car <meven.car@enioka.com>
    SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
*/

#include "clipboard.h"

#include "clipboard_debug.h"
#include "session.h"
#include "remotedesktop.h"

#include <KSystemClipboard>

#include <QDBusConnection>
#include <QWaylandClientExtensionTemplate>

#include <qwayland-ext-data-control-v1.h>

#include <fcntl.h>
#include <unistd.h>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

class DataControlManager : public QWaylandClientExtensionTemplate<DataControlManager>, public QtWayland::ext_data_control_manager_v1
{
    Q_OBJECT
public:
    DataControlManager()
        : QWaylandClientExtensionTemplate<DataControlManager>(1)
    {
        initialize();
    }

    ~DataControlManager() override
    {
        destroy();
    }

    void instantiate()
    {
        initialize();
    }
};

class DataControlOffer : public QtWayland::ext_data_control_offer_v1
{
public:
    DataControlOffer(::ext_data_control_offer_v1 *id)
        : QtWayland::ext_data_control_offer_v1(id)
    {
    }

    ~DataControlOffer() override
    {
        destroy();
    }

    QStringList formats() const
    {
        return m_receivedFormats;
    }

    static DataControlOffer *get(::ext_data_control_offer_v1 *object)
    {
        auto derivated = QtWayland::ext_data_control_offer_v1::fromObject(object);
        return dynamic_cast<DataControlOffer *>(derivated); // dynamic because of the dual inheritance
    }

protected:
    void ext_data_control_offer_v1_offer(const QString &mime_type) override
    {
        if (!m_receivedFormats.contains(mime_type)) {
            m_receivedFormats << mime_type;
        }
    }

private:
    QStringList m_receivedFormats;
};

class DataControlSource : public QObject, public QtWayland::ext_data_control_source_v1
{
    Q_OBJECT
public:
    DataControlSource(::ext_data_control_source_v1 *id, Session *session, const QStringList &mimeTypes)
        : ext_data_control_source_v1(id)
        , owner(session)
    {
        for (const QString &format : mimeTypes) {
            offer(format);
        }
    }
    ~DataControlSource() override
    {
        destroy();
    }

    const Session *owner;

Q_SIGNALS:
    void cancelled();
    void dataRequested(const DataControlSource &source, const QString &mime_type, int fd);

protected:
    void ext_data_control_source_v1_send(const QString &mime_type, int32_t fd) override
    {
        Q_EMIT dataRequested(*this, mime_type, fd);
    }
    void ext_data_control_source_v1_cancelled() override
    {
        Q_EMIT cancelled();
    }
};

class DataControlDevice : public QObject, public QtWayland::ext_data_control_device_v1
{
    Q_OBJECT
public:
    DataControlDevice(struct ::ext_data_control_device_v1 *id)
        : QtWayland::ext_data_control_device_v1(id)
    {
    }

    ~DataControlDevice()
    {
        destroy();
    }

    void setSource(std::unique_ptr<DataControlSource> source)
    {
        set_selection(source ? source->object() : nullptr);
        // Note the previous selection is destroyed after the set_selection request.
        m_source = std::move(source);
        if (m_source) {
            connect(m_source.get(), &DataControlSource::cancelled, this, [this]() {
                m_source.reset();
            });
        }
    }

    DataControlOffer *currentOffer()
    {
        return m_currentOffer.get();
    }
    DataControlSource *source()
    {
        return m_source.get();
    }

Q_SIGNALS:
    void offerChanged();

protected:
    void ext_data_control_device_v1_data_offer(::ext_data_control_offer_v1 *id) override
    {
        // this will become memory managed when we retrieve the selection event
        // a compositor calling data_offer without doing that would be a bug
        new DataControlOffer(id);
    }

    void ext_data_control_device_v1_selection(::ext_data_control_offer_v1 *id) override
    {
        if (!id) {
            m_source.reset();
        } else {
            m_currentOffer.reset(DataControlOffer::get(id));
        }
        Q_EMIT offerChanged();
    }

    void ext_data_control_device_v1_primary_selection(::ext_data_control_offer_v1 *id) override
    {
        if (!id) {
            return;
        }
        delete DataControlOffer::get(id);
    }

private:
    std::unique_ptr<DataControlSource> m_source;
    std::unique_ptr<DataControlOffer> m_currentOffer;
};

ClipboardPortal::ClipboardPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
    , m_dataControlManager(std::make_unique<DataControlManager>())
{
    auto waylandApp = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
    if (!waylandApp) {
        qCWarning(XdgDesktopPortalKdeClipboard) << "Couldn't resolve QWaylandApplication - clipboard will not work";
        return;
    }
    if (!m_dataControlManager->isActive()) {
        qCWarning(XdgDesktopPortalKdeClipboard) << "Couldn't bind data control - clipboard will not work";
        return;
    }
    auto seat = waylandApp->seat();
    if (!seat) {
        return;
    }
    m_dataControlDevice = std::make_unique<DataControlDevice>(m_dataControlManager->get_data_device(seat));
}

void ClipboardPortal::RequestClipboard(const QDBusObjectPath &session_handle, const QVariantMap &options)
{
    qCDebug(XdgDesktopPortalKdeClipboard) << "RequestClipboard called with parameters:";
    qCDebug(XdgDesktopPortalKdeClipboard) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeClipboard) << "    options: " << options;

    auto session = Session::getSession(session_handle.path());
    auto remoteDesktopSession = qobject_cast<RemoteDesktopSession *>(session);
    if (!remoteDesktopSession) {
        qCWarning(XdgDesktopPortalKdeClipboard) << "Tried enabling clipboard on non remote desktop session" << session_handle.path() << "type"
                                                << session->type();
        return;
    }

    if (!m_dataControlDevice) {
        qCWarning(XdgDesktopPortalKdeClipboard) << "Data control is not active - not enabling clipboard";
        return;
    }

    remoteDesktopSession->setClipboardEnabled(true);

    connect(session, &Session::closed, m_dataControlDevice.get(), [session, this] {
        if (m_dataControlDevice->source() && m_dataControlDevice->source()->owner == session) {
            qCDebug(XdgDesktopPortalKdeClipboard) << "Session closed while owning the clipboard, unsetting clipboard";
            m_dataControlDevice->setSource(nullptr);
        }
    });

    connect(m_dataControlDevice.get(), &DataControlDevice::offerChanged, this, [session, this] {
        Q_EMIT SelectionOwnerChanged(
            QDBusObjectPath(session->handle()),
            {
                {u"mime_types"_s, m_dataControlDevice->currentOffer() ? m_dataControlDevice->currentOffer()->formats() : QStringList()},
                {u"session_is_owner"_s, m_dataControlDevice->source() && m_dataControlDevice->source()->owner == session},
            });
    });
}

QDBusUnixFileDescriptor ClipboardPortal::SelectionRead(const QDBusObjectPath &session_handle, const QString &mime_type, const QDBusMessage &message)
{
    qCDebug(XdgDesktopPortalKdeClipboard) << "SelectionRead called with parameters:";
    qCDebug(XdgDesktopPortalKdeClipboard) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeClipboard) << "    mime_type: " << mime_type;

    auto remoteDesktopSession = Session::getSession<RemoteDesktopSession>(session_handle.path());
    if (!remoteDesktopSession || !remoteDesktopSession->clipboardEnabled()) {
        auto error = message.createErrorReply(QDBusError::InvalidArgs, u"Not a clipboard enabled session"_s);
        qCWarning(XdgDesktopPortalKdeClipboard) << error.errorMessage();
        QDBusConnection::sessionBus().send(error);
        return QDBusUnixFileDescriptor();
    }

    if (!m_dataControlDevice->currentOffer()->formats().contains(mime_type)) {
        auto error = message.createErrorReply(QDBusError::InvalidArgs, u"requesting not offered format"_s);
        qCWarning(XdgDesktopPortalKdeClipboard) << error.errorMessage();
        return QDBusUnixFileDescriptor();
    }

    int pipeFds[2];
    if (pipe2(pipeFds, O_CLOEXEC) != 0) {
        auto error = message.createErrorReply(QDBusError::Failed, u"Failed to create pipe"_s);
        qCWarning(XdgDesktopPortalKdeClipboard) << error.errorMessage();
        QDBusConnection::sessionBus().send(error);
        return QDBusUnixFileDescriptor();
    }
    auto _ = qScopeGuard([pipeFds] {
        close(pipeFds[0]);
        close(pipeFds[1]);
    });

    auto dbusFd = QDBusUnixFileDescriptor(pipeFds[0]);
    m_dataControlDevice->currentOffer()->receive(mime_type, pipeFds[1]);
    return dbusFd;
}

void ClipboardPortal::SetSelection(const QDBusObjectPath &session_handle, const QVariantMap &options)
{
    qCDebug(XdgDesktopPortalKdeClipboard) << "SetSelection called with parameters:";
    qCDebug(XdgDesktopPortalKdeClipboard) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeClipboard) << "    options: " << options;

    auto remoteDesktopSession = Session::getSession<RemoteDesktopSession>(session_handle.path());
    if (!remoteDesktopSession || !remoteDesktopSession->clipboardEnabled()) {
        qCWarning(XdgDesktopPortalKdeClipboard) << "Not a clipboard enabled session" << session_handle.path();
        return;
    }

    auto mimeTypes = options.value(u"mime_types"_s).toStringList();
    if (mimeTypes.empty()) {
        m_dataControlDevice->setSource(nullptr);
        return;
    }

    auto dataSource = std::make_unique<DataControlSource>(m_dataControlManager->create_data_source(), remoteDesktopSession, mimeTypes);
    connect(dataSource.get(), &DataControlSource::dataRequested, this, &ClipboardPortal::dataRequested);
    m_dataControlDevice->setSource(std::move(dataSource));
}

void ClipboardPortal::dataRequested(const DataControlSource &source, const QString &mimeType, int fd)
{
    static uint transferSerialCounter = 0;
    const uint transferSerial = transferSerialCounter++;
    m_pendingTransfers.push_back({.serial = transferSerial, .fd = fd});
    Q_EMIT SelectionTransfer(QDBusObjectPath(source.owner->handle()), mimeType, transferSerial);
}

QDBusUnixFileDescriptor ClipboardPortal::SelectionWrite(const QDBusObjectPath &session_handle, uint serial, const QDBusMessage &message)
{
    qCDebug(XdgDesktopPortalKdeClipboard) << "SelectionWrite called with parameters:";
    qCDebug(XdgDesktopPortalKdeClipboard) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeClipboard) << "    uint: " << serial;

    auto remoteDesktopSession = Session::getSession<RemoteDesktopSession>(session_handle.path());
    if (!remoteDesktopSession || !remoteDesktopSession->clipboardEnabled()) {
        auto error = message.createErrorReply(QDBusError::InvalidArgs, u"Not a clipboard enabled session"_s);
        qCWarning(XdgDesktopPortalKdeClipboard) << error.errorMessage();
        QDBusConnection::sessionBus().send(error);
        return QDBusUnixFileDescriptor();
    }

    auto transfer = std::ranges::find(m_pendingTransfers, serial, &Transfer::serial);
    if (transfer == m_pendingTransfers.end()) {
        auto error = message.createErrorReply(QDBusError::InvalidArgs, u"No pending transfer with this serial"_s);
        qCWarning(XdgDesktopPortalKdeClipboard) << error.errorMessage() << serial;
        QDBusConnection::sessionBus().send(error);
        return QDBusUnixFileDescriptor();
    }

    return QDBusUnixFileDescriptor(transfer->fd);
}

void ClipboardPortal::SelectionWriteDone(const QDBusObjectPath &session_handle, uint serial, bool success, const QDBusMessage &message)
{
    qCDebug(XdgDesktopPortalKdeClipboard) << "SelectionWriteDone called with parameters:";
    qCDebug(XdgDesktopPortalKdeClipboard) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeClipboard) << "    uint: " << serial;
    qCDebug(XdgDesktopPortalKdeClipboard) << "    success: " << success;

    auto remoteDesktopSession = Session::getSession<RemoteDesktopSession>(session_handle.path());
    if (!remoteDesktopSession || !remoteDesktopSession->clipboardEnabled()) {
        auto error = message.createErrorReply(QDBusError::InvalidArgs, u"Not a clipboard enabled session"_s);
        qCWarning(XdgDesktopPortalKdeClipboard) << error.errorMessage() << session_handle.path();
        QDBusConnection::sessionBus().send(error);
        return;
    }

    auto transfer = std::ranges::find(m_pendingTransfers, serial, &Transfer::serial);
    if (transfer == m_pendingTransfers.end()) {
        auto error = message.createErrorReply(QDBusError::InvalidArgs, u"No pending transfer with this serial"_s);
        qCWarning(XdgDesktopPortalKdeClipboard) << error.errorMessage() << serial;
        QDBusConnection::sessionBus().send(error);
        return;
    }
    close(transfer->fd);
    m_pendingTransfers.erase(transfer);
}

#include "clipboard.moc"
#include "moc_clipboard.cpp"
