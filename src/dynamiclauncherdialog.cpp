// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "dynamiclauncherdialog.h"

#include <QIcon>

#include "dynamiclauncherdialog_debug.h"

DynamicLauncherDialog::DynamicLauncherDialog(const QString &mainText,
                                             const QString &subtitle,
                                             const QIcon &icon,
                                             const QString &name,
                                             const QUrl &launcherURL,
                                             QObject *parent)
    : QuickDialog(parent)
    , m_name(name)
    , m_icon(icon)
{
    create(QStringLiteral("DynamicLauncherDialog"),
           {
               {QStringLiteral("mainText"), mainText},
               {QStringLiteral("subtitle"), subtitle},
               {QStringLiteral("launcherName"), name},
               {QStringLiteral("launcherIcon"), icon},
               {QStringLiteral("launcherURL"), launcherURL},
               {QStringLiteral("dialog"), QVariant::fromValue(this)},
           });
}

#include "moc_dynamiclauncherdialog.cpp"
