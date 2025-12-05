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
    property alias allowRestore: allowRestoreItem.checked
    required property bool persistenceRequested

    iconName: "dialog-input-devices"
    title: KI18n.i18nc("@title:window", "Input Capture Requested")
    subtitle: app === "" ? KI18n.i18nc("The application is unknown", "An application requested to capture input events") : KI18n.i18nc("%1 is the name of the application", "%1 requested to capture input events", app)

    QQC2.CheckBox {
        id: allowRestoreItem
        visible: dialog.persistenceRequested
        text: KI18n.i18nc("@option:check", "Allow restoring on future sessions")
    }

    width: contentWidth
    height: contentHeight

    standardButtons: QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel

    Component.onCompleted:  dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = KI18n.i18nc("@action:button", "Allow")
}

