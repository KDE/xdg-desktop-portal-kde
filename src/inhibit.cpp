/*
 * SPDX-FileCopyrightText: 2017 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 */

#include "inhibit.h"
#include "inhibit_debug.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QTimer>

class SessionStateMonitorSession : public Session
{
    Q_OBJECT
public:
    SessionStateMonitorSession(QObject *parent = nullptr, const QString &appId = QString(), const QString &path = QString());
    ~SessionStateMonitorSession() = default;
    SessionType type() const override
    {
        return SessionStateMonitor;
    }
    void setWaitingForResponse(bool waiting)
    {
        m_waitingForResponse = waiting;
    }
    bool waitingForResponse() const
    {
        return m_waitingForResponse;
    }

private:
    bool m_waitingForResponse = false;
};

InhibitPortal::InhibitPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    // TODO when lockscreen state changes, update state
}

void InhibitPortal::Inhibit(const QDBusObjectPath &handle, const QString &app_id, const QString &window, uint flags, const QVariantMap &options)
{
    qCDebug(XdgDesktopPortalKdeInhibit) << "Inhibit called with parameters:";
    qCDebug(XdgDesktopPortalKdeInhibit) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeInhibit) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeInhibit) << "    window: " << window;
    qCDebug(XdgDesktopPortalKdeInhibit) << "    flags: " << flags;
    qCDebug(XdgDesktopPortalKdeInhibit) << "    options: " << options;

    auto request = new Request(handle, this, QStringLiteral("org.freedesktop.impl.portal.Inhibit"));

    if (flags & 1) {
        qDebug() << "DAVE; making blocking logout";

        m_appsBlockingLogout[handle] = app_id;
        connect(request, &Request::closeRequested, this, [this, handle]() {
            m_appsBlockingLogout.remove(handle);

            qDebug() << "no longer blocking" << m_appsBlockingLogout;
        });
    }

    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                          QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent"),
                                                          QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
                                                          QStringLiteral("AddInhibition"));
    uint policies = 0;
    if (flags & 4) { // Suspend
        policies |= 1; // InterruptSession a.k.a. logind "sleep"
    }
    if (flags & 8) { // Idle
        policies |= 4; // ChangeScreenSettings a.k.a. logind "idle"
    }
    if (policies == 0) {
        qCDebug(XdgDesktopPortalKdeInhibit) << "Inhibition error: flags not supported by KDE policy agent:" << flags;
        return;
    }
    message << policies << app_id << options.value(QStringLiteral("reason")).toString();

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [handle, this, request = QPointer<Request>(request)](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<uint> reply = *watcher;
        if (reply.isError()) {
            qCDebug(XdgDesktopPortalKdeInhibit) << "Inhibition error: " << reply.error().message();
        } else {
            QDBusConnection sessionBus = QDBusConnection::sessionBus();
            auto inhibitId = reply.value();

            if (!request) {
                QDBusMessage uninhibitMessage = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                               QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent"),
                                                                               QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
                                                                               QStringLiteral("ReleaseInhibition"));

                uninhibitMessage << inhibitId;
                return;
            }

            // Once the request is closed by the application, release our inhibitor
            connect(request, &Request::closeRequested, this, [this, inhibitId] {
                QDBusMessage uninhibitMessage = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                               QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent"),
                                                                               QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
                                                                               QStringLiteral("ReleaseInhibition"));

                uninhibitMessage << inhibitId;

                QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(uninhibitMessage);
                QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
                connect(watcher, &QDBusPendingCallWatcher::finished, this, [uninhibitMessage](QDBusPendingCallWatcher *watcher) {
                    QDBusPendingReply<> reply = *watcher;
                    QDBusMessage messageReply;
                    if (reply.isError()) {
                        qCDebug(XdgDesktopPortalKdeInhibit) << "Uninhibit error: " << reply.error().message();
                        messageReply = uninhibitMessage.createErrorReply(reply.error());

                        QDBusConnection::sessionBus().asyncCall(messageReply);
                    }
                });
            });
        }
    });
}

uint InhibitPortal::CreateMonitor(const QDBusObjectPath &handle, const QDBusObjectPath &session_handle, const QString &app_id, const QString &window)
{
    qDebug() << "DAVE; making monitor";

    if (m_monitors.contains(session_handle)) {
        qCDebug(XdgDesktopPortalKdeInhibit) << "CreateMonitor called for existing session:" << session_handle;
        return -1;
    }
    Q_UNUSED(window);
    // WTF is the request handle for??

    auto session = new SessionStateMonitorSession(this, app_id, session_handle.path());

    auto lockState = LockScreenInactive;
    auto sessionState = Running;
    // XDP doesn't forward unless, we've completed creating the monitor
    QTimer::singleShot(0, session, [session, this, lockState, sessionState] {
        updateState(session, lockState, sessionState);
    });

    connect(session, &Session::closed, this, [this, session_handle]() {
        qDebug() << "dave: monitor gone";
        m_monitors.remove(session_handle);
    });
    m_monitors.insert(session_handle, session);

    return 0;
}

void InhibitPortal::QueryEndResponse(const QDBusObjectPath &handle)
{
    auto session = m_monitors.value(handle);
    if (!session) {
        return;
    }
    session->setWaitingForResponse(false);
    bool anySessionWaiting = std::any_of(m_monitors.constBegin(), m_monitors.constEnd(), [](SessionStateMonitorSession *session) {
        return session->waitingForResponse();
    });
    if (!anySessionWaiting) {
        queryCanShutDownComplete();
    }
}

void InhibitPortal::queryCanShutDown(const QDBusMessage &message, bool &reply)
{
    Q_UNUSED(reply);
    qDebug() << "DAVE; query can logout";

    if (m_pendingCanShutDownReply.type() != QDBusMessage::InvalidMessage) {
        qWarning() << "querying shutdown before previous call completed, this may skip clients";
    }

    m_pendingCanShutDownReply = message;
    message.setDelayedReply(true);

    auto lockState = LockScreenInactive;
    auto sessionState = QueryEnd;
    for (auto session : std::as_const(m_monitors)) {
        updateState(session, lockState, sessionState);
        session->setWaitingForResponse(true);
    }

    // if we don't have any monitors, we don't need to wait for a reply
    int timeout = m_monitors.isEmpty() ? 0 : 1500;
    QTimer::singleShot(timeout, this, &InhibitPortal::queryCanShutDownComplete);
}

void InhibitPortal::endSession()
{
    auto lockState = LockScreenInactive;
    auto sessionState = Ending;
    for (auto session : std::as_const(m_monitors)) {
        updateState(session, lockState, sessionState);
    }
}

void InhibitPortal::queryCanShutDownComplete()
{
    if (m_pendingCanShutDownReply.type() != QDBusMessage::MethodCallMessage) {
        return;
    }

    qDebug() << "DAVE: done! can logout?" << m_appsBlockingLogout;

    QDBusMessage message = m_pendingCanShutDownReply.createReply();
    message << m_appsBlockingLogout.isEmpty();
    QDBusConnection::sessionBus().send(message);
    m_pendingCanShutDownReply = QDBusMessage();
}

SessionStateMonitorSession::SessionStateMonitorSession(QObject *parent, const QString &appId, const QString &path)
    : Session(parent, appId, path)
{
}

void InhibitPortal::updateState(SessionStateMonitorSession *session, LockScreenState lockState, SessionState sessionState)
{
    QVariantMap data;
    data[QStringLiteral("screensaver-active")] = (lockState == LockScreenActive);
    data[QStringLiteral("session-state")] = static_cast<uint>(sessionState);

    Q_EMIT StateChanged(QDBusObjectPath(session->handle()), data);
}

#include "inhibit.moc"
