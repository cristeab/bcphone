import QtQuick
import QtQuick.Controls
import ".."

Item {
    id: control

    property bool incoming: true
    property alias text: msgLbl.text
    property alias time: timeLbl.text
    property string attUrl: ""
    property bool selected: false

    height: msgFrame.height + 10

    MouseArea {
        anchors.fill: parent
        onClicked: control.selected = !control.selected
    }

    Image {
        anchors {
            left: control.incoming ? downloadAttBtn.right : undefined
            leftMargin: control.incoming ? Theme.windowMargin : 0
            right: control.incoming ? undefined : downloadAttBtn.left
            rightMargin: control.incoming ? 0 : Theme.windowMargin
            bottom: downloadAttBtn.bottom
        }
        visible: control.selected
        height: 20
        width: height
        source: "qrc:/img/check-circle.svg"
        mipmap: true
    }
    CustomIconButton {
        id: downloadAttBtn
        visible: "" !== control.attUrl
        enabled: downloadAttBtn.visible
        backgroundColor: "transparent"
        anchors {
            left: control.incoming ? msgFrame.right : undefined
            leftMargin: control.incoming ? Theme.windowMargin : 0
            right: control.incoming ? undefined : msgFrame.left
            rightMargin: control.incoming ? 0 : Theme.windowMargin
            bottom: msgFrame.bottom
        }
        height: Theme.toolButtonHeight
        width: downloadAttBtn.visible ? height : 0
        source: "qrc:/img/paperclip.svg"
        toolTip: qsTr("Download Attachment(s)")
        downloadLink: true
        onClicked: Qt.openUrlExternally(control.attUrl)
    }
    Rectangle {
        id: msgFrame
        readonly property color backgroundColor: control.incoming ? Theme.incomingColor : Theme.outgoingColor
        anchors {
            right: control.incoming ? undefined : parent.right
            rightMargin: control.incoming ? undefined : Theme.windowMargin
            left: control.incoming ? parent.left : undefined
            leftMargin: control.incoming ? Theme.windowMargin : undefined
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: -10
        }
        width: parent.width - Theme.windowMargin
        height: msgLbl.height + Theme.windowMargin / 2
        radius: 5
        color: msgFrame.backgroundColor
        Label {
            id: msgLbl
            width: parent.width
            clip: true
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            font.pointSize: Theme.textFontSize
            topPadding: Theme.windowMargin / 2
            bottomPadding: Theme.windowMargin / 2
            leftPadding: 1.5 * Theme.windowMargin
            rightPadding: 1.5 * Theme.windowMargin
            wrapMode: Text.Wrap
        }
        Label {
            id: timeLbl
            anchors {
                right: parent.right
                bottom: parent.bottom
            }
            horizontalAlignment: Text.AlignRight
            font {
                italic: true
                pointSize: Theme.tabButtonFontSize
            }
            rightPadding: Theme.windowMargin / 2
        }
    }
    Item {
        id: arrowFrame
        anchors {
            bottom: control.incoming ? msgFrame.top : undefined
            left: control.incoming ? parent.left : undefined
            leftMargin: control.incoming ? 3 * Theme.windowMargin : undefined
            top: control.incoming ? undefined : msgFrame.bottom
            right: control.incoming ? undefined : parent.right
            rightMargin: control.incoming ? undefined : 3 * Theme.windowMargin
        }
        rotation: control.incoming ? -90 : 90
        height: 10
        width: height
        Canvas {
            id: canvas
            anchors.centerIn: parent
            height: arrowFrame.height
            width: height
            antialiasing: true
            onPaint: {
                var ctx = canvas.getContext('2d')
                ctx.lineWidth = 2
                ctx.beginPath()
                ctx.fillStyle = msgFrame.backgroundColor
                ctx.strokeStyle = msgFrame.backgroundColor
                ctx.moveTo(0, 0)
                ctx.lineTo(canvas.width - 3, canvas.height / 2)
                ctx.lineTo(0, canvas.height)
                ctx.stroke()
                ctx.fill()
            }
        }
    }
}
