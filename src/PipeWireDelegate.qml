// SPDX-License-Identifier: LGPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
// SPDX-FileCopyrightText: 2025-2026 Harald Sitter <sitter@kde.org>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.pipewire as PipeWire
import org.kde.taskmanager as TaskManager
import org.kde.kitemmodels as ItemModels
import org.kde.ki18n

Kirigami.AbstractCard {
    id: root

    required property string itemName
    required property var iconSource
    required property string itemDescription
    // Whether or not this is a synthetic "fake" output (e.g. an action to create a region)
    required property bool synthetic
    required property int nodeId
    required property bool exclusive
    /*! The count of synthetic outputs in the model this delegate belongs to */
    required property int syntheticCount
    /*! Whether this delegate is presenting an output (i.e. screen) rather than a window */
    required property bool isOutput
    /*! The geometry of the output or window. Primarily for mapping tasks to outputs. */
    required property rect geometry
    /*! The background image to use for the card. Should only be set for outputs! Applied via onCompleted. */
    required property url backgroundImage

    function selectAndAccept(): void {
        // To be implemented by the user of the delegate. Depends entirely on context (dialog, model, etc).
        throw "Not implemented"
    }

    Accessible.name: itemName
    hoverEnabled: true
    showClickFeedback: true

    Component {
        id: outputBackgroundComponent

        Item {
            // A blurred translucent wallpaper as primary background.
            Kirigami.ShadowedImage {
                id: wallpaperImage

                anchors.fill: parent
                radius: Kirigami.Units.cornerRadius

                fillMode: Image.PreserveAspectCrop
                source: root.backgroundImage

                opacity: root.hovered ? 0.30 : 0.20

                layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
                layer.effect: MultiEffect {
                    blurEnabled: true
                    blur: 1.0
                    blurMax: 32
                }
            }

            // Above the wallpaper is a rectangle with just a border. The border of the wallpaper would get blurred too
            // if these were combined!
            Kirigami.ShadowedImage {
                anchors.fill: wallpaperImage
                radius: wallpaperImage.radius

                shadow.size: 1
                border.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, Kirigami.Theme.frameContrast)
                border.width: 1
                color: "transparent"
            }
        }
    }

    header: GridLayout {
        columnSpacing: Kirigami.Units.smallSpacing
        rowSpacing: 0
        columns: 3

        Kirigami.Icon {
            implicitWidth: Kirigami.Units.iconSizes.sizeForLabels
            implicitHeight: implicitWidth
            source: root.iconSource
        }
        QQC2.Label {
            font.bold: true
            elide: Text.ElideRight
            text: root.itemName
            Layout.fillWidth: true
        }
        Loader {
            id: checkboxLoader
            active: root.checkable

            Component {
                id: checkboxComponent
                QQC2.CheckBox {
                    checked: root.checked
                    onToggled: root.checked = checked
                }
            }

            Component {
                id: radioComponent
                QQC2.RadioButton {
                    checked: root.checked
                    onToggled: root.checked = checked
                }
            }

            sourceComponent: root.exclusive ? radioComponent : checkboxComponent
        }

        Item { // spacer
            visible: descriptionLabel.visible
        }
        QQC2.Label {
            id: descriptionLabel
            Layout.columnSpan: 2
            Layout.fillWidth: true
            text: root.itemDescription
            wrapMode: Text.Wrap
            visible: text.length > 0
            opacity: 0.75 // in lieu of a semantic subtitle color
        }
    }

    Component {
        id: preview

        Item {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 8
            Layout.preferredHeight: Kirigami.Units.gridUnit * 8
            Layout.fillWidth: true
            Layout.fillHeight: true

            PipeWire.PipeWireSourceItem {
                id: pipeWireSourceItem
                anchors.fill: parent
                nodeId: root.nodeId
            }

            ShaderEffectSource {
                id: textureSource
                sourceItem: pipeWireSourceItem
                sourceRect: pipeWireSourceItem.paintedRect
                hideSource: true
            }

            Kirigami.ShadowedTexture {
                anchors.centerIn: parent
                width: pipeWireSourceItem.paintedRect.width
                height: pipeWireSourceItem.paintedRect.height
                radius: Kirigami.Units.cornerRadius
                shadow.size: 2
                color: Kirigami.Theme.textColor
                border.color: Kirigami.Theme.textColor
                border.width: 0
                source: textureSource
            }

            Kirigami.Icon {
                anchors.fill: parent
                visible: pipeWireSourceItem.nodeId === 0
                source: root.iconSource
            }
        }
    }

    Component {
        id: outputItem

        ColumnLayout {
            component Tasks: RowLayout {
                id: tasksLayout

                readonly property int cutoff: 4
                readonly property int count: tasksModel.count
                readonly property int hasOverflow: tasksModel.count > cutoff
                readonly property int remainder: Math.max(0, tasksModel.count - cutoff)

                Layout.alignment: Qt.AlignHCenter
                spacing: 0
                implicitHeight: Kirigami.Units.gridUnit

                property TaskManager.TasksModel tasksModel: TaskManager.TasksModel {
                    filterByScreen: true
                    screenGeometry: root.geometry
                    sortMode: TaskManager.TasksModel.SortLastActivated
                }

                Repeater {
                    model: ItemModels.KSortFilterProxyModel {
                        sourceModel: tasksLayout.tasksModel
                        filterRowCallback: row => row < tasksLayout.cutoff
                    }
                    delegate: Kirigami.Icon {
                        required property var decoration
                        implicitHeight: Kirigami.Units.gridUnit
                        width: height
                        source: decoration
                    }
                }

                QQC2.Label {
                    visible: text.length > 0
                    color: Kirigami.Theme.disabledTextColor
                    text: {
                        if (tasksLayout.count === 0) {
                            return KI18n.i18nc("@info:status", "No windows open")
                        }
                        if (!tasksLayout.hasOverflow) {
                            return ""
                        }
                        return KI18n.i18nc("%1 is a number of elided icon entries", "+%1", tasksLayout.remainder)
                    }
                }
            }

            component Heading : Kirigami.Heading {
                level: 2
                wrapMode: Text.Wrap
                elide: Text.ElideMiddle
                verticalAlignment: Text.AlignTop
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 2
                lineHeight: lineCount > 1 ? 1.0 : 2.0 // if we only have one line still pretend it is two lines!
                text: KI18n.i18nc("@title %1 refers to a screen name", "Share “%1”", root.itemName)
            }

            Layout.fillWidth: true
            Layout.fillHeight: true

            spacing: Kirigami.Units.smallSpacing

            // We may end up using a row layout when space permits so this is built using components even though
            // technically not necessary.
            ColumnLayout {
                Heading {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                }
                Tasks {}
            }

            Loader {
                property Item itemAsItem: item as Item // item confusingly is a QtObject, cast it to Item.

                Layout.preferredWidth: itemAsItem.Layout.preferredWidth
                Layout.preferredHeight: itemAsItem.Layout.preferredHeight
                Layout.fillWidth: itemAsItem.Layout.fillWidth
                Layout.fillHeight: itemAsItem.Layout.fillHeight

                sourceComponent: preview
            }
        }
    }

    // A synthetic delegate has no preview and as such has no contentItem and is smaller than a normal one.
    // Try to squeeze them all in one cell. Should be fine, we scale the others if need be through the uniformity rule.
    // Conversely the big delegates are spanning multiple rows of synthetic delegates.
    Layout.rowSpan: synthetic ? 1 : Math.max(syntheticCount, 1)
    contentItem: {
        if (synthetic) {
            return null
        }
        if (isOutput) {
            header.visible = false
            return outputItem.createObject(null) as Item
        }
        return preview.createObject(null) as Item
    }

    Layout.preferredHeight: contentItem?.Layout?.preferredHeight ?? -1

    Component.onCompleted: {
        // Awkwardly apply a conditional background. By default we want the default impl for the card background
        // for highlighting logic etc. But! When we have a background image we want to use that instead.
        // So we store the default background and then conditionally apply one or the other.
        const defaultBackground = background
        background = Qt.binding(function () {
            if (isOutput && !synthetic && backgroundImage && backgroundImage !== "") {
                return outputBackgroundComponent.createObject(null) as Item
            }
            return defaultBackground
        })
    }
}
