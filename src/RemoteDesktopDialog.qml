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
    iconName: "krfb"
    property alias outputsModel: outputsView.model

    property alias withScreenSharing: outputsView.enabled
    property bool withMultipleScreenSharing: false
    property alias withKeyboard: keyboardCheck.checked
    property alias withPointer: pointerCheck.checked
    property alias withTouch: touchCheck.checked
    acceptable: !withScreenSharing || outputsModel.hasSelection

    RowLayout {
        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: root.width * 0.4
            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            background: Rectangle {
                color: Kirigami.Theme.backgroundColor
                property color borderColor: Kirigami.Theme.textColor
                border.color: Qt.rgba(borderColor.r, borderColor.g, borderColor.b, 0.3)
            }
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
                        if (!root.withMultipleScreenSharing && to === Qt.Checked) {
                            root.outputsModel.clearSelection()
                        }
                        outputsView.model.setData(outputsView.model.index(model.row, 0), to, Qt.CheckStateRole)
                    }
                }
            }
        }

        QQC2.Frame {
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent
                QQC2.CheckBox {
                    id: keyboardCheck
                    text: i18n("Keyboard")
                }
                QQC2.CheckBox {
                    id: pointerCheck
                    text: i18n("Pointer")
                }
                QQC2.CheckBox {
                    id: touchCheck
                    text: i18n("Touch")
                }
                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }
            }
        }
    }

    standardButtons: QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel
    Component.onCompleted: {
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = i18n("Share")
    }
}
