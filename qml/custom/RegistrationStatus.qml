import QtQuick
import QtQuick.Controls
import Softphone 1.0
import ".."

Rectangle {
    id: control

    function statusColor() {
        let col = Theme.notRegisteredColor
        if (Softphone.Registered === softphone.sipRegistrationStatus) {
            col = Theme.registeredColor
        } else if (Softphone.RegistrationInProgress === softphone.sipRegistrationStatus) {
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
        visible: regSymbMouseArea.containsMouse && ("" !== text)
        text: softphone.sipRegistrationText
    }
}
