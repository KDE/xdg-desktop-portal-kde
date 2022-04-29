/*
 * SPDX-FileCopyrightText: 2019 Red Hat Inc
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2019 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Nate Graham <nate@kde.org>
 */


import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import org.kde.kirigami 2.9 as Kirigami
import org.kde.plasma.workspace.dialogs 1.0 as PWD

import org.kde.xdgdesktopportal 1.0

PWD.SystemDialog
{
    id: root
    iconName: "applications-all"
    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        // Using a TextEdit here instead of a Label because it can know when any
        // links are hovered, which is needed for us to be able to use the correct
        // cursor shape for it
        TextEdit {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: xi18nc("@info", "Select an application to open <filename>%1</filename>. More applications are available in <link>Discover</link>.", AppChooserData.fileName)
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

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: grid.cellHeight * 3
            Layout.maximumHeight: grid.cellHeight * 3

            Component.onCompleted: background.visible = true

            GridView {
                id: grid

                currentIndex: -1 // Don't pre-select anything as that doesn't make sense here

                cellHeight: Kirigami.Units.iconSizes.huge + (Kirigami.Units.gridUnit * 4)
                cellWidth: Kirigami.Units.iconSizes.huge + (Kirigami.Units.gridUnit * 4)
                model: AppModel
                delegate: Item {
                    height: grid.cellHeight
                    width: grid.cellWidth

                    HoverHandler {
                        acceptedButtons: Qt.NoButton
                        cursorShape: hovered ? Qt.PointingHandCursor : Qt.ArrowCursor
                    }
                    TapHandler {
                        onTapped: AppChooserData.applicationSelected(model.applicationDesktopFile)
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: Kirigami.Theme.highlightColor
                        visible: model.applicationDesktopFile === AppChooserData.defaultApp
                        radius: 2
                    }

                    ColumnLayout {
                        anchors {
                            top: parent.top
                            left: parent.left
                            right: parent.right
                            margins: Kirigami.Units.largeSpacing
                        }
                        spacing: Kirigami.Units.largeSpacing

                        Kirigami.Icon {
                            anchors.horizontalCenter: parent.horizontalCenter
                            Layout.preferredWidth: Kirigami.Units.iconSizes.huge
                            Layout.preferredHeight: Kirigami.Units.iconSizes.huge
                            source: model.applicationIcon
                            smooth: true
                        }

                        Label {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            text: model.applicationName
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }

        Button {
            id: showAllAppsButton
            Layout.alignment: Qt.AlignHCenter
            icon.name: "view-more-symbolic"
            text: i18n("Show More")

            onClicked: {
                visible = false
                AppModel.showOnlyPreferredApps = false
            }
        }

        Kirigami.SearchField {
            id: searchField
            Layout.fillWidth: true
            visible: !showAllAppsButton.visible
            opacity: visible
            onTextChanged: AppModel.filter = text
        }
    }
}

