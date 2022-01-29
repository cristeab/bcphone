import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import ".."

Column {
    id: control

    property bool selectFolder: false
    property alias text: controlLabel.text
    property alias editText: controlTextField.text
    property alias error: controlTextField.error

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
    Row {
        id: browseRow
        spacing: 5
        CustomTextField {
            id: controlTextField
            width: control.width - browseButton.width - browseRow.spacing
        }
        Button {
            id: browseButton
            width: height
            text: qsTr("...")
            onClicked: {
                if (control.selectFolder) {
                    folderDlgLoader.active = true
                    folderDlgLoader.item.visible = true
                } else {
                    fileDlgLoader.active = true
                    fileDlgLoader.item.visible = true
                }
            }
        }
    }
}
