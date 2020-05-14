/*
 * Copyright © 2020 Red Hat, Inc
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#include "background.h"
#include "utils.h"
#include "waylandintegration.h"

#include <QDBusMetaType>
#include <QDBusContext>
#include <QDBusMessage>
#include <QDBusConnection>

#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QSettings>

#include <KConfigGroup>
#include <KDesktopFile>
#include <KLocalizedString>
#include <KNotification>
#include <KShell>

#include <KWayland/Client/plasmawindowmanagement.h>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeBackground, "xdp-kde-background")

BackgroundPortal::BackgroundPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    connect(WaylandIntegration::waylandIntegration(), &WaylandIntegration::WaylandIntegration::plasmaWindowManagementInitialized, this, [=] () {
        connect(WaylandIntegration::plasmaWindowManagement(), &KWayland::Client::PlasmaWindowManagement::windowCreated, this, [this] (KWayland::Client::PlasmaWindow *window) {
            addWindow(window);
        });

        m_windows = WaylandIntegration::plasmaWindowManagement()->windows();
        for (KWayland::Client::PlasmaWindow *window : m_windows) {
            addWindow(window);
        }
    });
}

BackgroundPortal::~BackgroundPortal()
{
}

QVariantMap BackgroundPortal::GetAppState()
{
    qCDebug(XdgDesktopPortalKdeBackground) << "GetAppState called: no parameters";
    return m_appStates;
}

uint BackgroundPortal::NotifyBackground(const QDBusObjectPath &handle,
                                        const QString &app_id,
                                        const QString &name,
                                        QVariantMap &results)
{
    Q_UNUSED(results);

    qCDebug(XdgDesktopPortalKdeBackground) << "NotifyBackground called with parameters:";
    qCDebug(XdgDesktopPortalKdeBackground) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeBackground) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeBackground) << "    name: " << name;

    KNotification *notify = new KNotification(QStringLiteral("notification"), KNotification::Persistent | KNotification::DefaultEvent, this);
    notify->setTitle(i18n("Background activity"));
    notify->setText(i18n("%1 is running in the background.", app_id));
    notify->setActions({i18n("Find out more")});
    notify->setProperty("activated", false);

    QObject *obj = QObject::parent();

    if (!obj) {
        qCWarning(XdgDesktopPortalKdeBackground) << "Failed to get dbus context";
        return 2;
    }

    void *ptr = obj->qt_metacast("QDBusContext");
    QDBusContext *q_ptr = reinterpret_cast<QDBusContext *>(ptr);

    if (!q_ptr) {
        qCWarning(XdgDesktopPortalKdeBackground) << "Failed to get dbus context";
        return 2;
    }

    QDBusMessage reply;
    QDBusMessage message = q_ptr->message();

    message.setDelayedReply(true);

    connect(notify, QOverload<uint>::of(&KNotification::activated), this, [=] (uint action) {
        if (action != 1) {
            return;
        }
        notify->setProperty("activated", true);

        const QString title = i18n("%1 is running in the background", app_id);
        const QString text = i18n("This might be for a legitimate reason, but the application has not provided one."
                                  "\n\nNote that forcing an application to quit might cause data loss.");
        QMessageBox messageBox(QMessageBox::Question, title, text);
        QPushButton *quitButton = messageBox.addButton(i18n("Force quit"), QMessageBox::RejectRole);
        QPushButton *allowButton = messageBox.addButton(i18n("Allow"), QMessageBox::AcceptRole);
        messageBox.exec();

        BackgroundPortal::NotifyResult result = BackgroundPortal::Ignore;
        if (messageBox.clickedButton() == quitButton) {
            result = BackgroundPortal::Forbid;
        } else if (messageBox.clickedButton() == allowButton) {
            result = BackgroundPortal::Allow;
        }

        const QVariantMap map = { {QStringLiteral("result"), static_cast<uint>(result)} };
        QDBusMessage reply = message.createReply({static_cast<uint>(0), map});
        if (!QDBusConnection::sessionBus().send(reply)) {
            qCWarning(XdgDesktopPortalKdeBackground) << "Failed to send response";
        }
    });
    connect(notify, &KNotification::closed, this, [=] () {
        if (notify->property("activated").toBool()) {
            return;
        }

        QVariantMap map;
        map.insert(QStringLiteral("result"), static_cast<uint>(BackgroundPortal::Ignore));
        QDBusMessage reply = message.createReply({static_cast<uint>(0), map});
        if (!QDBusConnection::sessionBus().send(reply)) {
            qCWarning(XdgDesktopPortalKdeBackground) << "Failed to send response";
        }
    });

    notify->sendEvent();

    return 0;
}

bool BackgroundPortal::EnableAutostart(const QString &app_id,
                                       bool enable,
                                       const QStringList &commandline,
                                       uint flags)
{
    qCDebug(XdgDesktopPortalKdeBackground) << "EnableAutostart called with parameters:";
    qCDebug(XdgDesktopPortalKdeBackground) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeBackground) << "    enable: " << enable;
    qCDebug(XdgDesktopPortalKdeBackground) << "    commandline: " << commandline;
    qCDebug(XdgDesktopPortalKdeBackground) << "    flags: " << flags;

    const QString fileName = app_id + QStringLiteral(".desktop");
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/autostart/");
    const QString fullPath = directory + fileName;
    const AutostartFlags autostartFlags = static_cast<AutostartFlags>(flags);

    if (!enable) {
        QFile file(fullPath);
        if (!file.remove()) {
            qCDebug(XdgDesktopPortalKdeBackground) << "Failed to remove " << fileName << " to disable autostart.";
        }
        return false;
    }

    QDir dir(directory);
    if (!dir.mkpath(dir.absolutePath())) {
        qCDebug(XdgDesktopPortalKdeBackground) << "Failed to create autostart directory.";
        return false;
    }

    KDesktopFile desktopFile(fullPath);
    KConfigGroup desktopEntryConfigGroup = desktopFile.desktopGroup();
    desktopEntryConfigGroup.writeEntry(QStringLiteral("Type"), QStringLiteral("Application"));
    desktopEntryConfigGroup.writeEntry(QStringLiteral("Name"), app_id);
    desktopEntryConfigGroup.writeEntry(QStringLiteral("Exec"), KShell::joinArgs(commandline));
    if (autostartFlags.testFlag(AutostartFlag::Activatable)) {
       desktopEntryConfigGroup.writeEntry(QStringLiteral("DBusActivatable"), true);
    }
    desktopEntryConfigGroup.writeEntry(QStringLiteral("X-Flatpak"), app_id);

    return true;
}

void BackgroundPortal::addWindow(KWayland::Client::PlasmaWindow *window)
{
    const QString appId = window->appId();
    const bool isActive = window->isActive();
    m_appStates[appId] = QVariant::fromValue<uint>(isActive ? Active : Running);

    connect(window, &KWayland::Client::PlasmaWindow::activeChanged, this, [this, window] () {
        setActiveWindow(window->appId(), window->isActive());
    });
    connect(window, &KWayland::Client::PlasmaWindow::unmapped, this, [this, window] () {
        uint windows = 0;
        const QString appId = window->appId();
        for (KWayland::Client::PlasmaWindow *otherWindow : WaylandIntegration::plasmaWindowManagement()->windows()) {
            if (otherWindow->appId() == appId && otherWindow->internalId() != window->internalId()) {
                windows++;
            }
        }

        if (!windows) {
            m_appStates.remove(appId);
            Q_EMIT RunningApplicationsChanged();
        }
    });

    Q_EMIT RunningApplicationsChanged();
}

void BackgroundPortal::setActiveWindow(const QString &appId, bool active)
{
    m_appStates[appId] = QVariant::fromValue<uint>(active ? Active : Running);

    Q_EMIT RunningApplicationsChanged();
}
