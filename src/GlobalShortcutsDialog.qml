/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/


import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kquickcontrols as KQC
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCMUtils

PortalDialog {

    id: root

    required property string app
    required property string component
    required property var newShortcuts
    required property var returningShortcuts

    iconName: "preferences-desktop-keyboard-shortcut"
    title: i18nc("@title:window", "Global Shortcuts Requested")
    subtitle: {
        const count = newShortcuts.rowCount()
        if (app === "") {
            return  i18ncp("The application is unknown", "An application wants to register the following shortcut:", "An application wants to register the following %1 shortcuts:", count)
        }
        return i18ncp("%2 is the name of the application", "%2 wants to register the following shortcut:", "%2 wants to register the following %1 shortcuts:", count, app)
    }

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
        Binding {
            when: content.parent
            target: content.parent
            property: "bottomPadding"
            value: 0
        }

        spacing: 0

        QQC2.ScrollView {
            id: view
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            clip: true
            Layout.fillWidth: true
            Layout.fillHeight: true
            // Let the implicitHeight be "5 items shown by default"
            implicitHeight: Math.min(list.currentItem.implicitHeight * 5, implicitContentHeight)
            ListView {
                id: list
                model: newShortcuts
                delegate: QQC2.ItemDelegate {
                    id: delegate
                    required property var model
                    width: ListView.view.width
                    hoverEnabled: false
                    down: false
                    contentItem: RowLayout {
                        spacing: Kirigami.Units.smallSpacing
                        Kirigami.TitleSubtitle {
                            Layout.fillWidth: true
                            title: model.display
                            font: delegate.font
                        }
                         Kirigami.Icon {
                            id: conflictIcon
                            visible: model.globalConflict || model.standardConflict || model.internalConflict
                            source: "data-warning"
                            QQC2.ToolTip.text:  model.conflictText ?? ""
                            QQC2.ToolTip.visible: hoverHandler.hovered
                            HoverHandler {
                                id: hoverHandler
                            }
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
            icon.name: "systemsettings"
            text: i18nc("@action:button", "See Other Shortcuts…")
            QQC2.ToolTip.text: i18nc("@info:tooltip", "View other shortcuts registered by this application")
            QQC2.ToolTip.visible: hovered
            onClicked: KCMUtils.KCMLauncher.openSystemSettings("kcm_keys", component)
        }
        Binding {
            delayed: true
            target: root.dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok)
            value: !(newShortcuts.hasGlobalConflict || newShortcuts.hasInternalConflict)
            property: "enabled"
        }
    }
}

