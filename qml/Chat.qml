import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import "custom"
import MessagesModel 1.0

Page {
    id: control

    ChatList {
        id: chatList
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
        }
    }

    ListView {
        id: chatDetails
        anchors {
            top: parent.top
            left: chatList.right
            leftMargin: Theme.windowMargin
            right: parent.right
            rightMargin:Theme.windowMargin
            bottom: chatSend.top
        }
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds
        spacing: 0
        clip: true
        model: softphone.messagesModel
        section.property: "dateRole"
        section.criteria: ViewSection.FullString
        section.labelPositioning: ViewSection.InlineLabels
        section.delegate: Label {
            width: parent.width
            text: section
            font.pointSize: Theme.textFontSize
            clip: true
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            topPadding: 0
            bottomPadding: Theme.windowMargin
            color: Theme.tabButtonColor
        }
        delegate: ChatBubble {
            incoming: MessagesModel.INBOUND === directionRole
            text: messageRole
            time: timeRole
            attUrl: mmsAttachmentsFileName
            width: chatDetails.width
        }
        Component.onCompleted: chatDetails.positionViewAtEnd()
    }

    ChatSend {
        id: chatSend
        anchors {
            left: chatDetails.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}
