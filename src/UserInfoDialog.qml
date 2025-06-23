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

    required property string realname
    required property string username
    property alias avatar: avatar.source

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Item {
            Layout.fillHeight: true
        }

        KirigamiComponents.Avatar {
            id: avatar

            readonly property int size: 4 * Kirigami.Units.gridUnit

            Layout.preferredWidth: size
            Layout.preferredHeight: size
            Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
        }

        component Heading: Kirigami.Heading {
            visible: text.length > 0

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignTop

            Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
            Layout.fillWidth: true
        }

        Heading {
            level: 1
            text: root.realname
        }

        Heading {
            // Take away the spacing here so things look a bit more packed since they are related information.
            Layout.topMargin: -parent.spacing

            level: 2
            text: root.username
            color: Kirigami.Theme.disabledTextColor
        }

        Item {
            Layout.fillHeight: true
        }
    }

    standardButtons: QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel

    Component.onCompleted: {
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = i18nc("@action:button", "Share")
    }
}
