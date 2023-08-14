/*
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 */

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.plasma.workspace.dialogs 1.0 as PWD

PWD.SystemDialog
{
    id: root
    property alias body: bodyLabel.text
    property string acceptLabel: ""
    property string rejectLabel: ""

    QQC2.Label {
        id: bodyLabel
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }

    standardButtons: QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel
    Component.onCompleted: {
        if (root.acceptLabel.length > 0) {
            dialogButtonBox.button(QQC2.DialogButtonBox.Ok).text = root.acceptLabel
        }
        if (root.rejectLabel.length > 0) {
            dialogButtonBox.button(QQC2.DialogButtonBox.Cancel).text = root.rejectLabel
        }
    }
}
