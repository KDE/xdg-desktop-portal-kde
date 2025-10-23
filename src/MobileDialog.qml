/*
 *  SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *  SPDX-FileCopyrightText: 2021-2024 Devin Lin <devin@kde.org>
 *  SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.plasma.lookandfeel
import Qt5Compat.GraphicalEffects

/**
 * Component to create CSD dialogs that come from the system.
 * \deprecated [6.5] Use QtQuick Dialog or similar element instead.
 */
Kirigami.AbstractApplicationWindow {
    id: root

    title: mainText
    /**
     * Main text of the dialog.
     */
    property string mainText: title

    /**
     * Subtitle of the dialog.
     */
    property string subtitle: ""

    /**
     * This property holds the icon used in the dialog.
     */
    property string iconName: ""

    /**
     * This property holds the list of actions for this dialog.
     *
     * Each action will be rendered as a button that the user will be able
     * to click.
     */
    property list<T.Action> actions

    default property Item mainItem

    /**
     * This property holds the QQC2.DialogButtonBox used in the footer of the dialog.
     */
    readonly property QQC2.DialogButtonBox dialogButtonBox: footerButtonBox

    /**
     * Provides dialogButtonBox.standardButtons
     *
     * Useful to be able to set it as dialogButtonBox will be null as the object gets built
     */
    property alias standardButtons: footerButtonBox.standardButtons

    /**
     * Controls whether the accept button is enabled
     */
    property bool acceptable: true

    /**
     * The layout of the action buttons in the footer of the dialog.
     *
     * By default, if there are more than 3 actions, it will have `Qt.Vertical`.
     *
     * Otherwise, with zero to 2 actions, it will have `Qt.Horizontal`.
     *
     * This will only affect mobile dialogs.
     */
    property int /*Qt.Orientation*/ layout: actions.length > 3 ? Qt.Vertical : Qt.Horizontal

    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint

    width: contentLayout.implicitWidth
    height: contentLayout.implicitHeight
    visible: false
    minimumHeight: contentLayout.minimumHeight
    minimumWidth: contentLayout.minimumWidth

    function present() {
        contentLayout.present()
    }
    signal accept()
    signal reject()
    property bool accepted: false
    onAccept: {
        accepted = true
        close()
    }
    onReject: close()

    onVisibleChanged: {
        if (!visible && !accepted) {
            console.log("rejecting")
            root.reject()
        }
        width = Qt.binding(() => contentLayout.implicitWidth)
        height = Qt.binding(() => contentLayout.implicitHeight)
    }

    Binding {
        target: dialogButtonBox?.standardButton(QQC2.DialogButtonBox.Ok) ?? null
        property: "enabled"
        when: dialogButtonBox?.standardButtons & QQC2.DialogButtonBox.Ok
        value: root.acceptable
    }

    Item {
        id: contentLayout

        readonly property real minimumHeight: implicitWidth
        readonly property real minimumWidth: implicitHeight

        anchors.fill: parent

        function present() {
            root.showMaximized();
        }

        onWindowChanged: {
            if (window) {
                window.color = Qt.rgba(0, 0, 0, 0.5);
            }
        }

        Item {
            id: windowItem
            anchors.centerIn: parent
            // margins for shadow
            implicitWidth: Math.min(Screen.width, control.implicitWidth + 2 * Kirigami.Units.gridUnit)
            implicitHeight: Math.min(Screen.height, control.implicitHeight + 2 * Kirigami.Units.gridUnit)

            // shadow
            RectangularGlow {
                id: glow
                anchors.topMargin: 1
                anchors.fill: control
                cached: true
                glowRadius: 2
                cornerRadius: Kirigami.Units.gridUnit
                spread: 0.1
                color: Qt.rgba(0, 0, 0, 0.4)
            }

            // actual window
            QQC2.Control {
                id: control
                anchors.fill: parent
                anchors.margins: glow.cornerRadius
                topPadding: Kirigami.Units.gridUnit
                bottomPadding: Kirigami.Units.gridUnit
                rightPadding: Kirigami.Units.gridUnit
                leftPadding: Kirigami.Units.gridUnit

                implicitWidth: Kirigami.Units.gridUnit * 22

                background: Item {
                    Rectangle { // border
                        anchors.fill: parent
                        anchors.margins: -1
                        radius: Kirigami.Units.largeSpacing + 1
                        color: Qt.darker(Kirigami.Theme.backgroundColor, 1.5)
                    }
                    Rectangle { // background colour
                        anchors.fill: parent
                        radius: Kirigami.Units.largeSpacing
                        color: Kirigami.Theme.backgroundColor
                    }
                }

                contentItem: ColumnLayout {
                    id: column
                    spacing: 0

                    // header
                    Kirigami.Heading {
                        Layout.fillWidth: true
                        Layout.maximumWidth: root.maximumWidth
                        level: 3
                        font.weight: Font.Bold
                        text: root.mainText
                        wrapMode: Text.Wrap
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                    }

                    QQC2.Label {
                        Layout.topMargin: Kirigami.Units.largeSpacing
                        Layout.bottomMargin: Kirigami.Units.largeSpacing
                        Layout.maximumWidth: root.maximumWidth
                        Layout.fillWidth: true
                        text: root.subtitle
                        visible: text.length > 0
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Kirigami.Icon {
                        Layout.topMargin: Kirigami.Units.largeSpacing
                        Layout.alignment: Qt.AlignCenter
                        source: root.iconName
                        implicitWidth: Kirigami.Units.iconSizes.large
                        implicitHeight: Kirigami.Units.iconSizes.large
                    }

                    // content
                    QQC2.Control {
                        id: content

                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        Layout.topMargin: Kirigami.Units.gridUnit
                        Layout.maximumWidth: root.maximumWidth

                        leftPadding: 0
                        rightPadding: 0
                        topPadding: 0
                        bottomPadding: 0

                        contentItem: root.mainItem
                        background: Item {}
                    }

                    QQC2.DialogButtonBox {
                        id: footerButtonBox
                        // ensure we never have no buttons, we always must have the cancel button available
                        standardButtons: (root.standardButtons === QQC2.DialogButtonBox.NoButton) ? QQC2.DialogButtonBox.Cancel : root.standardButtons

                        Layout.topMargin: Kirigami.Units.largeSpacing
                        Layout.fillWidth: true
                        Layout.maximumWidth: root.maximumWidth
                        leftPadding: 0
                        rightPadding: 0
                        topPadding: 0
                        bottomPadding: 0

                        onAccepted: root.accept()
                        onRejected: root.reject()

                        Repeater {
                            model: root.actions
                            delegate: QQC2.Button {
                                action: modelData
                            }
                        }
                    }
                }
            }
        }
    }
}
