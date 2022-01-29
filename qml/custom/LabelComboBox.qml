import QtQuick
import QtQuick.Controls
import ".."

Column {
    id: control
    property alias text: controlLabel.text
    property alias model: controlCombo.model
    property alias currentIndex: controlCombo.currentIndex
    property alias textRole: controlCombo.textRole
    spacing: 5
    Label {
        id: controlLabel
        width: parent.width
        elide: Text.ElideRight
        font {
            italic: true
            pointSize: Theme.labelFontSize
        }
    }
    ComboBox {
        id: controlCombo
        width: parent.width
        textRole: "name"
    }
}
