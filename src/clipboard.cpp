/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
*/

#include "clipboard.h"

#include "clipboard_debug.h"
#include "session.h"

#include <KSystemClipboard>

#include <QDBusConnection>
#include <QMimeData>
#include <QSocketNotifier>
#include <QTimer>

#include <fcntl.h>
#include <unistd.h>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

class PortalMimeData : public QMimeData
{
public:
    PortalMimeData(const QStringList &formats, ClipboardPortal *portal, Session *session)
        : m_formats(formats)
        , m_portal(portal)
        , m_session(session)
    {
    }
    QStringList formats() const override
    {
        return m_formats;
    }

    QVariant retrieveData(const QString &mimetype, QMetaType preferredType) const override
    {
        Q_UNUSED(preferredType);
        if (!m_formats.contains(mimetype)) {
            return QVariant();
        }
        return m_portal->fetchData(m_session, mimetype);
    }
    QStringList m_formats;
    ClipboardPortal *m_portal;
    Session *m_session;
};

ClipboardPortal::ClipboardPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
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
    remoteDesktopSession->setClipboardEnabled(true);

    connect(session, &Session::closed, KSystemClipboard::instance(), [session] {
        auto clipboard = KSystemClipboard::instance()->mimeData(QClipboard::Clipboard);
        if (auto portalSource = dynamic_cast<const PortalMimeData *>(clipboard); portalSource && portalSource->m_session == session) {
            qCDebug(XdgDesktopPortalKdeClipboard) << "Session closed while owning the clipboard, unsetting clipboard";
            KSystemClipboard::instance()->clear(QClipboard::Clipboard);
        }
    });

    connect(KSystemClipboard::instance(), &KSystemClipboard::changed, session, [session, this] {
        auto clipboard = KSystemClipboard::instance()->mimeData(QClipboard::Clipboard);
        auto portalSource = dynamic_cast<const PortalMimeData *>(clipboard);
        Q_EMIT SelectionOwnerChanged(QDBusObjectPath(session->handle()),
                                     {
                                         {u"mime_types"_s, clipboard ? clipboard->formats() : QStringList()},
                                         {u"session_is_owner"_s, portalSource && portalSource->m_session == session},
                                     });
    });
}

QDBusUnixFileDescriptor ClipboardPortal::SelectionRead(const QDBusObjectPath &session_handle, const QString &mime_type, const QDBusMessage &message)
{
    qCDebug(XdgDesktopPortalKdeClipboard) << "SelectionRead called with parameters:";
    qCDebug(XdgDesktopPortalKdeClipboard) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeClipboard) << "    mime_type: " << mime_type;

    auto remoteDesktopSession = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));
    if (!remoteDesktopSession || !remoteDesktopSession->clipboardEnabled()) {
        auto error = message.createErrorReply(QDBusError::InvalidArgs, u"Not a clipboard enabled session"_s);
        qCWarning(XdgDesktopPortalKdeClipboard) << error.errorMessage();
        QDBusConnection::sessionBus().send(error);
        return QDBusUnixFileDescriptor();
    }

    int pipeFds[2];
    if (pipe2(pipeFds, O_CLOEXEC) != 0) {
        auto error = message.createErrorReply(QDBusError::Failed, u"Failed to create pipe"_s);
        qCWarning(XdgDesktopPortalKdeClipboard) << error.errorMessage();
        QDBusConnection::sessionBus().send(error);
        return QDBusUnixFileDescriptor();
    }

    auto dbusFd = QDBusUnixFileDescriptor(pipeFds[0]);
    const auto mimeData = KSystemClipboard::instance()->mimeData(QClipboard::Clipboard);
    auto data = mimeData ? mimeData->data(mime_type) : QByteArray();

    auto notifier = new QSocketNotifier(pipeFds[1], QSocketNotifier::Write);
    connect(notifier, &QSocketNotifier::activated, this, [data, fd = pipeFds[1], notifier]() mutable {
        ssize_t bytesWritten = write(fd, data.data(), data.size());
        if (bytesWritten <= 0 || bytesWritten == data.size()) {
            close(fd);
            notifier->setEnabled(false);
            notifier->deleteLater();
        } else {
            data = data.sliced(bytesWritten);
        }
    });

    close(pipeFds[0]);
    return dbusFd;
}

void ClipboardPortal::SetSelection(const QDBusObjectPath &session_handle, const QVariantMap &options)
{
    qCDebug(XdgDesktopPortalKdeClipboard) << "SetSelection called with parameters:";
    qCDebug(XdgDesktopPortalKdeClipboard) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeClipboard) << "    options: " << options;

    auto remoteDesktopSession = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));
    if (!remoteDesktopSession || !remoteDesktopSession->clipboardEnabled()) {
        qCWarning(XdgDesktopPortalKdeClipboard) << "Not a clipboard enabled session" << session_handle.path();
        return;
    }

    auto mimeTypes = options.value(u"mime_types"_s).toStringList();
    if (mimeTypes.empty()) {
        KSystemClipboard::instance()->clear(QClipboard::Clipboard);
        return;
    }
    KSystemClipboard::instance()->setMimeData(new PortalMimeData(mimeTypes, this, remoteDesktopSession), QClipboard::Clipboard);
}

QVariant ClipboardPortal::fetchData(Session *session, const QString &mimetype)
{
    static uint transferSerialCounter = 0;
    const uint transferSerial = transferSerialCounter++;
    Q_EMIT SelectionTransfer(QDBusObjectPath(session->handle()), mimetype, transferSerial);

    QEventLoop loop;
    m_pendingTransfers.emplace(transferSerial, Transfer{.loop = loop, .fd = -1, .data = {}});

    connect(session, &Session::closed, &loop, [&loop] {
        qCInfo(XdgDesktopPortalKdeClipboard) << "Session closed while transfer was in progress";
        loop.exit(1);
    });
    QTimer::singleShot(1s, &loop, [&loop] {
        qCWarning(XdgDesktopPortalKdeClipboard) << "Timeout waiting for data";
        KSystemClipboard::instance()->clear(QClipboard::Clipboard);
        loop.exit(1);
    });

    int error = loop.exec();

    auto completedTransfer = m_pendingTransfers.extract(transferSerial);
    int fd = completedTransfer.mapped().fd;
    auto closeFd = qScopeGuard([fd] {
        if (fd != -1) {
            close(fd);
        }
    });

    if (error || fd == -1) {
        return QVariant();
    }
    // Read outstanding data
    auto &data = completedTransfer.mapped().data;
    char buffer[4096];
    while (true) {
        auto bytesRead = read(fd, buffer, sizeof(buffer));
        if (bytesRead < 0) {
            qCWarning(XdgDesktopPortalKdeClipboard) << "Error reading" << strerror(errno);
            return QVariant();
            break;
        } else if (bytesRead == 0) {
            break;
        } else {
            data.append(buffer, bytesRead);
        }
    }
    return data;
}

QDBusUnixFileDescriptor ClipboardPortal::SelectionWrite(const QDBusObjectPath &session_handle, uint serial, const QDBusMessage &message)
{
    qCDebug(XdgDesktopPortalKdeClipboard) << "SelectionWrite called with parameters:";
    qCDebug(XdgDesktopPortalKdeClipboard) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeClipboard) << "    uint: " << serial;

    auto remoteDesktopSession = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));
    if (!remoteDesktopSession || !remoteDesktopSession->clipboardEnabled()) {
        auto error = message.createErrorReply(QDBusError::InvalidArgs, u"Not a clipboard enabled session"_s);
        qCWarning(XdgDesktopPortalKdeClipboard) << error.errorMessage();
        QDBusConnection::sessionBus().send(error);
        return QDBusUnixFileDescriptor();
    }

    auto transfer = m_pendingTransfers.find(serial);
    if (transfer == m_pendingTransfers.end()) {
        auto error = message.createErrorReply(QDBusError::InvalidArgs, u"No pending transfer with this serial"_s);
        qCWarning(XdgDesktopPortalKdeClipboard) << error.errorMessage() << serial;
        QDBusConnection::sessionBus().send(error);
        return QDBusUnixFileDescriptor();
    }

    int pipeFds[2];
    if (pipe2(pipeFds, O_CLOEXEC | O_NONBLOCK) != 0) {
        auto error = message.createErrorReply(QDBusError::Failed, u"Failed to create pipe"_s);
        qCWarning(XdgDesktopPortalKdeClipboard) << error.errorMessage();
        QDBusConnection::sessionBus().send(error);
        transfer->second.loop.exit(1);
        return QDBusUnixFileDescriptor();
    }

    QDBusUnixFileDescriptor dbusFd(pipeFds[1]);
    close(pipeFds[1]);

    transfer->second.fd = pipeFds[0];

    connect(new QSocketNotifier(pipeFds[0], QSocketNotifier::Read, &transfer->second.loop), &QSocketNotifier::activated, [transfer] {
        char buffer[4096];
        while (true) {
            auto bytesRead = read(transfer->second.fd, buffer, sizeof(buffer));
            if (bytesRead < 0 && errno != EAGAIN) {
                qCWarning(XdgDesktopPortalKdeClipboard) << "Error reading" << strerror(errno);
                transfer->second.data.clear();
                transfer->second.loop.exit(1);
                break;
            } else if (bytesRead == 0) {
                break;
            } else {
                transfer->second.data.append(buffer, bytesRead);
            }
        }
    });

    return dbusFd;
}

void ClipboardPortal::SelectionWriteDone(const QDBusObjectPath &session_handle, uint serial, bool success, const QDBusMessage &message)
{
    qCDebug(XdgDesktopPortalKdeClipboard) << "SelectionWriteDone called with parameters:";
    qCDebug(XdgDesktopPortalKdeClipboard) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeClipboard) << "    uint: " << serial;
    qCDebug(XdgDesktopPortalKdeClipboard) << "    success: " << success;

    auto remoteDesktopSession = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));
    if (!remoteDesktopSession || !remoteDesktopSession->clipboardEnabled()) {
        auto error = message.createErrorReply(QDBusError::InvalidArgs, u"Not a clipboard enabled session"_s);
        qCWarning(XdgDesktopPortalKdeClipboard) << error.errorMessage() << session_handle.path();
        QDBusConnection::sessionBus().send(error);
        return;
    }

    auto transfer = m_pendingTransfers.find(serial);
    if (transfer == m_pendingTransfers.end()) {
        auto error = message.createErrorReply(QDBusError::InvalidArgs, u"No pending transfer with this serial"_s);
        qCWarning(XdgDesktopPortalKdeClipboard) << error.errorMessage() << serial;
        QDBusConnection::sessionBus().send(error);
        return;
    }

    transfer->second.loop.exit(success ? 0 : 1);
}

#include "moc_clipboard.cpp"
