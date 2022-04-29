/*
 * SPDX-FileCopyrightText: 2019 Red Hat Inc
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2019 Jan Grulich <jgrulich@redhat.com>
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
            Layout.fillHeight: true
            Layout.minimumHeight: grid.cellHeight * 3

            Component.onCompleted: background.visible = true

            GridView {
                id: grid

                cellHeight: Kirigami.Units.iconSizes.huge + 50
                cellWidth: Kirigami.Units.iconSizes.huge + 80
                model: AppModel
                delegate: appDelegate
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

    readonly property var p0: Component {
        id: appDelegate

        Item {
            height: grid.cellHeight
            width: grid.cellWidth

            Rectangle {
                anchors.fill: parent
                color: Kirigami.Theme.highlightColor
                visible: model.applicationDesktopFile === AppChooserData.defaultApp
                radius: 2
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true

                onContainsMouseChanged: cursorShape = containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: AppChooserData.applicationSelected(model.applicationDesktopFile)
            }

            Column {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.gridUnit / 2
                spacing: Kirigami.Units.gridUnit / 3

                Kirigami.Icon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    height: Kirigami.Units.iconSizes.huge
                    width: Kirigami.Units.iconSizes.huge
                    source: model.applicationIcon
                    smooth: true
                }

                Label {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
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

