// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12 as Controls
import org.kde.kirigami 2.8 as Kirigami
import org.kde.kirigamifilepicker 0.1

/**
 * The PlacesGlobalDrawer type provides a GlobalDrawer containing common places on the file system
 */
Kirigami.OverlayDrawer {
    id: root

    signal placeOpenRequested(url place)

    handleClosedIcon.source: null
    handleOpenIcon.source: null
    width: Math.min(applicationWindow().width * 0.8, Kirigami.Units.gridUnit * 20)

    leftPadding: 0
    rightPadding: 0

    contentItem: ListView {
        spacing: 0
        model: FilePlacesModel {
            id: filePlacesModel
        }

        section.property: "group"
        section.delegate: Kirigami.Heading {
            leftPadding: Kirigami.Units.smallSpacing
            level: 6
            text: section
        }

        delegate: Kirigami.BasicListItem {
            visible: !model.hidden
            width: parent.width
            text: model.display
            icon: model.iconName
            separatorVisible: false
            onClicked: {
                root.placeOpenRequested(model.url)
            }
        }
    }
}
