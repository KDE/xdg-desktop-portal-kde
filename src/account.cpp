/*
 * SPDX-FileCopyrightText: 2020 Red Hat Inc
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2020 Jan Grulich <jgrulich@redhat.com>
 */

#include "account.h"
#include "account_debug.h"
#include "userinfodialog.h"
#include "utils.h"

#include <QDBusConnection>

AccountPortal::AccountPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

void AccountPortal::GetUserInformation(const QDBusObjectPath &handle,
                                       const QString &app_id,
                                       const QString &parent_window,
                                       const QVariantMap &options,
                                       const QDBusMessage &message,
                                       [[maybe_unused]] uint &response,
                                       [[maybe_unused]] QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdeAccount) << "GetUserInformation called with parameters:";
    qCDebug(XdgDesktopPortalKdeAccount) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeAccount) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeAccount) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeAccount) << "    options: " << options;

    QString reason;

    if (options.contains(QStringLiteral("reason"))) {
        reason = options.value(QStringLiteral("reason")).toString();
    }

    UserInfoDialog *userInfoDialog = new UserInfoDialog(reason);
    Utils::setParentWindow(userInfoDialog->windowHandle(), parent_window);
    message.setDelayedReply(true);

    connect(userInfoDialog, &QuickDialog::finished, this, [message, userInfoDialog](QuickDialog::Result result) {
        QVariantMap results;
        if (result == QuickDialog::Result::Accepted) {
            results.insert(QStringLiteral("id"), userInfoDialog->id());
            results.insert(QStringLiteral("name"), userInfoDialog->name());
            const QString image = userInfoDialog->image();
            results.insert(QStringLiteral("image"), image.isEmpty() ? QStringLiteral("file://") : image);
        }
        const auto reply = message.createReply({qToUnderlying(result), results});
        QDBusConnection::sessionBus().send(reply);
    });
}
