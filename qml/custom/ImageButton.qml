import QtQuick
import QtQuick.Controls
import ".."

Button {
    id: control

    property alias source: contentImage.source
    property color color: "white"
    property int radius: 6
    property alias imageScale: contentImage.scale

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0

    background: Rectangle {
        color: (control.pressed || control.checked)?Qt.darker(control.color):control.color
        height: control.height
        width: control.width
        radius: height / 2
    }
    contentItem: Image {
        id: contentImage
        anchors {
            fill: parent
            margins: Theme.windowMargin
        }
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        mipmap: true
        scale: 1
    }
}
