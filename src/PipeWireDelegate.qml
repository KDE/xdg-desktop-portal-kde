/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.pipewire as PipeWire
import org.kde.taskmanager as TaskManager
import org.kde.kitemmodels as ItemModels
import org.kde.dbusaddons

import org.kde.xdgdesktopportal

PipeWireCard {
    id: card

    required property var nodeId
    required property string itemName
    required property rect screenGeometry
    required property bool firstUnevenDelegate
    readonly property bool ultraWide: aspectRatio >= 22 / 9.0 // should be 16 / 9.0 but some screens are wide, but not ultra wide
    property real aspectRatio: screenGeometry.width / screenGeometry.height
    // The percentage by which the preview will be scaled relative to width
    // TODO check if this is actually a good idea. It cuts into the Share label when doing this
    property real previewWidthDivisor: state == "wide" ? 0.5 : 2.0
    // The wallaper to render. Obtained via dbus automatically.
    property string wallpaper

    QtObject {
        id: plasmashell

        property DBusInterface iface: DBusInterface {
            name: "org.kde.plasmashell"
            path: "/PlasmaShell"
            iface: "org.kde.PlasmaShell"
            proxy: plasmashell
            bus: "session"
        }
    }

    Component.onCompleted: {
        // FIXME hacky hacky! Probably need a c++ object to resolve this and route resolution through kpackage or something
        const promise = new Promise((resolve, reject) => { plasmashell.iface.asyncCall("wallpaper", "u", [screenIndex], resolve, reject) }).then((reply) => {
            console.log("Wallpaper set to: " + reply.Image)
            if (!reply.Image) {
                wallpaper = "file:///usr/share/wallpapers/Next/contents/images/5120x2880.png"
            } else if (reply.Image.endsWith("/")) {
                wallpaper = reply.Image + "contents/images/5120x2880.png"
            } else {
                wallpaper = reply.Image
            }
        }).catch((e) => {
            console.error("Failed to set wallpaper: " + e)
            fail(e)
        })
    }

    required property int screenIndex

    topPadding: 0
    leftPadding: Kirigami.Units.largeSpacing * 4
    bottomPadding: Kirigami.Units.largeSpacing * 2
    rightPadding: Kirigami.Units.largeSpacing * 4

    topInset: 1
    leftInset: 1
    bottomInset: 1
    rightInset: 1

    component Labels: ColumnLayout {
        Kirigami.Heading {
            Layout.fillWidth: true

            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            elide: Text.ElideMiddle
            verticalAlignment: Text.AlignTop
            maximumLineCount: 2
            lineHeight: lineCount > 1 ? 1.0 : 2.0
            text: i18nc("@title %1 refers to a screen name", "Share “%1”", card.itemName)
        }

        RowLayout {
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
                screenGeometry: card.screenGeometry
                sortMode: TaskManager.TasksModel.SortLastActivated
            }

            Repeater {
                model: ItemModels.KSortFilterProxyModel {
                    sourceModel: tasksLayout.tasksModel
                    filterRowCallback: row => row < tasksLayout.cutoff
                }
                delegate: Kirigami.Icon {
                    required property var modelData
                    implicitHeight: Kirigami.Units.gridUnit
                    width: height
                    source: ServiceAddon.iconFromAppId(modelData.AppId)
                }
            }

            QQC2.Label {
                visible: text.length > 0
                color: Kirigami.Theme.disabledTextColor
                text: {
                    if (tasksLayout.count === 0) {
                        return i18nc("@info:status", "No windows open")
                    }
                    if (!tasksLayout.hasOverflow) {
                        return ""
                    }
                    return i18nc("%1 is a number of elided icon entries", "+%1", tasksLayout.remainder)
                }
            }
        }
    }

    contentItem: GridLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        columns: card.state == "wide" ? 2 : 1
        layoutDirection: card.state == "wide" ? Qt.RightToLeft : Qt.LeftToRight

        rowSpacing: Kirigami.Units.smallSpacing
        columnSpacing: Kirigami.Units.smallSpacing

        Item {
            Layout.preferredHeight: card.implicitHeight
            Layout.preferredWidth: card.implicitWidth / card.previewWidthDivisor
            Layout.fillWidth: true
            Layout.fillHeight: true

            // FIXME something about the shadow texture causes extreme blurryness to occur
            PipeWire.PipeWireSourceItem {
                id: pipeWireSourceItem
                anchors.fill: parent
                nodeId: card.nodeId
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
                border.color: Kirigami.Theme.alternateBackgroundColor
                border.width: 1
                source: textureSource
            }
        }

        Labels {}
    }

    states: [
        State {
            name: "wide"
            when: card.firstUnevenDelegate || card.ultraWide
        },
        State {
            name: ""
        }
    ]
}
