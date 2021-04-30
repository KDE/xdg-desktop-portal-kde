/*
 * SPDX-FileCopyrightText: 2016 Red Hat Inc
 * SPDX-License-Identifier: LGPL-2.0-or-later
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

void NotificationPortal::AddNotification(const QString &app_id, const QString &id, const QVariantMap &notification)
{
    qCDebug(XdgDesktopPortalKdeNotification) << "AddNotification called with parameters:";
    qCDebug(XdgDesktopPortalKdeNotification) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeNotification) << "    id: " << id;
    qCDebug(XdgDesktopPortalKdeNotification) << "    notification: " << notification;

    // We have to use "notification" as an ID because any other ID will not be configured
    KNotification *notify = new KNotification(QStringLiteral("notification"), KNotification::CloseOnTimeout | KNotification::DefaultEvent, this);
    if (notification.contains(QStringLiteral("title"))) {
        notify->setTitle(notification.value(QStringLiteral("title")).toString());
    }
    if (notification.contains(QStringLiteral("body"))) {
        notify->setText(notification.value(QStringLiteral("body")).toString());
    }
    if (notification.contains(QStringLiteral("icon"))) {
        QVariant iconVariant = notification.value(QStringLiteral("icon"));
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

    const QString priority = notification.value(QStringLiteral("priority")).toString();
    if (priority == QLatin1String("low")) {
        notify->setUrgency(KNotification::LowUrgency);
    } else if (priority == QLatin1String("normal")) {
        notify->setUrgency(KNotification::NormalUrgency);
    } else if (priority == QLatin1String("high")) {
        notify->setUrgency(KNotification::HighUrgency);
    } else if (priority == QLatin1String("urgent")) {
        notify->setUrgency(KNotification::CriticalUrgency);
    }

    if (notification.contains(QStringLiteral("default-action")) && notification.contains(QStringLiteral("default-action-target"))) {
        // default action is conveniently mapped to action number 0 so it uses the same action invocation method as the others
        notify->setDefaultAction(notification.value(QStringLiteral("default-action")).toString());
    }

    if (notification.contains(QStringLiteral("buttons"))) {
        QList<QVariantMap> buttons;
        QDBusArgument dbusArgument = notification.value(QStringLiteral("buttons")).value<QDBusArgument>();
        while (!dbusArgument.atEnd()) {
            dbusArgument >> buttons;
        }

        QStringList actions;
        for (const QVariantMap &button : qAsConst(buttons)) {
            actions << button.value(QStringLiteral("label")).toString();
        }

        if (!actions.isEmpty()) {
            notify->setActions(actions);
        }
    }

    notify->setHint(QStringLiteral("desktop-entry"), app_id);

    notify->setProperty("app_id", app_id);
    notify->setProperty("id", id);
    connect(notify, static_cast<void (KNotification::*)(uint)>(&KNotification::activated), this, &NotificationPortal::notificationActivated);
    connect(notify, &KNotification::closed, this, &NotificationPortal::notificationClosed);
    notify->sendEvent();

    m_notifications.insert(QStringLiteral("%1:%2").arg(app_id, id), notify);
}

void NotificationPortal::notificationActivated(uint action)
{
    KNotification *notify = qobject_cast<KNotification *>(sender());

    if (!notify) {
        return;
    }

    const QString appId = notify->property("app_id").toString();
    const QString id = notify->property("id").toString();

    qCDebug(XdgDesktopPortalKdeNotification) << "Notification activated:";
    qCDebug(XdgDesktopPortalKdeNotification) << "    app_id: " << appId;
    qCDebug(XdgDesktopPortalKdeNotification) << "    id: " << id;
    qCDebug(XdgDesktopPortalKdeNotification) << "    action: " << action;

    QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/org/freedesktop/portal/desktop"),
                                                      QStringLiteral("org.freedesktop.impl.portal.Notification"),
                                                      QStringLiteral("ActionInvoked"));
    message << appId << id << QString::number(action) << QVariantList();
    QDBusConnection::sessionBus().send(message);
}

void NotificationPortal::RemoveNotification(const QString &app_id, const QString &id)
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
    KNotification *notify = qobject_cast<KNotification *>(sender());

    if (!notify) {
        return;
    }

    const QString appId = notify->property("app_id").toString();
    const QString id = notify->property("id").toString();

    m_notifications.remove(QStringLiteral("%1:%2").arg(appId, id));
    notify->deleteLater();
}
