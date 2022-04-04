import QtQuick
import QtQuick.Controls
import QtMultimedia
import Softphone 1.0
import "custom"

Page {
    id: dialpadFrame

    readonly property real buttonVSpacing: 15
    readonly property real buttonHSpacing: 20

    readonly property var keyIndex: { "1": 0, "2": 1, "3": 2, "4": 3, "5": 4, "6": 5,
        "7": 6, "8": 7, "9": 8, "*": 9, "0": 10, "#": 11 }
    readonly property var btnSound: [ "qrc:/audio/Dtmf-1.wav",
        "qrc:/audio/Dtmf-2.wav", "qrc:/audio/Dtmf-3.wav",
        "qrc:/audio/Dtmf-4.wav", "qrc:/audio/Dtmf-5.wav",
        "qrc:/audio/Dtmf-6.wav", "qrc:/audio/Dtmf-7.wav",
        "qrc:/audio/Dtmf-8.wav", "qrc:/audio/Dtmf-9.wav",
        "qrc:/audio/Dtmf-star.wav", "qrc:/audio/Dtmf-0.wav",
        "qrc:/audio/Dtmf-pound.wav" ]

    function dialpadButtonAction(key) {
        softphone.dialedText += key
        if (Theme.isWindows) {
            softphone.playDigit(key)
        } else {
            const index = keyIndex[key]
            playSound.source = btnSound[index]
            playSound.play()
        }
        if (softphone.activeCall && !softphone.blindTransfer && !softphone.conference) {
            softphone.sendDtmf(key)
        }
    }

    function callAction() {
        //remove formatting, if any
        const num = Theme.removeFormatting(softphone.dialedText)
        softphone.makeCall(num)
    }
    function eraseButtonAction() {
        if (0 < softphone.dialedText.length) {
            softphone.dialedText = softphone.dialedText.substring(0, softphone.dialedText.length - 1)
        }
    }

    SoundEffect {
        id: playSound
        volume: softphone.settings.dialpadSoundVolume
    }

    focus: dialpadFrame.visible
    Keys.onPressed: {
        if (!dialpadFrame.visible) return
        if (event.text in dialpadFrame.keyIndex) {
            dialpadFrame.dialpadButtonAction(event.text)
            event.accepted = true
        }
        if (Qt.Key_Enter === event.key || Qt.Key_Return === event.key) {
            dialpadFrame.callAction()
            event.accepted = true
        }
        if (Qt.Key_Backspace === event.key) {
            dialpadFrame.eraseButtonAction()
            event.accepted = true
        }
    }

    background: Item {}

    Label {
        id: callDurationLabel
        anchors {
            top: parent.top
            topMargin: Theme.windowMargin
            horizontalCenter: parent.horizontalCenter
        }
        text: callDurationTimer.text
        visible: softphone.activeCall
        font.pointSize: Theme.textFontSize
        elide: Text.ElideRight
        width: appWin.width / 2
        horizontalAlignment: Text.AlignHCenter
        clip: true
    }

    LabelToolTip {
        id: extLabel
        anchors {
            top: parent.top
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: Theme.windowMargin
            right: callDurationLabel.left
        }
        text: softphone.settings.userName
        maximumLineCount: 1
        clip: true
        elide: Text.ElideRight
        forceTooltip: true
    }
    RegistrationStatus {
        anchors {
            top: parent.top
            topMargin: Theme.windowMargin
            right: parent.right
            rightMargin: Theme.windowMargin
        }
    }

    TabButton {
        id: backspaceBtn
        anchors {
            left: dialNumberTextField.right
            leftMargin: 3
            verticalCenter: dialNumberTextField.verticalCenter
        }
        display: AbstractButton.IconOnly
        icon {
            source: "qrc:/img/backspace.svg"
            color: backspaceBtn.pressed ? Qt.darker(Theme.tabButtonColor) : Theme.tabButtonColor
        }
        height: dialNumberTextField.height
        background: Rectangle {
            color: Theme.backgroundColor
        }
        visible: 0 < softphone.dialedText.length
        onClicked: dialpadFrame.eraseButtonAction()
    }
    Label {
        id: dialNumberTextField
        anchors {
            bottom: contactLabel.top
            bottomMargin: 5
            horizontalCenter: parent.horizontalCenter
        }
        width: dialpad.width
        text: Theme.formatTelephoneNumber(softphone.dialedText)
        horizontalAlignment: Text.AlignHCenter
        fontSizeMode: Text.HorizontalFit
        font.pointSize: 1.5 * Theme.titleFontSize
        background: Item {}
        onTextChanged: softphone.dialedText = dialNumberTextField.text
    }
    Label {
        id: contactLabel

        function contactLabelText() {
            if (softphone.blindTransfer) {
                return qsTr("Blind Transfer to ") + softphone.blindTransferUserName
            }
            if (softphone.conference) {
                return qsTr("Conference Mode")
            }
            return softphone.activeCallModel.currentUserName
        }

        anchors {
            bottom: dialpad.top
            bottomMargin: 20
            horizontalCenter: parent.horizontalCenter
        }
        text: contactLabel.contactLabelText()
        width: dialpad.width
        clip: true
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    Grid {
        id: dialpad
        anchors {
            bottom: btnRow.top
            bottomMargin: 15
            horizontalCenter: parent.horizontalCenter
        }
        columnSpacing: dialpadFrame.buttonHSpacing
        rowSpacing: dialpadFrame.buttonVSpacing
        columns: 3

        Repeater {
            id: dialpadRepeater
            function itemVerticalOffset(idx) {
                let out = 0
                if (9 === idx) {
                    out = 8
                } else if (11 === idx) {
                    out = 3
                }
                return out
            }
            property var keysArr: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "0", "#"]
            model: keysArr.length
            DialpadButton {
                height: Theme.dialpadButtonHeight
                width: height
                text: dialpadRepeater.keysArr[index]
                verticalOffset: dialpadRepeater.itemVerticalOffset(index)
                onClicked: dialpadFrame.dialpadButtonAction(text)
            }
        }
    }//grid

    Row {
        id: btnRow

        readonly property real largeButtonHeight: 0.9 * dialpadFrame.buttonHeight
        readonly property real smallButtonHeight: 0.7 * dialpadFrame.buttonHeight

        spacing: (parent.width - btnRow.largeButtonHeight - 6 * btnRow.smallButtonHeight) / 8
        anchors {
            bottom: parent.bottom
            bottomMargin: 15
            horizontalCenter: parent.horizontalCenter
        }

        ImageButton {
            id: confBtn
            anchors.verticalCenter: callBtn.verticalCenter
            visible: softphone.activeCall
            height: btnRow.smallButtonHeight
            width: height
            checkable: true
            checked: softphone.conference
            source: "qrc:/img/conference.svg"
            ToolTip {
                visible: confBtn.hovered
                text: softphone.conference ? qsTr("Cancel Conference Mode") : qsTr("Enable Conference Mode")
            }
            onClicked: {
                softphone.conference = !softphone.conference
                if (softphone.conference) {
                    softphone.currentUserId = softphone.dialedText
                    softphone.dialedText = ""
                } else {
                    softphone.dialedText = softphone.currentUserId
                }
            }
        }
        ImageButton {
            id: transBtn
            anchors.verticalCenter: callBtn.verticalCenter
            visible: softphone.activeCall
            height: btnRow.smallButtonHeight
            width: height
            checkable: true
            source: "qrc:/img/transfer.svg"
            ToolTip {
                visible: transBtn.hovered
                text: softphone.blindTransfer ? qsTr("Cancel Transfer") : qsTr("Blind Transfer")
            }
            onClicked: {
                softphone.blindTransfer = !softphone.blindTransfer
                if (softphone.blindTransfer) {
                    softphone.currentUserId = softphone.dialedText
                    softphone.dialedText = ""
                    softphone.blindTransferUserName = ""
                } else {
                    softphone.dialedText = softphone.currentUserId
                }
            }
        }
        ImageButton {
            id: videoBtn
            enabled: softphone.hasVideo
            anchors.verticalCenter: callBtn.verticalCenter
            visible: softphone.activeCall || softphone.enableVideo
            height: btnRow.smallButtonHeight
            width: height
            source: softphone.enableVideo ? "qrc:/img/video-slash.svg" : "qrc:/img/video.svg"
            ToolTip {
                visible: videoBtn.hovered
                text: softphone.enableVideo ? qsTr("Disable Video") : qsTr("Enable Video")
            }
            onClicked: softphone.enableVideo = !softphone.enableVideo
        }

        ImageButton {
            id: callBtn
            function callBtnTooltip() {
                if (softphone.blindTransfer) {
                    return qsTr("Transfer")
                }
                if (softphone.conference) {
                    return qsTr("Add Call")
                }
                return softphone.activeCall ? qsTr("Hangup") : qsTr("Dial")
            }
            function callBtnIcon() {
                if (softphone.conference) {
                    return "qrc:/img/add-user-white.svg"
                }
                return softphone.activeCall ? "qrc:/img/hangup.svg" : "qrc:/img/phone.svg"
            }
            function callBtnColor() {
                if (softphone.conference) {
                    return Theme.confCallButtonColor
                }
                return softphone.activeCall ? Theme.activeCallButtonColor : Theme.callButtonColor
            }
            enabled: ("" !== softphone.dialedText) && (Softphone.REGISTERED === softphone.registrationStatus)
            height: btnRow.largeButtonHeight
            width: height
            source: callBtn.callBtnIcon()
            color: callBtn.callBtnColor()
            ToolTip {
                visible: callBtn.hovered
                text: callBtn.callBtnTooltip()
            }
            onClicked: {
                if (softphone.blindTransfer) {
                    const num = Theme.removeFormatting(softphone.dialedText)
                    softphone.unsupervisedTransfer(num)
                    softphone.blindTransfer = false
                    softphone.blindTransferUserName = ""
                    return
                }
                if (softphone.conference) {
                    console.log("Conference call")
                    dialpadFrame.callAction()
                    return
                }
                if (softphone.activeCall) {
                    softphone.hangupAll()
                } else {
                    console.log("Normal call")
                    dialpadFrame.callAction()
                }
            }
        }

        ImageButton {
            id: holdBtn
            anchors.verticalCenter: callBtn.verticalCenter
            visible: softphone.activeCall
            height: btnRow.smallButtonHeight
            width: height
            source: softphone.holdCall ? "qrc:/img/unhold.svg" : "qrc:/img/hold.svg"
            ToolTip {
                visible: holdBtn.hovered
                text: softphone.holdCall ? qsTr("Unhold") : qsTr("Hold")
            }
            onClicked: softphone.holdCall = !softphone.holdCall
        }
        ImageButton {
            id: muteBtn
            anchors.verticalCenter: callBtn.verticalCenter
            visible: softphone.activeCall
            height: btnRow.smallButtonHeight
            width: height
            source: softphone.muteMicrophone ? "qrc:/img/microphone-slash.svg" : "qrc:/img/microphone.svg"
            ToolTip {
                visible: muteBtn.hovered
                text: softphone.muteMicrophone ? qsTr("Disable Mic") : qsTr("Enable Mic")
            }
            onClicked: softphone.muteMicrophone = !softphone.muteMicrophone
        }

        ImageButton {
            id: recBtn
            anchors.verticalCenter: callBtn.verticalCenter
            visible: softphone.activeCall
            height: btnRow.smallButtonHeight
            width: height
            source: softphone.record ? "qrc:/img/record-stop.svg" : "qrc:/img/record-start.svg"
            ToolTip {
                visible: recBtn.hovered
                text: softphone.record ? qsTr("Stop Recording") : qsTr("Start Recording")
            }
            onClicked: softphone.record = !softphone.record
        }
    }
}
