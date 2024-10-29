/*
 * SPDX-FileCopyrightText: 2016 Red Hat Inc
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>
 */

#include "notification.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>

#include <KNotificationReplyAction>

#include "fdo_application_interface.h"
#include "notification_debug.h"
#include "portalicon.h"

using namespace Qt::StringLiterals;

using Action = QPair<QString, QVariant>;
Q_DECLARE_METATYPE(QList<Action>);

namespace
{
constexpr auto KEY_PURPOSE = "purpose"_L1;
constexpr auto CATEGORY_IM_MESSAGE = "im.message"_L1;
constexpr auto PURPOSE_SYSTEM_CUSTOM_ALERT = "system.custom-alert"_L1;
constexpr auto PURPOSE_IM_REPLY_WITH_TEXT = "im.reply-with-text"_L1;

constexpr auto KEY_MARKUP_BODY = "markup-body"_L1;
constexpr auto KEY_DISPLAY_HINT = "display-hint"_L1;
constexpr auto KEY_CATEGORY = "category"_L1;
} // namespace

NotificationPortal::NotificationPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    PortalIcon::registerDBusType();
}

static QString appPathFromId(const QString &app_id)
{
    QString ret = QLatin1Char('/') + app_id;
    ret.replace('.', '/');
    ret.replace('-', '_');
    return ret;
}

void NotificationPortal::fillNotification(KNotification *notify, const QString &app_id, const QString &id, const QVariantMap &notification)
{
    if (notification.contains(QStringLiteral("title"))) {
        notify->setTitle(notification.value(QStringLiteral("title")).toString());
    }
    if (notification.contains(QStringLiteral("body"))) {
        notify->setText(notification.value(QStringLiteral("body")).toString());
    }
    if (notification.contains(KEY_MARKUP_BODY)) {
        notify->setText(notification.value(KEY_MARKUP_BODY).toString());
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
            } else if (icon.str == "file-descriptor"_L1 && iconData.metaType() == QMetaType::fromType<QDBusUnixFileDescriptor>()) {
                QFile file;
                if (!file.open(iconData.value<QDBusUnixFileDescriptor>().fileDescriptor(), QIODevice::ReadOnly)) {
                    qCWarning(XdgDesktopPortalKdeNotification) << "Failed to open icon file descriptor";
                }

                const auto pixmap = QPixmap::fromImage(QImage::fromData(file.readAll()));
                if (pixmap.isNull()) {
                    qCWarning(XdgDesktopPortalKdeNotification) << "Failed to load image from file descriptor";
                } else {
                    notify->setPixmap(pixmap);
                }
            }
        }
    }

    if (notification.contains(u"sound"_s)) {
        qCWarning(XdgDesktopPortalKdeNotification) << "Custom sound is not supported by KNotification";
    }

    static const QMap<QString, KNotification::Urgency> priorityMap{
        {u"low"_s, KNotification::LowUrgency},
        {u"normal"_s, KNotification::NormalUrgency},
        {u"high"_s, KNotification::HighUrgency},
        {u"urgent"_s, KNotification::CriticalUrgency},
    };
    const QString priority = notification.value(QStringLiteral("priority")).toString();
    notify->setUrgency(priorityMap.value(priority, KNotification::NormalUrgency));

    auto actionInvoked = [notify, app_id, id](const QString &actionId, const QVariant &actionTarget, const std::optional<QString> &replyText = std::nullopt) {
        QVariantMap activationPlatformData;
        QVariantMap notificationPlatformData;
        if (!notify->xdgActivationToken().isEmpty()) {
            activationPlatformData = {
                {QStringLiteral("activation-token"), notify->xdgActivationToken()},

                // apparently gtk uses "desktop-startup-id"
                {QStringLiteral("desktop-startup-id"), notify->xdgActivationToken()},
            };

            notificationPlatformData = {
                {u"activation-token"_s, notify->xdgActivationToken()},
            };
        }

        qCDebug(XdgDesktopPortalKdeNotification) << "Notification activated:";
        qCDebug(XdgDesktopPortalKdeNotification) << "    app_id: " << app_id;
        qCDebug(XdgDesktopPortalKdeNotification) << "    id: " << id;
        qCDebug(XdgDesktopPortalKdeNotification) << "    action: " << actionId << actionTarget;

        QVariantList params;
        if (actionTarget.isValid()) {
            params += actionTarget;
        }
        params += notificationPlatformData;
        if (replyText) {
            params += replyText.value();
        }

        OrgFreedesktopApplicationInterface iface(app_id, appPathFromId(app_id), QDBusConnection::sessionBus());
        if (actionId.startsWith("app.") && iface.isValid()) {
            iface.ActivateAction(actionId.mid(4), params, activationPlatformData);
        } else {
            if (iface.isValid()) {
                iface.Activate(activationPlatformData);
            }

            QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/org/freedesktop/portal/desktop"),
                                                              QStringLiteral("org.freedesktop.impl.portal.Notification"),
                                                              QStringLiteral("ActionInvoked"));
            message << app_id << id << actionId << params;
            QDBusConnection::sessionBus().send(message);
        }
    };

    if (notification.contains(QStringLiteral("default-action")) && notification.contains(QStringLiteral("default-action-target"))) {
        KNotificationAction *action = notify->addDefaultAction(notification.value(QStringLiteral("default-action")).toString());

        connect(action, &KNotificationAction::activated, this, [actionInvoked, notification] {
            actionInvoked(notification.value(QStringLiteral("default-action")).toString(), notification.value(QStringLiteral("default-action-target")));
        });
    }

    QList<QVariantMap> customAlertFakeButtons;

    if (notification.contains(QStringLiteral("buttons"))) {
        const auto dbusArgument = notification.value(QStringLiteral("buttons")).value<QDBusArgument>();
        const auto buttons = qdbus_cast<QList<QVariantMap>>(dbusArgument);

        notify->clearActions();
        for (const QVariantMap &button : std::as_const(buttons)) {
            if (button.contains(KEY_PURPOSE)) {
                // WARNING: when implementing more button purposes, make sure to update supportedOptions()
                const auto purpose = button.value(KEY_PURPOSE).toString();
                if (purpose == PURPOSE_IM_REPLY_WITH_TEXT) {
                    auto replyAction = std::make_unique<KNotificationReplyAction>(button.value(QStringLiteral("label")).toString());
                    connect(replyAction.get(), &KNotificationReplyAction::replied, [button, actionInvoked](const QString &text) {
                        actionInvoked(button.value(u"action"_s).toString(), button.value(u"target"_s), text);
                    });
                    notify->setReplyAction(std::move(replyAction));
                } else if (purpose == PURPOSE_SYSTEM_CUSTOM_ALERT) {
                    customAlertFakeButtons << button;
                }
            } else {
                auto action = notify->addAction(button.value(QStringLiteral("label")).toString());
                connect(action, &KNotificationAction::activated, this, [button, actionInvoked] {
                    actionInvoked(button.value(QStringLiteral("action")).toString(), button.value(QStringLiteral("target")));
                });
            }
        }
    }

    if (notification.contains(KEY_DISPLAY_HINT)) {
        const auto displayHint = notification.value(KEY_DISPLAY_HINT).toStringList();
        KNotification::NotificationFlags flags = KNotification::NotificationFlag::DefaultEvent;
        if (displayHint.contains("persistent"_L1)) {
            flags |= KNotification::Persistent;
        }
        if (displayHint.contains("transient"_L1)) {
            notify->setHint(u"transient"_s, true);
        }
        notify->setFlags(flags);
        // show-as-new is supported elsewhere!
        // Not supported:
        //   * ``tray``
        //   * ``hide-on-lockscreen``
        //   * ``hide-content-on-lockscreen``
    }

    if (notification.contains(KEY_CATEGORY)) {
        // Not supported (we don't have any of them as a workspace-level notification. they are all app specific):
        //   * ``alarm.ringing``
        //   * ``call.incoming``
        //   * ``call.ongoing``
        //   * ``call.missed``
        //   * ``weather.warning.extreme``
        //   * ``cellbroadcast.danger.extreme``
        //   * ``cellbroadcast.danger.severe``
        //   * ``cellbroadcast.amber-alert``
        //   * ``cellbroadcast.test``
        //   * ``os.battery.low``
        //   * ``browser.web-notification``
    }

    notify->setHint(QStringLiteral("desktop-entry"), app_id);
    notify->setHint(QStringLiteral("x-kde-xdgTokenAppId"), app_id);

    // TODO: move this to the actual showEvent call
    for (const auto &button : customAlertFakeButtons) {
        actionInvoked(button.value(QStringLiteral("action")).toString(), button.value(QStringLiteral("target")));
    }
}

void NotificationPortal::AddNotification(const QString &app_id, const QString &id, const QVariantMap &notification)
{
    qCDebug(XdgDesktopPortalKdeNotification) << "AddNotification called with parameters:";
    qCDebug(XdgDesktopPortalKdeNotification) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeNotification) << "    id: " << id;
    qCDebug(XdgDesktopPortalKdeNotification) << "    notification: " << notification;

    const QString notificationId = QStringLiteral("%1:%2").arg(app_id, id);
    const bool showAsNew = (notification.value(KEY_DISPLAY_HINT).toString() == "show-as-new"_L1);
    const bool recycleNotification = m_notifications.contains(notificationId) && !showAsNew;

    KNotification *notify = [this, notificationId, recycleNotification] {
        if (auto existingNotify = m_notifications.take(notificationId).get(); existingNotify) {
            if (recycleNotification) {
                return existingNotify;
            }

            disconnect(existingNotify, &KNotification::closed, this, &NotificationPortal::notificationClosed);
            existingNotify->close();
            existingNotify->deleteLater();
        }
        // We have to use "notification" as an ID because any other ID will not be configured.
        // The problem in particular is that IDs rely on knotifyrc files that aren't being installed by flatpak.
        auto notify = new KNotification(QStringLiteral("notification"), KNotification::CloseOnTimeout | KNotification::DefaultEvent, this);
        connect(notify, &KNotification::closed, this, &NotificationPortal::notificationClosed);
        return notify;
    }();

    fillNotification(notify, app_id, id, notification);

    m_notifications.insert(notificationId, notify);
    notify->sendEvent();
}

void NotificationPortal::RemoveNotification(const QString &app_id, const QString &id)
{
    qCDebug(XdgDesktopPortalKdeNotification) << "RemoveNotification called with parameters:";
    qCDebug(XdgDesktopPortalKdeNotification) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeNotification) << "    id: " << id;

    KNotification *notify = m_notifications.take(QStringLiteral("%1:%2").arg(app_id, id));
    if (notify) {
        disconnect(notify, &KNotification::closed, this, &NotificationPortal::notificationClosed);
        notify->close();
    }
}

void NotificationPortal::notificationClosed()
{
    KNotification *notify = qobject_cast<KNotification *>(sender());
    Q_ASSERT(notify);
    const QString appId = notify->property("app_id").toString();
    const QString id = notify->property("id").toString();

    auto n = m_notifications.take(QStringLiteral("%1:%2").arg(appId, id));
    if (n) {
        Q_ASSERT(n == notify);
        n->close();
    }
}

uint NotificationPortal::version() const
{
    return 2;
}

QVariantHash NotificationPortal::supportedOptions() const
{
    return {
        {u"category"_s, QStringList(CATEGORY_IM_MESSAGE)},
        {u"button-purpose"_s, QStringList(PURPOSE_IM_REPLY_WITH_TEXT)},
    };
}
