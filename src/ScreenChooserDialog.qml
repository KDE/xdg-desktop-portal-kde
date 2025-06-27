/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.workspace.dialogs as PWD
import org.kde.taskmanager 0.1 as TaskManager
import org.kde.kitemmodels as KItemModels

PWD.SystemDialog {
    id: root

    property alias outputsModel: outputsView.model
    required property var windowsModel
    property bool multiple: false
    property alias allowRestore: allowRestoreItem.checked
    required property string applicationName

    readonly property int contentHeight: header.height + scrollView.contentHeight + footer.height + Kirigami.Units.largeSpacing // spacing is to give us some leeway, no particular reason
    readonly property int defaultHeight: Math.min(Math.round(Screen.height * 0.8), contentHeight)
    readonly property list<rect> geometryFilters: {
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
    readonly property bool hasFilters: geometryFilters.length > 0 || searchText.length > 0

    signal clearSelection()
    signal invalidateFilter()

    component OutputChip: Kirigami.Chip {
        required property var model
        required property rect geometry
    }

    onSearchTextChanged: invalidateFilter()
    onInvalidateFilter: {
        console.log("Invalidating filter")
        windowsView.model.invalidateFilter()
    }

    acceptable: (outputsModel && outputsModel.hasSelection) || (windowsModel && windowsModel.hasSelection)
    onVisibleChanged: Qt.callLater(() => {
        // TODO make this hack go away!
        // SystemDialog is way too opinionated about sizing. Break its bindings for width lest we get jumping window sizes.
        width = width + Kirigami.Units.largeSpacing * 4 // the spacing is to account for a potential scrollbar. super hacky
        height = defaultHeight
    })
    minimumHeight: 0

    header: QQC2.Control {
        Layout.fillWidth: true

        topPadding: Kirigami.Units.largeSpacing * 2
        bottomPadding: Kirigami.Units.largeSpacing * 2
        leftPadding: Kirigami.Units.largeSpacing * 2
        rightPadding: Kirigami.Units.largeSpacing * 2

        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        background: Rectangle {
            Kirigami.Theme.colorSet: Kirigami.Theme.Window
            color: Kirigami.Theme.alternateBackgroundColor
        }

        contentItem: ColumnLayout {
            Kirigami.Heading {
                Layout.fillWidth: true
                text: i18nc("@info:title", "Choose what to share with %1", root.applicationName)
            }
            RowLayout {
                Layout.fillWidth: true

                Flow {
                    spacing: Kirigami.Units.largeSpacing
                    Layout.fillWidth: true

                    Repeater {
                        id: chipsRepeater
                        model: outputsView.count > 0 ? root.outputsModel : undefined
                        delegate: OutputChip {
                            text: model.display
                            closable: false
                        }
                    }
                }

                Kirigami.SearchField {
                    id: searchField
                    Layout.alignment: Qt.AlignRight
                    placeholderText: i18nc("placeholder text for searchfield", "Search…")
                    Component.onCompleted: Qt.callLater(forceActiveFocus) // focus by default for easy searching
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            // FIXME TODO activate!
                        }
                    }
                }

            }
        }
    }

    QQC2.ScrollView {
        id: scrollView
        anchors.fill: parent
        clip: true

        RowLayout {
            spacing: 0
            Item { // scoots the content layout to the right. I don't see a better way to do this :(
                implicitWidth: (scrollView.width - contentLayout.width) / 2
            }

            ColumnLayout {
                id: contentLayout

                Layout.minimumWidth: cardWidthWithSpacing * 2

                readonly property int cardHeight: Kirigami.Units.gridUnit * 12
                readonly property int cardWidth: Kirigami.Units.gridUnit * 24
                readonly property int cardWidthWithSpacing: cardWidth + layoutSpacing * 2
                readonly property int cardsPerRow: Math.max(2, Math.floor(scrollView.width / cardWidthWithSpacing))
                readonly property int layoutSpacing: Kirigami.Units.largeSpacing * 2
                readonly property int maxColumns: 4

                GridLayout {
                    id: outputsLayout

                    // Don't let the action cards reflow into a output row when the row is too short
                    // (i.e. we have fewer outputs than the cardsPerRow allows)
                    // But also don't have less than 2 columns because we need them for the action cards :/
                    property int neededColumns: Math.min(outputsView.count - outputsView.wideCount, contentLayout.cardsPerRow)
                    property int maxColumns: Math.min(neededColumns, contentLayout.maxColumns)

                    columns: Math.max(2, maxColumns)
                    rowSpacing: contentLayout.layoutSpacing
                    columnSpacing: contentLayout.layoutSpacing
                    visible: !root.hasFilters

                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: false
                    Layout.margins: contentLayout.layoutSpacing

                    Repeater {
                        id: outputsView
                        model: null
                        readonly property int wideCount: {
                            let count = 0
                            for (let i = 0; i < outputsView.count; ++i) {
                                if (outputsView.itemAt(i).state === "wide") {
                                    count++
                                }
                            }
                            return count
                        }
                        PipeWireDelegate {
                            id: outputDelegate

                            required property int index
                            required property var model

                            property TaskManager.ScreencastingRequest waylandItem: TaskManager.ScreencastingRequest {
                                outputName: outputDelegate.model.name
                            }

                            Layout.columnSpan: state === "wide" ? 2 : 1

                            implicitWidth: contentLayout.cardWidth
                            implicitHeight: contentLayout.cardHeight
                            hoverEnabled: true

                            nodeId: waylandItem.nodeId
                            screenIndex: model.index
                            screenGeometry: model.screen.geometry
                            itemName: model.display
                            firstUnevenDelegate: outputsView.count === 1 || (outputsView.count % 2 && index === 0)

                            activeFocusOnTab: true
                        }
                    }

                    Kirigami.AbstractCard {
                        implicitWidth: contentLayout.cardWidth
                        hoverEnabled: true
                        contentItem: GridLayout {
                            columnSpacing: 0
                            rowSpacing: 0
                            columns: 2

                            Kirigami.Icon {
                                implicitHeight: Kirigami.Units.iconSizes.sizeForLabels
                                implicitWidth: implicitHeight
                                source: "transform-crop-symbolic"
                            }
                            QQC2.Label  {
                                font.bold: true
                                text: i18nc("@title", "Share region")
                            }

                            Item{} // spacer
                            QQC2.Label {
                                text: i18nc("@info", "Crops a specific area of your screens")
                                wrapMode: Text.Wrap
                                color: Kirigami.Theme.disabledTextColor
                            }
                        }
                    }

                    Kirigami.AbstractCard {
                        implicitWidth: contentLayout.cardWidth
                        hoverEnabled: true
                        contentItem: GridLayout {
                            columnSpacing: 0
                            rowSpacing: 0
                            columns: 2

                            Kirigami.Icon {
                                implicitHeight: Kirigami.Units.iconSizes.sizeForLabels
                                implicitWidth: implicitHeight
                                source: "window-duplicate-symbolic"
                            }
                            QQC2.Label {
                                font.bold: true
                                text: i18nc("@title", "Share virtual screen")
                            }

                            Item{} // spacer
                            QQC2.Label {
                                text: i18nc("@info", "Creates a screen inside a window, then share")
                                wrapMode: Text.Wrap
                                color: Kirigami.Theme.disabledTextColor
                            }
                        }
                    }
                }

                RowLayout {
                    visible: windowsView.count > 0 && outputsView.visible

                    Layout.fillWidth: true
                    Layout.margins: contentLayout.layoutSpacing

                    Kirigami.Separator {
                        Layout.fillWidth: true
                    }

                    QQC2.Label {
                        font.bold: true
                        text: i18nc("@label separator line label between screen selection and window selection", "or pick a specific window")
                    }

                    Kirigami.Separator {
                        Layout.fillWidth: true
                    }
                }

                GridLayout {
                    property int neededColumns: Math.min(windowsView.count, contentLayout.cardsPerRow)
                    property int maxColumns: Math.min(neededColumns, contentLayout.maxColumns)

                    columns: maxColumns
                    rowSpacing: contentLayout.layoutSpacing
                    columnSpacing: contentLayout.layoutSpacing

                    visible: windowsView.count > 0

                    Layout.fillWidth: true
                    Layout.margins: contentLayout.layoutSpacing

                    Repeater {
                        id: windowsView
                        model: KItemModels.KSortFilterProxyModel {
                            sourceModel: root.windowsModel
                            filterRowCallback: (row) => {
                                const index = sourceModel.index(row, 0)

                                // Are output filters selected?
                                if (root.geometryFilters.length > 0) {
                                    let every = root.geometryFilters.every((outputGeometry) => {
                                        return sourceModel.geometryIntersects(index, outputGeometry)
                                    })
                                    if (!every) {
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
                        PipeWireDelegateWindow {
                            id: windowDelegate

                            required property int index
                            required property var model

                            property TaskManager.ScreencastingRequest waylandItem: TaskManager.ScreencastingRequest {
                                uuid: windowDelegate.model.Uuid
                            }

                            implicitWidth: contentLayout.cardWidth
                            implicitHeight: contentLayout.cardHeight
                            hoverEnabled: true

                            nodeId: waylandItem.nodeId
                            itemName: model.display
                            iconSource: model.decoration

                            activeFocusOnTab: true
                        }
                    }
                }
            }
        }
    }

    footer: QQC2.DialogButtonBox {
        id: dialogButtonBox

        Layout.fillWidth: true

        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        background: Rectangle {
            Kirigami.Theme.colorSet: Kirigami.Theme.Window
            color: Kirigami.Theme.alternateBackgroundColor
        }

        visible: count > 0

        onAccepted: root.accept()
        onRejected: root.reject()

        QQC2.CheckBox {
            id: allowRestoreItem
            checked: true
            text: i18nc("@option:check", "Allow restoring on future sessions")
        }

        Repeater {
            model: root.actions

            delegate: QQC2.Button {
                required property QQC2.Action modelData
                action: modelData
            }
        }

        standardButtons: root.multiple ? QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel : QQC2.DialogButtonBox.NoButton
    }

    // The actual SystemdDialog can leave me well alone and not show its buttonbox.
    standardButtons: QQC2.DialogButtonBox.NoButton

    Component.onCompleted: {
        if (root.multiple) {
            dialogButtonBox.standardButton(QQC2.DialogButtonBox.Ok).text = i18n("Share")
        }

        // TODO port to new views
        // If there's only one thing in the list, pre-select it to save the user a click
        // if (outputsView.count === 1 && windowsView.count === 0) {
            // outputsView.model.setData(outputsView.model.index(0, 0), Qt.Checked, Qt.CheckStateRole);
        // } else if (windowsView.count === 1 && outputsView.count === 0) {
            // windowsView.model.setData(outputsView.model.index(0, 0), Qt.Checked, Qt.CheckStateRole);
        // }
    }
}
