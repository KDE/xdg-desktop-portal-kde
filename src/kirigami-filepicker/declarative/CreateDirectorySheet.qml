// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import org.kde.kirigamifilepicker

Kirigami.PromptDialog {
    id: sheet

    property string parentPath

    title: i18n("Create New Folder")

    standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel

    onAccepted: {
        DirModelUtils.mkdir(parentPath + "/" + nameField.text);
        sheet.close();
    }
    onRejected: {
        nameField.clear();
        sheet.close();
    }

    QQC2.TextField {
        id: nameField
        placeholderText: i18n("Folder name")
    }
}
