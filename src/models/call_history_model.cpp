#include "call_history_model.h"
#include "settings.h"
#include <QQmlEngine>

CallHistoryModel::CallHistoryModel(QObject *parent) : QAbstractListModel(parent)
{
    qmlRegisterInterface<CallHistoryModel>("CallHistoryModel", 1);
    _history = Settings::callHistoryInfo();
}

int CallHistoryModel::rowCount(const QModelIndex& /*parent*/) const
{
    return _history.size();
}

QVariant CallHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!isValidIndex(index.row())) {
        qCritical() << "invalid model index";
        return QVariant();
    }
    QVariant out;
    const auto &history = _history.at(index.row());
    switch (role) {
    case IsContact:
        out = ContactsModel::INVALID_CONTACT_ID != history.contactId;
        break;
    case UserName:
        out = history.userName;
        break;
    case PhoneNumber:
        out = history.phoneNumber;
        break;
    case CallDate:
        out = history.dateTime.toString("MMMM d, yyyy");
        break;
    case CallTime:
        out = history.dateTime.toString("hh:mm:ss");
        break;
    case CallStatusRole:
        out = callStatusToString(history.callStatus);
        break;
    default:
        qCritical() << "unknown role" << role;
    }
    return out;
}

QHash<int,QByteArray> CallHistoryModel::roleNames() const
{
    static const auto roles = QHash<int, QByteArray> {
        { IsContact, "isContact" },
        { UserName, "userName" },
        { PhoneNumber, "phoneNumber" },
        { CallDate, "callDate" },
        { CallTime, "callTime" },
        { CallStatusRole, "callStatus" }
    };
    return roles;
}

void CallHistoryModel::clear()
{
    emit layoutAboutToBeChanged();
    _history.clear();
    emit layoutChanged();
    Settings::saveCallHistoryInfo(_history);
}

void CallHistoryModel::deleteContact(int index)
{
    if (isValidIndex(index)) {
        emit layoutAboutToBeChanged();
        _history.removeAt(index);
        sortHistory();
        emit layoutChanged();
        Settings::saveCallHistoryInfo(_history);
    }
}

void CallHistoryModel::onContactAdded(int contactIndex)
{
    qDebug() << "onContactAdded" << contactIndex;
    if (isValidIndex(_currentIndex) && (nullptr != _contactsModel)) {
        emit layoutAboutToBeChanged();
        _history[_currentIndex].contactId = _contactsModel->contactId(contactIndex);;
        _history[_currentIndex].userName = formatUserName(_contactsModel->firstName(contactIndex),
                                                          _contactsModel->lastName(contactIndex));
        _history[_currentIndex].phoneNumber = _contactsModel->phoneNumber(contactIndex);
        emit layoutChanged();
    }
   setCurrentIndex(-1);
}

void CallHistoryModel::addContact(int callId, const QString &user,
                                  const QString &phone, CallStatus callStatus)
{
    qDebug() << "addContact" << callId << user << phone << callStatus;
    emit layoutAboutToBeChanged();
    CallHistoryInfo item(user, phone);
    item.callStatus = callStatus;
    item.callId = callId;
    if (nullptr != _contactsModel) {
        const auto contactIndex = _contactsModel->indexFromPhoneNumber(phone);
        if (ContactsModel::INVALID_CONTACT_INDEX != contactIndex) {
            item.userName = formatUserName(_contactsModel->firstName(contactIndex),
                                           _contactsModel->lastName(contactIndex));
        }
    }
    if (item.userName.isEmpty()) {
        // try to get the user name from history
        item.userName = userName(phone);
    }

    //limit the history size
    if (MAX_HISTORY_SIZE < _history.size()) {
        qWarning() << "Maximum history size reached, removing oldest";
        _history.removeLast();
    }

    _history.push_front(item);
    emit layoutChanged();
    Settings::saveCallHistoryInfo(_history);
}

void CallHistoryModel::updateContact(int callId, const QString &user, const QString &phone)
{
    qDebug() << "updateContact" << callId << user << phone;
    const auto index = calId2index(callId);
    if (isValidIndex(index)) {
        emit layoutAboutToBeChanged();
        if (!user.isEmpty()) {
            _history[index].userName = user;
        } else {
            _history[index].userName = userName(phone);
        }
        if (!phone.isEmpty()) {
            _history[index].phoneNumber = phone;
        }
        emit layoutChanged();
        Settings::saveCallHistoryInfo(_history);
    }
}

void CallHistoryModel::updateCallStatus(int callId, CallHistoryModel::CallStatus callStatus,
                                        bool confirmed)
{
    const auto index = calId2index(callId);
    if (isValidIndex(index)) {
        if (confirmed && (CallStatus::UNKNOWN == callStatus)) {
            _history[index].confirmed = confirmed;
            qDebug() << "updateCallStatus" << callId << confirmed;
        } else if (!_history[index].confirmed) {
            emit layoutAboutToBeChanged();
            _history[index].callStatus = callStatus;
            emit layoutChanged();
            Settings::saveCallHistoryInfo(_history);
            qDebug() << "updateCallStatus" << callId << callStatus;
        }
    }
}

void CallHistoryModel::onContactsReady()
{
    emit layoutAboutToBeChanged();
    _history = Settings::callHistoryInfo();
    qDebug() << "onContactsReady" << _history.size();
    if (nullptr != _contactsModel) {
        for (auto &it: _history) {
            const auto contactIndex = _contactsModel->indexFromContactId(it.contactId);
            if (ContactsModel::INVALID_CONTACT_INDEX != contactIndex) {
                it.userName = formatUserName(_contactsModel->firstName(contactIndex),
                                             _contactsModel->lastName(contactIndex));
                it.phoneNumber = _contactsModel->phoneNumber(contactIndex);
            }
        }
    }
    sortHistory();
    emit layoutChanged();
}

QString CallHistoryModel::formatUserName(const QString &firstName, const QString &lastName)
{
    QString userName = firstName;
    if (!userName.isEmpty()) {
        userName += ", ";
    }
    userName += lastName;
    return userName;
}

static
uint qHash(const CallHistoryModel::CallStatus &key, uint seed)
{
    return qHash(static_cast<int>(key), seed);
}
QString CallHistoryModel::callStatusToString(CallStatus callStatus)
{
    static const QHash<CallStatus, QString> statusMap {
        { CallStatus::UNKNOWN, "unknown" },
        { CallStatus::OUTGOING, "outgoing" },
        { CallStatus::INCOMING, "incoming" },
        { CallStatus::REJECTED, "rejected" },
        { CallStatus::TRANSFERRED, "transferred" }
    };
    if (statusMap.contains(callStatus)) {
        return statusMap.value(callStatus);
    }
    return "unknown";
}

void CallHistoryModel::sortHistory()
{
    std::sort(_history.begin(), _history.end(), [](const CallHistoryInfo &left,
              const CallHistoryInfo &right) {
        return left.dateTime > right.dateTime;
    });
}

int CallHistoryModel::calId2index(int callId)
{
    for (int i = 0; i < _history.size(); ++i) {
        if (callId == _history.at(i).callId) {
            return i;
        }
    }
    return ContactsModel::INVALID_CONTACT_INDEX;
}
