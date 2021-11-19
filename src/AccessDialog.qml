/*
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 */

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.14 as Kirigami
import org.kde.plasma.workspace.dialogs 1.0 as PWD

PWD.DesktopSystemDialog
{
    id: root
    property alias body: bodyLabel.text
    property string acceptLabel: undefined
    property string rejectLabel: undefined

    QQC2.Label {
        id: bodyLabel
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }

    dialogButtonBox.standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    Component.onCompleted: {
        if (root.acceptLabel) {
            dialogButtonBox.button(DialogButtonBox.Ok).text = root.acceptLabel
        }
        if (root.rejectLabel) {
            dialogButtonBox.button(DialogButtonBox.Cancel).text = root.rejectLabel
        }
    }
}
