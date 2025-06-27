pragma ComponentBehavior: Bound
/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.pipewire as PipeWire
import org.kde.taskmanager as TaskManager
import org.kde.kitemmodels as ItemModels

import org.kde.xdgdesktopportal

PipeWireCardWindow {
    id: card

    required property var nodeId
    required property var iconSource
    required property string itemName

    contentItem: ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true

        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            Layout.fillWidth: true
            Kirigami.Icon {
                implicitWidth: Kirigami.Units.iconSizes.small
                implicitHeight: implicitWidth
                source: card.iconSource
            }
            QQC2.Label {
                font.bold: true
                elide: Text.ElideRight
                text: card.itemName
                Layout.fillWidth: true
            }
            QQC2.CheckBox {
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            PipeWire.PipeWireSourceItem {
                id: pipeWireSourceItem
                anchors.fill: parent
                nodeId: card.nodeId
            }

            ShaderEffectSource {
                id: textureSource
                sourceItem: pipeWireSourceItem
                sourceRect: pipeWireSourceItem.paintedRect
                hideSource: true
            }

            Kirigami.ShadowedTexture {
                anchors.centerIn: parent
                width: pipeWireSourceItem.paintedRect.width
                height: pipeWireSourceItem.paintedRect.height
                radius: Kirigami.Units.cornerRadius
                shadow.size: 2
                border.color: Kirigami.Theme.alternateBackgroundColor
                border.width: 1
                source: textureSource
            }
        }
    }
}
