/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
*/

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.ki18n

PortalDialog {
    id: dialog
    readonly property string app: ""
    readonly property alias allowRestore: allowRestoreItem.checked
    required property bool persistenceRequested

    iconName: "dialog-input-devices"

    title: KI18n.i18nc("@title:window", "Input Capture Requested")
    mainText: app === ""
        ? KI18n.i18nc("@info", "An unidentifiable application wants to take over the pointer and keyboard.")
        : KI18n.i18nc("@info %1 is the name of the application", "%1 wants take over the pointer and keyboard.", app)
    subtitle: app === ""
        ? KI18n.xi18nc("@info:usagetip", "While the application is capturing input, you will not be able to move the pointer yourself or type on the keyboard.<nl/><nl/>Only allow if you know which application made the request.")
        : KI18n.i18nc("@info:usagetip", "While the application is capturing input, you will not be able to move the pointer yourself or type on the keyboard.")

    footerItem: QQC2.CheckBox {
        id: allowRestoreItem
        visible: dialog.persistenceRequested
        text: KI18n.i18nc("@option:check", "Allow the application to do this without asking next time")
    }

    width: contentWidth
    height: contentHeight

    Component.onCompleted: {
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = KI18n.i18nc("@action:button Allow the application to monitor input", "Allow");
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Cancel).text = KI18n.i18nc("@action:button Deny the application's request to monitor input", "Deny");
    }
}

