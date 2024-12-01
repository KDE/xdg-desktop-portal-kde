/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/


import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kquickcontrols as KQC
import org.kde.kirigami as Kirigami
import org.kde.plasma.workspace.dialogs as PWD
import org.kde.kcmutils as KCMUtils

PWD.SystemDialog {

    id: root

    required property string app
    required property string component
    required property var newShortcuts
    required property var returningShortcuts

    iconName: "preferences-desktop-keyboard-shortcut"
    title: i18nc("@title:window", "Global Shortcuts requested")
    subtitle: app === "" ? i18nc("The application is unknown", "An application wants to register the following shortcuts") : i18nc("%1 is the name of the application", "%1 wants to register the following shortcuts", app)

    standardButtons: QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel
    ColumnLayout {
        id: content
        Binding {
            when: content.parent
            target: content.parent
            property: "leftPadding"
            value: 0
        }
        Binding {
            when: content.parent
            target: content.parent
            property: "rightPadding"
            value: 0
        }

        spacing: 0

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        QQC2.ScrollView {
            id: view
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            clip: true
            Layout.fillWidth: true
            Layout.fillHeight: true
            ListView {
                id: list
                model: newShortcuts
                delegate: QQC2.ItemDelegate {
                    id: delegate
                    required property var model
                    width: ListView.view.width
                    contentItem: RowLayout {
                        spacing: Kirigami.Units.smallSpacing 
                        Kirigami.TitleSubtitle {
                            Layout.fillWidth: true
                            title: model.display
                            font: delegate.font
                        }
                        KQC.KeySequenceItem {
                            id: keySequenceItem
                            Layout.alignment: Qt.AlignRight
                            showCancelButton: true
                            keySequence: model.keySequence
                            onKeySequenceModified: {
                                model.keySequence = keySequence
                            }
                        }
                        Kirigami.Icon {
                            id: conflictIcon
                            visible: model.globalConflict || model.standardConflict
                            source: "data-warning"
                            QQC2.ToolTip.text:  model.conflictText
                            QQC2.ToolTip.visible: hoverHandler.hovered
                            HoverHandler {
                                id: hoverHandler
                            }
                        }
                    }
                }
            }
            background: Rectangle {
                color: Kirigami.Theme.backgroundColor
            }
        }
        Kirigami.Separator {
            Layout.fillWidth: true
        }
        QQC2.Button {
            parent: root.dialogButtonBox
            QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.HelpRole
            visible: returningShortcuts.length != 0
            icon.name: "preferences-desktop-keyboard-shortcut"
            text: i18nc("@action:button", "View other shortcuts registered by this application…")
            onClicked: KCMUtils.KCMLauncher.openSystemSettings("kcm_keys", component)
            Component.onCompleted: console.log(returningShortcuts,  returningShortcuts.length != 0)
        }
        Binding {
            delayed: true
            target: root.dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok)
            value: !newShortcuts.hasGlobalConflict
            property: "enabled"
        }
    }
}

