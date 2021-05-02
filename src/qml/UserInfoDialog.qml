/*
 * SPDX-FileCopyrightText: 2020 Red Hat Inc
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2020 Jan Grulich <jgrulich@redhat.com>
 */


import QtQuick 2.12
import QtQuick.Controls 2.12 as QQC2
import QtQuick.Layouts 1.12
import org.kde.plasma.core 2.0
import org.kde.kirigami 2.9 as Kirigami

Item {
    id: root

    signal accepted()
    signal rejected()

    Rectangle {
        id: background
        anchors.fill: parent
        color: Kirigami.Theme.backgroundColor
    }


    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            Kirigami.Icon {
                height: units.iconSizes.enormous
                width: units.iconSizes.enormous
                source: image
            }

            ColumnLayout {
                Kirigami.Heading {
                    id: nameText
                    Layout.fillWidth: true
                    text: name
                }
                Kirigami.Heading {
                    id: idText
                    Layout.fillWidth: true
                    level: 3
                    text: id
                }
            }
        }

        QQC2.Label {
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            text: i18n("Share your personal information with the requesting application?")
            wrapMode: Text.WordWrap
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: reason
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.alignment: Qt.AlignBottom | Qt.AlignRight

            QQC2.Button {
                text: i18n("Share")

                onClicked: accepted()
            }
            QQC2.Button {
                text: i18n("Cancel")

                onClicked: rejected()
            }
        }
    }
}
