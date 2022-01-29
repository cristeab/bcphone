import QtQuick
import QtQuick.Controls
import ".."

Rectangle {
    id: control
    property alias text: textFieldControl.text
    color: "lightgray"
    border {
        width: 1
        color: Theme.sepColor
    }
    radius: 5
    Image {
        id: controlImage
        height: 0.4 * parent.height
        width: height
        anchors {
            left: parent.left
            leftMargin: Theme.windowMargin
            verticalCenter: parent.verticalCenter
        }
        source: "qrc:/img/search.svg"
        mipmap: true
        fillMode: Image.PreserveAspectFit
    }
    TextField {
        id: textFieldControl
        anchors {
            left: controlImage.right
            right: parent.right
        }
        height: control.height
        selectByMouse: true
        background: Item {}
    }
}
