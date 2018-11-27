/*
 * Copyright © 2016 Red Hat, Inc
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

#include "notification.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QLoggingCategory>

QDBusArgument &operator<<(QDBusArgument &argument, const NotificationPortal::PortalIcon &icon)
{
    argument.beginStructure();
    argument << icon.str << icon.data;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, NotificationPortal::PortalIcon &icon)
{
    argument.beginStructure();
    argument >> icon.str >> icon.data;
    argument.endStructure();
    return argument;
}

Q_DECLARE_METATYPE(NotificationPortal::PortalIcon)

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeNotification, "xdp-kde-notification")

NotificationPortal::NotificationPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    qDBusRegisterMetaType<PortalIcon>();
}

NotificationPortal::~NotificationPortal()
{
}

void NotificationPortal::AddNotification(const QString &app_id,
                                   const QString &id,
                                   const QVariantMap &notification)
{
    qCDebug(XdgDesktopPortalKdeNotification) << "AddNotification called with parameters:";
    qCDebug(XdgDesktopPortalKdeNotification) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeNotification) << "    id: " << id;
    qCDebug(XdgDesktopPortalKdeNotification) << "    notification: " << notification;

    // We have to use "notification" as an ID because any other ID will not be configured
    KNotification *notify = new KNotification(QLatin1String("notification"), KNotification::CloseOnTimeout | KNotification::DefaultEvent, this);
    if (notification.contains(QLatin1String("title"))) {
        notify->setTitle(notification.value(QLatin1String("title")).toString());
    }
    if (notification.contains(QLatin1String("body"))) {
        notify->setText(notification.value(QLatin1String("body")).toString());
    }
    if (notification.contains(QLatin1String("icon"))) {
        QVariant iconVariant = notification.value(QLatin1String("icon"));
        if (iconVariant.type() == QVariant::String) {
            notify->setIconName(iconVariant.toString());
        } else {
            QDBusArgument argument = iconVariant.value<QDBusArgument>();
            PortalIcon icon = qdbus_cast<PortalIcon>(argument);
            QVariant iconData = icon.data.variant();
            if (icon.str == QStringLiteral("themed") && iconData.type() == QVariant::StringList) {
                notify->setIconName(iconData.toStringList().first());
            } else if (icon.str == QStringLiteral("bytes") && iconData.type() == QVariant::ByteArray) {
                QPixmap pixmap;
                if (pixmap.loadFromData(iconData.toByteArray(), "PNG")) {
                    notify->setPixmap(pixmap);
                }
            }
        }
    }
    if (notification.contains(QLatin1String("priority"))) {
        // TODO KNotification has no option for priority
    }
    if (notification.contains(QLatin1String("default-action"))) {
        // TODO KNotification has no option for default action
    }
    if (notification.contains(QLatin1String("default-action-target"))) {
        // TODO KNotification has no option for default action
    }
    if (notification.contains(QLatin1String("buttons"))) {
        QList<QVariantMap> buttons;
        QDBusArgument dbusArgument = notification.value(QLatin1String("buttons")).value<QDBusArgument>();
        while (!dbusArgument.atEnd()) {
            dbusArgument >> buttons;
        }

        QStringList actions;
        for (const QVariantMap &button : buttons) {
            actions << button.value(QLatin1String("label")).toString();
        }

        if (!actions.isEmpty()) {
            notify->setActions(actions);
        }
    }

    notify->setProperty("app_id", app_id);
    notify->setProperty("id", id);
    connect(notify, static_cast<void (KNotification::*)(uint)>(&KNotification::activated), this, &NotificationPortal::notificationActivated);
    connect(notify, &KNotification::closed, this, &NotificationPortal::notificationClosed);
    notify->sendEvent();

    m_notifications.insert(QStringLiteral("%1:%2").arg(app_id, id), notify);
}

void NotificationPortal::notificationActivated(uint action)
{
    KNotification *notify = qobject_cast<KNotification*>(sender());

    if (!notify) {
        return;
    }

    const QString appId = notify->property("app_id").toString();
    const QString id = notify->property("id").toString();

    qCDebug(XdgDesktopPortalKdeNotification) << "Notification activated:";
    qCDebug(XdgDesktopPortalKdeNotification) << "    app_id: " << appId;
    qCDebug(XdgDesktopPortalKdeNotification) << "    id: " << id;
    qCDebug(XdgDesktopPortalKdeNotification) << "    action: " << action;

    QDBusMessage message = QDBusMessage::createSignal(QLatin1String("/org/freedesktop/portal/desktop"),
                                                      QLatin1String("org.freedesktop.impl.portal.Notification"),
                                                      QLatin1String("ActionInvoked"));
    message << appId << id << QString::number(action) << QVariantList();
    QDBusConnection::sessionBus().send(message);
}

void NotificationPortal::RemoveNotification(const QString &app_id,
                                      const QString &id)
{
    qCDebug(XdgDesktopPortalKdeNotification) << "RemoveNotification called with parameters:";
    qCDebug(XdgDesktopPortalKdeNotification) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeNotification) << "    id: " << id;

    KNotification *notify = m_notifications.take(QStringLiteral("%1:%2").arg(app_id, id));
    if (notify) {
        notify->close();
        notify->deleteLater();
    }
}

void NotificationPortal::notificationClosed()
{
    KNotification *notify = qobject_cast<KNotification*>(sender());

    if (!notify) {
        return;
    }

    const QString appId = notify->property("app_id").toString();
    const QString id = notify->property("id").toString();

    m_notifications.remove(QStringLiteral("%1:%2").arg(appId, id));
    notify->deleteLater();
}
