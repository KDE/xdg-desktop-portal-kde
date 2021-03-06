/*
 * SPDX-FileCopyrightText: 2017 Red Hat Inc
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 */

#include "access.h"
#include "accessdialog.h"
#include "utils.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeAccess, "xdp-kde-access")

AccessPortal::AccessPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

AccessPortal::~AccessPortal()
{
}

uint AccessPortal::AccessDialog(const QDBusObjectPath &handle,
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

    auto accessDialog = new ::AccessDialog();
    Utils::setParentWindow(accessDialog, parent_window);
    accessDialog->setBody(body);
    accessDialog->setTitle(title);
    accessDialog->setSubtitle(subtitle);

    if (options.contains(QStringLiteral("modal"))) {
        accessDialog->setModal(options.value(QStringLiteral("modal")).toBool());
    }

    if (options.contains(QStringLiteral("deny_label"))) {
        accessDialog->setRejectLabel(options.value(QStringLiteral("deny_label")).toString());
    }

    if (options.contains(QStringLiteral("grant_label"))) {
        accessDialog->setAcceptLabel(options.value(QStringLiteral("grant_label")).toString());
    }

    if (options.contains(QStringLiteral("icon"))) {
        accessDialog->setIcon(options.value(QStringLiteral("icon")).toString());
    }

    // TODO choices

    if (accessDialog->exec()) {
        accessDialog->deleteLater();
        return 0;
    }
    accessDialog->deleteLater();

    return 1;
}
