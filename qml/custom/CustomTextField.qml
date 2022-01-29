import QtQuick
import QtQuick.Controls
import ".."

TextField {
    id: control
    property bool error: false
    selectByMouse: true
    background: Rectangle {
        anchors.fill: parent
        color: "transparent"
        Rectangle {
            width: parent.width
            height: 1
            anchors.bottom: parent.bottom
            color: "transparent"
            border.color: control.error ? Theme.errorColor : "lightgray"
        }
    }
}
