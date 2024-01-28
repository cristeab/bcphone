import QtQuick
import QtQuick.Controls
import "../custom"
import ".."

Dialog {
    id: control
    implicitWidth: appWin.width - 2 * Theme.windowMargin
    implicitHeight: appWin.height - 2 * Theme.windowMargin
    x: (appWin.width-width)/2
    y: (appWin.height-height)/2
    standardButtons: Dialog.Ok | Dialog.Cancel
    title: (-1 === msgDlgProps.contactIndex) ? qsTr("Add Contact") : qsTr("Edit Contact")
    modal: true
    closePolicy: Popup.NoAutoClose
    visible: false

    Component.onCompleted: {
        firstName.editText = softphone.contactsModel.firstName(msgDlgProps.contactIndex)
        lastName.editText = softphone.contactsModel.lastName(msgDlgProps.contactIndex)
        email.editText = softphone.contactsModel.email(msgDlgProps.contactIndex)
        phoneNumber.editText = softphone.contactsModel.phoneNumber(msgDlgProps.contactIndex)
        mobileNumber.editText = softphone.contactsModel.mobileNumber(msgDlgProps.contactIndex)
        address.editText = softphone.contactsModel.address(msgDlgProps.contactIndex)
        state.editText = softphone.contactsModel.state(msgDlgProps.contactIndex)
        city.editText = softphone.contactsModel.city(msgDlgProps.contactIndex)
        zipCode.editText = softphone.contactsModel.zip(msgDlgProps.contactIndex)
        comment.editText = softphone.contactsModel.comment(msgDlgProps.contactIndex)
        if (msgDlgProps.addContactFromHistory) {
            msgDlgProps.addContactFromHistory = false
            const index = softphone.callHistoryModel.currentIndex
            firstName.editText = softphone.callHistoryModel.userName(index)
            phoneNumber.editText = softphone.callHistoryModel.phoneNumber(index)
        }
    }

    function validate() {
        if ("" === firstName.editText) {
            firstName.error = true
            return false
        }
        if ("" === phoneNumber.editText) {
            phoneNumber.error = true
            return false
        }
        return true
    }

    onAccepted: {
        if (!control.validate()) {
            editContactDlg.item.visible = true
            editContactDlg.active = true

            msgDlgProps.okCancel = false
            dialogMessage.show(true, qsTr("First name or phone number are empty"))
            return
        }
        const contactId = softphone.contactsModel.contactId(msgDlgProps.contactIndex)
        softphone.contactsModel.addUpdate(contactId,
                                          firstName.editText,
                                          lastName.editText,
                                          email.editText,
                                          phoneNumber.editText,
                                          mobileNumber.editText,
                                          address.editText,
                                          state.editText,
                                          city.editText,
                                          zipCode.editText,
                                          comment.editText)
        msgDlgProps.contactIndex = -1
        editContactDlg.active = false
    }
    onRejected: {
        msgDlgProps.contactIndex = -1
        editContactDlg.active = false
    }

    Flickable {
        anchors {
            left: parent.left
            leftMargin: Theme.windowMargin
            right: parent.right
            rightMargin: Theme.windowMargin
            margins: Theme.windowMargin
        }
        height: parent.height
        contentWidth: devicesLayout.width
        contentHeight: devicesLayout.height
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.VerticalFlick

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

        Column {
            id: devicesLayout
            spacing: 10
            LabelTextField {
                id: firstName
                text: qsTr("First Name <font color='red'>*</font>")
                width: control.width - 2 * Theme.windowMargin
            }
            LabelTextField {
                id: lastName
                text: qsTr("Last Name")
                width: firstName.width
            }
            LabelTextField {
                id: phoneNumber
                text: qsTr("Phone Number <font color='red'>*</font>")
                width: firstName.width
            }
            LabelTextField {
                id: mobileNumber
                text: qsTr("Mobile Number")
                width: firstName.width
            }
            LabelTextField {
                id: email
                text: qsTr("Email")
                width: firstName.width
            }
            LabelTextField {
                id: address
                text: qsTr("Address")
                width: firstName.width
            }
            LabelTextField {
                id: state
                text: qsTr("State")
                width: firstName.width
            }
            LabelTextField {
                id: city
                text: qsTr("City")
                width: firstName.width
            }
            LabelTextField {
                id: zipCode
                text: qsTr("Zip Code")
                width: firstName.width
            }
            LabelTextField {
                id: comment
                text: qsTr("Comment")
                width: firstName.width
            }
        }
    }
}
