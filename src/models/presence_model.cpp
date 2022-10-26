#include "presence_model.h"
#include "call_history_model.h"
#include "sip_client.h"
#include "settings.h"

PresenceModel::PresenceModel(QObject *parent) : QAbstractListModel(parent)
{
}

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
    if (PJSUA_MAX_BUDDIES <= _presenceInfo.size()) {
        emit errorMessage(tr("Maximum number of buddies exceeded"));
        return;
    }
    pjsua_buddy_config buddyCfg;
    pjsua_buddy_config_default(&buddyCfg);
    std::string uriBuffer;
    const bool rc = SipClient::callUri(&buddyCfg.uri, userId, uriBuffer);
    if (!rc) {
        return;
    }
    pjsua_buddy_id buddyId{};
    buddyCfg.subscribe = PJ_TRUE;
    buddyCfg.user_data = nullptr;
    const pj_status_t status = pjsua_buddy_add(&buddyCfg, &buddyId);
    if (PJ_SUCCESS != status) {
        emit errorMessage(tr("Cannot add buddy"));
        return;
    }
    qDebug() << "Added buddy" << userId << buddyId;
    emit layoutAboutToBeChanged();
    PresenceInfo info;
    info.id = buddyId;
    info.phoneNumber = userId;
    if (nullptr != _contactsModel) {
        const auto index = _contactsModel->indexFromPhoneNumber(userId);
        if (ContactsModel::INVALID_CONTACT_INDEX != index) {
            info.userName = CallHistoryModel::formatUserName(_contactsModel->firstName(index),
                                                             _contactsModel->lastName(index));
        }
    }
    _presenceInfo << info;
    emit layoutChanged();
    Settings::setBuddyList(_presenceInfo);
}

void PresenceModel::removeBuddy(int index)
{
    qDebug() << "removeBuddy" << index;
    if (!isValidIndex(index)) {
        qWarning() << "Invalid index" << index;
        return;
    }
    const pj_status_t status = pjsua_buddy_del(_presenceInfo.at(index).id);
    if (PJ_SUCCESS != status) {
        emit errorMessage(tr("Cannot del buddy"));
        return;
    }
    emit layoutAboutToBeChanged();
    _presenceInfo.removeAt(index);
    emit layoutChanged();
    Settings::setBuddyList(_presenceInfo);
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
    const auto &buddies = Settings::buddyList();
    if (buddies.isEmpty()) {
        return;
    }
    emit layoutAboutToBeChanged();
    _presenceInfo.clear();
    for (const auto &userId: buddies) {
        pjsua_buddy_config buddyCfg;
        pjsua_buddy_config_default(&buddyCfg);
        std::string uriBuffer;
        const bool rc = SipClient::callUri(&buddyCfg.uri, userId, uriBuffer);
        if (!rc) {
            return;
        }
        pjsua_buddy_id buddyId{};
        buddyCfg.subscribe = PJ_TRUE;
        buddyCfg.user_data = nullptr;
        const pj_status_t status = pjsua_buddy_add(&buddyCfg, &buddyId);
        if (PJ_SUCCESS != status) {
            emit errorMessage(tr("Cannot add buddy"));
            return;
        }
        qDebug() << "Added buddy" << userId << buddyId;
        PresenceInfo info;
        info.id = buddyId;
        info.phoneNumber = userId;
        if (nullptr != _contactsModel) {
            const auto index = _contactsModel->indexFromPhoneNumber(userId);
            if (ContactsModel::INVALID_CONTACT_INDEX != index) {
                info.userName = CallHistoryModel::formatUserName(_contactsModel->firstName(index),
                                                                 _contactsModel->lastName(index));
            }
        }
        _presenceInfo << info;
    }
    emit layoutChanged();
}
