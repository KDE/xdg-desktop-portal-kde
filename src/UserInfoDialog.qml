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
import org.kde.ki18n

PortalDialog {
    id: root

    title: KI18n.i18nc("@title:window", "User Information Requested")
    //mainText and subtitle are set in C++
    required property string realname
    required property string username
    property alias avatar: avatar.source

    ColumnLayout {
        spacing: 0

        Item {
            Layout.fillHeight: true
        }

        ColumnLayout {
            spacing: Kirigami.Units.largeSpacing

            KirigamiComponents.Avatar {
                id: avatar

                readonly property int size: 6 * Kirigami.Units.gridUnit

                Layout.preferredWidth: size
                Layout.preferredHeight: size
                Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
            }

            component Heading: Kirigami.Heading {
                visible: text.length > 0
                wrapMode: Text.WordWrap

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignTop

                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                Layout.fillWidth: true
            }

            Heading {
                id: realNameHeading

                level: 1
                text: root.realname
            }

            Heading {
                // Take away the spacing here so things look a bit more packed since they are related information.
                Layout.topMargin: realNameHeading.visible ? -parent.spacing : 0

                level: 2
                text: root.username
                color: Kirigami.Theme.disabledTextColor
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Component.onCompleted: {
        dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = KI18n.i18nc("@action:button", "Share")
    }
}
