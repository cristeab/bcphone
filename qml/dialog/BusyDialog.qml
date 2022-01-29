import QtQuick
import QtQuick.Controls

Dialog {
    id: control
    contentWidth: appWin.width
    contentHeight: appWin.height
    Component.onCompleted: control.visible = true
    background: Rectangle {
        color: "#80000000"
    }
    BusyIndicator {
        running: true
        anchors.centerIn: parent
    }
}
