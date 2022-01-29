import QtQuick
import QtQuick.Controls
import ".."

Column {
    id: control
    property int numDecimals: 2
    property string text: ""
    property alias value: controlSlider.value
    property alias from: controlSlider.from
    property alias to: controlSlider.to
    property alias stepSize: controlSlider.stepSize
    spacing: 0
    Label {
        id: controlLabel
        text: control.text + " (" + controlSlider.value.toFixed(control.numDecimals) + ")"
        width: parent.width
        elide: Text.ElideRight
        font {
            italic: true
            pointSize: Theme.labelFontSize
        }
    }
    Slider {
        id: controlSlider
        width: parent.width
    }
}
