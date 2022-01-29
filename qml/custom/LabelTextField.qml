import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import ".."

Column {
    id: control

    property alias text: controlLabel.text
    property alias editText: controlTextField.text
    property alias error: controlTextField.error
    property alias validator: controlTextField.validator
    property alias echoMode: controlTextField.echoMode

    signal editingFinished()

    spacing: 5

    onEditTextChanged: control.error = false

    Label {
        id: controlLabel
        width: parent.width
        elide: Text.ElideRight
        color: control.error ? Theme.errorColor : Material.foreground
        font {
            italic: true
            pointSize: Theme.labelFontSize
        }
    }
    CustomTextField {
        id: controlTextField
        width: parent.width
        onEditingFinished: control.editingFinished()
    }
}
