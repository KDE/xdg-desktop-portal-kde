/*
 * SPDX-FileCopyrightText: 2020 Red Hat Inc
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2020 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as KirigamiComponents

PortalDialog {
    id: root

    title: i18nc("@action:button", "Share user info")
    required property string realname
    required property string username
    property var avatar

    QQC2.CheckBox {
        id: allowRestoreItem
        text: i18nc("@option:check", "Allow restoring on future sessions")
    }

    width: contentWidth
    height: contentHeight

    standardButtons: QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel

    Component.onCompleted: {
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = i18nc("@action:button", "Share")
    }
}
