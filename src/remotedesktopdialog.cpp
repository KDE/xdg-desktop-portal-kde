/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#include "remotedesktopdialog.h"
#include "remotedesktopdialog_debug.h"
#include "utils.h"

#include <KLocalizedString>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QWindow>

RemoteDesktopDialog::RemoteDesktopDialog(const QString &appName,
                                         RemoteDesktopPortal::DeviceTypes deviceTypes,
                                         bool screenSharingEnabled,
                                         ScreenCastPortal::PersistMode persistMode,
                                         QObject *parent)
    : QuickDialog(parent)
{
    const QVariantMap props = {{QStringLiteral("title"), i18nc("Title of the dialog that requests remote input privileges", "Remote control requested")},
                               {QStringLiteral("description"), buildDescription(appName, deviceTypes, screenSharingEnabled)},
                               {QStringLiteral("persistenceRequested"), persistMode != ScreenCastPortal::PersistMode::NoPersist}};
    create(QStringLiteral("RemoteDesktopDialog"), props);
}

bool RemoteDesktopDialog::allowRestore() const
{
    return m_theDialog->property("allowRestore").toBool();
}

QString RemoteDesktopDialog::buildDescription(const QString &appName, RemoteDesktopPortal::DeviceTypes deviceTypes, bool screenSharingEnabled)
{
    const QString applicationName = Utils::applicationName(appName);
    QString description = applicationName.isEmpty()
        ? i18nc("Begins the logical sentence 'an application requested access to see what's on the screen and/or control input devices'",
                "An application requested access to:\n")
        : i18nc("Begins the logical sentence '[application name] requested access to see what's on the screen and/or control input devices'",
                "%1 requested access to:",
                applicationName);
    if (screenSharingEnabled) {
        description += i18nc("Completes the logical sentence 'an application requested access to see what's on the screen'. '-' is Markdown, do not translate",
                             "\n - See what’s on the screen");
    }
    if (deviceTypes != RemoteDesktopPortal::None) {
        description += i18nc("Completes the logical sentence 'an application requested access to control input devices'. '-' is Markdown, do not translate",
                             "\n - Control input devices");
    }
    return description;
}
