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

#include "account.h"
#include "userinfodialog.h"
#include "utils.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeAccount, "xdp-kde-account")

AccountPortal::AccountPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

AccountPortal::~AccountPortal()
{
}

uint AccountPortal::GetUserInformation(const QDBusObjectPath &handle,
                                       const QString &app_id,
                                       const QString &parent_window,
                                       const QVariantMap &options,
                                       QVariantMap &results)
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
    Utils::setParentWindow(userInfoDialog, parent_window);

    int result = userInfoDialog->exec();

    if (result) {
        results.insert(QStringLiteral("id"), userInfoDialog->id());
        results.insert(QStringLiteral("name"), userInfoDialog->name());
        results.insert(QStringLiteral("image"), userInfoDialog->image());
    }

    userInfoDialog->deleteLater();

    return !result;
}
