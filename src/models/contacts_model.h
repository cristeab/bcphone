#pragma once

#include <QAbstractListModel>
#include <QVector>

class ContactsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum { INVALID_CONTACT_ID = -1, INVALID_CONTACT_INDEX = -1 };
    enum ContactRoles {
        ContactId = Qt::UserRole+1,
        FirstName,
        LastName,
        Email,
        PhoneNumber,
        MobileNumber,
        Address,
        State,
        City,
        Zip,
        Comment
    };
    struct ContactInfo {
        int id = INVALID_CONTACT_ID;
        QString firstName;
        QString lastName;
        QString email;
        QString phoneNumber;
        QString mobileNumber;
        QString address;
        QString state;
        QString city;
        QString zip;
        QString comment;
        bool isValid() const {
            return INVALID_CONTACT_ID != id;
        }
        void clear() {
            id = INVALID_CONTACT_ID;
            firstName = lastName = email = phoneNumber = mobileNumber = address = state = city = zip = comment = "";
        }
    };

    explicit ContactsModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int,QByteArray> roleNames() const;

    Q_INVOKABLE int contactId(int index) const {
        const int id = isValidIndex(index) ? _contacts.at(index).id : INVALID_CONTACT_ID;
        return id;
    }
    Q_INVOKABLE QString firstName(int index) const {
        return isValidIndex(index) ? _contacts.at(index).firstName : "";
    }
    Q_INVOKABLE QString lastName(int index) const {
        return isValidIndex(index) ? _contacts.at(index).lastName : "";
    }
    Q_INVOKABLE QString email(int index) const {
        return isValidIndex(index) ? _contacts.at(index).email : "";
    }
    Q_INVOKABLE QString phoneNumber(int index) const {
        return isValidIndex(index) ? _contacts.at(index).phoneNumber : "";
    }
    Q_INVOKABLE QString mobileNumber(int index) const {
        return isValidIndex(index) ? _contacts.at(index).mobileNumber : "";
    }
    Q_INVOKABLE QString address(int index) const {
        return isValidIndex(index) ? _contacts.at(index).address : "";
    }
    Q_INVOKABLE QString state(int index) const {
        return isValidIndex(index) ? _contacts.at(index).state : "";
    }
    Q_INVOKABLE QString city(int index) const {
        return isValidIndex(index) ? _contacts.at(index).city : "";
    }
    Q_INVOKABLE QString zip(int index) const {
        return isValidIndex(index) ? _contacts.at(index).zip : "";
    }
    Q_INVOKABLE QString comment(int index) const {
        return isValidIndex(index) ? _contacts.at(index).comment : "";
    }
    Q_INVOKABLE void addUpdate(int id,
                               const QString& firstName,
                               const QString& lastName,
                               const QString& email,
                               const QString& phoneNumber,
                               const QString& mobileNumber,
                               const QString& address,
                               const QString& state,
                               const QString& city,
                               const QString& zipCode,
                               const QString& comment);

    void clear();
    int append(const ContactInfo &contactInfo);
    Q_INVOKABLE bool remove(int contactId);
    bool update(const ContactInfo &contactInfo);

    int indexFromContactId(int contactId);
    int indexFromPhoneNumber(const QString &phoneNumber);

private:
    bool isValidIndex(int index) const {
        return ((index >= 0) && (index < _contacts.count()));
    }
    void sortContacts();
    QVector<ContactInfo> _contacts;
};
