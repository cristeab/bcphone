import QtQuick
import QtQuick.Controls
import ".."

TextField {
    id: control
    readonly property real showPwdBtnHeight: 0.5 * control.height
    property bool error: false
    property bool showPwd: false
    echoMode: control.showPwd ? TextInput.Normal : TextInput.Password
    selectByMouse: true
    rightInset: Theme.windowMargin
    onPressed: (event) => {
        event.accepted = (event.x < (control.width - control.showPwdBtnHeight))
    }
    background: Rectangle {
        anchors.fill: parent
        color: "transparent"
        Rectangle {
            width: parent.width
            height: 1
            anchors.bottom: parent.bottom
            color: "transparent"
            border.color: control.error ? Theme.errorColor : Theme.sepColor
        }
        Image {
            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
            }
            source: control.showPwd ?  "qrc:/img/eye-solid.svg" : "qrc:/img/eye-slash-solid.svg"
            height: control.showPwdBtnHeight
            width: height
            mipmap: true
            fillMode: Image.PreserveAspectFit
            CustomMouseArea {
                anchors.fill: parent
                onClicked: control.showPwd = !control.showPwd
            }
        }
    }
}
