import QtQuick

MouseArea {
    id: control
    property var prevCursorShape: null
    property bool enableMouseChange: true
    hoverEnabled: true
    onEntered: {
        if (control.enableMouseChange) {
            control.prevCursorShape = control.cursorShape
            control.cursorShape = Qt.PointingHandCursor
        }
    }
    onExited: {
        if ((null !== control.prevCursorShape) && control.enableMouseChange) {
            control.cursorShape = control.prevCursorShape
            control.prevCursorShape = null
        }
    }
}
