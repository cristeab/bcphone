#include "sip_client.h"

SipClient::SipClient(QObject *parent) : QObject{parent}
{

}

bool SipClient::init()
{

}

void SipClient::release()
{

}

bool SipClient::registerAccount()
{

}

void SipClient::manuallyRegister()
{

}

bool SipClient::makeCall(const QString &userId)
{

}

bool SipClient::sendDtmf(const QString &dtmf)
{

}

bool SipClient::answer(int callId)
{

}

bool SipClient::hold(int callId)
{

}

bool SipClient::hangup(int callId)
{

}

bool SipClient::unsupervisedTransfer(const QString &phoneNumber)
{

}

bool SipClient::swap(int callId)
{

}

bool SipClient::merge(int callId)
{

}

bool SipClient::playDigit(const QString& digit)
{

}

bool SipClient::mute(bool value, int callId)
{

}

bool SipClient::record(bool value, int callId)
{

}

void SipClient::errorHandler(const QString &title, pj_status_t status)
{
    QString fullError = title;
    if (PJ_SUCCESS != status) {
        static std::array<char, 1024> message;
        pj_strerror(status, message.data(), message.size());
        fullError.append(":").append(message.data());
    }
    emit errorMessage(fullError);
}
