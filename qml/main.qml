import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtMultimedia
import Qt.labs.platform
import "custom"

ApplicationWindow {
    id: appWin

    title: softphone.settings.appName + " v" + softphone.settings.appVersion
    width: 400
    height: 600
    minimumHeight: height
    minimumWidth: width
    maximumHeight: height
    maximumWidth: width
    visible: true

    signal closeIncomingCallDialog(int callId)

    function raiseWindow() {
        appWin.show()
        appWin.raise()
        appWin.requestActivate()
        if (!softphone.loggedOut) {
            tabView.showDialpad()
        }
    }
    function updateWindowRect() {
        softphone.winPosX = appWin.x
        softphone.winPosY = appWin.y
        softphone.winWidth = appWin.width
    }
    function logout() {
        softphone.loggedOut = true
        softphone.settings.clear()
    }

    onClosing: (close)=> {
        if (null != trayIcon) {
            trayIcon.showMessage(softphone.settings.appName,
                                 qsTr("The program will keep running in the system tray. To terminate the program, choose 'Quit' in the context menu of the system tray entry."))
        }
        close.accepted = false
        appWin.showMinimized()
    }

    Timer {
        id: callDurationTimer

        property int durationSec: -1
        property string text: "00:00"

        function convertToString(v) {
            if (v < 10) {
                return '0' + v
            }
            return v
        }
        function showDuration() {
            const hrs = Math.floor(callDurationTimer.durationSec / 3600)
            const mins = Math.floor((callDurationTimer.durationSec % 3600) / 60)
            const sec = callDurationTimer.durationSec % 60
            var out = callDurationTimer.convertToString(mins) + ':' + callDurationTimer.convertToString(sec)
            if (0 < hrs) {
                out = callDurationTimer.convertToString(hrs) + ':' + out
            }
            callDurationTimer.text = out
        }

        interval: 1000
        repeat: true
        running: softphone.confirmedCall
        onRunningChanged: {
            if (!running) {
                callDurationTimer.durationSec = -1
                callDurationTimer.text = "00:00"
            }
        }

        triggeredOnStart: true
        onTriggered: {
            callDurationTimer.durationSec += 1
            callDurationTimer.showDuration()
        }
    }

    SystemTrayIcon {
        id: trayIcon
        Component.onCompleted: visible = true
        icon.source: "qrc:/img/logo.png"
        onActivated: appWin.raiseWindow()
        menu: Menu {
            MenuItem {
                text: qsTr("Quit")
                onTriggered: Qt.quit()
            }
        }
    }

    QtObject {
        id: msgDlgProps
        property bool okCancel: false
        property var acceptCallback: null
        property int callbackArg: -1
        property var cancelCallback: null
        property int contactIndex: -1
        property bool addContactFromHistory: false
    }

    Component.onCompleted: {
        appWin.updateWindowRect()
        if (softphone.loggedOut) {
            tabView.showSettings()
        }
    }
    onXChanged: appWin.updateWindowRect()
    onYChanged: appWin.updateWindowRect()
    onWidthChanged: appWin.updateWindowRect()

    background: Rectangle {
        color: Theme.backgroundColor
    }

    ActiveCalls {
        id: activeCallDrawer
        width: 0.75 * appWin.width
        height: appWin.height - bar.height - Theme.windowMargin
    }
    CustomIconButton {
        visible: 1 < softphone.activeCallModel.callCount
        source: "qrc:/img/open-drawer.svg"
        toolTip: qsTr("Show Active Call(s)")
        onClicked: activeCallDrawer.open()
        width: activeCallDrawer.dragMargin
        height: activeCallDrawer.height
    }

    StackView {
        id: tabView
        function showDialpad() {
            if (2 !== bar.currentButtonIndex) {
                bar.currentButtonIndex = 2
                tabView.replace("qrc:/qml/Dialpad.qml")
            }
        }
        function showSettings() {
            if (4 !== bar.currentButtonIndex) {
                bar.currentButtonIndex = 4
                tabView.replace("qrc:/qml/Settings.qml")
            }
        }
        anchors {
            top: parent.top
            bottom: bar.top
        }
        width: parent.width
        initialItem: "qrc:/qml/Dialpad.qml"
    }

    Connections {
        target: softphone
        function onIncoming(callCount, callId, userId, userName, isConf) {
            if (softphone.settings.autoAnswer) {
                return;
            }

            if (1 > callCount) {
                console.error("Unexpected call count " + callCount)
                return
            }
            const dlgText = ("" === userName) ? userId : userName + " (" + userId + ")"
            const isAccept = (1 === callCount) || isConf
            const leftText = isAccept ? qsTr("Accept") : qsTr("Hold & Accept")
            const leftAction = isAccept ? softphone.answer : softphone.holdAndAnswer
            const comp = Qt.createComponent("qrc:/qml/dialog/IncomingCallDialog.qml")
            const dlg = comp.createObject(appWin, {
                                              "callId": callId,
                                              "text": dlgText,
                                              "leftButtonText": leftText,
                                              "rightButtonText": qsTr("Decline"),
                                              "actions": [leftAction, softphone.hangup],
                                              "visible": true
                                          })
            if (null === dlg) {
                console.error("Cannot create incoming call dialog")
            }
        }
        function onDisconnected(callId) {
            appWin.closeIncomingCallDialog(callId)
            enableVideoTimer.start()
        }
        function onLoggedOutChanged() {
            if (softphone.loggedOut) {
                tabView.showSettings()
            }
        }
    }
    //preview must be disabled with delay in order to avoid deadlocks
    Timer {
        id: enableVideoTimer
        interval: 500
        repeat: false
        running: false
        onTriggered: softphone.enableVideo = false
    }

    Loader {
        active: "" !== softphone.dialogMessage
        source: "qrc:/qml/dialog/MessageDialog.qml"
    }
    Loader {
        id: editContactDlg
        function show(index) {
            msgDlgProps.contactIndex = index
            editContactDlg.active = true
            editContactDlg.item.visible = true
        }
        active: false
        source: "qrc:/qml/dialog/EditContactDialog.qml"
    }

    TabBar {
        id: bar

        property int currentButtonIndex: 2
        readonly property var names: [qsTr("Recents"), qsTr("Contacts"), qsTr("Keypad"), qsTr("Chat"), qsTr("Settings")]
        readonly property var icons: ["qrc:/img/clock.svg", "qrc:/img/address-book.svg", "qrc:/img/dialpad.svg", "qrc:/img/chat.svg", "qrc:/img/settings.svg"]
        readonly property var pages: ["qrc:/qml/CallHistory.qml", "qrc:/qml/Contacts.qml", "qrc:/qml/Dialpad.qml", "qrc:/qml/Chat.qml", "qrc:/qml/Settings.qml"]

        anchors {
            bottom: parent.bottom
            bottomMargin: 0
        }
        width: parent.width
        position: TabBar.Footer
        spacing: 0

        Repeater {
            model: bar.names.length
            TabButton {
                id: tabButton

                property bool isSelected: bar.currentButtonIndex === index
                property color textColor: isSelected ? Theme.tabButtonColorSel : Theme.tabButtonColor

                display: AbstractButton.TextUnderIcon
                text: "<font color='" + tabButton.textColor + "'>" + bar.names[index] + "</font>"
                icon {
                    source: bar.icons[index]
                    color: tabButton.textColor
                }
                width: bar.width / bar.names.length
                background: Rectangle {
                    color: Theme.backgroundColor
                }
                font.pointSize: Theme.tabButtonFontSize
                onClicked: {
                    if (bar.currentButtonIndex !== index) {
                        bar.currentButtonIndex = index
                        tabView.replace(bar.pages[index])
                    }
                }
            }
        }
    }

    Component {
        id: fileDlgComp
        FileDialog {
            id: fileDlg
            title: qsTr("Please choose a file")
            folder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
            fileMode: FileDialog.OpenFile
            nameFilters: ["Wav Files (*.wav)", "All Files (*.html *.*)"]
            onAccepted: softphone.playbackModel.append(fileDlg.file)
        }
    }
    Loader {
        id: fileDlgLoader
        active: false
        sourceComponent: fileDlgComp
    }

    Component {
        id: folderDlgComp
        FolderDialog {
            id: folderDlg
            title: qsTr("Please choose a folder")
            folder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
            options: FolderDialog.ShowDirsOnly
            onAccepted: softphone.settings.recPath = folderDlgComp.folder
        }
    }
    Loader {
        id: folderDlgLoader
        active: false
        sourceComponent: folderDlgComp
    }

    //busy indicator
    Loader {
        id: busyDlg
        active: softphone.showBusy
        source: "qrc:/qml/dialog/BusyDialog.qml"
    }
}
