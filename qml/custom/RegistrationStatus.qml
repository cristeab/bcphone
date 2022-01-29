import QtQuick
import QtQuick.Controls
import Softphone 1.0
import ".."

Rectangle {
    id: control

    function statusColor() {
        let col = "red"
        if (Softphone.REGISTERED === softphone.registrationStatus) {
            col = "green"
        } else if (Softphone.IN_PROGRESS === softphone.registrationStatus) {
            col = "yellow"
        }
        return col
    }

    height: 20
    width: height
    radius: height / 2
    color: control.statusColor()
    MouseArea {
        id: regSymbMouseArea
        anchors.fill: parent
        hoverEnabled: true
    }
    ToolTip {
        visible: regSymbMouseArea.containsMouse
        text: softphone.registrationText
    }
}
