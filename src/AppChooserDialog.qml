/*
 * SPDX-FileCopyrightText: 2019 Red Hat Inc
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2019 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Nate Graham <nate@kde.org>
 * SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
 */

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import Qt.labs.platform
import org.kde.kirigami as Kirigami

import org.kde.xdgdesktopportal

PortalDialog {
    id: root

    iconName: "applications-all"
    scrollable: true

    required property AppFilterModel appModel
    required property AppChooserData appChooserData

    readonly property bool showingTerminalCommand: root.appChooserData.shellAccess && searchField.editText.startsWith("/")

    property bool remember: false
    onRememberChanged: root.appChooserData.remember = remember

    readonly property QQC2.Action discoverAction: QQC2.Action{
        icon.name: "plasmadiscover"
        text: i18nc("Find some more apps that can open this content using the Discover app store", "Find More in Discover…")
        onTriggered: root.appChooserData.openDiscover()
    }
    readonly property QQC2.Action openWithTerminalAction: QQC2.Action {
        icon.name: "system-run"
        text: i18nc("@action:button", "Open")
        onTriggered: searchField.acceptResult()
    }

    headerItem: ColumnLayout {
        QQC2.CheckBox {
            Layout.fillWidth: true
            visible: root.appChooserData.mimeName !== ""
            text: i18nc("@option:check %1 is description of a file type, like 'PNG image'", "Set as default app to open %1 files", root.appChooserData.mimeDesc)
            checked: root.remember
            onToggled: {
                root.remember = checked;
            }
        }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.ComboBox {
                id: searchField
                property bool ready: false
                property string text: editText

                function acceptResult() {
                    if (showingTerminalCommand) {
                        root.appChooserData.applicationSelected(searchField.text, root.remember)
                    } else {
                        grid.currentItem.activate();
                    }
                }

                implicitWidth: Kirigami.Units.gridUnit * 20
                Layout.fillWidth: true
                editable: true

                Keys.onDownPressed: {
                    grid.forceActiveFocus();
                    grid.currentIndex = 0;
                }
                model: root.appChooserData.history
                onModelChanged: {
                    editText = ""
                    ready = true
                }
                onEditTextChanged: {
                    if (!ready) {
                        return
                    }
                    root.appModel.filter = editText;
                    if (editText.length > 0 && grid.count === 1) {
                        grid.currentIndex = 0;
                    }
                }

                Connections {
                    target: root.appChooserData
                    function onFileDialogFinished(text) {
                        if (text !== "") {
                            searchField.editText = text
                        }
                    }
                }
                onAccepted: acceptResult()

                // PortalDialog manages focus automatically, forcefully claim it here
                Component.onCompleted: forceActiveFocus()
            }

            QQC2.Button {
                icon.name: "view-more-symbolic"
                text: i18n("Show All Installed Applications")

                checkable: true
                checked: !root.appModel.showOnlyPreferredApps
                visible: root.appModel.sourceModel.hasPreferredApps
                onVisibleChanged: root.appModel.showOnlyPreferredApps = visible

                onToggled: root.appModel.showOnlyPreferredApps = !root.appModel.showOnlyPreferredApps
            }

            QQC2.Button {
                visible: root.appChooserData.shellAccess
                icon.name: "document-open-symbolic"
                text: i18nc("@action:button", "Choose Other…")
                onClicked: {
                    root.appChooserData.openFileDialog(root)
                }
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: Kirigami.Settings.tabletMode ? pressed : hovered
                QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay
            }
        }
    }

    GridView {
        id: grid

        readonly property int gridDelegateIconSize: Kirigami.Units.iconSizes.huge
        readonly property int gridDelegateWidth: gridDelegateIconSize + (Kirigami.Units.gridUnit * 4)
        readonly property int gridDelegateHeight: gridDelegateWidth + Kirigami.Units.gridUnit

        activeFocusOnTab: true
        onFocusChanged: {
            if (focus && !(currentItem ?? false)) {
                currentIndex = 0;
            }
        }
        clip: true

        Keys.onReturnPressed: currentItem.click();
        Keys.onEnterPressed: currentItem.click();

        currentIndex: -1 // Don't pre-select anything as that doesn't make sense here

        cellWidth: {
            const columns = Math.max(Math.floor((parent as QQC2.Control).availableWidth / gridDelegateWidth), 2);
            return Math.floor((parent as QQC2.Control).availableWidth / columns) - 1;
        }
        cellHeight: gridDelegateHeight

        implicitHeight: contentHeight

        model: root.appModel
        delegate: QQC2.ItemDelegate {
            id: delegate

            required property int index
            required property var model

            height: grid.cellHeight
            width: grid.cellWidth

            display: QQC2.AbstractButton.TextUnderIcon

            icon.name: delegate.model.applicationIcon
            icon.width: grid.gridDelegateIconSize
            icon.height: grid.gridDelegateIconSize

            text: switch (delegate.model.applicationDesktopFile) {
                case root.appChooserData.defaultApp:
                    return xi18nc("@info", "%1<nl/><emphasis>Default app for this file type</emphasis>", delegate.model.applicationName);
                case root.appChooserData.lastUsedApp:
                    return xi18nc("@info", "%1<nl/><emphasis>Last used app for this file type</emphasis>", delegate.model.applicationName);
                default:
                    return delegate.model.applicationName;
            }
            font.bold: delegate.model.applicationDesktopFile === root.appChooserData.defaultApp

            onClicked: root.appChooserData.applicationSelected(model.applicationDesktopFile, root.remember)
        }

        Loader {
            id: placeholderLoader

            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4

            active: grid.count === 0
            sourceComponent: Kirigami.PlaceholderMessage {
                anchors.centerIn: parent

                icon.name: root.showingTerminalCommand ? "system-run": "edit-none"
                text: {
                    if (root.showingTerminalCommand) {
                        return xi18nc("@info", "Open with <command>%1</command>?", searchField.editText)
                    } else if (searchField.editText.length > 0) {
                        return i18n("No matches")
                    } else {
                        return xi18nc("@info", "No installed applications can open <filename>%1</filename>", root.appChooserData.fileName)
                    }
                }

                helpfulAction: root.showingTerminalCommand ? root.openWithTerminalAction : root.discoverAction
            }
        }
    }

    footerItem: ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        ColumnLayout {
            visible: root.appChooserData.shellAccess
            spacing: Kirigami.Units.smallSpacing

            QQC2.CheckBox {
                id: openInTerminal
                onCheckedChanged: root.appChooserData.openInTerminal = checked
                text: i18nc("@option:check", "Run in terminal")
            }
            QQC2.CheckBox {
                // NOTE: this only ever works for konsole and xterm as per KTerminalLauncherJob. Trouble is this
                // information is not exposed through API, so we have no way of hiding this when not supported.
                Layout.leftMargin: Kirigami.Units.gridUnit
                enabled: openInTerminal.checked
                onCheckedChanged: root.appChooserData.lingerTerminal = checked
                text: i18nc("@option:check", "Do not close when command exits")
            }
        }

        QQC2.Button {
            action: root.discoverAction
        }
    }
}

