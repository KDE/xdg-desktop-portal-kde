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

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeNotification, "xdg-desktop-portal-kde-notification")


Notification::Notification(QObject *parent)
    : QObject(parent)
{
}

Notification::~Notification()
{
}

void Notification::AddNotification(const QString &app_id,
                                   const QString &id,
                                   const QVariantMap &notification)
{
    qCDebug(XdgDesktopPortalKdeNotification) << "AddNotification called with parameters:";
    qCDebug(XdgDesktopPortalKdeNotification) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeNotification) << "    id: " << id;
    qCDebug(XdgDesktopPortalKdeNotification) << "    notification: " << notification;

    KNotification *notify = new KNotification(id, KNotification::CloseOnTimeout | KNotification::DefaultEvent, this);
    if (notification.contains(QLatin1String("title"))) {
        notify->setTitle(notification.value(QLatin1String("title")).toString());
    }
    if (notification.contains(QLatin1String("body"))) {
        notify->setText(notification.value(QLatin1String("body")).toString());
    }
    if (notification.contains(QLatin1String("icon"))) {
        // TODO: needs check whether it works or not
        notify->setIconName(notification.value(QLatin1String("icon")).toString());
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
        // TODO There is an option to set notification actions, however there is
        // no way how to get the response back so probably no point implementing
        // it right now
    }

    notify->setProperty("id", QString(app_id + QLatin1Char(':') + id));
    connect(notify, &KNotification::closed, this, &Notification::notificationClosed);
    notify->sendEvent();
}

void Notification::RemoveNotification(const QString &app_id,
                                      const QString &id)
{
    KNotification *notify = m_notifications.take(QString(app_id + QLatin1Char(':') + id));
    notify->close();
    notify->deleteLater();
}

void Notification::notificationClosed()
{
    KNotification *notify = qobject_cast<KNotification*>(sender());
    m_notifications.remove(notify->property("id").toString());
    notify->deleteLater();
}
