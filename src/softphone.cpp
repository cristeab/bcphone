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

    //setup tone generator
    _toneGenTimer.setInterval(TONE_GENERATOR_TIMEOUT_MS);
    _toneGenTimer.setSingleShot(true);
    connect(&_toneGenTimer, &QTimer::timeout, this, &Softphone::releaseToneGenerator);

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

std::tuple<QString,QString> Softphone::extractUserNameAndId(const QString &info)
{
    const static QRegularExpression re("\"([\\w|\\s]*)\"\\s*<sip:([\\w|\\s]*)@.*");
    qDebug() << "Matching" << info;
    QRegularExpressionMatch match = re.match(info);
    QString userName;
    QString userId;
    if (match.hasMatch()) {
        userName = match.captured(1);
        userId = match.captured(2);
    } else {
        const static QRegularExpression re1("sip:([\\w|\\s]*)@.*");
        match = re1.match(info);
        if (match.hasMatch()) {
            userId = userName = match.captured(1);
        } else {
            qWarning() << "No match found" << info;
            //use brute force parser
            auto tok = info.split('@');
            tok = tok.front().split("<sip:");
            if (1 < tok.size()) {
                userName = tok.front();
            }
            userId = tok.last();
        }
    }
    return std::make_tuple(userName, userId);
}

bool Softphone::setAudioCodecPriority(const QString &codecId, int priority)
{
    if ((0 > priority) || (priority > 255)) {
        qWarning() << "Invalid audio codec priority" << codecId << priority;
        return false;
    }
    const auto stdCodecId = codecId.toStdString();
    pj_str_t pjCodecId;
    pj_cstr(&pjCodecId, stdCodecId.c_str());
    const auto status = pjsua_codec_set_priority(&pjCodecId, priority);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot set audio codec priority", status);
    }
    return PJ_SUCCESS == status;
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

bool Softphone::setVideoCodecBitrate(const QString &codecId, int bitrate)
{
    const auto stdCodecId = codecId.toStdString();
    pj_str_t pjCodecId;
    pj_cstr(&pjCodecId, stdCodecId.c_str());
    pjmedia_vid_codec_param param;
    auto status = pjsua_vid_codec_get_param(&pjCodecId, &param);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot set video codec priority", status, false);
        return false;
    }
    param.enc_fmt.det.vid.avg_bps = bitrate;
    param.enc_fmt.det.vid.max_bps = bitrate;
    status = pjsua_vid_codec_set_param(&pjCodecId, &param);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot set video codec priority", status, false);
        return false;
    }
    return true;
}

void Softphone::onIncomingCall(pjsua_acc_id acc_id, pjsua_call_id callId,
                               pjsip_rx_data *rdata)
{
    PJ_UNUSED_ARG(acc_id);
    PJ_UNUSED_ARG(rdata);
    if (nullptr == _instance) {
        return;
    }

    pjsua_call_info ci;
    pjsua_call_get_info(callId, &ci);

    QString info = toString(ci.remote_info);
    qDebug() << "Incoming call from" << info;

    const auto userNameAndId = extractUserNameAndId(info);
    const auto isConf = _instance->_activeCallModel->isConference();
    emit _instance->incoming(pjsua_call_get_count(), callId, std::get<1>(userNameAndId),
                             std::get<0>(userNameAndId), isConf);
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
    if (PJ_FALSE == pjsua_buddy_is_valid(buddy_id)) {
        qWarning() << "Invalid buddy ID" << buddy_id;
        return;
    }
    pjsua_buddy_info info;
    const pj_status_t status = pjsua_buddy_get_info(buddy_id, &info);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot get buddy info", status, true);
        return;
    }
    if ((nullptr != _instance) && (nullptr != _instance->_presenceModel)) {
        _instance->_presenceModel->updateStatus(buddy_id, toString(info.rpid.note));
    }
}

void Softphone::dumpStreamStats(pjmedia_stream *strm)
{
    pjmedia_rtcp_stat stat;
    const pj_status_t status = pjmedia_stream_get_stat(strm, &stat);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot get stream stats", status, false);
        return;
    }

    auto showStreamStat = [](const char *prefix, const pjmedia_rtcp_stream_stat &s) {
        qInfo() << prefix << "packets =" << s.pkt << "packets, payload ="
                << s.bytes << "bytes, loss =" << s.loss << "packets, percent loss ="
                << s.loss * 100.0 / (s.loss + s.pkt) << "%, dup =" << s.dup
                << "packets, reorder =" << s.reorder << "packets, discard ="
                << s.discard << "packets, jitter (min, mean, max) ="
                << s.jitter.min << s.jitter.mean << s.jitter.max << "ms";
    };
    showStreamStat("RX stat:", stat.rx);
    showStreamStat("TX stat:", stat.tx);
    qInfo() << "RTT (min, mean, max) =" << stat.rtt.min << stat.rtt.mean
            << stat.rtt.max << "ms";
}

void Softphone::pjsuaLogCallback(int level, const char *data, int /*len*/)
{
    qDebug() << "PJSUA:" << level << data;
}

bool Softphone::init()
{
    //destroy previous instance if any
    if (_pjsuaStarted) {
        qDebug() << "PJSUA already initialized";
        return true;
    }

    //create PJSUA
    pj_status_t status = pjsua_create();
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot create PJSUA", status, true);
        return false;
    }

    //init PJSUA
    {
        pjsua_config cfg;
        pjsua_logging_config log_cfg;

        pjsua_config_default(&cfg);
        cfg.nat_type_in_sdp = 2;
        cfg.max_calls = PJSUA_MAX_CALLS;
        cfg.cb.on_reg_state = &onRegState;
        cfg.cb.on_incoming_call = &onIncomingCall;
        cfg.cb.on_call_media_state = &onCallMediaState;
        cfg.cb.on_call_state = &onCallState;
        cfg.cb.on_stream_created = &onStreamCreated;
        cfg.cb.on_stream_destroyed = &onStreamDestroyed;
        cfg.cb.on_buddy_state = &onBuddyState;

        //configure STUN if any
        /*if (!_settings->stunServer().isEmpty()) {
            cfg.stun_srv_cnt = 1;
            if (0 < _settings->stunPort()) {
                port = ":" + QString::number(_settings->stunPort());
            }
            const auto srv = _settings->stunServer() + port;
            pj_cstr(cfg.stun_srv, srv.toStdString().c_str());
            cfg.stun_map_use_stun2 = PJ_FALSE;
            cfg.stun_ignore_failure = PJ_TRUE;
            cfg.stun_try_ipv6 = PJ_FALSE;
            qInfo() << "STUN server configured" << srv;
        }*/

        //user agent contains app version and PJSIP version
        static const std::string pjsipVer(pj_get_version());
        static const std::string userAgent = (_settings->appName() + "/" +
                                 _settings->appVersion()).toStdString() +
                                 " (PJSIP/" + pjsipVer + ")";
        pj_cstr(&cfg.user_agent, userAgent.c_str());

        pjsua_logging_config_default(&log_cfg);
        log_cfg.msg_logging = PJ_TRUE;
        log_cfg.level = 10;
        log_cfg.console_level = 10;
        log_cfg.cb = Softphone::pjsuaLogCallback;

        pjsua_media_config media_cfg;
        pjsua_media_config_default(&media_cfg);
        media_cfg.channel_count = 1;
        media_cfg.ec_options = PJMEDIA_ECHO_DEFAULT |
                PJMEDIA_ECHO_USE_NOISE_SUPPRESSOR |
                PJMEDIA_ECHO_AGGRESSIVENESS_DEFAULT;
        media_cfg.no_vad = PJ_FALSE;

        status = pjsua_init(&cfg, &log_cfg, &media_cfg);
        if (PJ_SUCCESS != status) {
            errorHandler("Cannot init PJSUA", status, true);
            return false;
        }
    }

    auto addTransport = [](int type) {
        pjsua_transport_config cfg;
        pjsua_transport_config_default(&cfg);
        cfg.port = 0;//any available source port
        const auto status = pjsua_transport_create(static_cast<pjsip_transport_type_e>(type), &cfg, nullptr);
        if (PJ_SUCCESS != status) {
            Softphone::errorHandler("Error creating transport", status, true);
            return false;
        }
        return true;
    };
    //add UDP transport
    auto rc = addTransport(PJSIP_TRANSPORT_UDP);
    if (!rc) {
        return rc;
    }
    //add TCP transport
    rc = addTransport(PJSIP_TRANSPORT_TCP);
    if (!rc) {
        return rc;
    }
    //add TLS transport
    rc = addTransport(PJSIP_TRANSPORT_TLS);
    if (!rc) {
        return rc;
    }

    //start PJSUA
    status = pjsua_start();
    if (PJ_SUCCESS != status) {
        errorHandler("Error starting pjsua", status, true);
        return false;
    }

    disableAudio(true);

    initAudioDevicesList();
    initVideoDevicesList();

    listAudioCodecs();
    listVideoCodecs();

    disableTcpSwitch(false);

    _pjsuaStarted = true;
    qInfo() << "Init PJSUA library";
    return true;
}

void Softphone::release()
{
    if (_pjsuaStarted) {
        hangupAll();
        releaseRingTonePlayers();
        unregisterAccount();
        pjsua_stop_worker_threads();
        pj_status_t status = pjsua_destroy();
        if (PJ_SUCCESS != status) {
            errorHandler("Cannot destroy", status);
        } else {
            qDebug() << "PJSUA successfully destroyed";
        }
        _pjsuaStarted = false;
    }
}

bool Softphone::registerAccount()
{
    if (!_pjsuaStarted) {
        qCritical() << "PJSUA library not started";
        errorDialog(tr("PJSUA library not started"));
        return false;
    }
    if (nullptr == _settings) {
        qCritical() << "Cannot register acccount";
        return false;
    }

    const auto& callerId = _settings->accountName();
    const auto& domain = _settings->sipServer();
    const auto destPort = QString::number(_settings->sipPort());
    const auto& username = _settings->userName();
    auto authUsername = _settings->authUserName();
    if (authUsername.isEmpty()) {
        authUsername = username;
    }
    const auto& password = _settings->password();
    const auto sipTransport = Softphone::sipTransport(_settings->sipTransport());

    if (domain.isEmpty() || username.isEmpty() || password.isEmpty()) {
        errorHandler(tr("Either domain, username or password is empty"),
                     PJ_SUCCESS, true);
        return false;
    }

    //unregister previous account if needed
    unregisterAccount();

    pjsua_acc_config cfg;
    pjsua_acc_config_default(&cfg);
    QString id;
    if (!callerId.isEmpty()) {
        id = "\"" + callerId + "\" <";
    }
    id += "sip:" + username + "@" + domain + ":" +
            destPort + sipTransport;
    if (!callerId.isEmpty()) {
        id += ">";
    }
    qInfo() << "ID URI" << id;
    std::string tmpId = id.toStdString();
    pj_cstr(&cfg.id, tmpId.c_str());
    QString regUri = "sip:" + domain + ":" + destPort + sipTransport;
    qInfo() << "Reg URI" << regUri;
    std::string tmpRegUri = regUri.toStdString();
    pj_cstr(&cfg.reg_uri, tmpRegUri.c_str());
    cfg.cred_count = 1;
    pj_cstr(&cfg.cred_info[0].realm, "*");
    pj_cstr(&cfg.cred_info[0].scheme, "digest");
    auto tmpUsername = authUsername.toStdString();
    pj_cstr(&cfg.cred_info[0].username, tmpUsername.c_str());
    cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
    auto tmpPassword = password.toStdString();
    pj_cstr(&cfg.cred_info[0].data, tmpPassword.c_str());

    const bool srtpEnabled = Settings::MediaTransport::Srtp == _settings->mediaTransport();
    cfg.use_srtp = srtpEnabled ? PJMEDIA_SRTP_MANDATORY : PJMEDIA_SRTP_DISABLED;
    pjsua_srtp_opt_default(&cfg.srtp_opt);
    cfg.srtp_opt.keying[0] = PJMEDIA_SRTP_KEYING_SDES;//TODO: check box
    const bool tlsEnabled = Settings::SipTransport::Tls == _settings->sipTransport();
    cfg.srtp_secure_signaling = tlsEnabled ? 1 : 0;

    if (_settings->proxyEnabled()) {
        qInfo() << "Outbound proxy is enabled";
        auto proxyServer = "sip:" + _settings->proxyServer();
        if (0 < _settings->proxyPort()) {
            proxyServer += ":" + QString::number(_settings->proxyPort());
        }
        proxyServer += sipTransport;
        cfg.proxy_cnt = 1;
        qInfo() << "Proxy URI" << proxyServer;
        const auto proxyUri = proxyServer.toStdString();
        pj_cstr(&cfg.proxy[0], proxyUri.c_str());
        cfg.reg_use_proxy = PJSUA_REG_USE_OUTBOUND_PROXY | PJSUA_REG_USE_ACC_PROXY;
    }

    cfg.vid_cap_dev = PJMEDIA_VID_DEFAULT_CAPTURE_DEV;
    cfg.vid_rend_dev = PJMEDIA_VID_DEFAULT_RENDER_DEV;
    cfg.vid_in_auto_show = PJ_FALSE;
    cfg.vid_out_auto_transmit = PJ_FALSE;

    cfg.allow_sdp_nat_rewrite = PJ_TRUE;

    pj_status_t status = pjsua_acc_add(&cfg, PJ_TRUE, &_accId);
    if (PJ_SUCCESS != status) {
        errorHandler("Error adding account", status, true);
        return false;
    }
    qDebug() << "successfully registered account";
    onRegState(_accId);
    return true;
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
    qDebug() << "makeCall" << userId;

    if (userId.isEmpty()) {
        setDialedText("");
        setDialogError(true);
        setActiveCall(false);
        return false;
    }

    std::string uriBuffer;
    pj_str_t uriStr;
    if (!callUri(&uriStr, userId, uriBuffer)) {
        setActiveCall(false);
        return false;
    }

    //open audio device only when needed
    enableAudio();

    pjsua_call_setting callSetting;
    pjsua_call_setting_default(&callSetting);
    callSetting.vid_cnt = 1;

    pjsua_call_id callId = PJSUA_INVALID_ID;
    const pj_status_t status = pjsua_call_make_call(_accId, &uriStr, &callSetting,
                                                    nullptr, nullptr, &callId);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot make call", status, true);
        setActiveCall(false);
        return false;
    }

    if (nullptr != _callHistoryModel) {
        const auto &userName = _callHistoryModel->userName(userId);
        _activeCallModel->addCall(callId, userName, userId);
        _callHistoryModel->addContact(callId, userName, userId,
                                      CallHistoryModel::CallStatus::OUTGOING);
    } else {
        qWarning() << "Call history model is null";
    }
    startPlayingRingTone(callId, false);
    setActiveCall(true);
    return true;
}

bool Softphone::answer(int callId)
{
    if (PJSUA_INVALID_ID == callId) {
        qCritical() << "Invalid call ID";
        return false;
    }

    pjsua_call_setting callSetting;
    pjsua_call_setting_default(&callSetting);
    callSetting.vid_cnt = 1;

    const pj_status_t status = pjsua_call_answer2(callId, &callSetting, 200,
                                                  nullptr, nullptr);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot answer call", status, true);
        return false;
    }
    return true;
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
    if (phoneNumber.isEmpty()) {
        errorHandler("Cannot make transfer without a destination phone number",
                     PJ_SUCCESS, true);
        return false;
    }
    const int currentCallId = _activeCallModel->currentCallId();
    if (PJSUA_INVALID_ID == currentCallId) {
        errorHandler("Cannot make transfer without an active call", PJ_SUCCESS, true);
        return false;
    }

    std::string uriBuffer;
    pj_str_t uriStr;
    if (!callUri(&uriStr, phoneNumber, uriBuffer)) {
        return false;
    }
    const pj_status_t status = pjsua_call_xfer(currentCallId, &uriStr, nullptr);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot transfer call", status, true);
        return false;
    }
    if (nullptr != _callHistoryModel) {
        _callHistoryModel->addContact(currentCallId, _blindTransferUserName, phoneNumber,
                                      CallHistoryModel::CallStatus::TRANSFERRED);
        _callHistoryModel->updateCallStatus(currentCallId,
                                            CallHistoryModel::CallStatus::UNKNOWN, true);
    }
    return true;
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
    const auto &confCids = _activeCallModel->confirmedCallsId();
    if (confCids.isEmpty()) {
        qCritical() << "No active call(s) to swap";
        return false;
    }
    qDebug() << "swap" << confCids << "with" << callId;
    for (auto confCid: confCids) {
        if (!hold(true, confCid)) {
            return false;
        }
    }
    bool rc = hold(false, callId);
    if (rc) {
        _activeCallModel->setCurrentCallId(callId);
    }
    return rc;
}

bool Softphone::merge(int callId)
{
    qDebug() << "merge" << callId;
    if (!hold(false, callId)) {
        return false;
    }
    setupConferenceCall(callId);
    _activeCallModel->update();
    return true;
}

bool Softphone::hold(bool value, int callId)
{
    qDebug() << "hold" << value;
    if (PJSUA_INVALID_ID == callId) {
        callId = _activeCallModel->currentCallId();
    }

    pj_status_t status = PJ_SUCCESS;
    if (value) {
        status = pjsua_call_set_hold(callId, nullptr);
        if (PJ_SUCCESS != status) {
            errorHandler("Cannot put call on hold", status, true);
            return false;
        }
    } else {
        status = pjsua_call_reinvite(callId, PJSUA_CALL_UNHOLD, nullptr);
        if (PJ_SUCCESS != status) {
            errorHandler("Cannot unhold call", status, true);
            return false;
        }
    }
    _activeCallModel->setCallState(callId, value ? ActiveCallModel::CallState::ON_HOLD : ActiveCallModel::CallState::CONFIRMED);
    qDebug() << "Finished to put call on hold" << value;
    return true;
}

bool Softphone::mute(bool value, int callId)
{
    if (PJSUA_INVALID_ID == callId) {
        callId = _activeCallModel->currentCallId();
    }
    const pjsua_conf_port_id confSlot = pjsua_call_get_conf_port(callId);
    if (PJSUA_INVALID_ID == confSlot) {
        qCritical() << "Cannot get conference slot of call ID" << callId;
        return false;
    }
    qDebug() << "mute" << value << callId;
    return setMicrophoneVolume(confSlot, value);
}

bool Softphone::rec(bool value, int callId)
{
    if (value) {
        if (recordingCall()) {
            qDebug() << "Recording already started";
            return true;
        }
    } else if (!recordingCall()) {
        qDebug() << "Recording already stopped";
        return true;
    }
    if (PJSUA_INVALID_ID == callId) {
        callId = _activeCallModel->currentCallId();
    }
    if (PJSUA_INVALID_ID == callId) {
        errorHandler("Cannot record without an active call", PJ_SUCCESS, true);
        return false;
    }
    qDebug() << "rec" << value << callId;
    setRecordingCall(value);
    return true;
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

bool Softphone::enableAudio()
{
    if (_audioEnabled) {
        return true;
    }
    if ((nullptr == _inputAudioDevices) || (nullptr == _outputAudioDevices)) {
        return false;
    }
    const auto captureDevInfo = _inputAudioDevices->deviceInfo();
    if (!captureDevInfo.isValid()) {
        const QString msg = QString("Invalid input audio device index %1").arg(captureDevInfo.toString());
        qCritical() << msg;
        errorHandler(msg, PJ_SUCCESS, true);
        pjsua_set_null_snd_dev();//for testing purposes only
        return false;
    }
    const auto playbackDevInfo = _outputAudioDevices->deviceInfo();
    if (!playbackDevInfo.isValid()) {
        const QString msg = QString("Invalid output audio device index %1").arg(playbackDevInfo.toString());
        qCritical() << msg;
        errorHandler(msg, PJ_SUCCESS, true);
        pjsua_set_null_snd_dev();//for testing purposes only
        return false;
    }

    //slow operation: about 1 sec
    const auto status = pjsua_set_snd_dev(captureDevInfo.index, playbackDevInfo.index);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot set audio devices", status, true);
        return false;
    }

    qDebug() << "Finished to set audio devices: captureDev" << captureDevInfo.index <<
                ", playbackDev" << playbackDevInfo.index;
    _audioEnabled = true;
    return true;
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

bool Softphone::initRingTonePlayer(pjsua_call_id id, bool incoming)
{
    if (0 != _playerId.count(id)) {
        qCritical() << "Call" << id << "already has a player associated with it";
        return false;
    }

    int ringToneFileIndex = incoming ? _settings->inboundRingTonesModelIndex() : _settings->outboundRingTonesModelIndex();
    const QString soundFileStr = _ringTonesModel->filePath(ringToneFileIndex);
    if (!QFile(soundFileStr).exists()) {
        qWarning() << "Ring tone file does not exist";
        return false;
    }
    qInfo() << "Init ringtone player" << soundFileStr;

    const std::string tmpSoundFile = soundFileStr.toStdString();
    pj_str_t soundFile;
    pj_cstr(&soundFile, tmpSoundFile.c_str());

    pjsua_player_id playerId = PJSUA_INVALID_ID;
    const pj_status_t status = pjsua_player_create(&soundFile, 0, &playerId);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot create player", status, false);
        return false;
    }
    _playerId[id] = playerId;
    return true;
}

bool Softphone::startPlayingRingTone(pjsua_call_id id, bool incoming)
{
    if (!initRingTonePlayer(id, incoming)) {
        qDebug() << "Cannot start playing ringtone for call" << id;
        return false;
    }
    if (0 == _playerId.count(id)) {
        qCritical() << "Cannot find a player ID for call" << id;
        return false;
    }
    pjsua_player_id playerId = _playerId[id];
    pj_status_t status = pjsua_conf_connect(pjsua_player_get_conf_port(playerId), 0);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot play ring tone to output device", status, false);
        return false;
    }
    qInfo() << "Start playing ringtone";
    return true;
}

void Softphone::stopPlayingRingTone(pjsua_call_id id)
{
    if (0 == _playerId.count(id)) {
        //qWarning() << "Cannot find player ID for call" << id;
        return;
    }
    pjsua_player_id playerId = _playerId[id];
    pj_status_t status = pjsua_conf_disconnect(pjsua_player_get_conf_port(playerId), 0);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot stop playing ring tone to output device", status, false);
        return;
    }
    qInfo() << "Stop playing ringtone";
    releaseRingTonePlayer(id);
}

bool Softphone::releaseRingTonePlayer(pjsua_call_id id)
{
    if (0 == _playerId.count(id)) {
        qCritical() << "Cannot find a player ID for call" << id;
        return false;
    }
    pjsua_player_id playerId = _playerId[id];
    const auto status = pjsua_player_destroy(playerId);
    _playerId.remove(id);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot destroy ringtone player", status, false);
        return false;
    }
    qInfo() << "Release ringtone player";
    return true;
}

void Softphone::releaseRingTonePlayers()
{
    const auto &keys = _playerId.keys();
    for(const auto &id: keys) {
        releaseRingTonePlayer(id);
    }
}

bool Softphone::createRecorder(pjsua_call_id callId)
{
    if (PJSUA_INVALID_ID == callId) {
        qCritical() << "Invalid call ID";
        return false;
    }
    if (_recId.contains(callId) && (PJSUA_INVALID_ID != _recId[callId])) {
        qWarning() << "Recorded already created for call ID " << callId;
        return false;
    }

    //generate recording file name
    const auto curDateTime = QDateTime::currentDateTime();
    const QString recFileName = _settings->recPath() + "/" +
            curDateTime.toString("MMMM_dd_yyyy-hh_mm_ss") + ".wav";
    pj_str_t recordFile;
    pj_cstr(&recordFile, recFileName.toUtf8().data());

    //create recorder
    pjsua_recorder_id recId = PJSUA_INVALID_ID;
    pj_status_t status = pjsua_recorder_create(&recordFile, 0,
                                               nullptr, 0, 0, &recId);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot create recorder", status);
        recId = PJSUA_INVALID_ID;
        return false;
    }
    _recId[callId] = recId;
    qInfo() << "Created recorder " << recFileName;
    return true;
}

bool Softphone::startRecording(pjsua_call_id callId)
{
    if (!createRecorder(callId)) {
        return false;
    }

    //start recording
    const pjsua_conf_port_id callConfPort = pjsua_call_get_conf_port(callId);
    if (PJSUA_INVALID_ID == callConfPort) {
        releaseRecorder(callId);
        qCritical() << "Cannot get call conf port";
        return false;
    }
    const pjsua_conf_port_id recConfPort = pjsua_recorder_get_conf_port(_recId[callId]);
    if (PJSUA_INVALID_ID == recConfPort) {
        releaseRecorder(callId);
        qCritical() << "Cannot get recorder conf port";
        return false;
    }
    pj_status_t status = pjsua_conf_connect(callConfPort, recConfPort);
    if (PJ_SUCCESS != status) {
        releaseRecorder(callId);
        errorHandler("Cannot start recording", status);
        return false;
    }
    qInfo() << "Recording started for call ID " << callId;
    return true;
}

bool Softphone::stopRecording(pjsua_call_id callId)
{
    if (PJSUA_INVALID_ID == callId) {
        qCritical() << "Invalid call ID";
        return false;
    }
    if (!_recId.contains(callId) || (PJSUA_INVALID_ID == _recId[callId])) {
        qWarning() << "Invalid recorder ID";
        return true;
    }

    if (PJ_TRUE == pjsua_call_is_active(callId)) {
        //stop recording
        const pjsua_conf_port_id callConfPort = pjsua_call_get_conf_port(callId);
        if (PJSUA_INVALID_ID == callConfPort) {
            releaseRecorder(callId);
            qCritical() << "Cannot get call conf port";
            return false;
        }
        const pjsua_conf_port_id recConfPort = pjsua_recorder_get_conf_port(_recId[callId]);
        if (PJSUA_INVALID_ID == recConfPort) {
            releaseRecorder(callId);
            qCritical() << "Cannot get recorder conf port";
            return false;
        }
        pj_status_t status = pjsua_conf_disconnect(callConfPort, recConfPort);
        if (PJ_SUCCESS != status) {
            releaseRecorder(callId);
            errorHandler("Cannot stop recording", status);
            return false;
        }
    }
    qInfo() << "Recording stopped for call ID " << callId;

    return releaseRecorder(callId);
}

bool Softphone::releaseRecorder(pjsua_call_id callId)
{
    if (!_recId.contains(callId) || (PJSUA_INVALID_ID == _recId[callId])) {
        qWarning() << "Invalid recorder ID";
        return true;
    }
    const auto status = pjsua_recorder_destroy(_recId[callId]);
    _recId.remove(callId);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot destroy recorder", status);
        return false;
    }
    qInfo() << "Recorder has been released";
    return true;
}

void Softphone::listAudioCodecs()
{
    std::array<pjsua_codec_info, 32> codecInfo;
    unsigned int codecCount = static_cast<unsigned int>(codecInfo.size());

    //read first default priorities
    pj_status_t status = pjsua_enum_codecs(codecInfo.data(), &codecCount);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot enum audio codecs", status);
        return;
    }
    QHash<QString,int> defaultPrio;
    for (unsigned int n = 0; n < codecCount; ++n) {
        const auto id = toString(codecInfo[n].codec_id);
        setAudioCodecPriority(id, codecInfo[n].priority);
        defaultPrio[id] = codecInfo[n].priority;
    }

    _audioCodecs->init();//init first codecs

    //get again codecs
    status = pjsua_enum_codecs(codecInfo.data(), &codecCount);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot enum audio codecs", status);
        return;
    }
    qInfo() << "Audio codecs:" << codecCount;
    QList<AudioCodecs::CodecInfo> audioCodecsInfo;
    for (unsigned int n = 0; n < codecCount; ++n) {
        const auto id = toString(codecInfo[n].codec_id);
        const auto priority = codecInfo[n].priority;
        const auto defaultPriority = defaultPrio.contains(id) ? defaultPrio[id] : -1;
        qInfo() << id << priority;
        audioCodecsInfo.append({ id, id, priority, 0 < priority, defaultPriority });
    }
    _audioCodecs->setCodecsInfo(audioCodecsInfo);
}

void Softphone::listVideoCodecs()
{
    std::array<pjsua_codec_info, 32> codecInfo;
    unsigned int codecCount = static_cast<unsigned int>(codecInfo.size());
    auto status = pjsua_vid_enum_codecs(codecInfo.data(), &codecCount);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot enum video codecs", status);
        return;
    }
    qInfo() << "Video codecs:" << codecCount;
    QHash<QString,int> defaultPrio;
    for (unsigned int n = 0; n < codecCount; ++n) {
        const QString id = QString::fromUtf8(codecInfo[n].codec_id.ptr,
                                             static_cast<int>(codecInfo[n].codec_id.slen));
        qInfo() << id << codecInfo[n].priority;
        defaultPrio[id] = codecInfo[n].priority;
        setVideoCodecBitrate(id, DEFAULT_BITRATE_KBPS * 1000);
    }

    _videoCodecs->init();//init first codecs

    //get again codecs
    status = pjsua_vid_enum_codecs(codecInfo.data(), &codecCount);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot enum video codecs", status);
        return;
    }
    qInfo() << "Video codecs:" << codecCount;
    QList<VideoCodecs::CodecInfo> videoCodecsInfo;
    for (unsigned int n = 0; n < codecCount; ++n) {
        const auto id = toString(codecInfo[n].codec_id);
        const auto priority = codecInfo[n].priority;
        const auto defaultPriority = defaultPrio.contains(id) ? defaultPrio[id] : -1;
        videoCodecsInfo.append({ id, id, priority, false, defaultPriority });
    }
    _videoCodecs->setCodecsInfo(videoCodecsInfo);
}

bool Softphone::sendDtmf(const QString &dtmf)
{
    const auto callId = _activeCallModel->currentCallId();
    if (PJSUA_INVALID_ID == callId) {
        qWarning() << "Cannot send DTMF without an active call";
        return false;
    }
    qDebug() << "send DTMF" << dtmf << "to" << callId;
    std::string tmpDtmf = dtmf.toStdString();
    pj_str_t dtmfStr;
    pj_cstr(&dtmfStr, tmpDtmf.c_str());
    pj_status_t status = pjsua_call_dial_dtmf(callId, &dtmfStr);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot send DTMF", status, true);
        return false;
    }
    return true;
}

void Softphone::manuallyRegister()
{
    qDebug() << "Manual registration";
    if (PJSUA_INVALID_ID != _accId) {
        const pj_status_t status = pjsua_acc_set_registration(_accId, PJ_TRUE);
        if (PJ_SUCCESS != status) {
            errorHandler("Cannot force registration", status, true);
        }
    } else {
        qWarning() << "Invalid acc ID";
    }
}

void Softphone::errorHandler(const QString &title, pj_status_t status, bool emitSignal)
{
    QString fullError = title;
    if (PJ_SUCCESS != status) {
        std::array<char, 1024> message;
        pj_strerror(status, message.data(), message.size());
        fullError.append(":").append(message.data());
    }
    if (emitSignal && (nullptr != _instance)) {
        _instance->errorDialog(fullError);
    }
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
    const auto ids = _activeCallModel->confirmedCallsId(true);
    qDebug() << "hangupAll" << ids.count();
    for (auto id: ids) {
        hangup(id);
    }
    if (!ids.isEmpty()) {
        setActiveCall(false);
    }
}

bool Softphone::disableTcpSwitch(bool value)
{
    pjsip_cfg_t *cfg = pjsip_cfg();
    bool rc = false;
    if (nullptr != cfg) {
        cfg->endpt.disable_tcp_switch = value?PJ_TRUE:PJ_FALSE;
        rc = true;
        qDebug() << "Disable TCP switch" << value;
    } else {
        qWarning() << "Cannot get PJSIP config";
    }
    return rc;
}

bool Softphone::callUri(pj_str_t *uri, const QString &userId, std::string &uriBuffer)
{
    auto* set = _instance->_settings;
    if (nullptr == set) {
        qWarning() << "Cannot generate URI";
        return false;
    }
    const auto& domain = set->sipServer();
    if (domain.isEmpty()) {
        qWarning() << "No SIP server";
        return false;
    }
    const auto destPort = QString::number(set->sipPort());
    if (destPort.isEmpty()) {
        qWarning() << "No SIP port";
        return false;
    }
    const auto sipTransport = Softphone::sipTransport(set->sipTransport());

    const QString sipUri = "sip:" + userId + "@" + domain + ":" + destPort + sipTransport;
    uriBuffer = sipUri.toStdString();
    const char *uriPtr = uriBuffer.c_str();
    const pj_status_t status = verifySipUri(uriPtr);
    if (PJ_SUCCESS != status) {
        errorHandler("URI verification failed", status, true);
        qCritical() << "URI verification failed" << sipUri;
        return false;
    }
    if (nullptr != uri) {
        pj_cstr(uri, uriPtr);
    }
    return true;
}

void Softphone::connectCallToSoundDevices(pjsua_conf_port_id confPortId)
{
    qDebug() << "Connect call conf port" << confPortId << "to sound devices";

    //ajust volume level
    setMicrophoneVolume(confPortId);
    setSpeakersVolume(confPortId);

    pj_status_t status = pjsua_conf_connect(confPortId, 0);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot connect conf slot to playback slot", status,
                     false);
    }

    status = pjsua_conf_connect(0, confPortId);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot connect capture slot to conf slot", status,
                     false);
    }
}

void Softphone::setupConferenceCall(pjsua_call_id callId)
{
    const auto callCount = pjsua_call_get_count();
    qDebug() << "setupConferenceCall" << callCount;
    if (1 < callCount) {
        pjsua_conf_port_id confPortId = pjsua_call_get_conf_port(callId);
        if (PJSUA_INVALID_ID == confPortId) {
            qCritical() << "Cannot get current call conf port";
            return;
        }
        const QVector<int> confCalls = _activeCallModel->confirmedCallsId();
        qDebug() << "Conference call detected, confirmed calls" << confCalls.size();
        for (auto otherCallId: confCalls) {
            if (callId == otherCallId) {
                continue;
            }
            const pjsua_conf_port_id otherCallConfPort = pjsua_call_get_conf_port(otherCallId);
            if (PJSUA_INVALID_ID == otherCallConfPort) {
                qWarning() << "Cannot get conference slot of call ID" << otherCallId;
                continue;
            }
            pj_status_t status = pjsua_conf_connect(confPortId, otherCallConfPort);
            if (PJ_SUCCESS != status) {
                errorHandler("Cannot connect current conf slot to other", status, false);
            }
            status = pjsua_conf_connect(otherCallConfPort, confPortId);
            if (PJ_SUCCESS != status) {
                errorHandler("Cannot connect other conf slot to current", status, false);
            }
        }
    }
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

QString Softphone::sipTransport(int type)
{
    QString out;
    switch (type) {
    case Settings::SipTransport::Udp:
        break;
    case Settings::SipTransport::Tcp:
        out = ";transport=tcp";
        break;
    case Settings::SipTransport::Tls:
        out = ";transport=tls";
        break;
    default:
        ;
    }
    return out;
}

bool Softphone::initToneGenerator()
{
    if (nullptr != _toneGenPool) {
        return true;//nothing to do
    }
    qInfo() << "Init tone generator";
    _toneGenPool = pjsua_pool_create("toneGen", 512, 512);
    if (nullptr == _toneGenPool) {
        qCritical() << "Cannot allocate pool for tone generator";
        return false;
    }
    enableAudio();
    auto status = pjmedia_tonegen_create(_toneGenPool, 8000, 1, 64, 16, 0, &_toneGenMediaPort);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Tone generator create"), status, false);
        return false;
    }
    status = pjsua_conf_add_port(_toneGenPool, _toneGenMediaPort, &_toneGenConfPort);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Tone generator add port"), status, false);
        return false;
    }
    status = pjsua_conf_adjust_rx_level(_toneGenConfPort, _settings->dialpadSoundVolume());
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Tone generator adjust rx level"), status, false);
        return false;
    }
    status = pjsua_conf_connect(_toneGenConfPort, 0);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Tone generator conf connect"), status, false);
        return false;
    }
    return true;
}

void Softphone::releaseToneGenerator()
{
    if ((nullptr != _toneGenMediaPort) && pjmedia_tonegen_is_busy(_toneGenMediaPort)) {
        qDebug() << "Tone gen is busy";
        return;
    }
    qInfo() << "Release tone generator";
    if (PJSUA_INVALID_ID != _toneGenConfPort) {
        disableAudio();
        const auto status = pjsua_conf_remove_port(_toneGenConfPort);
        if (PJ_SUCCESS != status) {
            errorHandler(tr("Tone generator conf remove"), status, false);
        }
        _toneGenConfPort = PJSUA_INVALID_ID;
    }
    if (nullptr != _toneGenMediaPort) {
        const auto status = pjmedia_port_destroy(_toneGenMediaPort);
        if (PJ_SUCCESS != status) {
            errorHandler(tr("Tone generator port destroy"), status, false);
        }
        _toneGenMediaPort = nullptr;
    }
    if (nullptr != _toneGenPool) {
        pj_pool_release(_toneGenPool);
        _toneGenPool = nullptr;
    }
}

bool Softphone::playDigit(const QString& digit)
{
    if (digit.isEmpty()) {
        return false;
    }
    if (!_pjsuaStarted) {
        return false;
    }

    initToneGenerator();

    if (nullptr == _toneGenPool) {
        qCritical() << "Generator pool is NULL";
        return false;
    }

    pjmedia_tone_digit toneDigit;
    toneDigit.digit = digit.toStdString().c_str()[0];
    toneDigit.on_msec = TONE_GENERATOR_ON_MS;
    toneDigit.off_msec = TONE_GENERATOR_OFF_MS;
    toneDigit.volume = 0;
    const auto status = pjmedia_tonegen_play_digits(_toneGenMediaPort, 1, &toneDigit, 0);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Play digit"), status, false);
        return false;
    }
    _toneGenTimer.start();
    return true;
}
