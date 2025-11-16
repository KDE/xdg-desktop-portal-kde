/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

ScreenChooserDialogTemplate {
    id: root

    required property var outputsModel
    required property var windowsModel
    property bool multiple: false
    property alias allowRestore: allowRestoreItem.checked

    iconName: "video-display"
    acceptable: (outputsModel && outputsModel.hasSelection) || (windowsModel && windowsModel.hasSelection)

    width: Kirigami.Units.gridUnit * 28
    height: Kirigami.Units.gridUnit * 30

    QQC2.ScrollView {
        contentWidth: availableWidth

        ColumnLayout {
            spacing: Kirigami.Units.largeSpacing
            anchors.left: parent.left
            anchors.right: parent.right

            PipeWireLayout {
                id: outputsLayout
                Layout.fillWidth: true
                dialog: root
                model: root.outputsModel
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing
                visible: windowsLayout.view.count > 0

                Kirigami.Separator {
                    Layout.fillWidth: true
                }

                QQC2.Label {
                    font.bold: true
                    text: i18nc("@label separator line label between screen selection and window selection", "Windows")
                    wrapMode: Text.WordWrap
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                }
            }

            PipeWireLayout {
                id: windowsLayout
                Layout.fillWidth: true
                dialog: root
                model: root.windowsModel
            }
        }
    }

    standardButtons: root.multiple ? QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel : QQC2.DialogButtonBox.NoButton
    dialogButtonBoxLeftItem: QQC2.CheckBox {
        id: allowRestoreItem
        checked: true
        text: i18n("Allow restoring on future sessions")
    }

    Component.onCompleted: {
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = i18n("Share")

        // If there's only one thing in the list, pre-select it to save the user a click
        if (outputsLayout.view.count === 1 && windowsLayout.view.count === 0) {
            outputsLayout.view.model.setData(outputsLayout.view.model.index(0, 0), Qt.Checked, Qt.CheckStateRole);
        } else if (windowsLayout.view.count === 1 && outputsLayout.view.count === 0) {
            windowsLayout.model.setData(outputsLayout.view.model.index(0, 0), Qt.Checked, Qt.CheckStateRole);
        }
    }
}
