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

class InhibitionRequest : public Request
{
public:
    InhibitionRequest(const QDBusObjectPath &handle, uint inhibitionId, QObject *parent = nullptr)
        : Request(handle, parent, QStringLiteral("org.freedesktop.impl.portal.Inhibit"))
        , m_inhibitionId(inhibitionId)
    {
    }
    void handleClose(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        QDBusMessage uninhibitMessage = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                       QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent"),
                                                                       QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
                                                                       QStringLiteral("ReleaseInhibition"));

        uninhibitMessage << m_inhibitionId;

        QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(uninhibitMessage);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [message, &connection](QDBusPendingCallWatcher *watcher) {
            watcher->deleteLater();
            QDBusPendingReply<> reply = *watcher;
            if (reply.isError()) {
                qCDebug(XdgDesktopPortalKdeInhibit) << "Uninhibit error: " << reply.error().message();
                connection.send(message.createErrorReply(reply.error()));
            }
            connection.send(message.createReply());
        });
    }

private:
    const uint m_inhibitionId;
};

InhibitPortal::InhibitPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

void InhibitPortal::Inhibit(const QDBusObjectPath &handle, const QString &app_id, const QString &window, uint flags, const QVariantMap &options)
{
    qCDebug(XdgDesktopPortalKdeInhibit) << "Inhibit called with parameters:";
    qCDebug(XdgDesktopPortalKdeInhibit) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeInhibit) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeInhibit) << "    window: " << window;
    qCDebug(XdgDesktopPortalKdeInhibit) << "    flags: " << flags;
    qCDebug(XdgDesktopPortalKdeInhibit) << "    options: " << options;

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
    connect(watcher, &QDBusPendingCallWatcher::finished, [handle, this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<uint> reply = *watcher;
        if (reply.isError()) {
            qCDebug(XdgDesktopPortalKdeInhibit) << "Inhibition error: " << reply.error().message();
            return;
        }
        auto inhibitId = reply.value();
        [[maybe_unused]] auto request = new InhibitionRequest(handle, inhibitId, this);
    });
}
