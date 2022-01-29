import QtQuick
import QtQuick.Controls
import Softphone 1.0
import ".."

Rectangle {
    id: control

    function statusColor() {
        let col = Theme.notRegisteredColor
        if (Softphone.REGISTERED === softphone.registrationStatus) {
            col = Theme.registeredColor
        } else if (Softphone.IN_PROGRESS === softphone.registrationStatus) {
            col = Theme.inProgressRegistrationColor
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
        visible: regSymbMouseArea.containsMouse && ("" !== softphone.registrationText)
        text: softphone.registrationText
    }
}
