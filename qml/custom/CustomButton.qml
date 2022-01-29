import QtQuick
import QtQuick.Controls
import ".."

Button {
    id: control
    property color backgroundColor: Theme.greenButtonColor
    height: Theme.buttonHeight

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0

    contentItem: Label {
        anchors.centerIn: parent
        text: control.text
        color: "white"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pointSize: Theme.buttonFontSize
    }
    background: Rectangle {
        color: control.pressed?Qt.darker(control.backgroundColor):control.backgroundColor
        height: control.height
        width: control.width
        radius: 3
    }
}
