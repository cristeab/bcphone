import QtQuick
import QtQuick.Controls
import ".."

ToolButton {
    id: control
    property alias toolTipText: controlToolTip.text
    function buttonColor() {
        if (control.enabled) {
            return control.pressed ? Qt.darker(Theme.tabButtonColor) : Theme.tabButtonColor
        }
        return Qt.lighter(Theme.tabButtonColor)
    }

    display: AbstractButton.IconOnly
    icon {
        height: Theme.toolButtonHeight
        width: Theme.toolButtonHeight
        color: control.buttonColor()
    }
    background: Rectangle {
        color: Theme.backgroundColor
    }
    ToolTip {
        id: controlToolTip
        visible: control.hovered
        text: qsTr("Clear All")
    }
}
