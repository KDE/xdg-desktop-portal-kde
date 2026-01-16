/*
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

PortalDialog {
    id: root

    property alias body: bodyLabel.text
    property string acceptLabel
    property string rejectLabel
    property var choices
    property var selectedChoices: new Object()

    width: contentWidth
    height: contentHeight

    QQC2.Label {
        id: bodyLabel
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }

    Kirigami.FormLayout {
        Layout.fillWidth: true
        visible: root.choices?.length > 0
        Repeater {
            model: root.choices
            delegate: Loader {
                Kirigami.FormData.label: modelData.label
                sourceComponent: modelData.choices.length == 0 ? checkBox : comboBox
                Component {
                    id: checkBox
                    QQC2.CheckBox {
                        Kirigami.FormData.label: modelData.label
                        checked: modelData.initialChoiceId === "true"
                        onToggled: {
                            root.selectedChoices[modelData.id] = checked ? "true" : "false"
                        }
                    }
                }
                Component {
                    id: comboBox
                    QQC2.ComboBox {
                        model: modelData.choices
                        textRole: "value"
                        valueRole: "id"
                        onActivated: root.selectedChoices[modelData.id] = currentValue
                        Component.onCompleted: currentIndex = indexOfValue(modelData.initialChoiceId)
                    }
                }
                Component.onCompleted: root.selectedChoices[modelData.id] = modelData.initialChoiceId
            }
        }
    }

    standardButtons: QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel

    Component.onCompleted: {
        if (root.acceptLabel.length > 0) {
            dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = Qt.binding(() => root.acceptLabel);
        }
        if (root.rejectLabel.length > 0) {
            dialogButtonBox.standardButton(QQC2.DialogButtonBox.Cancel).text = Qt.binding(() => root.rejectLabel);
        }
    }
}
