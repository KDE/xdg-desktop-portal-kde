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

RemoteDesktopDialog::RemoteDesktopDialog(const QString &appName, RemoteDesktopPortal::DeviceTypes deviceTypes, bool screenSharingEnabled, QObject *parent)
    : QuickDialog(parent)
{
    QString description = i18n("Requested access to:\n");
    if (screenSharingEnabled) {
        description += i18nc("Will allow the app to see what's on the outputs", "- Screens\n");
    }
    if (deviceTypes != RemoteDesktopPortal::None) {
        description += i18nc("Will allow the app to send input events", "- Input devices\n");
    }

    QVariantMap props = {{"description", description}};

    const QString applicationName = Utils::applicationName(appName);
    if (applicationName.isEmpty()) {
        props.insert(QStringLiteral("title"), i18n("Select what to share with the requesting application"));
    } else {
        props.insert(QStringLiteral("title"), i18n("Select what to share with %1", applicationName));
    }

    create(QStringLiteral("qrc:/RemoteDesktopDialog.qml"), props);
}
