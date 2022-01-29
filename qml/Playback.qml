import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import "custom"

Page {
    id: control

    readonly property int itemWidth: appWin.width - 2 * Theme.windowMargin

    SoundEffect {
        id: playbackPlayer
        volume: softphone.settings.speakersVolume
    }

    Column {
        id: controlCol
        anchors {
            top: parent.top
            topMargin: Theme.windowMargin / 2
            left: parent.left
            leftMargin: Theme.windowMargin
            right: parent.right
            rightMargin: Theme.windowMargin
        }
        spacing: 0
        LabelSlider {
            enabled: !softphone.settings.loopPlayback && (0 < playbackList.count)
            numDecimals: 0
            text: qsTr("Playback Count")
            width: control.itemWidth
            from: 1
            to: 20
            stepSize: 1
            value: softphone.settings.playbackCount
            onValueChanged: softphone.settings.playbackCount = value
        }
        CheckBox {
            enabled: (0 < playbackList.count)
            text: qsTr("Playback in Loop")
            checked: softphone.settings.loopPlayback
            onCheckedChanged: softphone.settings.loopPlayback = checked
        }
    }

    CustomToolButton {
        id: addPlaybackBtn
        anchors {
            top: controlCol.bottom
            topMargin: Theme.windowMargin
            left: controlCol.left
        }
        icon.source: "qrc:/img/plus-solid.svg"
        toolTipText: qsTr("Add Playback File")
        onClicked: {
            fileDlgLoader.active = true
            fileDlgLoader.item.visible = true
        }
    }
    CustomToolButton {
        anchors {
            top: controlCol.bottom
            topMargin: Theme.windowMargin
            right: controlCol.right
        }
        icon.source: "qrc:/img/minus-solid.svg"
        toolTipText: qsTr("Remove Selected Playback File(s)")
        onClicked: softphone.playbackModel.removeSelected()
    }

    Label {
        id: playbackListLabel
        anchors {
            top: addPlaybackBtn.bottom
            topMargin: Theme.windowMargin
            left: controlCol.left
        }
        width: controlCol.width
        text: qsTr("Playback Audio File(s)")
    }
    ListView {
        id: playbackList
        anchors {
            top: playbackListLabel.bottom
            topMargin: Theme.windowMargin
            left: playbackListLabel.left
            bottom: parent.bottom
            bottomMargin: Theme.windowMargin / 2
        }
        spacing: 0
        width: controlCol.width
        clip: true
        boundsBehavior: ListView.StopAtBounds
        model: softphone.playbackModel
        delegate: SwipeDelegate {
            id: delegateControl
            height: Theme.buttonHeight + 2 * Theme.windowMargin
            width: playbackList.width
            background: Rectangle {
                color: Theme.backgroundColor
            }
            contentItem: RowLayout {
                height: delegateControl.height
                spacing: 0
                CheckBox {
                    checked: selected
                    onCheckedChanged: softphone.playbackModel.setSelected(index, checked)
                }
                Label {
                    text: filePath
                    Layout.fillWidth: true
                    clip: true
                    elide: Text.ElideRight
                }
                ImageButton {
                    id: playButton
                    property bool play: false
                    Layout.preferredHeight: Theme.buttonHeight
                    Layout.preferredWidth: Theme.buttonHeight
                    source: (playButton.play && (index === playbackList.currentIndex)) ? "qrc:/img/stop-circle-regular.svg" : "qrc:/img/play-circle-regular.svg"
                    onClicked: {
                        playbackList.currentIndex = index
                        playButton.play = !playButton.play
                        playbackPlayer.stop()
                        if (playButton.play) {
                            playbackPlayer.source = softphone.playbackModel.filePath(index)
                            playbackPlayer.play()
                        }
                    }
                }
            }
        }
    }
}
