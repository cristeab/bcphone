import QtQuick
import QtQuick.Controls
import Softphone 1.0
import "custom"

Page {
    Flickable {
        id: settingsFlick
        anchors {
            top: parent.top
            topMargin: Theme.windowMargin / 2
            bottom: logoutBtn.top
            bottomMargin: Theme.windowMargin / 2
            left: parent.left
            leftMargin: Theme.windowMargin
            right: parent.right
            rightMargin: Theme.windowMargin
        }
        contentWidth: devicesLayout.width
        contentHeight: devicesLayout.height
        clip: true
        boundsMovement: Flickable.StopAtBounds
        boundsBehavior: Flickable.DragAndOvershootBounds
        transform: Rotation {
            axis { x: 0; y: 1; z: 0 }
            origin.x: settingsFlick.width / 2
            origin.y: settingsFlick.height / 2
            angle: Math.min(30, Math.max(-30, settingsFlick.horizontalOvershoot))
        }

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

        Column {
            id: devicesLayout
            spacing: 10
            // registration settings
            LabelTextField {
                text: qsTr("SIP Server")
                width: callOutputSrc.width
                editText: softphone.settings.sipServer
                onEditingFinished: softphone.settings.sipServer = editText
            }
            LabelTextField {
                text: qsTr("SIP Server Port")
                width: callOutputSrc.width
                validator: IntValidator { bottom: 0; top: 65535 }
                editText: softphone.settings.sipPort
                onEditingFinished: softphone.settings.sipPort = parseInt(editText)
            }
            LabelComboBox {
                width: callOutputSrc.width
                text: qsTr("SIP Transport")
                model: ["UDP", "TCP", "TLS"]
                textRole: ""
                currentIndex: softphone.settings.sipTransport
                onCurrentIndexChanged: softphone.settings.sipTransport = currentIndex
            }
            LabelComboBox {
                width: callOutputSrc.width
                text: qsTr("Media Transport")
                model: ["RTP", "SRTP"]
                textRole: ""
                currentIndex: softphone.settings.mediaTransport
                onCurrentIndexChanged: softphone.settings.mediaTransport = currentIndex
            }
            LabelTextField {
                text: qsTr("User Name")
                width: callOutputSrc.width
                editText: softphone.settings.userName
                onEditingFinished: softphone.settings.userName = editText
            }
            LabelTextFieldPwd {
                text: qsTr("Password")
                width: callOutputSrc.width
                editText: softphone.settings.password
                onEditingFinished: softphone.settings.password = editText
            }
            LabelTextField {
                text: qsTr("Authorization ID")
                width: callOutputSrc.width
                editText: softphone.settings.authUserName
                onEditingFinished: softphone.settings.authUserName = editText
            }
            Switch {
                id: proxySwitch
                text: qsTr("Use Outbound Proxy")
                checked: softphone.settings.proxyEnabled
                onCheckedChanged: softphone.settings.proxyEnabled = checked
            }
            LabelTextField {
                enabled: proxySwitch.checked
                text: qsTr("Outbound Proxy Server")
                width: callOutputSrc.width
                editText: softphone.settings.proxyServer
                onEditingFinished: softphone.settings.proxyServer = editText
            }
            LabelTextField {
                enabled: proxySwitch.checked
                text: qsTr("Outbound Proxy Port")
                width: callOutputSrc.width
                validator: IntValidator { bottom: 0; top: 65535 }
                editText: softphone.settings.proxyPort
                onEditingFinished: softphone.settings.proxyPort = parseInt(editText)
            }
            // other settings
            LabelComboBox {
                id: callOutputSrc
                text: qsTr("Call Output Source")
                width: appWin.width - 4 * Theme.windowMargin
                model: softphone.outputAudioDevices
                currentIndex: softphone.settings.outputAudioModelIndex
                onCurrentIndexChanged: softphone.settings.outputAudioModelIndex = currentIndex
            }
            LabelSlider {
                text: qsTr("Call Output Volume")
                width: callOutputSrc.width
                from: 0
                to: 5
                stepSize: 0.1
                value: softphone.settings.speakersVolume
                onValueChanged: softphone.settings.speakersVolume = value
            }
            LabelComboBox {
                id: inputAudioDevs
                text: qsTr("Microphone Input Source")
                width: callOutputSrc.width
                model: softphone.inputAudioDevices
                currentIndex: softphone.settings.inputAudioModelIndex
                onCurrentIndexChanged: softphone.settings.inputAudioModelIndex = inputAudioDevs.currentIndex
            }
            LabelSlider {
                text: qsTr("Microphone Input Volume")
                width: callOutputSrc.width
                from: 0
                to: 5
                stepSize: 0.1
                value: softphone.settings.microphoneVolume
                onValueChanged: softphone.settings.microphoneVolume = value
            }
            LabelComboBox {
                id: inboundRingtones
                text: qsTr("Inbound Call Ringtone")
                width: callOutputSrc.width
                model: softphone.ringTonesModel
                currentIndex: softphone.settings.inboundRingTonesModelIndex
                onCurrentIndexChanged: softphone.settings.inboundRingTonesModelIndex = inboundRingtones.currentIndex
            }
            LabelComboBox {
                id: outboundRingtones
                text: qsTr("Outbound Call Ringtone")
                width: callOutputSrc.width
                model: softphone.ringTonesModel
                currentIndex: softphone.settings.outboundRingTonesModelIndex
                onCurrentIndexChanged: softphone.settings.outboundRingTonesModelIndex = outboundRingtones.currentIndex
            }
            LabelSlider {
                text: qsTr("Dialpad Sound Volume")
                width: callOutputSrc.width
                from: 0
                to: 2
                stepSize: 0.1
                value: softphone.settings.dialpadSoundVolume
                onValueChanged: softphone.settings.dialpadSoundVolume = value
            }
            LabelComboBox {
                id: videoDevs
                text: qsTr("Video Source")
                width: callOutputSrc.width
                model: softphone.videoDevices
                currentIndex: softphone.settings.videoModelIndex
                onCurrentIndexChanged: softphone.settings.videoModelIndex = videoDevs.currentIndex
            }
            LabelTextFieldBrowser {
                selectFolder: true
                text: qsTr("Recordings Path")
                width: callOutputSrc.width
                editText: softphone.settings.recPath
                onEditTextChanged: softphone.settings.recPath = editText
            }
            CustomTableView {
                codecsModel: softphone.audioCodecs
                text: qsTr("Audio Codecs Priority")
                width: callOutputSrc.width
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Restore Default Audio Codec Priorities")
                onClicked: softphone.audioCodecs.restoreAudioCodecDefaultPriorities()
            }
            CustomTableView {
                codecsModel: softphone.videoCodecs
                text: qsTr("Video Codecs Priority")
                width: callOutputSrc.width
                additionalRows: 1
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Restore Default Video Codec Priorities")
                onClicked: softphone.videoCodecs.restoreVideoCodecDefaultPriorities()
            }
            Item {
                height: Theme.windowMargin / 2
                width: parent.width
            }
        } // Column
    } // Flickable

    CustomButton {
        id: logoutBtn
        anchors {
            bottom: parent.bottom
            bottomMargin: Theme.windowMargin
            horizontalCenter: parent.horizontalCenter
        }
        visible: !softphone.loggedOut
        backgroundColor: Theme.blueButtonColor
        text: qsTr("Logout")
        onClicked: {
            msgDlgProps.okCancel = true
            msgDlgProps.acceptCallback = appWin.logout
            softphone.dialogError = false
            softphone.dialogMessage = qsTr("Are you sure you want to logout ?")
        }
    }
    CustomButton {
        id: registerBtn
        anchors {
            verticalCenter: logoutBtn.verticalCenter
            left: logoutBtn.right
            leftMargin: Theme.windowMargin
        }
        enabled: true
        backgroundColor: Theme.blueButtonColor
        text: qsTr("Register")
        onClicked: {
            registerBtn.focus = true
            softphone.registerAccount()
        }
    }
}
