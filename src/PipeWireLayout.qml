// SPDX-License-Identifier: LGPL-2.0-or-later
// SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

pragma ComponentBehavior: Bound

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.taskmanager as TaskManager

Kirigami.CardsLayout {
    id: root

    required property ScreenChooserDialogTemplate dialog
    required property var model

    readonly property alias view: view

    uniformCellWidths: true

    Repeater {
        id: view

        model: root.model

        PipeWireDelegate {
            id: delegate

            required property int index
            required property var model

            function selectAndAccept(): void {
                root.dialog.clearSelection()
                view.model.setData(view.model.index(model.row, 0), Qt.Checked, Qt.CheckStateRole)
                root.dialog.dialogButtonBox.accepted()
            }

            checkable: root.dialog.multiple
            itemName : model.display ?? ""
            iconSource : model.decoration ?? ""
            itemDescription : model.description ?? ""
            synthetic: model.isSynthetic ?? false
            exclusive: false
            autoExclusive: exclusive
            checked: model.checked === Qt.Checked
            nodeId: waylandItem.nodeId

            activeFocusOnTab: true
            highlighted: activeFocus

            Accessible.role: root.dialog.multiple ? Accessible.CheckBox : Accessible.Button

            TaskManager.ScreencastingRequest {
                id: waylandItem
                outputName: delegate.model.name
                uuid: delegate.model.Uuid
            }

            // Only active if this is a multi-select dialog
            onToggled: {
                const to = model.checked !== Qt.Checked ? Qt.Checked : Qt.Unchecked;
                view.model.setData(view.model.index(model.row, 0), to, Qt.CheckStateRole)
            }

            // If this is isn't a multi-select dialog, accept on click
            // since the cards are functioning as buttons
            onClicked: {
                if (!root.dialog.multiple) {
                    selectAndAccept()
                }
            }

            // If this is a multi-select dialog, let people choose just
            // one thing quickly by double-clicking
            onDoubleClicked: {
                if (root.dialog.multiple) {
                    selectAndAccept()
                }
            }
        }
    }
}
