import QtQuick
import QtQuick.Controls.Material

pragma Singleton

QtObject {
    function formatWithNa(val) {
        return ("" !== val) ? val : "N/A"
    }
    function formatWithComma(left, right) {
        if (("" !== left) && ("" !== right)) {
            return left + ", " + right
        }
        if ("" !== left) {
            return left
        }
        return right
    }
    function formatWithComma4(v0, v1, v2, v3) {
        return formatWithComma(formatWithComma(formatWithComma(v0, v1), v2), v3)
    }
    function formatNotEmpty(first, second) {
        if ("" !== first) {
            return first
        }
        if ("" !== second) {
            return second
        }
        return "N/A"
    }

    function removeFormattingHelper(num) {
        return num.replace('(', '').replace(')', '').replace(/ /g, '')
    }
    function formatTelephoneNumber(num) {
        const onlyDigitsRe = new RegExp('^\\d+$')
        if (onlyDigitsRe.test(num) && (10 === num.length)) {
            return '(' + num.substr(0, 3) + ') ' + num.substr(3, 3) + ' ' + num.substr(6)
        }
        if (14 > num.length) {
            return removeFormattingHelper(num)
        }
        return num
    }
    function removeFormatting(num) {
        if (14 <= num.length) {
            return removeFormattingHelper(num)
        }
        return num
    }

    readonly property real dialpadButtonFontSize: 24
    readonly property real dialpadSubTextFontSize: 8
    readonly property real buttonFontSize: 12
    readonly property real tabButtonFontSize: 10
    readonly property real textFontSize: 12
    readonly property real labelFontSize: 12
    readonly property real tabFontSize: 14
    readonly property real titleFontSize: 20

    readonly property real dialogMargin: 80
    readonly property real windowMargin: 10
    readonly property real buttonHeight: 35
    readonly property real toolButtonHeight: 20
    readonly property real spinBoxWidth: 120
    readonly property real dialpadButtonHeight: 65

    readonly property color backgroundColor: Material.background
    readonly property color tabButtonColor: Material.foreground
    readonly property color tabButtonColorSel: Material.accent
    readonly property color sepColor: Material.color(Material.Grey)
    readonly property color sepColor2: Material.color(Material.BlueGrey)
    readonly property color dialButtonBackgroundColor: "#e2e2e2"
    readonly property color dialButtonTextColor: "black"
    readonly property color callButtonColor: Material.color(Material.Green)
    readonly property color activeCallButtonColor: Material.color(Material.Red)
    readonly property color confCallButtonColor: Material.color(Material.DeepOrange)
    readonly property color blueButtonColor: Material.color(Material.Blue)
    readonly property color greenButtonColor: Material.color(Material.Green)
    readonly property color errorColor: Material.accent

    readonly property color swipeRemoveItemColor: Material.color(Material.Amber)
    readonly property color swipeEditItemColor: Material.color(Material.Cyan)

    readonly property color incomingColor: Material.color(Material.Lime)
    readonly property color outgoingColor: Material.color(Material.Purple)

    readonly property color notRegisteredColor: Material.color(Material.Red)
    readonly property color registeredColor: Material.color(Material.Green)
    readonly property color inProgressRegistrationColor: Material.color(Material.Yellow)
}
