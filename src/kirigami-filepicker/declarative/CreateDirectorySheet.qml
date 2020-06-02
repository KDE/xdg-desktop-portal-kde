// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2 as Controls

import org.kde.kirigami 2.5 as Kirigami
import org.kde.kirigamifilepicker 0.1

Kirigami.OverlaySheet {
    id: sheet
    property string parentPath: ""

    header: Kirigami.Heading {
        text: i18n("Create new folder")
    }

    ColumnLayout {
        Controls.Label {
            Layout.fillWidth: true

            wrapMode: Controls.Label.WordWrap

            text: i18n("Create new folder in %1", sheet.parentPath.replace("file://", ""))
        }

        Controls.TextField {
            id: nameField
            Layout.fillWidth: true

            placeholderText: i18n("folder name")
        }
        RowLayout {
            Layout.fillWidth: true
            Controls.Button {
                Layout.alignment: Qt.AlignLeft
                Layout.fillWidth: true

                text: i18n("Ok")

                onClicked: {
                    DirModelUtils.mkdir(parentPath + "/" + nameField.text)
                    sheet.close()
                }
            }
            Controls.Button {
                Layout.alignment: Qt.AlignRight
                Layout.fillWidth: true

                text: i18n("Cancel")

                onClicked: {
                    nameField.clear()
                    sheet.close()
                }
            }
        }
    }
}
