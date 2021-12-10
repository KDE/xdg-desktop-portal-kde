/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#include "session.h"
#include "desktopportal.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(XdgSessionKdeSession, "xdp-kde-session")

static QMap<QString, Session *> sessionList;

Session::Session(QObject *parent, const QString &appId, const QString &path)
    : QDBusVirtualObject(parent)
    , m_appId(appId)
    , m_path(path)
{
}

Session::~Session()
{
}

bool Session::handleMessage(const QDBusMessage &message, const QDBusConnection &connection)
{
    Q_UNUSED(connection);

    if (message.path() != m_path) {
        return false;
    }

    /* Check to make sure we're getting properties on our interface */
    if (message.type() != QDBusMessage::MessageType::MethodCallMessage) {
        return false;
    }

    qCDebug(XdgSessionKdeSession) << message.interface();
    qCDebug(XdgSessionKdeSession) << message.member();
    qCDebug(XdgSessionKdeSession) << message.path();

    if (message.interface() == QLatin1String("org.freedesktop.impl.portal.Session")) {
        if (message.member() == QLatin1String("Close")) {
            close();
            Q_EMIT closed();
            QDBusMessage reply = message.createReply();
            return connection.send(reply);
        }
    } else if (message.interface() == QLatin1String("org.freedesktop.DBus.Properties")) {
        if (message.member() == QLatin1String("Get")) {
            if (message.arguments().count() == 2) {
                const QString interface = message.arguments().at(0).toString();
                const QString property = message.arguments().at(1).toString();

                if (interface == QLatin1String("org.freedesktop.impl.portal.Session") && property == QLatin1String("version")) {
                    QList<QVariant> arguments;
                    arguments << 1;

                    QDBusMessage reply = message.createReply();
                    reply.setArguments(arguments);
                    return connection.send(reply);
                }
            }
        }
    }

    return false;
}

QString Session::introspect(const QString &path) const
{
    QString nodes;

    if (path.startsWith(QLatin1String("/org/freedesktop/portal/desktop/session/"))) {
        nodes = QStringLiteral(
            "<interface name=\"org.freedesktop.impl.portal.Session\">"
            "    <method name=\"Close\">"
            "    </method>"
            "<signal name=\"Closed\">"
            "</signal>"
            "<property name=\"version\" type=\"u\" access=\"read\"/>"
            "</interface>");
    }

    qCDebug(XdgSessionKdeSession) << nodes;

    return nodes;
}

bool Session::close()
{
    QDBusMessage reply = QDBusMessage::createSignal(m_path, QStringLiteral("org.freedesktop.impl.portal.Session"), QStringLiteral("Closed"));
    const bool result = QDBusConnection::sessionBus().send(reply);

    sessionList.remove(m_path);
    QDBusConnection::sessionBus().unregisterObject(m_path);

    deleteLater();

    return result;
}

Session *Session::createSession(QObject *parent, SessionType type, const QString &appId, const QString &path)
{
    QDBusConnection sessionBus = QDBusConnection::sessionBus();

    Session *session = nullptr;
    if (type == ScreenCast) {
        session = new ScreenCastSession(parent, appId, path);
    } else {
        session = new RemoteDesktopSession(parent, appId, path);
    }

    if (sessionBus.registerVirtualObject(path, session, QDBusConnection::VirtualObjectRegisterOption::SubPath)) {
        connect(session, &Session::closed, [session, path]() {
            sessionList.remove(path);
            QDBusConnection::sessionBus().unregisterObject(path);
            session->deleteLater();
        });
        sessionList.insert(path, session);
        return session;
    } else {
        qCDebug(XdgSessionKdeSession) << sessionBus.lastError().message();
        qCDebug(XdgSessionKdeSession) << "Failed to register session object: " << path;
        session->deleteLater();
        return nullptr;
    }
}

Session *Session::getSession(const QString &sessionHandle)
{
    return sessionList.value(sessionHandle);
}

ScreenCastSession::ScreenCastSession(QObject *parent, const QString &appId, const QString &path)
    : Session(parent, appId, path)
{
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

ScreenCastPortal::CursorModes ScreenCastSession::cursorMode() const
{
    return m_cursorMode;
}

void ScreenCastSession::setOptions(const QVariantMap &options)
{
    m_multipleSources = options.value(QStringLiteral("multiple")).toBool();
    m_cursorMode = ScreenCastPortal::CursorModes(options.value(QStringLiteral("cursor_mode")).toUInt());
    m_types = ScreenCastPortal::SourceType(options.value(QStringLiteral("types")).toUInt());
}

RemoteDesktopSession::RemoteDesktopSession(QObject *parent, const QString &appId, const QString &path)
    : ScreenCastSession(parent, appId, path)
    , m_screenSharingEnabled(false)
{
}

RemoteDesktopSession::~RemoteDesktopSession()
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
    m_screenSharingEnabled = enabled;
}
