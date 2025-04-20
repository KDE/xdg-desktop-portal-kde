/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.pipewire 0.1 as PipeWire

Kirigami.AbstractCard {
    id: root

    property alias nodeId: pipeWireSourceItem.nodeId
    required property var title
    required property var titleIcon

    showClickFeedback: true
    Accessible.name: title
    implicitHeight: Kirigami.Units.gridUnit * 10
    implicitWidth: Kirigami.Units.gridUnit * 15

    Controls.CheckBox {
        checked: root.checked
        onToggled: root.toggled()

        anchors {
            right: root.right
            bottom: root.bottom
            bottomMargin: root.topPadding
        }

    }

    contentItem: Kirigami.Padding {
        topPadding: -root.topPadding + root.background.border.width
        leftPadding: -root.leftPadding + root.background.border.width
        rightPadding: -root.rightPadding + root.background.border.width
        bottomPadding: root.contentItem ? 0 : -root.bottomPadding + root.background.border.width

        contentItem: PipeWire.PipeWireSourceItem {
            id: pipeWireSourceItem
        }

    }

    footer: RowLayout {
        Kirigami.Icon {
            source: titleIcon
        }

        Kirigami.Heading {
            text: title
            level: 3
            Layout.fillWidth: true
        }

    }

}
