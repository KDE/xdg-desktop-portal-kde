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
import org.kde.plasma.workspace.dialogs as PWD

PWD.SystemDialog {
    id: root

    property alias name: nameText.text
    property alias details: detailsText.text
    property alias avatar: avatar.source

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        KirigamiComponents.Avatar {
            id: avatar

            readonly property int size: 4 * Kirigami.Units.gridUnit

            Layout.preferredWidth: size
            Layout.preferredHeight: size
            Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
        }

        Kirigami.Heading {
            id: nameText

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignTop

            Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
            Layout.fillWidth: true
        }

        QQC2.Label {
            id: detailsText

            wrapMode: Text.WordWrap

            Layout.alignment: Qt.AlignBottom
            Layout.fillWidth: true
        }
    }

    standardButtons: QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel

    Component.onCompleted: {
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = i18nc("@action:button", "Share")
    }
}
