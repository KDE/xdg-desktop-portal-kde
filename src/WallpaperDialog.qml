/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.xdgdesktopportal
import org.kde.ki18n

PortalDialog
{
    id: root
    required property int location
    required property string app
    required property url image

    iconName: "preferences-desktop-wallpaper"

    title: KI18n.i18nc("@title:window", "Wallpaper Change Requested")
    mainText: {
        if (app === "") {
            switch (location) {
            case WallpaperLocation.Desktop:
                return KI18n.i18nc("@info", "Allow an unidentifiable application to change the desktop wallpaper?")
            case WallpaperLocation.Lockscreen:
                return KI18n.i18nc("@info", "Allow an unidentifiable application to change the lock screen wallpaper?")
            case WallpaperLocation.Both:
                return KI18n.i18nc("@info", "Allow an unidentifiable application to change desktop and lock screen wallpapers?")
            }
        } else {
            switch (location) {
            case WallpaperLocation.Desktop:
                return KI18n.i18nc("@info %1 is the application name", "Allow %1 to change the desktop wallpaper?", app)
            case WallpaperLocation.Lockscreen:
                return KI18n.i18nc("@info %1 is the application name", "Allow %1 to change the lock screen wallpaper?", app)
            case WallpaperLocation.Both:
                return KI18n.i18nc("@info %1 is the application name", "Allow %1 to change desktop and lock screen wallpapers?", app)
            }
        }
    }
    subtitle: app === "" ? KI18n.i18nc("@info:usagetip", "Only allow if you know which application made the request.") : undefined

    Item {
        implicitHeight: Kirigami.Units.gridUnit * 10
        Image {
            id: image
            anchors.fill: parent
            fillMode: Image.PreserveAspectFit
            source: root.image
        }
    }

    Component.onCompleted: {
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = KI18n.i18nc("@action:button Allow the application to change the wallpaper", "Allow")
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Cancel).text = KI18n.i18nc("@action:button Deny the applicatinon's request to change the wallpaper", "Deny")
    }
}
