/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.14 as Kirigami
import org.kde.plasma.workspace.dialogs 1.0 as PWD

PWD.SystemDialog
{
    id: root
    property alias outputsModel: outputsView.model
    property alias windowsModel: windowsView.model
    property bool multiple: false
    iconName: "video-display"
    acceptable: (outputsModel && outputsModel.hasSelection) || (windowsModel && windowsModel.hasSelection)

    signal clearSelection()

    ColumnLayout {
        spacing: 0

        QQC2.TabBar {
            id: tabView
            Layout.fillWidth: true
            visible: root.outputsModel && root.windowsModel
            currentIndex: outputsView.count > 0 ? 0 : 1

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
            Layout.preferredHeight: Kirigami.Units.gridUnit * 10
            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            background: Rectangle {
                color: Kirigami.Theme.backgroundColor
                property color borderColor: Kirigami.Theme.textColor
                border.color: Qt.rgba(borderColor.r, borderColor.g, borderColor.b, 0.3)
            }

            StackLayout {
                anchors.fill: parent
                currentIndex: tabView.currentIndex

                QQC2.ScrollView {
                    ListView {
                        id: outputsView
                        model: null
                        delegate: Kirigami.BasicListItem {
                            icon: model.decoration
                            label: model.display
                            highlighted: false
                            checked: model.checked === Qt.Checked
                            onClicked: {
                                var to = model.checked !== Qt.Checked ? Qt.Checked : Qt.Unchecked;
                                if (!root.multiple && to === Qt.Checked) {
                                    root.clearSelection()
                                }
                                outputsView.model.setData(outputsView.model.index(model.row, 0), to, Qt.CheckStateRole)
                            }
                        }
                    }
                }
                QQC2.ScrollView {
                    ListView {
                        id: windowsView
                        model: null
                        delegate: Kirigami.BasicListItem {
                            icon: model.DecorationRole
                            label: model.DisplayRole
                            highlighted: false
                            checked: model.checked === Qt.Checked
                            onClicked: {
                                var to = model.checked !== Qt.Checked ? Qt.Checked : Qt.Unchecked;
                                if (!root.multiple && to === Qt.Checked) {
                                    root.clearSelection()
                                }
                                windowsView.model.setData(windowsView.model.index(model.row, 0), to, Qt.CheckStateRole)
                            }
                        }
                    }
                }
            }
        }
    }

    standardButtons: QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel
    Component.onCompleted: {
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = i18n("Share")
    }
}
