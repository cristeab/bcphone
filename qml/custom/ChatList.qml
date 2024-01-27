import QtQuick
import QtQuick.Controls
import ".."
import Softphone 1.0

Item {
    width: Theme.activeCallListWidth
    SearchTextField {
        id: searchField
        height: 35
        anchors {
            top: parent.top
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: Theme.windowMargin
            right: parent.right
            rightMargin: Theme.windowMargin
        }
        onTextChanged: softphone.chatList.filterRegularExpression = new RegExp(text)
    }
    ListView {
        id: listViewControl
        anchors {
            top: searchField.bottom
            bottom: parent.bottom
        }
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        width: parent.width
        clip: true
        boundsBehavior: ListView.StopAtBounds
        model: softphone.chatList
        currentIndex: -1
        delegate: ItemDelegate {
            property variant myData: model
            height: 2.2 * Theme.buttonHeight
            width: listViewControl.width

            ToolButton {
                id: avatarFrame
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    leftMargin: Theme.windowMargin
                }
                height: 0.7 * parent.height
                width: height
                enabled: false
                icon {
                    source: "qrc:/img/user-avatar.svg"
                    color: (index === listViewControl.currentIndex) ? Theme.tabButtonColorSel : Theme.tabButtonColor
                }
            }
            Label {
                anchors {
                    verticalCenter: avatarFrame.verticalCenter
                    left: avatarFrame.right
                    leftMargin: Theme.windowMargin
                    right: countFrame.left
                    rightMargin: Theme.windowMargin
                }
                text: label
                font.pointSize: Theme.tabFontSize
                clip: true
                elide: Text.ElideRight
                color: avatarFrame.icon.color
            }
            Rectangle {
                id: countFrame
                visible: 0 < count
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    rightMargin: Theme.windowMargin
                }
                height: countLabel.height + 6
                width: height
                radius: height / 2
                border {
                    width: 1
                    color: avatarFrame.icon.color
                }
                color: "transparent"
                Label {
                    id: countLabel
                    anchors.centerIn: parent
                    text: count
                    font.pointSize: Theme.textFontSize
                    color: avatarFrame.icon.color
                }
            }
            Rectangle {
                anchors.bottom: parent.bottom
                height: 1
                width: parent.width
                color: Theme.sepColor
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    listViewControl.currentIndex = index
                    softphone.currentDestination = extension
                    softphone.messagesModel.filterRegularExpression = new RegExp(extension)
                }
            }
        }
        Component.onCompleted: {
            let regExpTxt = ""
            if (0 < listViewControl.count) {
                listViewControl.currentIndex = 0
                regExpTxt = listViewControl.currentItem.myData.extension
            } else {
                listViewControl.currentIndex = -1
                regExpTxt = "[^a-zA-Z0-9]" //empty chat details list
            }
            softphone.messagesModel.filterRegularExpression = new RegExp(regExpTxt)
        }
    }
    CustomIconButton {
        anchors {
            right: parent.right
            rightMargin: Theme.windowMargin
            bottom: parent.bottom
            bottomMargin: 2 * Theme.windowMargin
        }
        height: Theme.toolButtonHeight
        width: height
        radius: height / 2
        backgroundColor: "transparent"
        source: "qrc:/img/message-solid.svg"
        scale: 1
        toolTip: qsTr("New Chat")
        onClicked: {
            //show contacts
            bar.currentButtonIndex = 3
            tabView.replace("qrc:/qml/Contacts.qml")
            softphone.dialpadStatus = Softphone.NEW_CHAT
        }
    }
}
