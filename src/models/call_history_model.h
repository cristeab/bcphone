#pragma once

#include "qmlhelpers.h"
#include "contacts_model.h"
#include "pjsua.h"
#include <QAbstractListModel>
#include <QVector>
#include <QDateTime>

class ContactsModel;

class CallHistoryModel : public QAbstractListModel
{
    Q_OBJECT
    QML_WRITABLE_PROPERTY(int, currentIndex, setCurrentIndex, -1)
public:
    enum class CallStatus { UNKNOWN, OUTGOING, INCOMING, REJECTED, TRANSFERRED };
    Q_ENUM(CallStatus)

    enum CallHistoryRoles {
        IsContact = Qt::UserRole+1,
        UserName,
        PhoneNumber,
        CallDate,
        CallTime,
        CallStatusRole
    };
    struct CallHistoryInfo {
        int contactId = ContactsModel::INVALID_CONTACT_ID;
        QString userName;
        QString phoneNumber;
        QDateTime dateTime;
        CallStatus callStatus = CallStatus::UNKNOWN;
        bool confirmed = false;
        int callId = PJSUA_INVALID_ID;
        CallHistoryInfo() = default;
        CallHistoryInfo(const QString &user, const QString &phone) :
            userName(user), phoneNumber(phone) {
            dateTime = QDateTime::currentDateTime();
        }
        void clear() {
            contactId = ContactsModel::INVALID_CONTACT_ID;
            userName = phoneNumber = "";
            dateTime = QDateTime();
        }
    };

    explicit CallHistoryModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int,QByteArray> roleNames() const;

    void setContactsModel(ContactsModel *cm) { _contactsModel = cm; }

    Q_INVOKABLE void clear();
    Q_INVOKABLE void deleteContact(int index);

    Q_INVOKABLE QString userName(int index) const {
        return isValidIndex(index) ? _history.at(index).userName : "";
    }
    QString userName(const QString &phone) const {
        for (const auto &it: _history) {
            if (phone == it.phoneNumber) {
                return it.userName;
            }
        }
        return "";
    }
    Q_INVOKABLE QString phoneNumber(int index) const {
        return isValidIndex(index) ? _history.at(index).phoneNumber : "";
    }
    void onContactAdded(int contactIndex);
    void addContact(int callId, const QString &user, const QString &phone,
                    CallStatus callStatus);
    void updateContact(int callId, const QString &user, const QString &phone);
    void updateCallStatus(int callId, CallStatus callStatus, bool confirmed);
    void onContactsReady();

    static QString formatUserName(const QString &firstName, const QString &lastName);

private:
    enum { MAX_HISTORY_SIZE = 100 };
    bool isValidIndex(int index) const {
        return ((index >= 0) && (index < _history.count()));
    }
    static QString callStatusToString(CallStatus callStatus);
    void sortHistory();
    int calId2index(int callId);
    QVector<CallHistoryInfo> _history;
    ContactsModel *_contactsModel = nullptr;
};
