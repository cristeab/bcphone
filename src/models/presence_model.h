#pragma once

#include "qmlhelpers.h"
#include "pjsua.h"
#include "model_constants.h"
#include <QQmlEngine>
#include <QAbstractListModel>
#include <QList>

class SipClient;
class ContactsModel;

class PresenceModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    QML_WRITABLE_PROPERTY(bool, selectBuddy, setSelectBuddy, false)

public:
    enum PresenceRoles {
        UserName = Qt::UserRole+1,
        PhoneNumber,
        Status
    };
    struct PresenceInfo {
	pjsua_buddy_id id{PJSUA_INVALID_ID};
        QString userName;
        QString phoneNumber;
        QString status;
	int contactId{models::INVALID_CONTACT_INDEX};
    };

    explicit PresenceModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int,QByteArray> roleNames() const override;

    Q_INVOKABLE void addBuddy(const QString &userId);
    Q_INVOKABLE void removeBuddy(int index);
    void updateStatus(pjsua_buddy_id id, const QString &status);
    void load();

    void setSipClient(SipClient *sipClient) { _sipClient = sipClient; }
    void setContactsModel(ContactsModel *contactsModel) { _contactsModel = contactsModel; }

signals:
    void updateModel();
    void errorMessage(const QString& msg);

private:
    bool isValidIndex(int index) const {
        return ((index >= 0) && (index < _presenceInfo.count()));
    }
    QList<PresenceInfo> _presenceInfo;
    SipClient *_sipClient = nullptr;
    ContactsModel *_contactsModel = nullptr;
};
