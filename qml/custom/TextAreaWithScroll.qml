import QtQuick
import QtQuick.Controls
import ".."

ScrollView {
    id: control

    signal send()
    property alias text: controlTextArea.text

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical.policy: ScrollBar.AsNeeded
    TextArea {
        id: controlTextArea
        Keys.onPressed: (event) => {
            if ((event.key === Qt.Key_Enter) &&
                    (event.modifiers & Qt.ControlModifier) &&
                    ("" !== control.text)) {
                control.send()
            }
        }
        clip: true
        wrapMode: TextEdit.Wrap
        background: Rectangle {
            color: Theme.backgroundColor
            radius: 5
            border {
                width: 1
                color: Theme.sepColor2
            }
        }
    }
}
