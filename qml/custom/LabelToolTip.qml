import QtQuick
import QtQuick.Controls

Label {
    id: controlLabel
    property alias toolTipZ: controlToolTip.z
    property bool forceTooltip: false
    MouseArea {
        id: controlLabelMouseArea
        enabled: controlLabel.truncated || controlLabel.forceTooltip
        anchors.fill: parent
        hoverEnabled: true
    }
    ToolTip {
        id: controlToolTip
        visible: controlLabelMouseArea.containsMouse && ("" !== controlLabel.text)
        text: controlLabel.text
    }
}
