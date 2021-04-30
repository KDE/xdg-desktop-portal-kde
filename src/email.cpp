/*
 * SPDX-FileCopyrightText: 2017 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#include "email.h"

#include <QDesktopServices>
#include <QLoggingCategory>
#include <QUrl>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeEmail, "xdp-kde-email")

EmailPortal::EmailPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

EmailPortal::~EmailPortal()
{
}

uint EmailPortal::ComposeEmail(const QDBusObjectPath &handle, const QString &app_id, const QString &window, const QVariantMap &options, QVariantMap &results)
{
    Q_UNUSED(results)

    qCDebug(XdgDesktopPortalKdeEmail) << "ComposeEmail called with parameters:";
    qCDebug(XdgDesktopPortalKdeEmail) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeEmail) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeEmail) << "    window: " << window;
    qCDebug(XdgDesktopPortalKdeEmail) << "    options: " << options;

    QString attachmentString;
    const QStringList attachments = options.value(QStringLiteral("attachments")).toStringList();
    for (const QString &attachment : attachments) {
        attachmentString += QStringLiteral("&attachment=%1").arg(attachment);
    }

    const QString mailtoUrl = QStringLiteral("mailto:%1?subject=%2&body=%3%4")
                                  .arg(options.value(QStringLiteral("address")).toString(),
                                       options.value(QStringLiteral("subject")).toString(),
                                       options.value(QStringLiteral("body")).toString(),
                                       attachmentString);
    return QDesktopServices::openUrl(QUrl(mailtoUrl));
}
