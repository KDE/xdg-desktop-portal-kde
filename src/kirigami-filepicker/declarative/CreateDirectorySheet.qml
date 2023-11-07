// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls

import org.kde.kirigami as Kirigami
import org.kde.kirigamifilepicker

Kirigami.OverlaySheet {
    id: sheet
    property string parentPath: ""

    header: Kirigami.Heading {
        text: i18n("Create New Folder")
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

            placeholderText: i18n("Folder name")
        }
        RowLayout {
            Layout.fillWidth: true
            Controls.Button {
                Layout.alignment: Qt.AlignLeft
                Layout.fillWidth: true

                text: i18n("OK")

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
