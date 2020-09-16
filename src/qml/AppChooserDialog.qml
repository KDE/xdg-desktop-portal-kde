/*
 * Copyright © 2019 Red Hat, Inc
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */


import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import org.kde.kirigami 2.9 as Kirigami

import org.kde.xdgdesktopportal 1.0

Item {
    id: root

    Rectangle {
        id: background
        anchors.fill: parent
        color: Kirigami.Theme.backgroundColor
    }

    ColumnLayout {
        anchors.fill: parent

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: i18n("Select an application to open <b>%1</b>. More applications are available in <a href=#discover>Discover</a>.", AppChooserData.fileName)
            textFormat: Text.RichText
            wrapMode: Text.WordWrap

            onLinkActivated: {
                AppChooserData.openDiscover()
            }
        }

        Rectangle {
            id: viewBackground
            Layout.fillWidth: true
            Layout.fillHeight: true
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            color: Kirigami.Theme.backgroundColor
            property color borderColor: Kirigami.Theme.textColor
            border.color: Qt.rgba(borderColor.r, borderColor.g, borderColor.b, 0.3)

            ScrollView {
                id: scrollView
                anchors.fill: parent

                GridView {
                    id: grid
                    anchors.fill: parent
                    cellHeight: Kirigami.Units.iconSizes.huge + 50
                    cellWidth: Kirigami.Units.iconSizes.huge + 80
                    model: AppModel
                    delegate: appDelegate
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

    Component {
        id: appDelegate

        Item {
            height: grid.cellHeight
            width: grid.cellWidth

            Rectangle {
                anchors.fill: parent
                color: Kirigami.Theme.highlightColor
                visible: ApplicationDesktopFile == AppChooserData.defaultApp
                radius: 2
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true

                onContainsMouseChanged: cursorShape = containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: AppChooserData.applicationSelected(ApplicationDesktopFile)
            }

            Column {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.gridUnit / 2
                spacing: Kirigami.Units.gridUnit / 3

                Kirigami.Icon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    height: Kirigami.Units.iconSizes.huge
                    width: Kirigami.Units.iconSizes.huge
                    source: ApplicationIcon
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
                    text: ApplicationName
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}

