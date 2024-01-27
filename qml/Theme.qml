import QtQuick
import QtQuick.Controls.Material
import Softphone 1.0

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

    function sipStatusColor() {
        let col = Theme.unknownSipStatusColor
        switch (softphone.sipRegistrationStatus) {
        case Softphone.Unregistered:
            col = Theme.unregisteredColor
            break
        case Softphone.RegistrationInProgress:
            col = Theme.inProgressRegistrationColor
            break
        case Softphone.Registered:
            col = Theme.registeredColor
            break
        }
        return col
    }

    readonly property int dialpadButtonFontSize: 24
    readonly property int dialpadSubTextFontSize: 8
    readonly property int buttonFontSize: 12
    readonly property int tabButtonFontSize: 10
    readonly property int textFontSize: 12
    readonly property int labelFontSize: 12
    readonly property int tabFontSize: 14
    readonly property int titleFontSize: 20

    readonly property int dialogMargin: 80
    readonly property int windowMargin: 10
    readonly property int buttonHeight: 35
    readonly property int toolButtonHeight: 20
    readonly property int spinBoxWidth: 120
    readonly property int dialpadButtonHeight: 65
    readonly property int sendAreaHeight: 75

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

    readonly property color unknownSipStatusColor: Material.color(Material.BlueGrey)
    readonly property color unregisteredColor: Material.color(Material.Red)
    readonly property color registeredColor: Material.color(Material.Green)
    readonly property color inProgressRegistrationColor: Material.color(Material.Yellow)
}
