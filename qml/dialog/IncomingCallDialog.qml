import QtQuick
import QtQuick.Controls
import "../custom"
import ".."

Dialog {
    id: control

    property int callId: softphone.invalidCallId
    property alias text: controlLabel.text
    property alias leftButtonText: leftBtn.text
    property alias rightButtonText: rightBtn.text
    property var actions: []

    //close dialog if the call is hangup remotely too early
    Connections {
        target: appWin
        function onCloseIncomingCallDialog(callId) {
            if (callId === control.callId) {
                control.destroy()
            }
        }
    }

    implicitWidth: appWin.width-Theme.dialogMargin
    implicitHeight: 200
    x: (appWin.width-width)/2
    y: (appWin.height-height)/2
    z: 2
    visible: true
    title: qsTr("Incoming Call")
    modal: true
    closePolicy: Popup.NoAutoClose
    standardButtons: Dialog.NoButton
    LabelToolTip {
        id: controlLabel
        anchors {
            top: parent.top
            bottom: btnRow.top
            bottomMargin: 2 * Theme.windowMargin
        }
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        clip: true
        font {
            bold: true
            pointSize: Theme.tabFontSize
        }
    }
    Row {
        id: btnRow
        readonly property int btnWidth: (btnRow.width - btnRow.spacing) / 2
        anchors.bottom: parent.bottom
        width: parent.width
        spacing: Theme.windowMargin
        CustomButton {
            id: leftBtn
            width: btnRow.btnWidth
            text: qsTr("Accept")
            backgroundColor: Theme.greenButtonColor
            onClicked: {
                if (control.actions[0]) {
                    control.actions[0](control.callId)
                }
                control.visible = false
                control.destroy()
            }
        }
        CustomButton {
            id: rightBtn
            width: btnRow.btnWidth
            text: qsTr("Decline")
            backgroundColor: Theme.errorColor
            onClicked: {
                if (control.actions[1]) {
                    control.actions[1](control.callId)
                }
                control.visible = false
                control.destroy()
            }
        }
    }
}
