#include "presence_model.h"
#include "call_history_model.h"
#include "sip_client.h"
#include "settings.h"

int PresenceModel::rowCount(const QModelIndex& /*parent*/) const
{
    return _presenceInfo.size();
}

QVariant PresenceModel::data(const QModelIndex &index, int role) const
{
    if (!isValidIndex(index.row())) {
        qCritical() << "invalid model index";
        return QVariant();
    }
    QVariant out;
    const auto &info = _presenceInfo.at(index.row());
    switch (role) {
    case UserName:
        out = info.userName;
        break;
    case PhoneNumber:
        out = info.phoneNumber;
        break;
    case Status:
        out = info.status;
        break;
    }
    return out;
}

QHash<int,QByteArray> PresenceModel::roleNames() const
{
    static const auto roles = QHash<int, QByteArray> {
        { UserName, "userName" },
        { PhoneNumber, "phoneNumber" },
        { Status, "status" }
    };
    return roles;
}

void PresenceModel::addBuddy(const QString &userId)
{
    if (nullptr == _sipClient) {
        emit errorMessage(tr("Cannot add buddy"));
        return;
    }
    const auto buddyId = _sipClient->addBuddy(userId);
    if (PJSUA_INVALID_ID == buddyId) {
        return;
    }
    emit layoutAboutToBeChanged();
    PresenceInfo info;
    info.id = buddyId;
    info.phoneNumber = userId;
    if (nullptr != _contactsModel) {
        const auto index = _contactsModel->indexFromPhoneNumber(userId);
	if (models::INVALID_CONTACT_INDEX != index) {
            info.userName = CallHistoryModel::formatUserName(_contactsModel->firstName(index),
                                                             _contactsModel->lastName(index));
        }
    }
    _presenceInfo << info;
    emit layoutChanged();
}

void PresenceModel::removeBuddy(int index)
{
    if (nullptr == _sipClient) {
        emit errorMessage(tr("Cannot remove buddy"));
        return;
    }
    qDebug() << "removeBuddy" << index;
    if (!isValidIndex(index)) {
        qWarning() << "Invalid index" << index;
        return;
    }
    const auto ok = _sipClient->removeBuddy(_presenceInfo.at(index).id);
    if (!ok) {
        return;
    }
    emit layoutAboutToBeChanged();
    _presenceInfo.removeAt(index);
    emit layoutChanged();
}

void PresenceModel::updateStatus(pjsua_buddy_id id, const QString &status)
{
    const auto rc = pjsua_buddy_is_valid(id);
    if (PJ_TRUE != rc) {
        emit errorMessage(tr("Invalid buddy ID"));
        return;
    }
    for (auto &info: _presenceInfo) {
        if (id == info.id) {
            emit layoutAboutToBeChanged();
            info.status = status;
            qDebug() << "Update status" << id << status;
            emit layoutChanged();
            emit updateModel();
            break;
        }
    }
}

void PresenceModel::load()
{
    if (nullptr == _sipClient) {
	emit errorMessage(tr("SIP client is NULL"));
        return;
    }
    if (nullptr == _contactsModel) {
	emit errorMessage(tr("Contacts model is NULL"));
	return;
    }

    emit layoutAboutToBeChanged();
    _presenceInfo.clear();
    for (int index = 0; index < _contactsModel->rowCount(); ++index) {
	const auto& userId{_contactsModel->phoneNumber(index)};
        const auto buddyId = _sipClient->addBuddy(userId);
        if (PJSUA_INVALID_ID == buddyId) {
            continue;
        }
        PresenceInfo info;
        info.id = buddyId;
        info.phoneNumber = userId;
        if (nullptr != _contactsModel) {
            const auto index = _contactsModel->indexFromPhoneNumber(userId);
	    if (models::INVALID_CONTACT_INDEX != index) {
                info.userName = CallHistoryModel::formatUserName(_contactsModel->firstName(index),
                                                                 _contactsModel->lastName(index));
            }
        }
        _presenceInfo << info;
    }
    qDebug() << "Loaded" << _presenceInfo.count() << "buddies";
    emit layoutChanged();
}
