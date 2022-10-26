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

const QString Softphone::_notAvailable = tr("N/A");

Softphone::Softphone() : _sipClient(new SipClient(_settings,
                                                  _ringTonesModel,
                                                  _callHistoryModel,
                                                  _activeCallModel,
                                                  this))
{
    setObjectName("softphone");
    qmlRegisterType<Softphone>("Softphone", 1, 0, "Softphone");

    //setup call duration timer
    _currentUserTimer.setInterval(1000);
    _currentUserTimer.setSingleShot(false);
    _currentUserTimer.setTimerType(Qt::PreciseTimer);
    _currentUserTimer.stop();
    connect(&_currentUserTimer, &QTimer::timeout, [&]() {
        ++_currentUserElapsedSec;
        emit currentUserElapsedSecChanged();
    });

    //init internal connections
    connect(this, &Softphone::confirmed, this, &Softphone::onConfirmed);
    connect(this, &Softphone::calling, this, &Softphone::onCalling);
    connect(this, &Softphone::incoming, this, &Softphone::onIncoming);
    connect(this, &Softphone::disconnected, this, &Softphone::onDisconnected);

    //init connections with active calls model
    connect(_activeCallModel, &ActiveCallModel::activeCallChanged, this, [this](bool value) {
        if (_conference) {
            setDialedText("");
        } else {
            setDialedText(_activeCallModel->currentPhoneNumber());
        }
    });
    connect(_activeCallModel, &ActiveCallModel::unholdCall, this, [this](int callId) {
        hold(false, callId);
    });

    //init connection with settings
    _inputAudioDevices->setSettings(_settings);
    _outputAudioDevices->setSettings(_settings);
    _videoDevices->setSettings(_settings);
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

    //connection with enable video
    connect(this, &Softphone::enableVideoChanged, this, &Softphone::onEnableVideo);

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

    //connection with call history model
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
            _audioCodecs->saveCodecsInfo();
        } else if (oldPriority != newPriority) {
            qDebug() << "Set old priority" << oldPriority;
            _audioCodecs->setCodecPriority(codecId, oldPriority);
            qInfo() << "Restored audio codec priority" << codecId << oldPriority;
        }
    });

    //connection with video codecs
    connect(_videoCodecs, &VideoCodecs::codecPriorityChanged,
            _videoCodecs, [this](const QString &codecId, int newPriority, int oldPriority) {
        if (_sipClient->setVideoCodecPriority(codecId, newPriority)) {
            _videoCodecs->saveCodecsInfo();
        } else if (oldPriority != newPriority) {
            _videoCodecs->setCodecPriority(codecId, oldPriority);
            qInfo() << "Restored video codec priority" << codecId << oldPriority;
        }
    });

    //setup presence model
    _presenceModel->setContactsModel(_contactsModel);

    //ringtones init
    _ringTonesModel->initDefaultRingTones();

    qInfo() << "SSL version" << QSslSocket::sslLibraryVersionString();
    qInfo() << "Can register" << _settings->canRegister();
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
}

Softphone::~Softphone()
{
    _sipClient->release();
}

void Softphone::onConfirmed(int callId)
{
    setConfirmedCall(true);
    _sipClient->stopPlayingRingTone(callId);

    startCurrentUserTimer();
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

void Softphone::onIncoming(int callCount, int callId, const QString &userId,
                           const QString &userName)
{
    Q_UNUSED(callCount)
    setActiveCall(true);
    //open audio device only when needed (automatically closed when the call ends)
    _sipClient->enableAudio();

    _activeCallModel->addCall(callId, userName, userId);
    _callHistoryModel->addContact(callId, userName, userId,
                                  CallHistoryModel::CallStatus::INCOMING);

    raiseWindow();
    _sipClient->startPlayingRingTone(callId, true);
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
    stopCurrentUserTimer();
    setDialedText(_activeCallModel->currentPhoneNumber());
    if (0 == _activeCallModel->callCount()) {
        setActiveCall(false);
        setConference(false);
    }
    setRecord(false);//TODO
    disableAudio();

    releaseVideoWindow();
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

void Softphone::startCurrentUserTimer()
{
    if (!_currentUserTimer.isActive()) {
        setCurrentUserElapsedSec(0);
        _currentUserTimer.start();
    }
}

void Softphone::stopCurrentUserTimer()
{
    /*if (_callModel->isEmpty()) {
        _currentUserTimer.stop();
    } else {
        setCurrentUserElapsedSec(_callModel->callDurationSec(_currentCallId));
    }*/
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

void Softphone::onEnableVideo()
{
    const auto currentCallId = _activeCallModel->currentCallId();
    if (PJSUA_INVALID_ID != currentCallId) {
        const pjsua_call_vid_strm_op op = _enableVideo ? PJSUA_CALL_VID_STRM_START_TRANSMIT : PJSUA_CALL_VID_STRM_STOP_TRANSMIT;
        const auto status = pjsua_call_set_vid_strm(currentCallId, op, nullptr);
        if (status == PJ_SUCCESS) {
            qInfo() << "Start transmitting" << _enableVideo;
        } else {
            const auto msg = _enableVideo ? tr("Cannot start transmitting video stream") : tr("Cannot stop transmitting video stream");
            errorHandler(msg, status, true);
        }
        if (_enableVideo) {
            initVideoWindow();
        } else {
            releaseVideoWindow();
        }
    }
}

void Softphone::initVideoWindow()
{
    const auto currentCallId = _activeCallModel->currentCallId();
    if (PJSUA_INVALID_ID == currentCallId) {
        return;
    }

    pjsua_call_info ci;
    auto status = pjsua_call_get_info(currentCallId, &ci);
    if (status != PJ_SUCCESS) {
        errorHandler(tr("Error get call info"), status, true);
        return;
    }
    for (unsigned i = 0; i < ci.media_cnt; ++i) {
        if ((ci.media[i].type == PJMEDIA_TYPE_VIDEO) &&
                (ci.media[i].dir & PJMEDIA_DIR_DECODING))
        {
            pjsua_vid_win_info wi;
            status = pjsua_vid_win_get_info(ci.media[i].stream.vid.win_in, &wi);
            if (status != PJ_SUCCESS) {
                errorHandler(tr("Error get vid win info"), status, true);
                return;
            }
            _videoWindow.reset(new QDialog());
            if (nullptr != _videoWindow) {
                _videoWindow->setContentsMargins(0, 0, 0, 0);
                qDebug() << "Show remote window";
                auto* layout = new QVBoxLayout(_videoWindow.get());
                layout->setSpacing(0);
                layout->setContentsMargins(0, 0, 0, 0);
                auto* remote = QWidget::createWindowContainer(QWindow::fromWinId((WId)wi.hwnd.info.win.hwnd), nullptr, Qt::Widget);
                layout->addWidget(remote);
                _videoWindow->setFixedWidth(wi.size.w);
                _videoWindow->setFixedHeight(wi.size.h);
                _videoWindow->setWindowTitle(_settings->appName());
                initPreviewWindow();
                _videoWindow->show();
            } else {
                qWarning() << "Cannot create widget from remote window";
            }
            break;
        }
    }
}

void Softphone::releaseVideoWindow()
{
    _videoWindow.reset(nullptr);
    releasePreviewWindow();
}

void Softphone::initPreviewWindow()
{
    const auto &devInfo = _videoDevices->deviceInfo();
    pjsua_vid_preview_param pre_param;
    pjsua_vid_preview_param_default(&pre_param);
    pre_param.rend_id = PJMEDIA_VID_DEFAULT_RENDER_DEV;
    pre_param.show = PJ_TRUE;
    pj_status_t status = pjsua_vid_preview_start(devInfo.index, &pre_param);
    if (status != PJ_SUCCESS) {
        errorHandler(tr("Error creating preview"), status, true);
        return;
    }
    auto wid = pjsua_vid_preview_get_win(devInfo.index);
    if (PJSUA_INVALID_ID != wid) {
        pjsua_vid_win_info wi;
        status = pjsua_vid_win_get_info(wid, &wi);
        if (status != PJ_SUCCESS) {
            errorHandler(tr("Cannot get window info"), status, true);
            return;
        }
        auto* preview = QWidget::createWindowContainer(QWindow::fromWinId((WId)wi.hwnd.info.win.hwnd), _videoWindow.get(), Qt::Widget);
        if (nullptr != preview) {
            if (nullptr != _videoWindow) {
                const auto w = _videoWindow->width();
                const auto h = _videoWindow->height();
                const auto prevW = w / 3;
                const auto prevH = h / 3;
                setVideoWindowSize(wi.is_native, wid, prevW, prevH);
                preview->setGeometry((w - prevW) / 2, h - prevH, prevW, prevH);
            } else {
                preview->setWindowTitle(_settings->appName() + tr(" - Preview Window"));
                _previewWindow.reset(preview);
            }
            preview->show();
            preview->raise();
        } else {
            qWarning() << "Cannot create preview widget";
        }
    }
}

void Softphone::releasePreviewWindow()
{
    _previewWindow.reset(nullptr);
    const auto &devInfo = _videoDevices->deviceInfo();
    const pjsua_vid_win_id wid = pjsua_vid_preview_get_win(devInfo.index);
    if (wid != PJSUA_INVALID_ID) {
        pjsua_vid_win_set_show(wid, PJ_FALSE);
        pj_status_t status = pjsua_vid_preview_stop(devInfo.index);
        if (status != PJ_SUCCESS) {
            errorHandler(tr("Error releasing preview"), status, true);
        }
    }
}

void Softphone::setVideoWindowSize(pj_bool_t isNative, pjsua_vid_win_id wid, int width, int height)
{
    if (PJ_TRUE != isNative) {
        pjmedia_rect_size size;
        size.w = width;
        size.h = height;
        const auto status = pjsua_vid_win_set_size(wid, &size);
        if (status != PJ_SUCCESS) {
            errorHandler(tr("Cannot set window size"), status, false);
        }
    } else {
        qWarning() << "Window is native";
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
