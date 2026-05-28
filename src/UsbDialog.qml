/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.ki18n

pragma ComponentBehavior: Bound

PortalDialog {
    id: root

    required property string app
    required property list<var> devices

    iconName: "drive-removable-media-usb"
    title: KI18n.i18nc("@title:window", "Device Access Requested")
    mainText: app === ""
        ? KI18n.i18ncp("@info", "Allow an unidentifiable application to access the following device?", "Allow an unidentifiable application to access the following devices?", devices.length)
        : KI18n.i18ncp("@info %2 is the name of the application", "Allow %2 to access the following device?", "Allow %2 to access the following devices?", devices.length, app)
    subtitle: app === ""
        ? KI18n.i18nc("@info:usagetip", "Only allow if you know which application made the request.")
        : ""
    scrollable: true
    contentPadding: false

    width: Kirigami.Units.gridUnit * 28
    height: Kirigami.Units.gridUnit * 30

    ListView {
        id: list
        implicitHeight: contentHeight
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

    Component.onCompleted: {
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = KI18n.i18nc("@action:button Allow the application to access devices", "Allow")
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Cancel).text = KI18n.i18nc("@action:button Deny the application's request to access devices", "Deny")
    }
}


