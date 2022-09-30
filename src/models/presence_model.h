#pragma once

#include "qmlhelpers.h"
#include "pjsua.h"
#include <QQmlEngine>
#include <QAbstractListModel>
#include <QList>

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
        pjsua_buddy_id id;
        QString userName;
        QString phoneNumber;
        QString status;
    };

    explicit PresenceModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int,QByteArray> roleNames() const override;

    Q_INVOKABLE void addBuddy(const QString &userId);
    Q_INVOKABLE void removeBuddy(int index);
    void updateStatus(pjsua_buddy_id id, const QString &status);
    void load();

    void setContactsModel(ContactsModel *contactsModel) { _contactsModel = contactsModel; }

signals:
    void updateModel();

private:
    bool isValidIndex(int index) const {
        return ((index >= 0) && (index < _presenceInfo.count()));
    }
    QList<PresenceInfo> _presenceInfo;
    ContactsModel *_contactsModel = nullptr;
};
