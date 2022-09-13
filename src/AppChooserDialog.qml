/*
 * SPDX-FileCopyrightText: 2019 Red Hat Inc
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2019 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Nate Graham <nate@kde.org>
 */


import QtQuick 2.15
import Qt.labs.platform 1.1
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import org.kde.kirigami 2.19 as Kirigami
import org.kde.plasma.workspace.dialogs 1.0 as PWD

import org.kde.xdgdesktopportal 1.0

PWD.SystemDialog
{
    id: root
    iconName: "applications-all"

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            spacing: Kirigami.Units.smallSpacing

            Kirigami.SearchField {
                id: searchField
                implicitWidth: Kirigami.Units.gridUnit * 20
                Layout.fillWidth: true
                focus: true
                // We don't want auto-accept here because it would cause the
                // selected app in the grid view to be immediately launched
                // without user input
                autoAccept: false

                Keys.onDownPressed: {
                    grid.forceActiveFocus();
                    grid.currentIndex = 0;
                }
                onTextChanged: {
                    AppModel.filter = text;
                    if (text.length > 0 && grid.count === 1) {
                        grid.currentIndex = 0;
                    }
                }
                onAccepted: grid.currentItem.activate();
            }

            Button {
                icon.name: "view-more-symbolic"
                text: i18n("Show All Installed Applications")

                checkable: true
                checked: !AppModel.showOnlyPreferredApps
                visible: AppModel.sourceModel.hasPreferredApps
                onVisibleChanged: AppModel.showOnlyPreferredApps = visible

                onClicked: {
                    AppModel.showOnlyPreferredApps = !AppModel.showOnlyPreferredApps
                }
            }
        }

        ScrollView {
            id: scrollView

            readonly property int viewWidth: width - (ScrollBar.vertical.visible ? ScrollBar.vertical.width : 0)

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: grid.cellHeight * 3

            Component.onCompleted: background.visible = true

            GridView {
                id: grid

                readonly property int gridDelegateSize: Kirigami.Units.iconSizes.huge + (Kirigami.Units.gridUnit * 4)

                Keys.onReturnPressed: currentItem.activate();
                Keys.onEnterPressed: currentItem.activate();

                currentIndex: -1 // Don't pre-select anything as that doesn't make sense here

                cellWidth: {
                    let columns = Math.max(Math.floor(scrollView.viewWidth / gridDelegateSize), 2);
                    return Math.floor(scrollView.viewWidth / columns) - 1;
                }
                cellHeight: gridDelegateSize

                model: AppModel
                delegate: Item {
                    id: delegate

                    height: grid.cellHeight
                    width: grid.cellWidth

                    function activate() {
                        AppChooserData.applicationSelected(model.applicationDesktopFile)
                    }

                    HoverHandler {
                        id: hoverhandler
                    }
                    TapHandler {
                        id: taphandler
                        onTapped: delegate.activate()
                    }

                    Rectangle {
                        readonly property color theColor: Kirigami.Theme.highlightColor
                        anchors.fill: parent
                        visible: hoverhandler.hovered || delegate == grid.currentItem
                        border.color: theColor
                        border.width: 1
                        color: taphandler.pressed ? theColor
                                                  : Qt.rgba(theColor.r, theColor.g, theColor.b, 0.3)
                        radius: Kirigami.Units.smallSpacing
                    }

                    ColumnLayout {
                        anchors {
                            top: parent.top
                            left: parent.left
                            right: parent.right
                            margins: Kirigami.Units.largeSpacing
                        }
                        spacing: 0 // Items add their own as needed here

                        Kirigami.Icon {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.huge
                            Layout.preferredHeight: Kirigami.Units.iconSizes.huge
                            Layout.bottomMargin: Kirigami.Units.largeSpacing
                            Layout.alignment: Qt.AlignHCenter
                            source: model.applicationIcon
                            smooth: true
                        }

                        Label {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            horizontalAlignment: Text.AlignHCenter
                            text: model.applicationName
                            font.bold: model.applicationDesktopFile === AppChooserData.defaultApp
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            horizontalAlignment: Text.AlignHCenter
                            visible: model.applicationDesktopFile === AppChooserData.defaultApp
                            font.bold: true
                            opacity: 0.7
                            text: i18n("Default app for this file type")
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                Loader {
                    id: placeholderLoader

                    anchors.centerIn: parent
                    width: parent.width -(Kirigami.Units.gridUnit - 8)

                    active: grid.count === 0
                    sourceComponent:Kirigami.PlaceholderMessage {
                        anchors.centerIn: parent

                        icon.name: "edit-none"
                        text: searchField.text.length > 0 ? i18n("No matches") : xi18nc("@info", "No installed applications can open <filename>%1</filename>", AppChooserData.fileName)

                        helpfulAction: Kirigami.Action {
                            iconName: "plasmadiscover"
                            text: i18nc("Find some more apps that can open this content using the Discover app", "Find More in Discover")
                            onTriggered: AppChooserData.openDiscover()
                        }
                    }
                }
            }
        }

        // Using a TextEdit here instead of a Label because it can know when any
        // links are hovered, which is needed for us to be able to use the correct
        // cursor shape for it.
        TextEdit {
            visible: !placeholderLoader.active && StandardPaths.findExecutable("plasma-discover") != ""
            Layout.fillWidth: true
            text: xi18nc("@info", "Don't see the right app? Find more in <link>Discover</link>.")
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            readOnly: true
            color: Kirigami.Theme.textColor
            selectedTextColor: Kirigami.Theme.highlightedTextColor
            selectionColor: Kirigami.Theme.highlightColor

            onLinkActivated: {
                AppChooserData.openDiscover()
            }

            HoverHandler {
                acceptedButtons: Qt.NoButton
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
        }
    }
}

