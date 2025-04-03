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

class SessionStateAdaptor : public QObject, public QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.SessionState")

public:
    explicit SessionStateAdaptor(InhibitPortal *parent = nullptr);

public Q_SLOTS:
    bool queryCanEndSession()
    {
        m_inhibitPortal->queryCanEndSession(message());
        return true; // reply value will be ignored, handled async above
    }
    void endSession()
    {
        m_inhibitPortal->endSession();
    }

private:
    InhibitPortal *m_inhibitPortal;
};

SessionStateAdaptor::SessionStateAdaptor(InhibitPortal *parent)
    : QObject(parent)
    , m_inhibitPortal(parent)
{
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/kde/SessionState"), this, QDBusConnection::ExportAllSlots);
}

InhibitPortal::InhibitPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    new SessionStateAdaptor(this);

    QDBusConnection::sessionBus().connect(QStringLiteral("org.freedesktop.ScreenSaver"),
                                          QStringLiteral("/ScreenSaver"),
                                          QStringLiteral("org.freedesktop.ScreenSaver"),
                                          QStringLiteral("ActiveChanged"),
                                          this, SLOT(lockScreenStateChanged(bool)));


    auto isLocked = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"),
                                                   QStringLiteral("/ScreenSaver"),
                                                   QStringLiteral("org.freedesktop.ScreenSaver"),
                                                   QStringLiteral("GetActive"));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(isLocked));
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<bool> reply = *watcher;
        watcher->deleteLater();
        if (reply.isError()) {
            qCDebug(XdgDesktopPortalKdeInhibit) << "Error getting initial screen lock state: " << reply.error().message();
            return;
        }
        if (reply.value()) {
            lockScreenStateChanged(true);
        } // else we're already off, we can do nothing
    });

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

    if (flags & 1) { // Block logout
        m_appsBlockingLogout[handle] = app_id;
        connect(request, &Request::closeRequested, this, [this, handle]() {
            m_appsBlockingLogout.remove(handle);
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
        watcher->deleteLater();
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
                    watcher->deleteLater();
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
    if (m_monitors.contains(session_handle)) {
        qCDebug(XdgDesktopPortalKdeInhibit) << "CreateMonitor called for existing session:" << session_handle;
        return -1;
    }
    Q_UNUSED(window);
    Q_UNUSED(handle);

    auto session = new SessionStateMonitorSession(this, app_id, session_handle.path());

    auto sessionState = Running;

    // XDP doesn't forward the initial state until we've completed creating the monitor
    QTimer::singleShot(0, session, [session, this, sessionState] {
        updateState(session, m_lockState, sessionState);
    });

    connect(session, &Session::closed, this, [this, session_handle]() {
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

void InhibitPortal::lockScreenStateChanged(bool state)
{
    m_lockState = state ? LockScreenActive : LockScreenInactive;
    for (auto session : std::as_const(m_monitors)) {
        updateState(session, m_lockState, Running);
        session->setWaitingForResponse(true);
    }
}

void InhibitPortal::queryCanEndSession(const QDBusMessage &message)
{
    if (m_pendingCanShutDownReply.type() != QDBusMessage::InvalidMessage) {
        qWarning() << "querying shutdown before previous call completed, this may skip clients";
    }

    m_pendingCanShutDownReply = message;
    message.setDelayedReply(true);

    auto sessionState = QueryEnd;
    for (auto session : std::as_const(m_monitors)) {
        updateState(session, m_lockState, sessionState);
        session->setWaitingForResponse(true);
    }

    // if we don't have any monitors, we don't need to wait for a reply
    int timeout = m_monitors.isEmpty() ? 0 : 1500;
    QTimer::singleShot(timeout, this, &InhibitPortal::queryCanShutDownComplete);
}

void InhibitPortal::endSession()
{
    auto sessionState = Ending;
    for (auto session : std::as_const(m_monitors)) {
        updateState(session, m_lockState, sessionState);
    }
}

void InhibitPortal::queryCanShutDownComplete()
{
    if (m_pendingCanShutDownReply.type() != QDBusMessage::MethodCallMessage) {
        return;
    }

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
