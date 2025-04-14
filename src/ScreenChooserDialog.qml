/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.workspace.dialogs as PWD
import org.kde.taskmanager 0.1 as TaskManager

PWD.SystemDialog {
    id: root

    property alias outputsModel: outputsView.model
    property alias windowsModel: windowsView.model
    property bool multiple: false
    property alias allowRestore: allowRestoreItem.checked

    iconName: "video-display"
    acceptable: (outputsModel && outputsModel.hasSelection) || (windowsModel && windowsModel.hasSelection)

    signal clearSelection()

    ColumnLayout {
        spacing: 0

        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 20
            Layout.preferredWidth: Kirigami.Units.gridUnit * 45
            contentWidth: availableWidth
            contentHeight: outputsLayout.height

            Kirigami.CardsLayout {
                id: outputsLayout
                maximumColumns: 3

                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                Repeater {
                    id: outputsView
                    model: null
                    PipeWireDelegate {
                        id: delegate

                        required property int index
                        required property var model

                        function selectAndAccept(): void {
                            root.clearSelection()
                            outputsView.model.setData(outputsView.model.index(model.row, 0), Qt.Checked, Qt.CheckStateRole)
                            dialogButtonBox.accepted()
                        }

                        banner {
                            title: model.display
                            titleIcon: model.decoration
                            titleLevel: 3
                        }
                        checkable: root.multiple
                        checked: model.checked === Qt.Checked
                        nodeId: waylandItem.nodeId

                        activeFocusOnTab: true
                        highlighted: activeFocus

                        Accessible.role: root.multiple ? Accessible.CheckBox : Accessible.Button

                        TaskManager.ScreencastingRequest {
                            id: waylandItem
                            outputName: delegate.model.name
                        }

                        // Only active if this is a multi-select dialog
                        onToggled: {
                            const to = model.checked !== Qt.Checked ? Qt.Checked : Qt.Unchecked;
                            outputsView.model.setData(outputsView.model.index(model.row, 0), to, Qt.CheckStateRole)
                        }

                        // If this is isn't a multi-select dialog, accept on click
                        // since the cards are functioning as buttons
                        onClicked: {
                            if (!root.multiple) {
                                selectAndAccept()
                            }
                        }

                        // If this is a multi-select dialog, let people choose just
                        // one thing quickly by double-clicking
                        onDoubleClicked: {
                            if (root.multiple) {
                                selectAndAccept()
                            }
                        }
                    }
                }

                Repeater {
                    id: windowsView
                    model: null
                    PipeWireDelegate {
                        id: delegate

                        required property int index
                        required property var model

                        function selectAndAccept(): void {
                            root.clearSelection()
                            windowsView.model.setData(windowsView.model.index(model.row, 0), Qt.Checked, Qt.CheckStateRole)
                            dialogButtonBox.accepted()
                        }

                        banner {
                            title: model.display ?? ""
                            titleIcon: model.decoration ?? ""
                            titleLevel: 3
                        }
                        checkable: root.multiple
                        checked: model.checked === Qt.Checked
                        nodeId: waylandItem.nodeId

                        activeFocusOnTab: true
                        highlighted: activeFocus

                        Accessible.role: root.multiple ? Accessible.CheckBox : Accessible.Button

                        TaskManager.ScreencastingRequest {
                            id: waylandItem
                            uuid: delegate.model.Uuid
                        }

                        // Only active if this is a multi-select dialog
                        onToggled: {
                            const to = model.checked !== Qt.Checked ? Qt.Checked : Qt.Unchecked;
                            windowsView.model.setData(windowsView.model.index(model.row, 0), to, Qt.CheckStateRole)
                        }

                        // If this is isn't a multi-select dialog, accept on click
                        // since the cards are functioning as buttons
                        onClicked: {
                            if (!root.multiple) {
                                selectAndAccept()
                            }
                        }

                        // If this is a multi-select dialog, let people choose just
                        // one thing quickly by double-clicking
                        onDoubleClicked: {
                            if (root.multiple) {
                                selectAndAccept()
                            }
                        }
                    }
                }
            }
        }

        QQC2.CheckBox {
            id: allowRestoreItem
            checked: true
            text: i18n("Allow restoring on future sessions")
        }
    }

    standardButtons: root.multiple ? QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel : QQC2.DialogButtonBox.NoButton

    Component.onCompleted: {
        if (root.multiple) {
            dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = i18n("Share")
        }

        // If there's only one thing in the list, pre-select it to save the user a click
        if (outputsView.count === 1 && windowsView.count === 0) {
            outputsView.model.setData(outputsView.model.index(0, 0), Qt.Checked, Qt.CheckStateRole);
        } else if (windowsView.count === 1 && outputsView.count === 0) {
            windowsView.model.setData(outputsView.model.index(0, 0), Qt.Checked, Qt.CheckStateRole);
        }
    }
}
