/*
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.14 as Kirigami
import org.kde.plasma.workspace.dialogs 1.0 as PWD

PWD.SystemDialog
{
    id: root
    property alias screenshotType: areaCombo.currentIndex
    property alias screenshotTypesModel: areaCombo.model
    property alias screenshotImage: screenshot.source
    property QtObject app

    title: i18n("Request Screenshot")
    iconName: "preferences-system-windows-effect-screenshot"
    acceptable: screenshot.source !== undefined


    Kirigami.FormLayout {
        Kirigami.Heading {
            text: i18n("Capture Mode")
        }
        QQC2.ComboBox {
            id: areaCombo
            Kirigami.FormData.label: i18n("Area:")
            textRole: "display"
        }
        QQC2.SpinBox {
            id: delayTime
            Kirigami.FormData.label: i18n("Delay:")
            from: 0
            to: 60
            stepSize: 1
            textFromValue: function(value, locale) {
                return i18np("%1 second", "%1 seconds", value)
            }
            valueFromText: function(text, locale) {
                return parseInt(text);
            }
        }

        Kirigami.Heading {
            text: i18n("Content Options")
        }
        QQC2.CheckBox {
            id: hasCursor
            text: i18n("Include cursor pointer")
            checked: true
        }
        QQC2.CheckBox {
            id: hasWindowBorders
            text: i18n("Include window borders")
            enabled: areaCombo.currentIndex === 2
            checked: true
        }

        Kirigami.Icon {
            id: screenshot
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
    standardButtons: QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel
    Component.onCompleted: {
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = i18n("Save")
    }
    actions: [
        Kirigami.Action {
            Timer {
                id: takeTimer
                repeat: false
                interval: delayTime.value
                onTriggered: root.app.takeScreenshotInteractive()
            }
            text: i18n("Take")
            enabled: !takeTimer.running
            onTriggered: takeTimer.restart()
        }
    ]
}
