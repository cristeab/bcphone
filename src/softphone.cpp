#include "softphone.h"
#include "sip_client.h"
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QThread>
#include <QStyle>
#include <QSslSocket>
#include <QWidget>
#include <QWindow>
#include <QDialog>
#include <QVBoxLayout>
#include <array>

Softphone::Softphone()
{
    setObjectName("softphone");
    qmlRegisterType<Softphone>("Softphone", 1, 0, "Softphone");

    _inputAudioDevices->setSettings(_settings);
    _outputAudioDevices->setSettings(_settings);
    _videoDevices->setSettings(_settings);

    //init connections with active calls model
    connect(_activeCallModel, &ActiveCallModel::activeCallChanged, this, [this](bool value) {
        if (_conference) {
            setDialedText("");
        } else {
            setDialedText(_activeCallModel->currentPhoneNumber());
        }
    });
    connect(_activeCallModel, &ActiveCallModel::unholdCall, _sipClient, &SipClient::unhold);

    //init connection with settings
    connect(_settings, &Settings::inputAudioModelIndexChanged, this, [&]() {
        const auto &devInfo = _inputAudioDevices->deviceInfoFromIndex(_settings->inputAudioModelIndex());
        Settings::saveInputAudioDeviceInfo(devInfo);
    });
    connect(_settings, &Settings::outputAudioModelIndexChanged, this, [&]() {
        const auto &devInfo = _outputAudioDevices->deviceInfoFromIndex(_settings->outputAudioModelIndex());
        Settings::saveOutputAudioDeviceInfo(devInfo);
    });
    connect(_settings, &Settings::videoModelIndexChanged, this, [&]() {
        const auto &devInfo = _videoDevices->deviceInfoFromIndex(_settings->videoModelIndex());
        Settings::saveVideoDeviceInfo(devInfo);
    });

    //connection with mute microphone
    connect(this, &Softphone::muteMicrophoneChanged, _activeCallModel, [this]() {
        const auto &cids = _activeCallModel->confirmedCallsId();
        for (auto id: cids) {
            mute(_muteMicrophone, id);
        }
    });

    //connection with record
    connect(this, &Softphone::recordChanged, _activeCallModel, [this]() {
        const auto cid = _activeCallModel->currentCallId();
        _sipClient->record(_record, cid);
    });

    //connection with hold
    connect(this, &Softphone::holdCallChanged, _activeCallModel, [this]() {
        const auto cid = _activeCallModel->currentCallId();
        hold(_holdCall, cid);
    });

    //init call history model
    _callHistoryModel->setContactsModel(_contactsModel);

    //connection with volume settings
    connect(_settings, &Settings::microphoneVolumeChanged, this,
            &Softphone::onMicrophoneVolumeChanged);
    connect(_settings, &Settings::speakersVolumeChanged, this,
            &Softphone::onSpeakersVolumeChanged);

    //connection with audio codecs
    connect(_audioCodecs, &AudioCodecs::codecPriorityChanged,
            _audioCodecs, [this](const QString &codecId, int newPriority, int oldPriority) {
        qDebug() << codecId << newPriority << oldPriority;
        if (_sipClient->setAudioCodecPriority(codecId, newPriority)) {
            Settings::saveAudioCodecInfo(_audioCodecs->codecInfo());
        } else if (oldPriority != newPriority) {
            qDebug() << "Set old priority" << oldPriority;
            const_cast<AudioCodecs*>(_audioCodecs)->setCodecPriority(codecId, oldPriority);
            qInfo() << "Restored audio codec priority" << codecId << oldPriority;
        }
    });

    //connection with video codecs
    connect(_videoCodecs, &VideoCodecs::codecPriorityChanged,
            _videoCodecs, [this](const QString &codecId, int newPriority, int oldPriority) {
        if (_sipClient->setVideoCodecPriority(codecId, newPriority)) {
            Settings::saveVideoCodecInfo(_videoCodecs->codecInfo());
        } else if (oldPriority != newPriority) {
            const_cast<VideoCodecs*>(_videoCodecs)->setCodecPriority(codecId, oldPriority);
            qInfo() << "Restored video codec priority" << codecId << oldPriority;
        }
    });

    //setup presence model
    _presenceModel->setContactsModel(_contactsModel);
    connect(_presenceModel, &PresenceModel::errorMessage, this, &Softphone::errorDialog);

    //ringtones init
    _ringTonesModel->initDefaultRingTones();
}

Softphone::~Softphone()
{
    _sipClient->release();
}

bool Softphone::start()
{
    //init SIP client
    _sipClient = SipClient::instance(this);
    if (nullptr == _sipClient) {
        return false;
    }
    connect(_sipClient, &SipClient::confirmed, this, &Softphone::onConfirmed);
    connect(_sipClient, &SipClient::calling, this, &Softphone::onCalling);
    connect(_sipClient, &SipClient::incoming, this, &Softphone::onIncoming);
    connect(_sipClient, &SipClient::disconnected, this, &Softphone::onDisconnected);
    connect(_sipClient, &SipClient::errorMessage, this, &Softphone::errorDialog);

    if (_sipClient->init() && _settings->canRegister()) {
        qInfo() << "Autologin";
        const auto rc = _sipClient->registerAccount();
        if (!rc) {
            setLoggedOut(true);
        }
    } else {
        setLoggedOut(true);
    }

    connect(this, &Softphone::audioDevicesChanged, _sipClient, &SipClient::initAudioDevicesList);
    return true;
}

void Softphone::onConfirmed(int callId)
{
    setConfirmedCall(true);
    _sipClient->stopPlayingRingTone(callId);

    _activeCallModel->setCallState(callId, ActiveCallModel::CallState::CONFIRMED);
    _callHistoryModel->updateCallStatus(callId,
                                        CallHistoryModel::CallStatus::UNKNOWN,
                                        true);
    _activeCallModel->setCurrentCallId(callId);
    if (_conference) {
        setConference(false);
        _sipClient->setupConferenceCall(callId);
        _activeCallModel->update();
    }
}

void Softphone::onCalling(int callId, const QString &userId, const QString &userName)
{
    _activeCallModel->addCall(callId, userName, userId);
    _callHistoryModel->updateContact(callId, userName, userId);
}

void Softphone::onIncoming(int callId, const QString &userId, const QString &userName)
{
    setActiveCall(true);
    //open audio device only when needed (automatically closed when the call ends)
    _sipClient->enableAudio();

    _activeCallModel->addCall(callId, userName, userId);
    _callHistoryModel->addContact(callId, userName, userId,
                                  CallHistoryModel::CallStatus::INCOMING);

    raiseWindow();
    _sipClient->startPlayingRingTone(callId, true);
    emit incoming(_activeCallModel->callCount(),
                  callId,
                  userId,
                  userName,
                  _activeCallModel->isConference());
}

void Softphone::onDisconnected(int callId)
{
    setConfirmedCall(false);
    setActiveCall(false);

    _sipClient->stopPlayingRingTone(callId);
    _activeCallModel->removeCall(callId);
    _callHistoryModel->updateCallStatus(callId,
                                        CallHistoryModel::CallStatus::REJECTED,
                                        false);

    setDialedText(_activeCallModel->currentPhoneNumber());
    if (0 == _activeCallModel->callCount()) {
        setActiveCall(false);
        setConference(false);
    }
    setRecord(false);//TODO
    disableAudio();

    _sipClient->releaseVideoWindow();

    emit disconnected(callId);
}

bool Softphone::makeCall(const QString &userId)
{
    const auto rc = _sipClient->makeCall(userId);
    if (rc) {
        setActiveCall(true);
    } else {
        setDialedText("");
        setDialogError(true);
        setActiveCall(false);
    }
    return rc;
}

bool Softphone::answer(int callId)
{
    return _sipClient->answer(callId);
}

bool Softphone::hangup(int callId)
{
    return _sipClient->hangup(callId);
}

bool Softphone::unsupervisedTransfer(const QString &phoneNumber)
{
    return _sipClient->unsupervisedTransfer(phoneNumber,  "");
}

bool Softphone::holdAndAnswer(int callId)
{
    const auto callIds = _activeCallModel->confirmedCallsId();
    for (auto cid: callIds) {
        hold(true, cid);
    }
    return answer(callId);
}

bool Softphone::swap(int callId)
{
    return _sipClient->swap(callId);
}

bool Softphone::merge(int callId)
{
    return _sipClient->merge(callId);
}

bool Softphone::hold(bool value, int callId)
{
    return value ? _sipClient->hold(callId) : _sipClient->unhold(callId);
}

bool Softphone::mute(bool value, int callId)
{
    return _sipClient->mute(value, callId);
}

bool Softphone::rec(bool value, int callId)
{
    return _sipClient->record(value, callId);
}

bool Softphone::disableAudio(bool force)
{
    Q_UNUSED(force)
    return _sipClient->disableAudio();
}

bool Softphone::sendDtmf(const QString &dtmf)
{
    return _sipClient->sendDtmf(dtmf);
}

void Softphone::manuallyRegister()
{
    _sipClient->manuallyRegister();
}

void Softphone::hangupAll()
{
    _sipClient->hangupAll();
}

void Softphone::raiseWindow()
{
    qDebug() << "Raise window";
    if (nullptr != _mainForm) {
        if (!QMetaObject::invokeMethod(_mainForm, "raiseWindow",
                                       Qt::AutoConnection)) {
            qCritical() << "Cannot call raiseWindow method";
        }
    } else {
        qCritical() << "mainForm object is NULL";
    }
}

void Softphone::onMicrophoneVolumeChanged()
{
    qDebug() << "onMicrophoneVolumeChanged";
    const auto currentCallId = _activeCallModel->currentCallId();
    _sipClient->setMicrophoneVolume(currentCallId);
}

void Softphone::onSpeakersVolumeChanged()
{
    qDebug() << "onSpeakersVolumeChanged";
    const auto currentCallId = _activeCallModel->currentCallId();
    _sipClient->setSpeakersVolume(currentCallId);
}

bool Softphone::playDigit(const QString& digit)
{
    return _sipClient->playDigit(digit);
}
