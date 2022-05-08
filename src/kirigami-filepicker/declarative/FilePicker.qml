// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2 as Controls
import org.kde.kirigami 2.4 as Kirigami

import org.kde.kirigamifilepicker 0.1

/**
 * The FilePicker type provides a file picker wrapped in a Kirigmi.Page.
 * It can be directly pushed to the pageStack.
 */
Kirigami.ScrollablePage {
    id: root

    property bool selectMultiple
    property bool selectExisting
    property var nameFilters: []
    property var mimeTypeFilters: []
    property alias folder: dirModel.folder
    property bool showHiddenFiles: false
    property string currentFile
    property string acceptLabel
    property bool selectFolder

    property alias createDirectorySheet: createDirectorySheet

    CreateDirectorySheet {
        id: createDirectorySheet
        parentPath: dirModel.folder
    }

    onCurrentFileChanged: {
        if (root.currentFile) {
            // Switch into directory of preselected file
            fileNameField.text = DirModelUtils.fileNameOfUrl(root.currentFile)
            dirModel.folder = DirModelUtils.directoryOfUrl(root.currentFile)
        }
    }

    // result
    property var fileUrls: []

    signal accepted(var urls)

    function addOrRemoveUrl(url) {
        var index = root.fileUrls.indexOf(url)
        if (index > -1) {
            // remove element
            root.fileUrls.splice(index, 1)
        } else {
            root.fileUrls.push(url)
        }
        root.fileUrlsChanged()
    }

    header: ColumnLayout {
        spacing: 0
        Controls.ToolBar {
            Layout.fillWidth: true
            Row {
                Controls.ToolButton {
                    icon.name: "folder-root-symbolic"
                    height: parent.height
                    width: height
                    onClicked: dirModel.folder = "file:///"
                }
                Repeater {
                    model: DirModelUtils.getUrlParts(dirModel.folder)

                    Controls.ToolButton {
                        icon.name: "arrow-right"
                        text: modelData
                        onClicked: dirModel.folder = DirModelUtils.partialUrlForIndex(
                                        dirModel.folder, index)
                    }
                }
            }
        }
        Kirigami.InlineMessage {
            id: errorMessage
            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            text: dirModel.lastError
            showCloseButton: true
        }
    }
    footer: Controls.ToolBar {
        visible: !root.selectExisting
        height: visible ? Kirigami.Units.gridUnit * 2 : 0

        RowLayout {
            anchors.fill: parent
            Controls.TextField {
                Layout.fillHeight: true
                Layout.fillWidth: true
                id: fileNameField
                placeholderText: i18n("File name")
            }
            Controls.ToolButton {
                Layout.fillHeight: true
                icon.name: "dialog-ok-apply"
                onClicked: {
                    root.fileUrls = [dirModel.folder + "/" + fileNameField.text]
                    root.accepted(root.fileUrls)
                }
            }
        }
    }

    mainAction: Kirigami.Action {
        visible: (root.selectMultiple || root.selectFolder) && root.selectExisting
        text: root.acceptLabel ? root.acceptLabel : i18n("Select")
        icon.name: "dialog-ok"

        onTriggered: {
            if (root.selectFolder) {
                root.accepted([dirModel.folder])
            } else {
                root.accepted(root.fileUrls)
            }
        }
    }

    DirModel {
        id: dirModel
        showDotFiles: root.showHiddenFiles
        mimeFilters: root.mimeTypeFilters
        onLastErrorChanged: errorMessage.visible = true
        onFolderChanged: errorMessage.visible = false
        thumbnailSize: Qt.size(Kirigami.Units.iconSizes.huge, Kirigami.Units.iconSizes.huge)
    }

    Controls.BusyIndicator {
        anchors.centerIn: parent

        width: Kirigami.Units.gridUnit * 4
        height: width

        visible: dirModel.isLoading
    }

    GridView {
        model: dirModel
        clip: true
        reuseItems: true

        delegate: Kirigami.AbstractCard {
            required property string name
            required property string iconName
            required property url url
            required property bool isDir
            required property var thumbnail

            width: 95
            height: 95

            contentItem: Item {
                Kirigami.Icon {
                    anchors.centerIn: parent
                    source: thumbnail ? thumbnail : iconName

                    width: Kirigami.Units.iconSizes.huge
                    height: width
                }

                Kirigami.Icon {
                    source: "emblem-checked"
                    visible: checked
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    width: parent.width / 3
                    height: parent.height / 3
                }
            }

            checkable: root.selectExisting && root.selectMultiple
            checked: root.fileUrls.includes(url)
            highlighted: false

            footer: Kirigami.Heading {
                text: name
                level: 3
                elide: Qt.ElideRight
            }

            onClicked: {
                // open
                if (root.selectExisting) {
                    // The delegate being clicked on represents a directory
                    if (isDir) {
                        // Change into folder
                        dirModel.folder = url
                    }
                    // The delegate represents a file
                    else {
                        if (root.selectMultiple) {
                            // add the file to the list of accepted files
                            // (or remove it if it is already there)
                            root.addOrRemoveUrl(url)
                        } else {
                            // If we only want to select one file,
                            // Write it into the output variable and close the dialog
                            root.fileUrls = [url]
                            root.accepted(root.fileUrls)
                        }
                    }
                }
                // save
                else {
                    if (isDir) {
                        dirModel.folder = url
                    } else {
                        fileNameField.text = name
                    }
                }
            }
        }
    }
}
