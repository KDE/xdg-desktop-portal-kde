/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels

ScreenChooserDialogTemplate {
    id: root

    required property var outputsModel
    required property var windowsModel
    property bool multiple: false
    property alias allowRestore: allowRestoreItem.checked

    /* The geometries of the selected chips */
    readonly property list<rect> geometrySelectors: {
        let chips = []
        for (let i = 0; i < chipsRepeater.count; ++i) {
            const chip = chipsRepeater.itemAt(i) as OutputChip
            if (chip.checked) {
                chips.push(chip.geometry)
            }
        }
        Qt.callLater(() => invalidateFilter())
        return chips
    }
    readonly property alias searchText: searchField.text
    readonly property bool hasFilters: geometrySelectors.length > 0 || searchText.length > 0

    signal invalidateFilter()
    onSearchTextChanged: invalidateFilter()
    onInvalidateFilter: {
        outputsLayout.model.invalidateFilter()
        windowsLayout.model.invalidateFilter()
    }

    component OutputChip: Kirigami.Chip {
        required property var model
        required property rect geometry
    }

    component GeometryFilterModel: KItemModels.KSortFilterProxyModel {
        filterRowCallback: (row) => {
            const index = sourceModel.index(row, 0)

            // Are output filters selected?
            if (root.geometrySelectors.length > 0) {
                const intersectsAllSelectedOutputs = root.geometrySelectors.some((outputGeometry) => {
                    // Unfortunately we can't be type accurate here because our input models are unrelated besides being QAbstractItemModels.
                    // As such the linter doesn't know that both OutputsModel and WindowsModel have geometryIntersects().
                    return sourceModel.geometryIntersects(index, outputGeometry)  // qmllint disable missing-property
                })
                if (!intersectsAllSelectedOutputs) {
                    return false
                }
            }

            // Are we searching?
            if (root.searchText.length > 0) {
                const searchText = root.searchText.toLowerCase()
                const windowText = sourceModel.data(index, Qt.DisplayRole).toLowerCase()
                if (!windowText.includes(searchText)) {
                    return false
                }
            }

            return true
        }
    }

    // Mind that either model may be 'null' when it is not applicable. We need to coalesce into bool.
    acceptable: (outputsModel?.hasSelection ?? false) || (windowsModel?.hasSelection ?? false)
    scrollable: true

    width: Kirigami.Units.gridUnit * 41
    height: Kirigami.Units.gridUnit * 41

    headerItem: ColumnLayout {
        RowLayout {
            Layout.fillWidth: true

            // Make sure the SearchField aligns to the right edge properly even when we have no chips content.
            Item {
                Layout.fillWidth: true
                visible: !chipsFlow.visible
            }

            Flow {
                id: chipsFlow
                spacing: Kirigami.Units.largeSpacing
                Layout.fillWidth: true

                // Confusing when there is only one chip
                visible: chipsRepeater.count > 1

                Repeater {
                    id: chipsRepeater
                    model: KItemModels.KSortFilterProxyModel {
                        sourceModel: root.outputsModel
                        filterRoleName: "isSynthetic"
                        filterString: "false"
                    }
                    delegate: OutputChip {
                        text: model.display
                        closable: false
                    }
                }
            }

            Kirigami.SearchField {
                id: searchField
                Layout.alignment: Qt.AlignRight
                Component.onCompleted: Qt.callLater(forceActiveFocus) // focus by default for easy searching
                Keys.onPressed: (event) => {
                    if (event.key !== Qt.Key_Return && event.key !== Qt.Key_Enter) {
                        return
                    }

                    const output = outputsLayout.view.itemAt(0) as PipeWireDelegate
                    if (output) {
                        event.accepted = true
                        output.selectAndAccept()
                    }

                    const window = windowsLayout.view.itemAt(0) as PipeWireDelegate
                    if (window) {
                        event.accepted = true
                        window.selectAndAccept()
                    }
                }
            }
        }
    }

    ColumnLayout {
        spacing: root.edgeSpacing

        PipeWireLayout {
            id: outputsLayout
            Layout.fillWidth: true
            dialog: root
            model: GeometryFilterModel {
                sourceModel: root.outputsModel
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing
            visible: outputsLayout.view.count > 0 && windowsLayout.view.count > 0

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            QQC2.Label {
                font.bold: true
                text: i18nc("@label separator line label between screen selection and window selection", "Windows")
                wrapMode: Text.WordWrap
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }
        }

        PipeWireLayout {
            id: windowsLayout
            Layout.fillWidth: true
            dialog: root
            model: GeometryFilterModel {
                sourceModel: root.windowsModel
            }
        }
    }

    standardButtons: root.multiple ? QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel : QQC2.DialogButtonBox.NoButton
    dialogButtonBoxLeftItem: QQC2.CheckBox {
        id: allowRestoreItem
        checked: true
        text: i18n("Allow restoring on future sessions")
    }

    Component.onCompleted: {
        if (root.multiple) {
            dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = i18n("Share")
        }

        // If there's only one thing in the list, pre-select it to save the user a click
        if (outputsLayout.view.count === 1 && windowsLayout.view.count === 0) {
            outputsLayout.view.model.setData(outputsLayout.view.model.index(0, 0), Qt.Checked, Qt.CheckStateRole);
        } else if (windowsLayout.view.count === 1 && outputsLayout.view.count === 0) {
            windowsLayout.model.setData(outputsLayout.view.model.index(0, 0), Qt.Checked, Qt.CheckStateRole);
        }
    }
}
