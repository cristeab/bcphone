import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Column {
    id: control
    property var codecsModel
    property alias text: controlLabel.text
    property int additionalRows: 5
    height: controlLabel.height + controlTable.height + control.spacing
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
    TableView {
        id: controlTable
        readonly property int delegateHeight: 35
        model: control.codecsModel
        width: parent.width
        height: (controlTable.rows + control.additionalRows) * controlTable.delegateHeight + (controlTable.rows - 1) * controlTable.rowSpacing
        clip: true
        boundsBehavior: ListView.StopAtBounds
        delegate: RowLayout {
            implicitWidth: controlTable.width
            implicitHeight: controlTable.delegateHeight
            Label {
                Layout.leftMargin: 4 * Theme.windowMargin
                visible: !delegateSpinBox.visible
                enabled: control.codecsModel.isEnabled(index)
                text: display
                clip: true
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                rightPadding: Theme.windowMargin
            }
            SpinBox {
                id: delegateSpinBox
                enabled: itemCheckBox.checked
                visible: index >= controlTable.rows
                Layout.preferredHeight: controlTable.delegateHeight - 2
                Layout.preferredWidth: Theme.spinBoxWidth
                value: visible ? display : 0
                onValueModified: edit = value
                editable: true
                from: 0
                to: 255
            }
            CheckBox {
                id: itemCheckBox
                visible: delegateSpinBox.visible
                checked: control.codecsModel.isEnabled(index - controlTable.rows)
                onCheckedChanged: {
                    if (control.codecsModel.setChecked(index - controlTable.rows, checked)) {
                        if (checked) {
                            edit = control.codecsModel.defaultPriority(index - controlTable.rows)
                        } else {
                            edit = 0
                        }
                    }
                }
            }
        }
    }
}
