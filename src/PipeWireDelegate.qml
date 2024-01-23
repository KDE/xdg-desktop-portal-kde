/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.pipewire 0.1 as PipeWire

Kirigami.Card {
    id: card

    property alias nodeId: pipeWireSourceItem.nodeId

    showClickFeedback: true

    banner {
        title: model.display
        titleIcon: model.decoration
        titleLevel: 6
        titleIconSize: Kirigami.Units.iconSizes.smallMedium
    }

    contentItem: PipeWire.PipeWireSourceItem {
        id: pipeWireSourceItem
        Layout.minimumHeight: Kirigami.Units.gridUnit * 10
        Layout.preferredHeight: Kirigami.Units.gridUnit * 10
        Layout.preferredWidth: Kirigami.Units.gridUnit * 7
        Layout.fillWidth: true
        Layout.fillHeight: true

        Kirigami.Icon {
            anchors.fill: parent
            visible: pipeWireSourceItem.nodeId === 0
            source: model.decoration
        }
    }

    Layout.preferredHeight: contentItem.Layout.preferredHeight
}
