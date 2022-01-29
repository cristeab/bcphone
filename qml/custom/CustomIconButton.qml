import QtQuick
import QtQuick.Controls
import ".."

Rectangle {
    id: control

    signal clicked()
    property alias source: controlImage.source
    property alias toolTip: controlToolTip.text
    property color backgroundColor: Theme.dialButtonBackgroundColor
    property alias scale: controlImage.scale
    property bool enableButton: true
    property bool downloadLink: false
    property alias rotation: controlImage.rotation

    color: (controlMouseArea.pressed && control.enableButton) ? Qt.darker(control.backgroundColor) : control.backgroundColor
    Image {
        id: controlImage
        anchors.fill: parent
        mipmap: true
        fillMode: Image.PreserveAspectFit
    }
    z: 100
    MouseArea {
        id: controlMouseArea
        property var defaultCursorShape: null
        anchors.fill: parent
        hoverEnabled: true
        onClicked: control.clicked()
        onEntered: {
            if (control.downloadLink) {
                controlMouseArea.defaultCursorShape = controlMouseArea.cursorShape
                controlMouseArea.cursorShape = Qt.PointingHandCursor
            }
        }
        onExited: {
            if (control.downloadLink && (null !== controlMouseArea.defaultCursorShape)) {
                controlMouseArea.cursorShape = controlMouseArea.defaultCursorShape
                controlMouseArea.defaultCursorShape = null
            }
        }
    }
    ToolTip {
        id: controlToolTip
        visible: controlMouseArea.containsMouse && ("" !== controlToolTip.text)
    }
}
