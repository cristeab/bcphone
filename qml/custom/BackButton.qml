import QtQuick
import QtQuick.Controls
import ".."

ToolButton {
    id: control

    property string source: "qrc:/img/arrow-left-solid.svg"
    property color iconColor: "#676258"
    property color backgroundColor: Theme.backgroundColor
    property string tooltip: qsTr("Back")
    property var callback: null

    display: AbstractButton.IconOnly
    icon {
        source: control.source
        color: control.pressed ? Qt.darker(control.iconColor) : control.iconColor
    }
    background: Rectangle {
        color: control.backgroundColor
    }
    onClicked: {
        if (null !== control.callback) {
            control.callback()
        } else {
            console.error("Back callback not implemented")
        }
    }
    ToolTip {
        visible: control.hovered
        text: control.tooltip
    }
}
