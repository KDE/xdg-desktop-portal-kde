/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
*/


import QtQuick
import QtQuick.Controls as QQC2
import org.kde.ki18n

PortalDialog {

    readonly property string app: ""

    iconName: "dialog-input-devices"
    title: KI18n.i18nc("@title:window", "Input Capture Requested")
    subtitle: app === "" ? KI18n.i18nc("The application is unknown", "An application requested to capture input events") : i18nc("%1 is the name of the application", "%1 requested to capture input events", app)

    width: contentWidth
    height: contentHeight

    Component.onCompleted:  dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = KI18n.i18nc("@action:button", "Allow")
}

