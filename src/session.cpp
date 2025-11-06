/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#include "session.h"
#include "desktopportal.h"
#include "session_debug.h"
#include "sessionadaptor.h"
#include "xdgshortcut.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QLoggingCategory>
#include <screencast_debug.h>

#include <KLocalizedString>
#include <KService>
#include <KStatusNotifierItem>

#include "remotedesktopdialog.h"
#include "utils.h"
#include <QMenu>

static QMap<QString, Session *> sessionList;

using namespace Qt::StringLiterals;

Session::Session(QObject *parent, const QString &appId, const QString &path)
    : QObject(parent)
    , m_appId(appId)
    , m_path(path)
{
    new SessionAdaptor(this);
    if (QDBusConnection::sessionBus().registerObject(path, this)) {
        m_valid = true;
        sessionList.insert(path, this);
    }
}

Session::~Session()
{
}

bool Session::close()
{
    QDBusMessage reply = QDBusMessage::createSignal(m_path, QStringLiteral("org.freedesktop.impl.portal.Session"), QStringLiteral("Closed"));
    const bool result = QDBusConnection::sessionBus().send(reply);

    Q_EMIT closed();

    sessionList.remove(m_path);
    QDBusConnection::sessionBus().unregisterObject(m_path);

    deleteLater();
    m_valid = false;

    return result;
}

Session *Session::getSession(const QString &sessionHandle)
{
    return sessionList.value(sessionHandle);
}

void Session::Close()
{
    close();
}


ScreenCastSession::ScreenCastSession(QObject *parent, const QString &appId, const QString &path, const QString &iconName)
    : Session(parent, appId, path)
    , m_item(new KStatusNotifierItem(this))
{
    m_item->setStandardActionsEnabled(false);
    m_item->setIconByName(iconName);

    auto menu = new QMenu;
    auto stopAction = menu->addAction(QIcon::fromTheme(QStringLiteral("process-stop")), i18nc("@action:inmenu stops screen/window sharing", "End"));
    connect(stopAction, &QAction::triggered, this, &Session::close);
    m_item->setContextMenu(menu);
    m_item->setIsMenu(true);
}

ScreenCastSession::~ScreenCastSession()
{
}

bool ScreenCastSession::multipleSources() const
{
    return m_multipleSources;
}

ScreenCastPortal::SourceType ScreenCastSession::types() const
{
    return m_types;
}

void ScreenCastSession::setPersistMode(ScreenCastPortal::PersistMode persistMode)
{
    m_persistMode = persistMode;
}

ScreenCastPortal::CursorModes ScreenCastSession::cursorMode() const
{
    return m_cursorMode;
}

void ScreenCastSession::setOptions(const QVariantMap &options)
{
    m_multipleSources = options.value(QStringLiteral("multiple")).toBool();
    m_cursorMode = ScreenCastPortal::CursorModes(options.value(QStringLiteral("cursor_mode")).toUInt());
    m_types = ScreenCastPortal::SourceType(options.value(QStringLiteral("types")).toUInt());

    if (m_types == 0) {
        m_types = ScreenCastPortal::Monitor;
    }
}

RemoteDesktopSession::RemoteDesktopSession(QObject *parent, const QString &appId, const QString &path)
    : ScreenCastSession(parent, appId, path, QStringLiteral("krfb"))
    , m_screenSharingEnabled(false)
    , m_clipboardEnabled(false)
{
    connect(this, &RemoteDesktopSession::closed, this, [this] {
        if (m_acquired) {
            WaylandIntegration::acquireStreamingInput(false);
        }
    });
}

RemoteDesktopSession::~RemoteDesktopSession()
{
}

void RemoteDesktopSession::setOptions(const QVariantMap &options)
{
}

RemoteDesktopPortal::DeviceTypes RemoteDesktopSession::deviceTypes() const
{
    return m_deviceTypes;
}

void RemoteDesktopSession::setDeviceTypes(RemoteDesktopPortal::DeviceTypes deviceTypes)
{
    m_deviceTypes = deviceTypes;
}

bool RemoteDesktopSession::screenSharingEnabled() const
{
    return m_screenSharingEnabled;
}

void RemoteDesktopSession::setScreenSharingEnabled(bool enabled)
{
    if (m_screenSharingEnabled == enabled) {
        return;
    }

    m_screenSharingEnabled = enabled;
}

bool RemoteDesktopSession::clipboardEnabled() const
{
    return m_clipboardEnabled;
}

void RemoteDesktopSession::setClipboardEnabled(bool enabled)
{
    m_clipboardEnabled = enabled;
}

void RemoteDesktopSession::setEisCookie(int cookie)
{
    m_cookie = cookie;
}

int RemoteDesktopSession::eisCookie() const
{
    return m_cookie;
}

void RemoteDesktopSession::acquireStreamingInput()
{
    WaylandIntegration::acquireStreamingInput(true);
    m_acquired = true;
}

void RemoteDesktopSession::refreshDescription()
{
    m_item->setTitle(i18nc("SNI title that indicates there's a process remotely controlling the system", "Remote Control"));
    m_item->setToolTipTitle(m_item->title());
    setDescription(RemoteDesktopDialog::buildNotificationDescription(m_appId, deviceTypes(), screenSharingEnabled()));
}

void ScreenCastSession::setStreams(const WaylandIntegration::Streams &streams)
{
    Q_ASSERT(!streams.isEmpty());
    m_streams = streams;

    m_item->setStandardActionsEnabled(false);
    if (qobject_cast<RemoteDesktopSession *>(this)) {
        refreshDescription();
    } else {
        const bool isWindow = m_streams[0].map[QLatin1String("source_type")] == ScreenCastPortal::Window;
        m_item->setToolTipSubTitle(i18ncp("%1 number of screens, %2 the app that receives them",
                                          "Sharing contents to %2",
                                          "%1 video streams to %2",
                                          m_streams.count(),
                                          Utils::applicationName(m_appId)));
        m_item->setTitle(i18nc("SNI title that indicates there's a process seeing our windows or screens", "Screen casting"));
        if (isWindow) {
            m_item->setOverlayIconByName(QStringLiteral("window"));
        } else {
            m_item->setOverlayIconByName(QStringLiteral("monitor"));
        }
    }
    m_item->setToolTipIconByName(m_item->overlayIconName());
    m_item->setToolTipTitle(m_item->title());

    for (const auto &s : streams) {
        connect(s.stream, &ScreencastingStream::closed, this, &ScreenCastSession::streamClosed);
        connect(s.stream, &ScreencastingStream::failed, this, [this](const QString &error) {
            qCWarning(XdgDesktopPortalKdeScreenCast) << "ScreenCast session failed" << error;
            streamClosed();
        });
    }
    m_item->setStatus(KStatusNotifierItem::Active);
}

void ScreenCastSession::setDescription(const QString &description)
{
    m_item->setToolTipSubTitle(description);
}

void ScreenCastSession::streamClosed()
{
    ScreencastingStream *stream = qobject_cast<ScreencastingStream *>(sender());
    auto it = std::remove_if(m_streams.begin(), m_streams.end(), [stream](const WaylandIntegration::Stream &s) {
        return s.stream == stream;
    });
    m_streams.erase(it, m_streams.end());

    if (m_streams.isEmpty()) {
        close();
    }
}

static QString kwinService()
{
    return u"org.kde.KWin"_s;
}

constexpr int kwinDBusTimeout = 3000;

static QString kwinInputCaptureInterface()
{
    return u"org.kde.KWin.EIS.InputCapture"_s;
}

InputCaptureSession::InputCaptureSession(QObject *parent, const QString &appId, const QString &path)
    : Session(parent, appId, path)
    , state(InputCapturePortal::State::Disabled)
{
}

InputCaptureSession::~InputCaptureSession() = default;

void InputCaptureSession::addBarrier(const QPair<QPoint, QPoint> &barrier)
{
    m_barriers.push_back(barrier);
}

void InputCaptureSession::clearBarriers()
{
    m_barriers.clear();
}

QDBusObjectPath InputCaptureSession::kwinInputCapture() const
{
    return m_kwinInputCapture;
}

void InputCaptureSession::connect(const QDBusObjectPath &path)
{
    m_kwinInputCapture = path;
    auto connectSignal = [this](const QString &signalName, const char *slot) {
        QDBusConnection::sessionBus().connect(kwinService(), m_kwinInputCapture.path(), kwinInputCaptureInterface(), signalName, this, slot);
    };
    connectSignal(u"disabled"_s, SIGNAL(disabled()));
    connectSignal(u"activated"_s, SIGNAL(activated(uint, QPointF)));
    connectSignal(u"deactivated"_s, SIGNAL(deactivated(uint)));
}

QDBusPendingReply<void> InputCaptureSession::enable()
{
    auto msg = QDBusMessage::createMethodCall(kwinService(), m_kwinInputCapture.path(), kwinInputCaptureInterface(), u"enable"_s);
    msg << QVariant::fromValue(m_barriers);
    return QDBusConnection::sessionBus().asyncCall(msg, kwinDBusTimeout);
}

QDBusPendingReply<void> InputCaptureSession::disable()
{
    auto msg = QDBusMessage::createMethodCall(kwinService(), m_kwinInputCapture.path(), kwinInputCaptureInterface(), u"disable"_s);
    return QDBusConnection::sessionBus().asyncCall(msg, kwinDBusTimeout);
}

QDBusPendingReply<void> InputCaptureSession::release(const QPointF &cusorPosition, bool applyPosition)
{
    auto msg = QDBusMessage::createMethodCall(kwinService(), m_kwinInputCapture.path(), kwinInputCaptureInterface(), u"release"_s);
    msg << cusorPosition << applyPosition;
    return QDBusConnection::sessionBus().asyncCall(msg, kwinDBusTimeout);
}

QDBusPendingReply<QDBusUnixFileDescriptor> InputCaptureSession::connectToEIS()
{
    auto msg = QDBusMessage::createMethodCall(kwinService(), m_kwinInputCapture.path(), kwinInputCaptureInterface(), u"connectToEIS"_s);
    return QDBusConnection::sessionBus().asyncCall(msg, kwinDBusTimeout);
}

