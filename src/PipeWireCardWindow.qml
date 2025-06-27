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
        Kirigami.ShadowedRectangle {
            property color defaultColor: Kirigami.Theme.backgroundColor
            property color hoverColor: Kirigami.ColorUtils.tintWithAlpha(defaultColor, Kirigami.Theme.highlightColor, 0.1)

            anchors.fill: parent
            radius: Kirigami.Units.cornerRadius

            color: root.hovered ? hoverColor : defaultColor

            shadow.size: 2
            border.color: Kirigami.Theme.alternateBackgroundColor
            border.width: 1
        }
    }
}
