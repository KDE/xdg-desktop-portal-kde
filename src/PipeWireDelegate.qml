// SPDX-License-Identifier: LGPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.pipewire as PipeWire

Kirigami.AbstractCard {
    id: root

    required property string itemName
    required property var iconSource
    required property string itemDescription
    // Whether or not this is a synthetic "fake" output (e.g. an action to create a region)
    required property bool synthetic
    required property int nodeId
    required property bool exclusive

    Accessible.name: itemName

    header: GridLayout {
        columnSpacing: Kirigami.Units.smallSpacing
        rowSpacing: 0
        columns: 3

        Kirigami.Icon {
            implicitWidth: Kirigami.Units.iconSizes.sizeForLabels
            implicitHeight: implicitWidth
            source: root.iconSource
        }
        QQC2.Label {
            font.bold: true
            elide: Text.ElideRight
            text: root.itemName
            Layout.fillWidth: true
        }
        Loader {
            id: checkboxLoader
            active: root.checkable

            Component {
                id: checkboxComponent
                QQC2.CheckBox {
                    checked: root.checked
                    onToggled: root.checked = checked
                }
            }

            Component {
                id: radioComponent
                QQC2.RadioButton {
                    checked: root.checked
                    onToggled: root.checked = checked
                }
            }

            sourceComponent: root.exclusive ? radioComponent : checkboxComponent
        }

        Item { // spacer
            visible: descriptionLabel.visible
        }
        QQC2.Label {
            id: descriptionLabel
            Layout.columnSpan: 2
            Layout.fillWidth: true
            text: root.itemDescription
            wrapMode: Text.Wrap
            visible: text.length > 0
            opacity: 0.75 // in lieu of a semantic subtitle color
        }
    }

    Component {
        id: preview

        Item {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 7
            Layout.preferredHeight: Kirigami.Units.gridUnit * 7
            Layout.fillWidth: true
            Layout.fillHeight: true

            PipeWire.PipeWireSourceItem {
                id: pipeWireSourceItem
                anchors.fill: parent
                nodeId: root.nodeId
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
                color: Kirigami.Theme.textColor
                border.color: Kirigami.Theme.textColor
                border.width: 0
                source: textureSource
            }

            Kirigami.Icon {
                anchors.fill: parent
                visible: pipeWireSourceItem.nodeId === 0
                source: root.iconSource
            }
        }
    }

    // A synthetic delegate has no preview and as such has no contentItem and is smaller than a normal one.
    Layout.rowSpan: synthetic ? 1 : 2
    contentItem: synthetic ? null : preview.createObject(null) as Item

    Layout.preferredHeight: contentItem?.Layout?.preferredHeight ?? -1
}
