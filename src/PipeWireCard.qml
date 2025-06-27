/*
 *  SPDX-FileCopyrightText: 2024 Oliver Beard <olib141@outlook.com>
 *  SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Effects
// import Qt5Compat.GraphicalEffects

import org.kde.kirigami as Kirigami

Kirigami.AbstractCard {
    id: root

    background: Item {
        // A blurred translucent wallpaper as primary background.
        Kirigami.ShadowedImage {
            id: wallpaperImage

            anchors.fill: parent
            radius: Kirigami.Units.cornerRadius

            fillMode: Image.PreserveAspectCrop
            source: root.wallpaper

            opacity: root.hovered ? 0.30 : 0.20

            layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
            layer.effect: MultiEffect {
                blurEnabled: true
                blur: 1.0
                blurMax: 32
            }
        }

        // Above the wallpaper is a rectangle with just a border. The border of the wallpaper would get blurred too
        // if these were combined!
        Kirigami.ShadowedImage {
            anchors.fill: wallpaperImage
            radius: wallpaperImage.radius

            shadow.size: 2
            border.color: Kirigami.Theme.alternateBackgroundColor
            border.width: 1
            color: "transparent"
        }
    }
}
