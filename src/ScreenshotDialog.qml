/*
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.ki18n

PortalDialog {
    id: root

    property alias screenshotType: areaCombo.currentIndex
    property alias screenshotTypesModel: areaCombo.model
    property alias screenshotImage: screenshot.source
    property alias withCursor: hasCursor.checked
    property alias withBorders: hasWindowBorders.checked
    property QtObject app
    required property string appName

    iconName: "preferences-system-windows-effect-screenshot"
    title: KI18n.i18nc("@title:window", "Screenshot Requested")
    mainText: appName === ""
        ? KI18n.i18nc("@info", "Allow an unidentifiable application to take a screenshot?")
        : KI18n.i18nc("@info %1 is the name of an application", "Allow %1 to take a screenshot?", appName)
    subtitle: appName === "" ? KI18n.i18nc("@info:usagetip", "Only allow if you know which application made the request.") : undefined

    acceptable: screenshot.valid

    width: Kirigami.Units.gridUnit * 28
    height: Kirigami.Units.gridUnit * 30

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        Kirigami.FormLayout {
            Kirigami.Heading {
                text: KI18n.i18n("Capture Mode")
            }
            QQC2.ComboBox {
                id: areaCombo
                Kirigami.FormData.label: KI18n.i18n("Area:")
                textRole: "display"
            }
            QQC2.SpinBox {
                id: delayTime
                Kirigami.FormData.label: KI18n.i18n("Delay:")
                from: 0
                to: 60
                stepSize: 1
                textFromValue: (value, locale) => KI18n.i18np("%1 second", "%1 seconds", value)
                valueFromText: (text, locale) => parseInt(text);
            }

            Kirigami.Heading {
                text: KI18n.i18n("Content Options")
            }
            QQC2.CheckBox {
                id: hasCursor
                text: KI18n.i18nc("@option:check Refers to the on-screen 'mouse pointer'", "Include pointer")
                checked: true
            }
            QQC2.CheckBox {
                id: hasWindowBorders
                text: KI18n.i18n("Include window borders")
                enabled: areaCombo.currentIndex === 2
                checked: true
            }
        }

        Kirigami.Icon {
            id: screenshot
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    Component.onCompleted: {
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = KI18n.i18nc("@action:button Allow the application to use the screenshot", "Allow")
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Cancel).text = KI18n.i18nc("@action:button Deny the application's request to take a screenshot", "Deny")
    }

    actions: [
        QQC2.Action {
            readonly property Timer takeTimer: Timer {
                repeat: false
                interval: delayTime.value * 1000
                onTriggered: root.app.takeScreenshotInteractive()
            }
            text: KI18n.i18nc("@action:button", "Take Screenshot")
            icon.name: "camera-photo-symbolic"
            enabled: !takeTimer.running
            onTriggered: takeTimer.restart()
        }
    ]
}
