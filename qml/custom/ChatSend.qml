import QtQuick
import ".."

Item {
    id: messageFrame
    height: messageTextField.height + Theme.windowMargin
    /*CustomIconButton {
        id: attachButton
        anchors {
            left: parent.left
            leftMargin: Theme.windowMargin
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: attachButton.hasAtt ? -5 : 0
        }
        height: Theme.toolButtonHeight
        width: height
        backgroundColor: "transparent"
        source: "qrc:/img/paperclip.svg"
        toolTip: qsTr("Attach File")
        onClicked: {
            fileDlgLoader.active = true
            fileDlgLoader.item.visible = true
        }
    }*/

    TextAreaWithScroll {
        id: messageTextField
        anchors {
            verticalCenter: parent.verticalCenter
            //left: attachButton.right
            left: parent.left
            leftMargin: Theme.windowMargin
            right: sendButton.left
            rightMargin: Theme.windowMargin
        }
        height: Theme.sendAreaHeight
        onSend: sendButton.sendAction()
    }
    CustomIconButton {
        id: sendButton

        function sendAction() {
            if (softphone.sendText(softphone.currentDestination, messageTextField.text)) {
                softphone.messagesModel.appendOutgoing(softphone.settings.extension,
                                                       softphone.currentDestination,
                                                       messageTextField.text)
                messageTextField.text = ""
            }
        }

        anchors {
            right: parent.right
            rightMargin: Theme.windowMargin
            verticalCenter: parent.verticalCenter
        }
        enableButton: "" !== messageTextField.text
        height: Theme.toolButtonHeight
        width: height
        backgroundColor: "transparent"
        source: "qrc:/img/paper-plane-regular.svg"
        toolTip: qsTr("Send")
        onClicked: sendButton.sendAction()
    }
}
