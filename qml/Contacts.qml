import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "custom"

Page {
    CustomToolButton {
        id: addUserBtn
        anchors {
            top: parent.top
            right: parent.right
        }
        icon.source: "qrc:/img/add-user.svg"
        toolTipText: qsTr("Add Contact")
        onClicked: editContactDlg.show(-1)
    }

    Rectangle {
        id: sep
        visible: 0 < listView.count
        anchors.top: addUserBtn.bottom
        width: parent.width
        height: 1
        color: Theme.sepColor
    }

    ListView {
        id: listView
        readonly property int avatarWidth: 40
        readonly property int itemWidth: (listView.width - listView.avatarWidth - 2 * Theme.windowMargin) / 2
        readonly property int swipeWidth: 70
        anchors {
            top: sep.bottom
            bottom: parent.bottom
        }
        width: parent.width
        clip: true
        boundsBehavior: ListView.StopAtBounds
        model: softphone.contactsModel
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
                rows: 4
                columns: 4
                flow: GridLayout.LeftToRight
                //1st column
                ToolButton {
                    Layout.preferredWidth: listView.avatarWidth
                    Layout.rowSpan: 4
                    display: AbstractButton.IconOnly
                    enabled: true
                    icon {
                        source: "qrc:/img/user-avatar.svg"
                        color: Theme.tabButtonColor
                    }
                    background: Rectangle {
                        color: Theme.backgroundColor
                    }
                    ToolTip {
                        visible: parent.hovered
                        text: softphone.blindTransfer ? qsTr("Transfer to") : qsTr("Call")
                    }
                    onClicked: {
                        softphone.dialedText = Theme.formatTelephoneNumber(phoneNumber)
                        bar.showTab(bar.dialpadIndex)
                        if (softphone.blindTransfer) {
                            softphone.blindTransferUserName = userNameLabel.text
                        } else {
                            softphone.makeCall(phoneNumber)
                        }
                    }
                }
                //1st row
                LabelToolTip {
                    id: userNameLabel
                    Layout.preferredWidth: 2 * listView.itemWidth
                    Layout.columnSpan: 2
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: Theme.formatWithComma(firstName, lastName)
                    font {
                        bold: true
                        pointSize: Theme.textFontSize + 2
                    }
                    clip: true
                    elide: Text.ElideRight
                    rightPadding: 15
                }
                //2nd row
                LabelToolTip {
                    Layout.preferredWidth: listView.itemWidth
                    text: qsTr("Phone: ") + Theme.formatWithNa(phoneNumber)
                    font {
                        italic: false
                        pointSize: Theme.textFontSize
                    }
                    clip: true
                    elide: Text.ElideRight
                }
                LabelToolTip {
                    Layout.preferredWidth: listView.itemWidth
                    Layout.fillHeight: true
                    Layout.rowSpan: 2
                    text: qsTr("Address: ") + Theme.formatWithNa(Theme.formatWithComma4(address, city, state, zip))
                    font.pointSize: Theme.textFontSize
                    clip: true
                    elide: Text.ElideRight
                    maximumLineCount: 2
                }
                //3rd row
                LabelToolTip {
                    Layout.preferredWidth: listView.itemWidth
                    text: qsTr("Mobile: ") + Theme.formatWithNa(mobileNumber)
                    font {
                        italic: false
                        pointSize: Theme.textFontSize
                    }
                    clip: true
                    elide: Text.ElideRight
                }
                //4th row
                LabelToolTip {
                    Layout.preferredWidth: 2 * listView.itemWidth
                    Layout.columnSpan: 2
                    text: comment
                    font {
                        italic: true
                        pointSize: Theme.textFontSize
                    }
                    clip: true
                    elide: Text.ElideRight
                }
                //4th column
                ToolButton {
                    Layout.preferredWidth: listView.avatarWidth
                    Layout.rowSpan: 4
                    display: AbstractButton.IconOnly
                    enabled: true
                    icon {
                        source: "qrc:/img/paper-plane-regular.svg"
                        color: Theme.tabButtonColor
                    }
                    background: Rectangle {
                        color: Theme.backgroundColor
                    }
                    ToolTip {
                        visible: parent.hovered
                        text: qsTr("Chat")
                    }
                    onClicked: {
                        listView.addEmptyChat(number, name)
                        bar.showTab(bar.chatIndex)
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
                    softphone.dialogMessage = qsTr("Are you sure you want to delete ") + Theme.formatWithComma(firstName, lastName) + qsTr(" from the list ?")
                    softphone.dialogError = false
                    msgDlgProps.okCancel = true
                    msgDlgProps.acceptCallback = softphone.contactsModel.remove
                    msgDlgProps.callbackArg = contactId
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
                        font.pointSize: Theme.textFontSize
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
                width: listView.swipeWidth
                anchors.left: parent.left
                color: SwipeDelegate.pressed ? Qt.darker(Theme.swipeEditItemColor, 1.1) : Theme.swipeEditItemColor
                SwipeDelegate.onClicked: {
                    editContactDlg.show(index)
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
                        text: qsTr("Edit")
                        color: Theme.tabButtonColor
                        font.pointSize: Theme.textFontSize
                        clip: true
                        elide: Text.ElideRight
                        verticalAlignment: Label.AlignVCenter
                        width: parent.width
                        horizontalAlignment: Label.AlignHCenter
                        topPadding: 3
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
