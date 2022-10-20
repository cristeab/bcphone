#pragma once

#include "pjsua.h"
#include <QObject>

class SipClient : public QObject
{
    Q_OBJECT
public:
    explicit SipClient(QObject *parent = nullptr);

    bool init();
    void release();

    bool registerAccount();
    void manuallyRegister();

    bool makeCall(const QString &userId);
    bool sendDtmf(const QString &dtmf);

    bool answer(int callId);
    bool hangup(int callId);

    bool hold(int callId);
    bool unsupervisedTransfer(const QString &phoneNumber);
    //TODO: add supervised transfer
    bool swap(int callId);
    bool merge(int callId);

    bool playDigit(const QString& digit);

    bool mute(bool value, int callId);
    bool record(bool value, int callId);

signals:
    void errorMessage(const QString& msg);

private:
    void errorHandler(const QString &title, pj_status_t status);
};
