/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.ki18n

PortalDialog {
    id: root

    property alias description: desc.text
    property alias allowRestore: allowRestoreItem.checked
    property alias persistenceRequested: allowRestoreItem.visible

    width: contentWidth
    height: contentHeight
    iconName: "krfb"

    QQC2.Label {
        id: desc
        textFormat: Text.MarkdownText
        Layout.fillHeight: true
    }

    footerItem: QQC2.CheckBox {
        id: allowRestoreItem
        checked: true
        text: KI18n.i18n("Allow restoring on future sessions")
    }

    Component.onCompleted: {
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = KI18n.i18nc("@action:button Approve the application gaining extra privileges", "Approve")
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Cancel).text = KI18n.i18nc("@action:button Deny the application gaining extra privileges", "Deny")
    }
}
