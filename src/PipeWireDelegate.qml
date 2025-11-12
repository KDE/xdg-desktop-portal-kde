/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

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
    required property int nodeId
    required property bool exclusive

    Accessible.name: itemName

    header: RowLayout {
        Layout.fillWidth: true

        Kirigami.Icon {
            implicitWidth: Kirigami.Units.iconSizes.small
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
    }

    contentItem: PipeWire.PipeWireSourceItem {
        id: pipeWireSourceItem
        Layout.preferredHeight: Kirigami.Units.gridUnit * 7
        Layout.preferredWidth: Kirigami.Units.gridUnit * 7
        Layout.fillWidth: true
        Layout.fillHeight: true

        nodeId: root.nodeId

        Kirigami.Icon {
            anchors.fill: parent
            visible: pipeWireSourceItem.nodeId === 0
            source: root.iconSource
        }
    }

    Layout.preferredHeight: contentItem.Layout.preferredHeight
}
