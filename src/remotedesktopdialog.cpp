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
    const QVariantMap props = {{QStringLiteral("title"), i18nc("Title of the dialog that requests remote input privileges", "Remote Control")},
                               {QStringLiteral("mainText"), buildMainText(appName)},
                               {QStringLiteral("description"), buildRequestDescription(deviceTypes, screenSharingEnabled)},
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
    return applicationName.isEmpty() ? i18nc("@info", "An application is asking for special privileges:\n")
                                     : i18nc("@info", "%1 is asking for special privileges:", applicationName);
}

QString RemoteDesktopDialog::buildRequestDescription(RemoteDesktopPortal::DeviceTypes deviceTypes, bool screenSharingEnabled)
{
    QString description = QString();
    if (screenSharingEnabled) {
        description += i18nc("@info As in, 'an application requested access to see what’s on the screen'. '-' is Markdown, do not translate",
                             "\n - See what’s on the screen");
    }
    if (deviceTypes != RemoteDesktopPortal::None) {
        description +=
            i18nc("@info As in, 'an application requested access to control input devices'. '-' is Markdown, do not translate", "\n - Control input devices");
    }
    return description;
}

QString RemoteDesktopDialog::buildNotificationDescription(const QString &appName, RemoteDesktopPortal::DeviceTypes deviceTypes, bool screenSharingEnabled)
{
    const QString applicationName = Utils::applicationName(appName);
    QString description = applicationName.isEmpty() ? i18nc("@info", "An application is exercising special permissions:\n")
                                                    : i18nc("@info", "%1 is exercising special privileges:", applicationName);

    if (screenSharingEnabled) {
        description += i18nc("@info As in, 'an application is exercising privileges to access to see what’s on the screen'. '-' is Markdown, do not translate",
                             "\n - See what’s on the screen");
    }
    if (deviceTypes != RemoteDesktopPortal::None) {
        description += i18nc("@info As in, 'an application is exercising privileges to control input devices'. '-' is Markdown, do not translate",
                             "\n - Control input devices");
    }
    return description;
}
