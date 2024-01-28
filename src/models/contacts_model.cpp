#include "contacts_model.h"
#include "settings.h"

ContactsModel::ContactsModel(QObject *parent) : QAbstractListModel(parent)
{
    _contacts = Settings::contactsInfo();
}

int ContactsModel::rowCount(const QModelIndex& /*parent*/) const
{
    return _contacts.size();
}

QVariant ContactsModel::data(const QModelIndex &index, int role) const
{
    if (!isValidIndex(index.row())) {
        qCritical() << "invalid model index";
        return QVariant();
    }
    QVariant out;
    const auto &contact = _contacts.at(index.row());
    switch (role) {
    case ContactId:
        out = contact.id;
        break;
    case FirstName:
        out = contact.firstName;
        break;
    case LastName:
        out = contact.lastName;
        break;
    case Email:
        out = contact.email;
        break;
    case PhoneNumber:
        out = contact.phoneNumber;
        break;
    case MobileNumber:
        out = contact.mobileNumber;
        break;
    case Address:
        out = contact.address;
        break;
    case State:
        out = contact.state;
        break;
    case City:
        out = contact.city;
        break;
    case Zip:
        out = contact.zip;
        break;
    case Comment:
        out = contact.comment;
        break;
    default:
        qCritical() << "unknown role" << role;
    }
    return out;
}

QHash<int,QByteArray> ContactsModel::roleNames() const
{
    static const auto roles = QHash<int, QByteArray> {
        { ContactId, "contactId" },
        { FirstName, "firstName" },
        { LastName, "lastName" },
        { Email, "email" },
        { PhoneNumber, "phoneNumber" },
        { MobileNumber, "mobileNumber" },
        { Address, "address" },
        { State, "state" },
        { City, "city" },
        { Zip, "zip" },
        { Comment, "comment" }
    };
    return roles;
}

bool ContactsModel::remove(int contactId)
{
    bool rc = false;
    for (int i = 0; i < _contacts.size(); ++i) {
        if (contactId == _contacts.at(i).id) {
            emit layoutAboutToBeChanged();
            _contacts.removeAt(i);
            emit layoutChanged();
            rc = true;
            Settings::saveContactsInfo(_contacts);
            break;
        }
    }
    return rc;
}

bool ContactsModel::update(const ContactInfo &contactInfo)
{
    for (int i = 0; i < _contacts.size(); ++i) {
        if (_contacts.at(i).id == contactInfo.id) {
            emit layoutAboutToBeChanged();
            _contacts.replace(i, contactInfo);
            emit layoutChanged();
            sortContacts();
            Settings::saveContactsInfo(_contacts);
            return true;
        }
    }
    return false;
}

void ContactsModel::clear()
{
    emit layoutAboutToBeChanged();
    _contacts.clear();
    emit layoutChanged();
    Settings::saveContactsInfo(_contacts);
}

int ContactsModel::append(const ContactInfo &contactInfo)
{
    emit layoutAboutToBeChanged();
    _contacts << contactInfo;
    sortContacts();
    emit layoutChanged();
    Settings::saveContactsInfo(_contacts);
    return _contacts.size() - 1;
}

int ContactsModel::indexFromContactId(int contactId)
{
    for (int i = 0; i < _contacts.size(); ++i) {
        if (contactId == _contacts.at(i).id) {
            return i;
        }
    }
    return INVALID_CONTACT_INDEX;
}

int ContactsModel::indexFromPhoneNumber(const QString &phoneNumber)
{
    for (int i = 0; i < _contacts.size(); ++i) {
        if (phoneNumber == _contacts.at(i).phoneNumber) {
            return i;
        }
    }
    return INVALID_CONTACT_INDEX;
}

void ContactsModel::sortContacts()
{
    std::sort(_contacts.begin(), _contacts.end(), [](const ContactInfo &left,
              const ContactInfo &right) {
        if (left.firstName == right.firstName) {
            return left.lastName > right.lastName;
        }
        return left.firstName > right.firstName;
    });
}

void ContactsModel::addUpdate(int id,
                              const QString& firstName,
                              const QString& lastName,
                              const QString& email,
                              const QString& phoneNumber,
                              const QString& mobileNumber,
                              const QString& address,
                              const QString& state,
                              const QString& city,
                              const QString& zipCode,
                              const QString& comment)
{
    ContactInfo contactInfo{id, firstName, lastName, email, phoneNumber, mobileNumber,
                           address, state, city, zipCode, comment};
    const auto index = indexFromContactId(id);
    if (isValidIndex(index)) {
	qDebug() << "Update contact" << id << index;
        update(contactInfo);
    } else {
	qDebug() << "Append contact" << id << index;
        append(contactInfo);
    }
}
