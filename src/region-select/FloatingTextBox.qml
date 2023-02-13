import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.14 as Layouts
import org.kde.kirigami 2.19 as Kirigami

FloatingBackground {
    id: root
    property alias contents: loader.sourceComponent
    property bool extraMargin: false
    
    radius: Kirigami.Units.mediumSpacing / 2 + border.width
    implicitWidth: loader.implicitWidth + loader.anchors.rightMargin + loader.anchors.leftMargin
    implicitHeight: loader.implicitHeight + loader.anchors.topMargin + loader.anchors.bottomMargin
    color: Qt.rgba(Kirigami.Theme.backgroundColor.r, Kirigami.Theme.backgroundColor.g, Kirigami.Theme.backgroundColor.b, 0.85)
    border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.2)
    border.width: 1

    Loader {
        id: loader
        anchors.fill: parent
        anchors.topMargin: anchors.leftMargin - fontMetrics.descent
        anchors.leftMargin: Kirigami.Units.mediumSpacing * 2
        anchors.rightMargin: anchors.leftMargin
        anchors.bottomMargin: anchors.topMargin * (root.extraMargin ? 2 : 1)
    }
    
}
