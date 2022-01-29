import QtQuick
import QtQuick.Controls
import Softphone 1.0
import "../custom"
import ".."

Dialog {
    id: loginDialog

    contentWidth: appWin.width-Theme.dialogMargin
    contentHeight: loginLabel.height + 9*srvText.height + sep.height + loginButton.height + 10*dialogColumn.spacing
    x: (appWin.width-width)/2
    y: (appWin.height-height)/2

    visible: softphone.loggedOut
    modal: true
    standardButtons: Dialog.NoButton
    closePolicy: Popup.NoAutoClose

    Rectangle {
        id: dialogOuterFrame
        Keys.onReturnPressed: {
            if (loginDialog.visible) {
                loginButton.loginAction()
            }
        }
        anchors.fill: parent
        color: Theme.backgroundColor
        Rectangle {
            id: dialogInnerFrame
            anchors {
                fill: parent
                margins: 10
            }
            radius: 5
            color: "transparent"
            border {
                width: 1
                color: Theme.greenButtonColor
            }
            Flickable {
                anchors {
                    fill: parent
                    margins: Theme.windowMargin / 2
                }
                contentWidth: dialogColumn.width
                contentHeight: dialogColumn.height
                flickableDirection: Flickable.VerticalFlick
                clip: true

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                Column {
                    id: dialogColumn
                    spacing: 10
                    Label {
                        id: loginLabel
                        width: loginDialog.width-80
                        text: qsTr("Please Login")
                        font.pointSize: Theme.tabFontSize
                    }
                    Rectangle {
                        id: sep
                        width: loginLabel.width
                        height: 1
                        color: "transparent"
                    }
                    CustomTextField {
                        id: srvText
                        placeholderText: qsTr("SIP Server")
                        leftPadding: Theme.windowMargin
                        rightPadding: Theme.windowMargin
                        width: loginLabel.width
                        font.pointSize: Theme.textFontSize
                        onTextChanged: error = false
                        text: softphone.settings.sipServer
                    }
                    CustomTextField {
                        id: portText
                        placeholderText: qsTr("SIP Server Port")
                        leftPadding: Theme.windowMargin
                        rightPadding: Theme.windowMargin
                        width: loginLabel.width
                        font.pointSize: Theme.textFontSize
                        validator: IntValidator { bottom: 0; top: 65535 }
                        onTextChanged: error = false
                        text: softphone.settings.sipPort
                    }
                    LabelComboBox {
                        width: loginLabel.width
                        text: qsTr("SIP Transport")
                        model: ["UDP", "TCP", "TLS"]
                        textRole: ""
                        currentIndex: softphone.settings.sipTransport
                        onCurrentIndexChanged: softphone.settings.sipTransport = currentIndex
                    }
                    LabelComboBox {
                        width: loginLabel.width
                        text: qsTr("Media Transport")
                        model: ["RTP", "SRTP"]
                        textRole: ""
                        currentIndex: softphone.settings.mediaTransport
                        onCurrentIndexChanged: softphone.settings.mediaTransport = currentIndex
                    }
                    CustomTextField {
                        id: userText
                        placeholderText: qsTr("User Name")
                        leftPadding: Theme.windowMargin
                        rightPadding: Theme.windowMargin
                        width: loginLabel.width
                        onTextChanged: error = false
                        font.pointSize: Theme.textFontSize
                        text: softphone.settings.userName
                    }
                    CustomTextField {
                        id: pwdText
                        placeholderText: qsTr("Password")
                        leftPadding: Theme.windowMargin
                        rightPadding: Theme.windowMargin
                        width: loginLabel.width
                        echoMode: TextInput.Password
                        onTextChanged: error = false
                        font.pointSize: Theme.textFontSize
                        text: softphone.settings.password
                    }
                    CustomTextField {
                        id: authText
                        placeholderText: qsTr("Authorization ID")
                        leftPadding: Theme.windowMargin
                        rightPadding: Theme.windowMargin
                        width: loginLabel.width
                        onTextChanged: error = false
                        font.pointSize: Theme.textFontSize
                        text: softphone.settings.authUserName
                    }
                    Switch {
                        id: proxySwitch
                        text: qsTr("Use Outbound Proxy")
                        checked: softphone.settings.proxyEnabled
                        onCheckedChanged: softphone.settings.proxyEnabled = checked
                    }
                    CustomTextField {
                        id: proxySrvText
                        enabled: proxySwitch.checked
                        placeholderText: qsTr("Outbound Proxy Server")
                        leftPadding: Theme.windowMargin
                        rightPadding: Theme.windowMargin
                        width: loginLabel.width
                        font.pointSize: Theme.textFontSize
                        onTextChanged: error = false
                        text: softphone.settings.proxyServer
                    }
                    CustomTextField {
                        id: proxyPortText
                        enabled: proxySwitch.checked
                        placeholderText: qsTr("Outbound Proxy Port")
                        leftPadding: Theme.windowMargin
                        rightPadding: Theme.windowMargin
                        width: loginLabel.width
                        font.pointSize: Theme.textFontSize
                        validator: IntValidator { bottom: 0; top: 65535 }
                        onTextChanged: error = false
                        text: softphone.settings.proxyPort
                    }
                    CustomButton {
                        id: loginButton
                        function loginAction() {
                            if ("" === srvText.text) {
                                srvText.error = true
                            } else if ("" === portText.text) {
                                portText.error = true
                            } else if ("" === userText.text) {
                                userText.error = true
                            } else if ("" === pwdText.text) {
                                pwdText.error = true
                            } else if (softphone.settings.proxyEnabled &&
                                       ("" === proxySrvText.text)) {
                                proxySrvText.error = true
                            } else if (softphone.settings.proxyEnabled &&
                                       ("" === proxyPortText.text)) {
                                proxyPortText.error = true
                            } else {
                                softphone.settings.sipServer = srvText.text
                                softphone.settings.sipPort = parseInt(portText.text)
                                softphone.settings.userName = userText.text
                                softphone.settings.password = pwdText.text
                                softphone.settings.proxyServer = proxySrvText.text
                                softphone.settings.proxyPort = parseInt(proxyPortText.text)
                                softphone.settings.authUserName = authText.text
                                softphone.showBusy = false
                                softphone.showBusy = softphone.registerAccount()
                            }
                        }
                        width: loginLabel.width
                        text: qsTr("Login")
                        onClicked: loginButton.loginAction()
                    }
                }
            }
        }
    }
}
