import QtQuick
import QtQuick.Controls
import ".."

Rectangle {
    id: control

    height: 20
    width: height
    radius: height / 2
    color: Theme.sipStatusColor()
    MouseArea {
        id: regSymbMouseArea
        anchors.fill: parent
        hoverEnabled: true
    }
    ToolTip {
        visible: regSymbMouseArea.containsMouse && ("" !== text)
        text: softphone.sipRegistrationText
    }
}
