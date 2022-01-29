import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "custom"

Drawer {
    id: control
    interactive: 0 < control.position
    edge: Qt.LeftEdge

    ListView {
        id: listView
        readonly property int avatarWidth: 40
        readonly property int itemWidth: listView.width - 3 * listView.avatarWidth - 3 * Theme.windowMargin
        anchors.fill: parent
        clip: true
        boundsBehavior: ListView.StopAtBounds
        model: softphone.activeCallModel
        delegate: SwipeDelegate {
            id: delegateControl
            height: 2.2 * Theme.buttonHeight
            background: Rectangle {
                color: Theme.backgroundColor
            }
            contentItem: GridLayout {
                id: delegateRow
                height: parent.height
                rowSpacing: 0
                rows: 2
                flow: GridLayout.TopToBottom
                //1st row
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
                    Layout.preferredWidth: listView.itemWidth
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignTop
                    text: qsTr("Phone: ") + Theme.formatWithNa(phoneNumber)
                    font {
                        italic: false
                        pointSize: Theme.textFontSize
                    }
                    clip: true
                    elide: Text.ElideRight
                }
                //buttons
                ToolButton {
                    Layout.preferredWidth: listView.avatarWidth
                    Layout.rowSpan: 2
                    display: AbstractButton.IconOnly
                    icon {
                        source: "qrc:/img/hangup.svg"
                        color: parent.pressed ? Qt.darker(Theme.tabButtonColor) : Theme.tabButtonColor
                    }
                    background: Rectangle {
                        color: Theme.backgroundColor
                    }
                    ToolTip {
                        visible: parent.hovered
                        text: qsTr("Hangup")
                    }
                    onClicked: softphone.hangup(callId)
                }
                ToolButton {
                    visible: !isCurrentCall
                    Layout.preferredWidth: listView.avatarWidth
                    Layout.rowSpan: 2
                    display: AbstractButton.IconOnly
                    icon {
                        source: "qrc:/img/swap.svg"
                        color: parent.pressed ? Qt.darker(Theme.tabButtonColor) : Theme.tabButtonColor
                    }
                    background: Rectangle {
                        color: Theme.backgroundColor
                    }
                    ToolTip {
                        visible: parent.hovered
                        text: qsTr("Swap")
                    }
                    onClicked: softphone.swap(callId)
                }
                ToolButton {
                    visible: !isCurrentCall
                    Layout.preferredWidth: listView.avatarWidth
                    Layout.rowSpan: 2
                    display: AbstractButton.IconOnly
                    icon {
                        source: "qrc:/img/merge.svg"
                        color: parent.pressed ? Qt.darker(Theme.tabButtonColor) : Theme.tabButtonColor
                    }
                    background: Rectangle {
                        color: Theme.backgroundColor
                    }
                    ToolTip {
                        visible: parent.hovered
                        text: qsTr("Merge")
                    }
                    onClicked: softphone.merge(callId)
                }
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: listView.itemWidth + 3 * listView.avatarWidth + 3 * Theme.windowMargin
                height: 1
                color: Theme.sepColor
            }
        }
    }
}
