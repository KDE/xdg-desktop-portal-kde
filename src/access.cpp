/*
 * Copyright © 2017 Red Hat, Inc
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

#include "access.h"
#include "accessdialog.h"

#include <QLoggingCategory>

#include <KLocalizedString>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeAccess, "xdg-desktop-portal-kde-access")

Access::Access(QObject *parent)
    : QObject(parent)
{
}

Access::~Access()
{
}

uint Access::accessDialog(const QDBusObjectPath &handle,
                          const QString &app_id,
                          const QString &parent_window,
                          const QString &title,
                          const QString &subtitle,
                          const QString &body,
                          const QVariantMap &options,
                          QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdeAccess) << "AccessDialog called with parameters:";
    qCDebug(XdgDesktopPortalKdeAccess) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeAccess) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeAccess) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeAccess) << "    title: " << title;
    qCDebug(XdgDesktopPortalKdeAccess) << "    subtitle: " << subtitle;
    qCDebug(XdgDesktopPortalKdeAccess) << "    body: " << body;
    qCDebug(XdgDesktopPortalKdeAccess) << "    options: " << options;

    AccessDialog *accessDialog = new AccessDialog();
    accessDialog->setBody(body);
    accessDialog->setTitle(title);
    accessDialog->setSubtitle(subtitle);

    if (options.contains(QLatin1String("modal"))) {
        accessDialog->setModal(options.value(QLatin1String("modal")).toBool());
    }

    if (options.contains(QLatin1String("deny_label"))) {
        accessDialog->setRejectLabel(options.value(QLatin1String("deny_label")).toString());
    }

    if (options.contains(QLatin1String("grant_label"))) {
        accessDialog->setAcceptLabel(options.value(QLatin1String("grant_label")).toString());
    }

    if (options.contains(QLatin1String("icon"))) {
        accessDialog->setIcon(options.value(QLatin1String("icon")).toString());
    }

    // TODO choices

    if (accessDialog->exec()) {
        accessDialog->deleteLater();
        return 0;
    }
    accessDialog->deleteLater();

    return 1;
}
