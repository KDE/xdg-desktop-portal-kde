// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.iconthemes as KIconThemes
import org.kde.ki18n

pragma ComponentBehavior: Bound

PortalDialog {
    id: root

    property var dialog
    property string appID
    property url launcherURL: ""
    property bool edit: false

    readonly property Component displayComponent: Component {
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter

            spacing: Kirigami.Units.smallSpacing

            Kirigami.Icon {
                id: icon
                Layout.alignment: Qt.AlignHCenter

                implicitWidth: Kirigami.Units.iconSizes.huge
                implicitHeight: implicitWidth
                source: root.dialog.icon
            }

            Kirigami.Heading {
                Layout.alignment: Qt.AlignHCenter

                level: 3
                wrapMode: Text.Wrap
                text: root.dialog.name
            }

            Kirigami.LinkButton {
                Layout.alignment: Qt.AlignHCenter

                visible: text.length > 0
                elide: Text.ElideMiddle
                text: root.dialog.launcherURL
                onClicked: Qt.openUrlExternally(root.dialog.launcherURL)
            }
        }
    }

    readonly property Component editComponent: Component {
        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Button {
                Layout.alignment: Qt.AlignHCenter

                contentItem: Kirigami.Icon {
                    id: icon
                    implicitHeight: implicitWidth
                    implicitWidth: Kirigami.Units.iconSizes.huge
                    source: root.dialog.icon

                    KIconThemes.IconDialog {
                        id: iconDialog
                        onIconNameChanged: root.dialog.icon = iconName
                    }

                    TapHandler {
                        onTapped: iconDialog.open()
                    }
                }
            }

            QQC2.Label {
                id: editNameLabel
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Math.max(implicitWidth, editNameTextField.width)

                text: KI18n.i18nc("@label name of a launcher/application", "Name")
            }

            QQC2.TextField {
                id: editNameTextField
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Math.max(implicitWidth, editNameLabel.width)

                onTextChanged: root.dialog.name = text
                Component.onCompleted: text = root.dialog.name
            }
        }
    }

    title: KI18n.i18nc("@title Something asked to create a launcher for an app or a website", "Launcher Requested");
    // mainText and subtitle are set in C++ because we need to know the type,
    // and the QML side doesn't know that

    standardButtons: QQC2.DialogButtonBox.NoButton

    ColumnLayout {
        spacing: 0

        Item {
            Layout.fillHeight: true
        }

        Loader {
            Layout.alignment: Qt.AlignHCenter

            sourceComponent: root.edit ? root.editComponent : root.displayComponent
        }

        Item {
            Layout.fillHeight: true
        }
    }

    actions: [
        Kirigami.Action {
            text: KI18n.i18nc("@action edit launcher name/icon", "Edit Info…")
            icon.name: "document-edit"
            onCheckedChanged: root.edit = checked
            checkable: true
        },
        Kirigami.Action {
            text: KI18n.i18nc("@action accept dialog and create launcher", "Create")
            icon.name: "dialog-ok"
            onTriggered: root.accept()
        },
        Kirigami.Action {
            text: KI18n.i18nc("@action", "Cancel")
            icon.name: "dialog-cancel"
            onTriggered: root.reject()
        }
    ]
}
