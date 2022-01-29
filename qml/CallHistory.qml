import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "custom"

Page {
    id: control

    function callSelected(phoneNumber) {
        softphone.dialedText = Theme.formatTelephoneNumber(phoneNumber)
        tabView.showDialpad()
        if (softphone.blindTransfer) {
            softphone.blindTransferUserName = userName
        } else {
            softphone.activeCall = true
            const num = Theme.removeFormatting(phoneNumber)
            softphone.makeCall(num)
        }
    }

    CustomToolButton {
        id: clearHistoryBtn
        anchors {
            top: parent.top
            right: parent.right
        }
        visible: 0 < listView.count
        icon.source: "qrc:/img/eraser-solid.svg"
        toolTipText: qsTr("Clear All")
        onClicked: {
            msgDlgProps.okCancel = true
            msgDlgProps.acceptCallback = softphone.callHistoryModel.clear
            softphone.dialogMessage = qsTr("Clear call history ?")
            softphone.dialogError = false
        }
    }

    ListView {
        id: listView
        readonly property int avatarWidth: 40
        readonly property int itemWidth: listView.width - 2 * listView.avatarWidth - 2 * Theme.windowMargin
        readonly property int swipeWidth: 70
        anchors {
            top: clearHistoryBtn.bottom
            bottom: parent.bottom
        }
        width: parent.width
        clip: true
        boundsBehavior: ListView.StopAtBounds
        model: softphone.callHistoryModel
        section {
            criteria: ViewSection.FullString
            property: "callDate"
            delegate: Column {
                width: parent.width
                Label {
                    text: section
                    width: parent.width
                    leftPadding: Theme.windowMargin
                    rightPadding: Theme.windowMargin
                    bottomPadding: Theme.windowMargin / 2
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    font.italic: true
                }
                Rectangle {
                    width: parent.width
                    height: 1
                    color: Theme.sepColor
                }
            }
        }
        delegate: SwipeDelegate {
            id: delegateControl
            height: 2.2 * Theme.buttonHeight
            background: Rectangle {
                color: Theme.backgroundColor
            }
            contentItem: GridLayout {
                id: delegateRow
                Behavior on x {
                    enabled: !delegateControl.down
                    NumberAnimation {
                        easing.type: Easing.InOutCubic
                        duration: 400
                    }
                }
                height: parent.height
                rowSpacing: 0
                rows: 2
                columns: 2
                flow: GridLayout.LeftToRight
                //1st row
                ToolButton {
                    Layout.preferredWidth: listView.avatarWidth
                    display: AbstractButton.IconOnly
                    enabled: true
                    icon {
                        source: "qrc:/img/" + callStatus + "-call.svg"
                        color: Theme.tabButtonColor
                    }
                    background: Rectangle {
                        color: Theme.backgroundColor
                    }
                    ToolTip {
                        visible: parent.hovered
                        text: callStatus
                    }
                    //onClicked: control.callSelected(phoneNumber)
                }
                LabelToolTip {
                    Layout.preferredWidth: listView.itemWidth
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignTop
                    text: Theme.formatWithNa(userName)
                    font {
                        bold: true
                        pointSize: Theme.textFontSize + 4
                    }
                    clip: true
                    elide: Text.ElideRight
                }
                //2nd row
                LabelToolTip {
                    Layout.preferredWidth: 2 * listView.avatarWidth
                    text: callTime
                    font {
                        italic: false
                        pointSize: Theme.textFontSize
                    }
                    clip: true
                    elide: Text.ElideRight
                }
                LabelToolTip {
                    Layout.preferredWidth: listView.itemWidth
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignTop
                    text: qsTr("Phone: ") + Theme.formatWithNa(phoneNumber)
                    font {
                        italic: false
                        pointSize: Theme.textFontSize
                        underline: "" !== phoneNumber
                    }
                    clip: true
                    elide: Text.ElideRight
                    CustomMouseArea {
                        anchors.fill: parent
                        onClicked: control.callSelected(phoneNumber)
                    }
                }
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Theme.sepColor
            }
            //swipe gestures handling
            swipe.enabled: true
            swipe.right: Rectangle {
                height: parent.height
                width: listView.swipeWidth
                anchors.right: parent.right
                color: SwipeDelegate.pressed ? Qt.darker(Theme.swipeRemoveItemColor, 1.1) : Theme.swipeRemoveItemColor
                SwipeDelegate.onClicked: {
                    softphone.dialogMessage = qsTr("Are you sure you want to delete ") + Theme.formatNotEmpty(userName, phoneNumber) + qsTr(" from the list ?")
                    softphone.dialogError = false
                    msgDlgProps.okCancel = true
                    msgDlgProps.acceptCallback = softphone.callHistoryModel.deleteContact
                    msgDlgProps.callbackArg = index
                    delegateControl.swipe.close()
                }
                Column {
                    anchors.fill: parent
                    spacing: 0
                    Item {
                        height: 5
                        width: parent.width
                    }
                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: parent.height/2 - 6
                        width: height

                        source: "qrc:/img/trash.svg"
                        fillMode: Image.PreserveAspectFit
                        mipmap: true
                        scale: 0.8
                    }
                    Label {
                        id: deleteLabel
                        text: qsTr("Delete")
                        color: Theme.tabButtonColor
                        font.pointSize: Theme.tabButtonFontSize
                        clip: true
                        elide: Text.ElideRight
                        verticalAlignment: Label.AlignVCenter
                        width: parent.width
                        horizontalAlignment: Label.AlignHCenter
                        topPadding: 3
                    }
                }
            }
            swipe.left: Rectangle {
                height: parent.height
                width: isContact ? 0 : listView.swipeWidth
                anchors.left: parent.left
                color: SwipeDelegate.pressed ? Qt.darker(Theme.swipeEditItemColor, 1.1) : Theme.swipeEditItemColor
                SwipeDelegate.onClicked: {
                    msgDlgProps.addContactFromHistory = true
                    softphone.callHistoryModel.currentIndex = index
                    editContactDlg.show(-1)
                    delegateControl.swipe.close()
                }
                Column {
                    anchors.fill: parent
                    spacing: 0
                    Item {
                        height: 5
                        width: parent.width
                    }
                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: parent.height/2 - 6
                        width: height
                        source: "qrc:/img/user-edit.svg"
                        fillMode: Image.PreserveAspectFit
                        mipmap: true
                    }
                    Label {
                        id: editLabel
                        text: qsTr("Add to Contacts")
                        color: Theme.tabButtonColor
                        font.pointSize: Theme.tabButtonFontSize
                        clip: true
                        elide: Text.ElideRight
                        verticalAlignment: Label.AlignVCenter
                        width: parent.width
                        horizontalAlignment: Label.AlignHCenter
                        topPadding: 3
                        wrapMode: Text.WordWrap
                    }
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Theme.tabButtonColor
                }
            }
        }
    }
}
