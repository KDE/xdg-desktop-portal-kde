// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "dynamiclauncherdialog.h"

#include <QIcon>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeDynamicLauncherDialog, "xdp-kde-dynamic-launcher-dialog")

DynamicLauncherDialog::DynamicLauncherDialog(const QString &title, const QIcon &icon, const QString &name, const QUrl &launcherURL, QObject *parent)
    : QuickDialog(parent)
    , m_name(name)
    , m_icon(icon)
{
    create(QStringLiteral("qrc:/DynamicLauncherDialog.qml"),
           {
               {QStringLiteral("title"), title},
               {QStringLiteral("launcherName"), name},
               {QStringLiteral("launcherIcon"), icon},
               {QStringLiteral("launcherURL"), launcherURL},
               {QStringLiteral("dialog"), QVariant::fromValue(this)},
           });
}
