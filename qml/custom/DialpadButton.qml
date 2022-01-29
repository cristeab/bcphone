import QtQuick
import QtQuick.Controls
import ".."

Button {
    id: control

    property int verticalOffset: 0
    readonly property var bottomTextArr: ["+", "", "ABC", "DEF", "GHI", "JKL", "MNO", "PQRS", "TUV", "WXYZ"]

    function bottomText() {
        var txt = ""
        if ("*" !== control.text && "#" !== control.text) {
            var index = parseInt(control.text)
            if ((0 <= index) && (index < control.bottomTextArr.length)) {
                txt = control.bottomTextArr[index]
            }
        }
        return txt
    }

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0

    background: Rectangle {
        color: control.pressed ? Qt.darker(Theme.dialButtonBackgroundColor) : Theme.dialButtonBackgroundColor
        height: parent.height
        width: height
        radius: width/2
    }
    contentItem: Item {
        id: contentRoot
        function offset() {
            var off = -7
            if ("*" === control.text) {
                off = -5
            }
            return off
        }
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: contentRoot.offset()
        }
        Label {
            id: mainText
            anchors {
                centerIn: parent
                verticalCenterOffset: control.verticalOffset
            }
            text: control.text
            font.pointSize: Theme.dialpadButtonFontSize
            color: Theme.dialButtonTextColor
            horizontalAlignment: Text.AlignHCenter
        }
        Label {
            anchors {
                top: mainText.bottom
                horizontalCenter: mainText.horizontalCenter
            }
            text: control.bottomText()
            font {
                pointSize: Theme.dialpadSubTextFontSize
                letterSpacing: 1.8
            }
            color: Theme.dialButtonTextColor
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
