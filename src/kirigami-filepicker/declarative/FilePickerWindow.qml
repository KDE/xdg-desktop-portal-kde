// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick 2.7
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.5 as Kirigami
import org.kde.kirigamifilepicker 0.1

/**
 * The FilePickerWindow type is used by the C++ MobileFileDialog class.
 * It should not be used from QML,
 * its only purpose is to create an integration with C++ using its FileChooserCallback.
 */
Kirigami.ApplicationWindow {
    id: root
    title: callback.title
    visible: false

    globalDrawer: PlacesGlobalDrawer {
        onPlaceOpenRequested: {
            filePicker.folder = place;
            close()
        }
    }

    contextDrawer: Kirigami.ContextDrawer {}

    onVisibleChanged: {
        // File picker was opened
        if (root.visible) {
            // reset old data
            filePicker.fileUrls = []
        }
    }

    onClosing: (close) => {
        close.accepted = false

        // Always make sure to exit the while loop in the filechooser portal
        callback.cancel()
        close.accepted = true
    }

    FileChooserCallback {
        id: callback
        objectName: "callback"

        Component.onCompleted: console.log(JSON.stringify(callback))
    }

    Connections {
        target: filePicker

        onAccepted: (urls) => {
            callback.accepted(urls)
        }
    }


    pageStack.initialPage: FilePicker {
        id: filePicker
        selectMultiple: callback.selectMultiple
        selectExisting: callback.selectExisting
        nameFilters: callback.nameFilters
        mimeTypeFilters: callback.mimeTypeFilters
        currentFile: callback.currentFile
        acceptLabel: callback.acceptLabel
        selectFolder: callback.selectFolder

        Component.onCompleted: {
            // set conditional properties
            if (callback.folder) {
                console.log("Initial folder set")
                filePicker.folder = callback.folder
            } else {
                console.log("Initial folder not set")
            }

            if (callback.title) {
                console.log("Custom title set")
                filePicker.title = callback.title
            } else {
                console.log("Custom title not set")
            }
        }
    }
}
