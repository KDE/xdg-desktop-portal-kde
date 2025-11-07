/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.xdgdesktopportal

PortalDialog
{
    id: root
    required property int location
    required property string app
    required property url image

    iconName: "preferences-desktop-wallpaper"
    title: i18nc("@title:window", "Set Wallpaper")
    subtitle: {
        if (app === "") {
            switch (location) {
            case WallpaperLocation.Desktop:
                return i18nc("the app is unknown", "Allow an application to set the desktop background?")
            case WallpaperLocation.Lockscreen:
                return i18nc("the app is unknown", "Allow an application to set the lock screen background?")
            case WallpaperLocation.Both:
                return i18nc("the app is unknown", "Allow an application to set desktop and lock screen backgrounds?")
            }
        } else {
            switch (location) {
            case WallpaperLocation.Desktop:
                return i18nc("%1 is the application name", "Allow %1 to set the desktop background?", app)
            case WallpaperLocation.Lockscreen:
                return i18nc("%1 is the application name", "Allow %1 to set the lock screen background?", app)
            case WallpaperLocation.Both:
                return i18nc("%1 is the application name", "Allow %1 to set desktop and lock screen backgrounds?", app)
            }
        }
    }

    Item {
        implicitHeight: Kirigami.Units.gridUnit * 10
        Image {
            id: image
            anchors.fill: parent
            fillMode: Image.PreserveAspectFit
            source: root.image
        }
    }

    standardButtons: QQC.DialogButtonBox.Ok | QQC.DialogButtonBox.Cancel
}
