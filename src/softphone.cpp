#include "softphone.h"
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

static Softphone *_instance = nullptr;
const QString Softphone::_notAvailable = tr("N/A");

Softphone::Softphone()
{
    setObjectName("softphone");
    qmlRegisterType<Softphone>("Softphone", 1, 0, "Softphone");
    _instance = this;

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
        if (_record) {
            startRecording(cid);
        } else {
            stopRecording(cid);
        }
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
        if (setAudioCodecPriority(codecId, newPriority)) {
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
        if (setVideoCodecPriority(codecId, newPriority)) {
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
    if (init() && _settings->canRegister()) {
        qInfo() << "Autologin";
        const auto rc = registerAccount();
        if (!rc) {
            setLoggedOut(true);
        }
    } else {
        setLoggedOut(true);
    }

    connect(this, &Softphone::audioDevicesChanged, this, &Softphone::initAudioDevicesList);
}

Softphone::~Softphone()
{
    release();
}

void Softphone::onConfirmed(int callId)
{
    setConfirmedCall(true);
    stopPlayingRingTone(callId);

    startCurrentUserTimer();
    _activeCallModel->setCallState(callId, ActiveCallModel::CallState::CONFIRMED);
    _callHistoryModel->updateCallStatus(callId,
                                        CallHistoryModel::CallStatus::UNKNOWN,
                                        true);
    _activeCallModel->setCurrentCallId(callId);
    if (_conference) {
        setConference(false);
        setupConferenceCall(callId);
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
    enableAudio();

    _activeCallModel->addCall(callId, userName, userId);
    _callHistoryModel->addContact(callId, userName, userId,
                                  CallHistoryModel::CallStatus::INCOMING);

    raiseWindow();
    startPlayingRingTone(callId, true);
}

void Softphone::onDisconnected(int callId)
{
    setConfirmedCall(false);
    setActiveCall(false);

    stopPlayingRingTone(callId);
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

void Softphone::onRegState(pjsua_acc_id acc_id)
{
    if (nullptr == _instance) {
        return;
    }
    pjsua_acc_info info;
    pj_status_t status = pjsua_acc_get_info(acc_id, &info);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot get account information", status, true);
    } else {
        bool registered = false;
        auto regStatus = RegistrationStatus::UNREGISTERED;
        switch (info.status) {
        case PJSIP_SC_OK:
            regStatus = RegistrationStatus::REGISTERED;
            registered = true;
            break;
        case PJSIP_SC_TRYING:
            //falls through
        case PJSIP_SC_PROGRESS:
            regStatus = RegistrationStatus::IN_PROGRESS;
            break;
        default:
            ;
        }
        _instance->setRegistrationStatus(regStatus);
        const pj_str_t statusText = info.status_text;
        const QString msg = QString::fromUtf8(pj_strbuf(&statusText),
                                        static_cast<int>(pj_strlen(&statusText)));
        _instance->setRegistrationText(msg);
        qDebug() << "Registration status" << info.status << msg;
        static uint8_t errCount = 0;
        if (!registered && (PJSIP_SC_TRYING != info.status)) {
            if (ERROR_COUNT_MAX < errCount) {
                errorHandler("Registration status "+msg, PJ_SUCCESS, true);
                _instance->setDialogRetry((PJSIP_SC_SERVICE_UNAVAILABLE == info.status) ||
                                          (PJSIP_SC_TEMPORARILY_UNAVAILABLE == info.status));
                errCount = 0;
            } else {
                qDebug() << "Ignoring current error";
                ++errCount;
            }
        }
        if (registered) {
            if (0 < errCount) {
                errCount = 0;
                _instance->setDialogMessage("");
            }
            if (_instance->loggedOut() || _instance->showBusy()) {
                _instance->setLoggedOut(false);
                _instance->setShowBusy(false);
                _instance->_settings->save();
            }
        }
    }
}

bool Softphone::setVideoCodecPriority(const QString &codecId, int priority)
{
    if ((0 > priority) || (priority > 255)) {
        qWarning() << "Invalid video codec priority" << codecId << priority;
        return false;
    }
    const auto stdCodecId = codecId.toStdString();
    pj_str_t pjCodecId;
    pj_cstr(&pjCodecId, stdCodecId.c_str());
    const auto status = pjsua_vid_codec_set_priority(&pjCodecId, priority);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot set video codec priority", status);
    }
    return PJ_SUCCESS == status;
}

void Softphone::onCallState(pjsua_call_id callId, pjsip_event *e)
{
    PJ_UNUSED_ARG(e);
    if (nullptr == _instance) {
        return;
    }

    pjsua_call_info ci;
    pjsua_call_get_info(callId, &ci);
    const QString info = toString(ci.state_text);
    const QString status = toString(ci.last_status_text);
    qDebug() << "Call" << callId << ", state =" << info << "(" << ci.last_status << ")" << status;

    const auto lastStatus = static_cast<SipErrorCodes>(ci.last_status);
    if ((SipErrorCodes::BadRequest <= lastStatus) &&
            (SipErrorCodes::RequestTerminated != lastStatus) &&
            (SipErrorCodes::RequestTimeout != lastStatus) &&
            !_instance->_manualHangup) {
        //show all SIP errors above Client Failure Responses
        _instance->setDialogError(true);
        _instance->setDialogMessage(status + " (" + QString::number(ci.last_status) + ")");
    }
    _instance->_manualHangup = false;

    switch (ci.state) {
    case PJSIP_INV_STATE_NULL:
        break;
    case PJSIP_INV_STATE_CALLING: {
        const QString remoteInfo = toString(ci.remote_info);
        const auto userNameAndId = extractUserNameAndId(remoteInfo);
        const QString &userName = std::get<0>(userNameAndId);
        const QString &userId = std::get<1>(userNameAndId);
        emit _instance->calling(callId, userId, userName);
    }
        break;
    case PJSIP_INV_STATE_INCOMING:
        break;
    case PJSIP_INV_STATE_EARLY:
        break;
    case PJSIP_INV_STATE_CONNECTING:
        break;
    case PJSIP_INV_STATE_CONFIRMED:
        _instance->connectCallToSoundDevices(ci.conf_slot);
        emit _instance->confirmed(callId);//must be last
        break;
    case PJSIP_INV_STATE_DISCONNECTED:
        emit _instance->disconnected(callId);
        break;
    default:
        qCritical() << "unhandled call state" << ci.state;
    }
}

void Softphone::onCallMediaState(pjsua_call_id callId)
{
    if (nullptr == _instance) {
        return;
    }
    pjsua_call_info ci;
    pj_status_t status = pjsua_call_get_info(callId, &ci);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot get media status", status, false);
        return;
    }
    pj_str_t statusTxt = ci.last_status_text;
    qDebug() << "Media state changed for call" << callId << ":" << toString(statusTxt)
             << ci.last_status;
    if (PJSUA_CALL_MEDIA_ACTIVE == ci.media_status) {
        qInfo() << "Media active" << ci.media_cnt;
        bool hasVideo = false;
        for (unsigned medIdx = 0; medIdx < ci.media_cnt; ++medIdx) {
            if (isMediaActive(ci.media[medIdx])) {
                pjsua_stream_info streamInfo;
                status = pjsua_call_get_stream_info(callId, medIdx, &streamInfo);
                if (PJ_SUCCESS == status) {
                    if (PJMEDIA_TYPE_AUDIO == streamInfo.type) {
                        _instance->connectCallToSoundDevices(ci.conf_slot);
                        const auto &fmt = streamInfo.info.aud.fmt;
                        qInfo() << "Audio codec info: encoding" << toString(fmt.encoding_name)
                                << ", clock rate" << fmt.clock_rate << "Hz, channel count"
                                << fmt.channel_cnt;

                    } else if (PJMEDIA_TYPE_VIDEO == streamInfo.type) {
                        const auto &fmt = streamInfo.info.vid.codec_info;
                        qInfo() << "Video codec info: encoding" << toString(fmt.encoding_name)
                                << ", encoding desc." << toString(fmt.encoding_desc)
                                << ", clock rate:" << fmt.clock_rate << "Hz";
                        hasVideo = true;
                    }
                } else {
                    errorHandler("Cannot get stream info", status, false);
                }
            }
        }
        _instance->setHasVideo(hasVideo);
    } else if ((PJSUA_CALL_MEDIA_LOCAL_HOLD != ci.media_status) &&
               (PJSUA_CALL_MEDIA_REMOTE_HOLD != ci.media_status)) {
        qWarning() << "Connection lost";
        if (_instance->dialogMessage().isEmpty()) {
            _instance->setDialogRetry(true);
            _instance->errorDialog(tr("You need an active Internet connection to make calls."));
            _instance->setRegistrationStatus(Softphone::UNREGISTERED);
            _instance->setRegistrationText(tr("Connection lost"));
        }
    }
}

void Softphone::onStreamCreated(pjsua_call_id call_id, pjmedia_stream *strm,
                                unsigned stream_idx, pjmedia_port **p_port)
{
    Q_UNUSED(strm)
    Q_UNUSED(p_port)
    qInfo() << "onStreamCreated" << call_id << stream_idx;
    //dumpStreamStats(strm);
}

void Softphone::onStreamDestroyed(pjsua_call_id call_id, pjmedia_stream *strm,
                                  unsigned stream_idx)
{
    qInfo() << "onStreamDestroyed" << call_id << stream_idx;
    dumpStreamStats(strm);
}

void Softphone::onBuddyState(pjsua_buddy_id buddy_id)
{

}

void Softphone::pjsuaLogCallback(int level, const char *data, int /*len*/)
{
    qDebug() << "PJSUA:" << level << data;
}

bool Softphone::registerAccount()
{
}

bool Softphone::unregisterAccount()
{
    if (PJSUA_INVALID_ID != _accId) {
        pj_status_t status = pjsua_acc_del(_accId);
        if (PJ_SUCCESS != status) {
            errorHandler("Error removing account", status);
            return false;
        }
        _accId = PJSUA_INVALID_ID;
        setRegistrationStatus(RegistrationStatus::UNREGISTERED);
        qDebug() << "account unregistered";
    } else {
        qDebug() << "no account to unregister";
    }
    return true;
}

bool Softphone::makeCall(const QString &userId)
{
    if (!) {
        setActiveCall(true);
    } else {
        setDialedText("");
        setDialogError(true);
        setActiveCall(false);
    }
}

bool Softphone::answer(int callId)
{

}

bool Softphone::hangup(int callId)
{
    if (PJSUA_INVALID_ID == callId) {
        qCritical() << "Invalid call ID";
        return false;
    }

    releaseVideoWindow();
    _manualHangup = true;
    const pj_status_t status = pjsua_call_hangup(callId, 0, nullptr, nullptr);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot hangup call", status, true);
        return false;
    }
    return true;
}

bool Softphone::unsupervisedTransfer(const QString &phoneNumber)
{

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

}

bool Softphone::merge(int callId)
{

}

bool Softphone::hold(bool value, int callId)
{

}

bool Softphone::mute(bool value, int callId)
{

}

bool Softphone::rec(bool value, int callId)
{

}

void Softphone::initAudioDevicesList()
{
    //refresh device list (needed when device changed notification is received)
    auto status = pjmedia_aud_dev_refresh();
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot refresh audio devices list", status, false);
    }

    const auto dev_count = static_cast<pjmedia_aud_dev_index>(pjmedia_aud_dev_count());
    qInfo() << "Found" << dev_count << "audio devices";
    QVector<AudioDevices::DeviceInfo> inputDevices;
    QVector<AudioDevices::DeviceInfo> outputDevices;
    for (pjmedia_aud_dev_index dev_idx = 0; dev_idx < dev_count; ++dev_idx) {
        pjmedia_aud_dev_info info;
        const auto status = pjmedia_aud_dev_get_info(dev_idx, &info);
        if (PJ_SUCCESS != status) {
            errorHandler("Cannot get audio device info.", status, true);
            break;
        }
        if (0 < info.input_count) {
            const AudioDevices::DeviceInfo audioInfo{info.name, dev_idx};
            inputDevices.push_back(audioInfo);
            //qDebug() << "Audio input" << audioInfo.name << audioInfo.index;
        }
        if (0 < info.output_count) {
            const AudioDevices::DeviceInfo audioInfo{info.name, dev_idx};
            outputDevices.push_back(audioInfo);
            //qDebug() << "Audio output" << audioInfo.name << audioInfo.index;
        }
    }
    if (inputDevices.isEmpty()) {
        errorDialog(tr("No input audio devices"));
    }

    _inputAudioDevices->init(inputDevices);
    const auto inDevInfo = Settings::inputAudioDeviceInfo();
    auto inDevModelIndex = _inputAudioDevices->deviceIndex(inDevInfo);
    if (AudioDevices::INVALID_MODEL_INDEX == inDevModelIndex) {
        qDebug() << "Invalid input device, using default device";
        inDevModelIndex = 0;
    }

    if (outputDevices.isEmpty()) {
        errorDialog(tr("No output audio devices"));
    }
    _outputAudioDevices->init(outputDevices);
    const auto outDevInfo = Settings::outputAudioDeviceInfo();
    auto outDevModelIndex = _outputAudioDevices->deviceIndex(outDevInfo);
    if (AudioDevices::INVALID_MODEL_INDEX == outDevModelIndex) {
        qDebug() << "Invalid output device, using default device";
        outDevModelIndex = 0;
    }

    //set last in settings the audio devices
    _settings->setInputAudioModelIndex(inDevModelIndex);
    _settings->setOutputAudioModelIndex(outDevModelIndex);

    qInfo() << "Found" << inputDevices.size() << "input audio devices and"
            << outputDevices.size() << "output audio devices";
}

void Softphone::initVideoDevicesList()
{
    auto status = pjmedia_vid_dev_refresh();
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot refresh video device list", status, false);
    }
    const auto dev_count = static_cast<pjmedia_vid_dev_index>(pjmedia_vid_dev_count());
    qInfo() << "Found" << dev_count << "video devices";
    QVector<VideoDevices::DeviceInfo> videoDevices;
    for (pjmedia_vid_dev_index dev_idx = 0; dev_idx < dev_count; ++dev_idx) {
        pjmedia_vid_dev_info info;
        status = pjmedia_vid_dev_get_info(dev_idx, &info);
        if (PJ_SUCCESS != status) {
            errorHandler("Cannot get video device info.", status);
            break;
        }
        switch (info.dir) {
        case PJMEDIA_DIR_ENCODING:
            [[fallthrough]];
        case PJMEDIA_DIR_ENCODING_DECODING: {
            const VideoDevices::DeviceInfo videoInfo{info.name, dev_idx};
            videoDevices.push_back(videoInfo);
            //qDebug() << "Video dir encoding" << dev_idx << info.name;
        }
            break;
        case PJMEDIA_DIR_NONE:
            //qDebug() << "Video dir none" << dev_idx << info.name;
            break;
        case PJMEDIA_DIR_DECODING:
            //qDebug() << "Video dir decoding" << dev_idx << info.name;
            break;
        default:
            qWarning() << "Video dir unknown" << dev_idx << info.name << info.dir;
        }
    }
    if (videoDevices.isEmpty()) {
        errorDialog(tr("No video devices"));
    }
    _videoDevices->init(videoDevices);
    const auto deviceInfo = Settings::videoDeviceInfo();
    auto deviceIndex = _videoDevices->deviceIndex(deviceInfo);
    if (VideoDevices::INVALID_MODEL_INDEX == deviceIndex) {
        qDebug() << "Invalid video device, using default device";
        deviceIndex = 0;
    }
    _settings->setVideoModelIndex(deviceIndex);

    qInfo() << "Found" << videoDevices.size() << "input video devices";
}

bool Softphone::disableAudio(bool force)
{
    if (!force && !_audioEnabled) {
        return true;
    }
    auto status = pjsua_set_null_snd_dev();
    if (PJ_SUCCESS != status) {
        Softphone::errorHandler("Cannot set null audio device", status, false);
        return false;
    }
    _audioEnabled = false;
    return true;
}

bool Softphone::sendDtmf(const QString &dtmf)
{

}

void Softphone::manuallyRegister()
{

}

bool Softphone::setMicrophoneVolume(pjsua_conf_port_id portId, bool mute)
{
    //microphone volume
    const qreal microphoneLevel = mute ? 0 : _settings->microphoneVolume();
    qInfo() << "Mic level" << microphoneLevel;
    const pj_status_t status = pjsua_conf_adjust_tx_level(portId,
                                                    static_cast<float>(microphoneLevel));
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot adjust tx level", status);
        return false;
    }
    return true;
}

bool Softphone::setSpeakersVolume(pjsua_conf_port_id portId, bool mute)
{
    //speakers volume
    const qreal speakersLevel = mute ? 0 : _settings->speakersVolume();
    qInfo() << "Speakers level" << speakersLevel;
    const pj_status_t status = pjsua_conf_adjust_rx_level(portId,
                                                    static_cast<float>(speakersLevel));
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot adjust rx level", status);
        return false;
    }
    return true;
}

void Softphone::hangupAll()
{
}


void Softphone::setupConferenceCall(pjsua_call_id callId)
{

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
    if ((PJSUA_INVALID_ID == currentCallId) ||
            (PJ_FALSE == pjsua_call_is_active(currentCallId))) {
        qDebug() << "No active call";
        return;
    }
    const pjsua_conf_port_id confSlot = pjsua_call_get_conf_port(currentCallId);
    if (PJSUA_INVALID_ID != confSlot) {
        setMicrophoneVolume(confSlot);
    } else {
        qCritical() << "Cannot get conference slot of call ID" << currentCallId;
    }
}

void Softphone::onSpeakersVolumeChanged()
{
    qDebug() << "onSpeakersVolumeChanged";
    const auto currentCallId = _activeCallModel->currentCallId();
    if ((PJSUA_INVALID_ID == currentCallId) ||
            (PJ_FALSE == pjsua_call_is_active(currentCallId))) {
        qDebug() << "No active call";
        return;
    }
    const pjsua_conf_port_id confSlot = pjsua_call_get_conf_port(currentCallId);
    if (PJSUA_INVALID_ID != confSlot) {
        setSpeakersVolume(confSlot);
    } else {
        qCritical() << "Cannot get conference slot of call ID" << currentCallId;
    }
}

bool Softphone::playDigit(const QString& digit)
{

}
