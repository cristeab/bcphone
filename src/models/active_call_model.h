#pragma once

#include "pjsua.h"
#include <QHash>
#include <QDateTime>
#include <QAbstractListModel>

class ActiveCallModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int callCount READ callCount NOTIFY callCountChanged)
    Q_PROPERTY(QString currentUserName READ currentUserName NOTIFY currentUserNameChanged)
    Q_PROPERTY(QString currentPhoneNumber READ currentPhoneNumber NOTIFY currentUserNameChanged)

public:
    enum class CallState { UNKNOWN, PENDING, CONFIRMED, ON_HOLD };
    enum CallHistoryRoles {
        UserName= Qt::UserRole+1,
        PhoneNumber,
        CallId,
        IsCurrentCall
    };

    explicit ActiveCallModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int,QByteArray> roleNames() const;

    void addCall(int callId, const QString &userName, const QString &phoneNumber);
    void setCallState(int callId, CallState callState);
    void removeCall(int callId);

    QVector<int> confirmedCallsId(bool includePending = false) const;
    bool isConference() const { return 1 < confirmedCallsId().size(); }

    bool isEmpty() const { return _callInfo.isEmpty(); }
    int callCount() const { return _callInfo.size(); }

    QString currentUserName() const;
    QString currentPhoneNumber() const;

    void setCurrentCallId(int callId);
    constexpr int currentCallId() const { return _currentCallId; }
    void update(bool active = true);

signals:
    void callCountChanged();
    void currentUserNameChanged();
    void activeCallChanged(bool value);
    void unholdCall(int callId);

private:
    struct CallInfo {
        QString userName;
        QString phoneNumber;
        QDateTime callStartTime;
        bool incoming = true;
        CallState callState = CallState::UNKNOWN;
    };
    bool isCurrentCall(int callId) const;
    bool isValidIndex(int index) const {
            return ((index >= 0) && (index < _callOrder.count()));
        }
    QHash<int, CallInfo> _callInfo;//key is the call ID
    QVector<int> _callOrder;
    int _currentCallId = PJSUA_INVALID_ID;
};
