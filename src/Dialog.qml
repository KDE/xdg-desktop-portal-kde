/*
 *  SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
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

    ColumnLayout {
        id: contentLayout

        readonly property real minimumHeight: implicitHeight
        readonly property real minimumWidth: Math.min(Math.round(Screen.width / 3), implicitWidth)

        function present() {
            root.show();
        }

        spacing: Kirigami.Units.smallSpacing

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        RowLayout {

            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing

            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Icon {
                id: icon
                visible: source
                implicitWidth: Kirigami.Units.iconSizes.large
                implicitHeight: Kirigami.Units.iconSizes.large
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

        // Main content area, to be provided by the implementation
        QQC2.Control {
            id: contentsControl

            Layout.fillWidth: true
            Layout.fillHeight: true

            // make sure we don't add padding if there is no content
            visible: contentItem !== null

            contentItem: root.mainItem
        }

        // Footer area with buttons
        QQC2.DialogButtonBox {
            id: footerButtonBox

            Layout.fillWidth: true

            visible: count > 0
            focus: true

            onAccepted: root.accept()
            onRejected: root.reject()

            function maybeAccept() {
                const button = standardButton(QQC2.DialogButtonBox.Ok);
                if (button?.enabled) {
                    root.accept()
                }
            }
            Keys.onEnterPressed: maybeAccept()
            Keys.onReturnPressed: maybeAccept()
            Keys.onEscapePressed: root.reject()

            Repeater {
                model: root.actions

                delegate: QQC2.Button {
                    action: modelData
                }
            }
        }
    }
}
