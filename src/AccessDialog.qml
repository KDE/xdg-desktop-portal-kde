/*
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.xdgdesktopportal

pragma ComponentBehavior: Bound

PortalDialog {
    id: root

    property alias body: bodyLabel.text
    property string acceptLabel
    property string rejectLabel
    property var choices
    property var selectedChoices: new Object()

    width: contentWidth
    height: contentHeight

    ColumnLayout {
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
                    id: delegate
                    required property var modelData
                    Kirigami.FormData.label: modelData.label
                    sourceComponent: modelData.choices.length == 0 ? checkBox : comboBox
                    Component {
                        id: checkBox
                        QQC2.CheckBox {
                            Kirigami.FormData.label: delegate.modelData.label
                            checked: delegate.modelData.initialChoiceId === "true"
                            onToggled: {
                                root.selectedChoices[delegate.modelData.id] = checked ? "true" : "false"
                            }
                        }
                    }
                    Component {
                        id: comboBox
                        QQC2.ComboBox {
                            model: delegate.modelData.choices
                            textRole: "value"
                            valueRole: "id"
                            onActivated: root.selectedChoices[delegate.modelData.id] = currentValue
                            Component.onCompleted: currentIndex = indexOfValue(delegate.modelData.initialChoiceId)
                        }
                    }
                    Component.onCompleted: root.selectedChoices[modelData.id] = modelData.initialChoiceId
                }
            }
        }
    }

    Component.onCompleted: {
        if (root.acceptLabel.length > 0) {
            dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = Qt.binding(() => root.acceptLabel);
        }
        if (root.rejectLabel.length > 0) {
            dialogButtonBox.standardButton(QQC2.DialogButtonBox.Cancel).text = Qt.binding(() => root.rejectLabel);
        }
    }
}
