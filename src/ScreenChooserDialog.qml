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

    ColumnLayout {
        spacing: 0

        QQC2.TabBar {
            id: tabView
            Layout.fillWidth: true
            visible: (root.outputsModel ?? false) && (root.windowsModel ?? false)
            currentIndex: outputsLayout.view.count > 0 ? 0 : 1

            QQC2.TabButton {
                text: i18n("Screens")
            }
            QQC2.TabButton {
                text: i18n("Windows")
            }
        }

        QQC2.Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 20
            Layout.preferredWidth: Kirigami.Units.gridUnit * 30

            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.View

            background: Rectangle {
                color: Kirigami.Theme.backgroundColor
                border.color: Qt.alpha(Kirigami.Theme.textColor, 0.3)
                border.width: 1
                visible: tabView.visible
            }

            StackLayout {
                anchors.fill: parent
                currentIndex: tabView.currentIndex

                QQC2.ScrollView {
                    contentWidth: availableWidth
                    contentHeight: outputsLayout.height
                    PipeWireLayout {
                        id: outputsLayout
                        anchors.left: parent.left
                        anchors.right: parent.right
                        dialog: root
                        model: root.outputsModel
                    }
                }

                QQC2.ScrollView {
                    contentWidth: availableWidth
                    contentHeight: windowsLayout.height
                    PipeWireLayout {
                        id: windowsLayout
                        anchors.left: parent.left
                        anchors.right: parent.right
                        dialog: root
                        model: root.windowsModel
                    }
                }
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
