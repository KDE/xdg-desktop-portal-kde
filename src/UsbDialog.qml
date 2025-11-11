/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

pragma ComponentBehavior: Bound

PortalDialog {
    id: root

    required property string app
    required property list<var> devices

    iconName: "drive-removable-media-usb"
    title: i18nc("@title:window", "Access to USB Devices Requested")
    subtitle: {
        if (app === "") {
            return  i18ncp("The application is unknown", "An application wants to access the following device:", "An application wants to access the following devices:", devices.length)
        }
        return i18ncp("%2 is the name of the application", "%2 wants to access the following device:", "%2 wants to access the following devices:", devices.length, app)
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
            implicitHeight: Math.min(list.currentItem?.implicitHeight * 5, implicitContentHeight)
            ListView {
                id: list
                model: root.devices
                delegate: QQC2.ItemDelegate {
                    id: delegate
                    required property var model
                    required property int index
                    width: ListView.view.width
                    hoverEnabled: false
                    down: false
                    contentItem:  RowLayout {
                        spacing: Kirigami.Units.smallSpacing
                        Kirigami.TitleSubtitle {
                            Layout.fillWidth: true
                            title: delegate.model.properties?.ID_MODEL_FROM_DATABASE ||  decodeString(delegate.model.properties?.ID_MODEL_ENC) ||delegate. model.properties?.ID_MODEL_ID || ""
                            subtitle: delegate.model.properties?.ID_VENDOR_FROM_DATABASE || decodeString(delegate.model.properties?.ID_VENDOR_ENC) ||delegate. model.properties?.ID_VENDOR_ID || ""
                            function decodeString(a)
                            {
                                return a?.replace(/\\x([\da-f]{2})/g, (match, capture) => String.fromCharCode(parseInt(capture, 16)))
                            }
                        }
                        QQC2.Button {
                            icon.name: "list-remove"
                            onClicked: {
                                root.devices.splice(delegate.index, 1)
                            }
                        }
                    }
                }
            }
            background: Rectangle {
                color: Kirigami.Theme.backgroundColor
            }

        }
    }
}


