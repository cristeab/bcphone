import QtQuick
import QtQuick.Controls
import "../custom"
import ".."

Dialog {
    id: control

    function getStandardButtons() {
        if (softphone.dialogRetry) {
            return Dialog.Retry | Dialog.Cancel
        }
        return msgDlgProps.okCancel ? (Dialog.Ok | Dialog.Cancel) :  Dialog.Ok
    }

    implicitWidth: appWin.width - Theme.dialogMargin
    implicitHeight: 200
    x: (appWin.width-width)/2
    y: (appWin.height-height)/2
    z: 2
    onAccepted: {
        if (null !== msgDlgProps.acceptCallback) {
            msgDlgProps.acceptCallback(msgDlgProps.callbackArg)
            msgDlgProps.acceptCallback = null
        }
        softphone.dialogMessage = ""
        softphone.dialogError = false
        msgDlgProps.okCancel = false
        if (softphone.dialogRetry) {
            softphone.dialogRetry = false
            softphone.manuallyRegister()
        }
    }
    onRejected: {
        if (null !== msgDlgProps.cancelCallback) {
            msgDlgProps.cancelCallback()
            msgDlgProps.cancelCallback = null
        }
        softphone.dialogMessage = ""
        softphone.dialogError = false
        msgDlgProps.okCancel = false
    }
    visible: "" !== controlLabel.text
    title: softphone.dialogError ? qsTr("Error") : qsTr("Information")
    modal: true
    closePolicy: Popup.NoAutoClose
    standardButtons: control.getStandardButtons()
    LabelToolTip {
        id: controlLabel
        text: softphone.dialogMessage
        anchors.fill: parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        clip: true
        font.pointSize: Theme.tabFontSize
    }
}
