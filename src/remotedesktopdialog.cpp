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
    const QVariantMap props = {{QStringLiteral("title"), i18nc("Title of the dialog that requests remote input privileges", "Remote Control Requested")},
                               {QStringLiteral("mainText"), buildMainText(appName)},
                               {QStringLiteral("subtitle"), buildSubText(appName)},
                               {QStringLiteral("description"), buildRequestDescription(appName, deviceTypes, screenSharingEnabled)},
                               {QStringLiteral("persistenceRequested"), persistMode != ScreenCastPortal::PersistMode::NoPersist}};
    create(QStringLiteral("RemoteDesktopDialog"), props);
}

bool RemoteDesktopDialog::allowRestore() const
{
    return m_theDialog->property("allowRestore").toBool();
}

QString RemoteDesktopDialog::buildMainText(const QString &appName)
{
    const QString applicationName = Utils::applicationName(appName);
    return applicationName.isEmpty() ? i18nc("@info", "Allow an unidentifiable application to gain remote control privileges?")
                                     : i18nc("@info", "Allow %1 to gain remote control privileges?", applicationName);
}

QString RemoteDesktopDialog::buildSubText(const QString &appName)
{
    const QString applicationName = Utils::applicationName(appName);
    return applicationName.isEmpty() ? i18nc("@info", "Only allow if you know which application made the request.") : QString();
}

QString RemoteDesktopDialog::buildRequestDescription(const QString &appName, RemoteDesktopPortal::DeviceTypes deviceTypes, bool screenSharingEnabled)
{
    const QString applicationName = Utils::applicationName(appName);
    QString description =
        applicationName.isEmpty() ? i18nc("@info", "The application will be able to:") : i18nc("@info", "%1 will be able to:", applicationName);

    if (screenSharingEnabled) {
        description += i18nc("@info As in, 'an application requested access to see what’s on the screen'. '-' is Markdown, do not translate",
                             "\n - See what’s on the screen");
    }
    if (deviceTypes != RemoteDesktopPortal::None) {
        description += i18nc("@info As in, 'an application requested access to control input devices'. '-' is Markdown, do not translate",
                             "\n - Move the pointer and type keystrokes");
    }
    return description;
}

QString RemoteDesktopDialog::buildNotificationDescription(const QString &appName, RemoteDesktopPortal::DeviceTypes deviceTypes, bool screenSharingEnabled)
{
    const QString applicationName = Utils::applicationName(appName);
    QString description = applicationName.isEmpty() ? i18nc("@info", "An unidentifiable application is exercising remote control privileges:")
                                                    : i18nc("@info", "%1 is exercising remote control privileges:", applicationName);

    if (screenSharingEnabled) {
        description += i18nc("@info As in, 'an application is exercising privileges to access to see what’s on the screen'. '-' is Markdown, do not translate",
                             "\n - See what’s on the screen");
    }
    if (deviceTypes != RemoteDesktopPortal::None) {
        description += i18nc("@info As in, 'an application is exercising privileges to control input devices'. '-' is Markdown, do not translate",
                             "\n - Move the pointer and type keystrokes");
    }
    return description;
}

#include "moc_remotedesktopdialog.cpp"
