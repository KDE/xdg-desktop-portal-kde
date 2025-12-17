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
import Qt5Compat.GraphicalEffects

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
     * Sits below the mainText and subtitle but above the content separator (visually part of the header)
     */
    property Item headerItem

    /**
     * Sits above the dialogButtonBox but below the content separator (visually part of the footer)
     */
    property Item footerItem

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
     * An optional content item that will be placed left of the action buttons in the footer of the dialog.
    */
    property Item dialogButtonBoxLeftItem

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

    readonly property alias contentWidth: contentLayout.implicitWidth
    readonly property alias contentHeight: contentLayout.implicitHeight

    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint
    visible: false

    function present() : void {
        Kirigami.Settings.isMobile ? showMaximized() : show()
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
    }

    Binding {
        target: root.dialogButtonBox?.standardButton(QQC2.DialogButtonBox.Ok) ?? null
        property: "enabled"
        when: root.dialogButtonBox?.standardButtons & QQC2.DialogButtonBox.Ok
        value: root.acceptable
    }

    Item {
        id: contentLayout

        anchors.fill: parent

        implicitWidth: control.implicitWidth
        implicitHeight: control.implicitHeight

        focus: true
        Keys.onEnterPressed: root.accept()
        Keys.onReturnPressed: root.accept()
        Keys.onEscapePressed: root.reject()

        Item {
            id: systemModalShadow
            visible: Kirigami.Settings.isMobile
            anchors.fill: parent

            onWindowChanged: (window) => {
                if (visible && window) {
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
                    anchors.fill: parent
                    cached: true
                    glowRadius: 2
                    cornerRadius: Kirigami.Units.gridUnit
                    spread: 0.1
                    color: Qt.rgba(0, 0, 0, 0.4)
                }
            }
        }

        QQC2.Control {
            id: control
            anchors.fill: Kirigami.Settings.isMobile ? undefined : parent
            anchors.centerIn: Kirigami.Settings.isMobile ? parent : undefined
            anchors.margins: Kirigami.Settings.isMobile ? glow.cornerRadius : 0
            topPadding: 0
            bottomPadding: 0
            rightPadding: 0
            leftPadding: 0

            background: Item {
                visible: Kirigami.Settings.isMobile

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

                Kirigami.Separator {
                    // Makes no sense on mobile because we don't run flush against a window edge at the top
                    visible: !Kirigami.Settings.isMobile
                    Layout.fillWidth: true
                }

                QQC2.Control {
                    id: headerControl

                    Layout.fillWidth: true

                    visible: visibleChildren.length > 0

                    topPadding: Kirigami.Units.largeSpacing
                    bottomPadding: Kirigami.Units.largeSpacing
                    leftPadding: Kirigami.Units.largeSpacing
                    rightPadding: Kirigami.Units.largeSpacing

                    contentItem: ColumnLayout {
                        spacing: headerControl.topPadding
                        visible: visibleChildren.length > 0

                        RowLayout {
                            id: headerRow

                            visible: titleHeading.text.length > 0 || subtitleLabel.text.length > 0 || icon.source

                            Layout.fillWidth: true
                            spacing: Kirigami.Units.largeSpacing

                            Kirigami.Icon {
                                id: icon
                                visible: source
                                implicitWidth: Kirigami.Units.iconSizes.medium
                                implicitHeight: Kirigami.Units.iconSizes.medium

                                source: root.iconName
                            }

                            ColumnLayout {
                                Layout.fillWidth: true

                                spacing: Kirigami.Units.smallSpacing

                                Kirigami.Heading {
                                    id: titleHeading
                                    Layout.fillWidth: true
                                    level: 2
                                    wrapMode: Text.Wrap
                                    textFormat: Text.RichText
                                    elide: Text.ElideRight
                                    text: root.mainText
                                }

                                QQC2.Label {
                                    id: subtitleLabel
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                    elide: Text.ElideRight
                                    textFormat: Text.RichText
                                    visible: text.length > 0
                                    text: root.subtitle
                                }
                            }
                        }

                        QQC2.Control {
                            topPadding: 0
                            rightPadding: 0
                            bottomPadding: 0
                            leftPadding: 0

                            Layout.fillWidth: true
                            contentItem: root.headerItem
                            visible: root.headerItem?.visible ?? false
                        }
                    }
                }

                Kirigami.Separator {
                    visible: contentsControl.visible
                    Layout.leftMargin: -control.leftPadding
                    Layout.rightMargin: -control.rightPadding
                    Layout.fillWidth: true
                }

                // Main content area, to be provided by the implementation
                QQC2.Control {
                    id: contentsControl

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: -control.leftPadding
                    Layout.rightMargin: -control.rightPadding

                    Kirigami.Theme.colorSet: Kirigami.Theme.View
                    background: Rectangle {
                        Kirigami.Theme.colorSet: Kirigami.Theme.View
                        color: Kirigami.Theme.backgroundColor
                    }
                    contentItem: root.mainItem

                    // make sure we don't add padding if there is no content
                    visible: contentItem?.visible ?? false
                }

                Kirigami.Separator {
                    visible: contentsControl.visible && footerControl.visible
                    Layout.leftMargin: -control.leftPadding
                    Layout.rightMargin: -control.rightPadding
                    Layout.fillWidth: true
                }

                QQC2.Control {
                    id: footerControl

                    Layout.fillWidth: true
                    visible: visibleChildren.length > 0

                    contentItem: ColumnLayout {
                        visible: visibleChildren.length > 0

                        QQC2.Control {
                            topPadding: 0
                            rightPadding: 0
                            bottomPadding: 0
                            leftPadding: 0

                            Layout.fillWidth: true
                            contentItem: root.footerItem
                            visible: root.footerItem?.visible ?? false
                        }

                        FlexboxLayout {
                            wrap: FlexboxLayout.Wrap
                            visible: visibleChildren.length > 0
                            rowGap: Kirigami.Units.smallSpacing

                            // WARNING: In Qt 6.10.0 there is a problem with fillHeight/Width not correctly laying out items when wrapped.
                            // Be super careful when making changes to the layout guides in the containers. Thoroughly test window resizing!

                            implicitHeight: {
                                // Height calculation in Qt 6.10.0 is a bit buggy. Help it a bit.
                                // If the buttonbox is below the left container we are in wrap mode and must take their
                                // combined height. Otherwise we are in row mode and must take the maximum height.
                                // https://bugreports.qt.io/browse/QTBUG-141400
                                if (footerButtonBox.x == leftContainer.x) {
                                    return footerButtonBox.implicitHeight + leftContainer.implicitHeight + rowGap
                                }
                                return Math.max(footerButtonBox.implicitHeight, leftContainer.implicitHeight)
                            }

                            QQC2.Container {
                                id: leftContainer

                                Layout.fillHeight: true // make sure the content gets placed centered by making sure we occupy the available height
                                Layout.minimumHeight: implicitHeight

                                padding: 0
                                contentItem: root.dialogButtonBoxLeftItem
                                visible: root.dialogButtonBoxLeftItem?.visible ?? false
                            }

                            QQC2.DialogButtonBox {
                                id: footerButtonBox

                                Layout.fillWidth: true // make sure the content gets placed right by making sure we occupy the available width
                                Layout.minimumWidth: implicitWidth
                                Layout.minimumHeight: implicitHeight

                                padding: 0
                                visible: count > 0

                                standardButtons: {
                                    if (Kirigami.Settings.isMobile) {
                                        // ensure we never have no buttons, we always must have the cancel button available
                                        return (root.standardButtons === QQC2.DialogButtonBox.NoButton) ? QQC2.DialogButtonBox.Cancel : root.standardButtons
                                    }
                                    return root.standardButtons
                                }

                                onAccepted: root.accept()
                                onRejected: root.reject()

                                Repeater {
                                    model: root.actions

                                    delegate: QQC2.Button {
                                        required property var modelData
                                        action: modelData
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
