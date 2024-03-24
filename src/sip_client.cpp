#include "sip_client.h"
#include "softphone.h"
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QWidget>
#include <QWindow>
#include <QDialog>
#include <QVBoxLayout>

#define GET_INSTANCE(accId) auto ptr = pjsua_acc_get_user_data(accId);\
    if (nullptr == ptr) {\
        formatErrorMessage(tr("Cannot get SIP client instance for account ID %1").arg(accId));\
        return;\
    }\
    auto instance = reinterpret_cast<SipClient*>(ptr);

#define GET_INSTANCE_CID(callId) pjsua_call_info ci{};\
    const auto status = pjsua_call_get_info(callId, &ci);\
    if (PJ_SUCCESS != status) {\
        formatErrorMessage("Cannot get call info", status);\
        return;\
    }\
    GET_INSTANCE(ci.acc_id)

#define PTR_TO_STR(ptr) (nullptr != ptr) ? SipClient::toString(*ptr) : QString()

SipClient* SipClient::create(Softphone *softphone)
{
    if (nullptr == softphone) {
        qCritical() << "Cannot init SIP client without backend";
        return nullptr;
    }

    if ((nullptr == softphone->settings()) ||
            (nullptr == softphone->inputAudioDevices()) ||
            (nullptr == softphone->outputAudioDevices()) ||
            (nullptr == softphone->videoDevices()) ||
            (nullptr == softphone->audioCodecs()) ||
            (nullptr == softphone->videoCodecs()) ||
            (nullptr == softphone->ringTonesModel()) ||
            (nullptr == softphone->callHistoryModel()) ||
            (nullptr == softphone->activeCallModel())) {
        qCritical() << "Cannot init SIP client without all models";
        return nullptr;
    }

    auto instance = new SipClient(softphone);
    instance->_settings = QPointer(softphone->settings());
    instance->_inputAudioDevices = QPointer(softphone->inputAudioDevices());
    instance->_outputAudioDevices = QPointer(softphone->outputAudioDevices());
    instance->_videoDevices = QPointer(softphone->videoDevices());
    instance->_audioCodecs = QPointer(softphone->audioCodecs());
    instance->_videoCodecs = QPointer(softphone->videoCodecs());
    instance->_ringTonesModel = QPointer(softphone->ringTonesModel());
    instance->_callHistoryModel = QPointer(softphone->callHistoryModel());
    instance->_activeCallModel = QPointer(softphone->activeCallModel());

    return instance;
}

SipClient::SipClient(QObject *parent) : QObject(parent)
{
    //setup tone generator
    _toneGenTimer.setInterval(TONE_GEN_TIMEOUT_MS);
    _toneGenTimer.setSingleShot(true);
    connect(&_toneGenTimer, &QTimer::timeout, this, &SipClient::releaseToneGenerator);
    // connect private signals
    connect(this, &SipClient::registrationStatusReady, this, &SipClient::processRegistrationStatus);
    connect(this, &SipClient::incomingCallReady, this, &SipClient::processIncomingCall);
    connect(this, &SipClient::callStateReady, this, &SipClient::processCallState);
    connect(this, &SipClient::callMediaStateReady, this, &SipClient::processCallMediaState);
    connect(this, &SipClient::streamStatsReady, this, &SipClient::dumpStreamStats);
    connect(this, &SipClient::buddyStateReady, this, &SipClient::processBuddyState);
}

void SipClient::onRegState(pjsua_acc_id accId)
{
    GET_INSTANCE(accId)

    pjsua_acc_info info{};
    const auto status = pjsua_acc_get_info(accId, &info);
    if (PJ_SUCCESS != status) {
        instance->errorHandler("Cannot get account information", status);
        return;
    }
    emit instance->registrationStatusReady(info);
}

void SipClient::onIncomingCall(pjsua_acc_id accId, pjsua_call_id callId, pjsip_rx_data *rdata)
{
    PJ_UNUSED_ARG(rdata);

    GET_INSTANCE(accId)

    pjsua_call_info ci{};
    const auto status = pjsua_call_get_info(callId, &ci);
    if (PJ_SUCCESS != status) {
        instance->errorHandler("Cannot get call info", status);
        return;
    }
    emit instance->incomingCallReady(callId, ci);
}

void SipClient::onCallState(pjsua_call_id callId, pjsip_event *e)
{
    PJ_UNUSED_ARG(e);

    GET_INSTANCE_CID(callId)
    emit instance->callStateReady(callId, ci);
}

void SipClient::onCallMediaState(pjsua_call_id callId)
{
    GET_INSTANCE_CID(callId)
    emit instance->callMediaStateReady(callId, ci);
}

void SipClient::onStreamCreated(pjsua_call_id callId, pjmedia_stream *strm,
                            unsigned stream_idx, pjmedia_port **p_port)
{
    Q_UNUSED(strm)
    Q_UNUSED(p_port)
    qDebug() << "onStreamCreated" << callId << stream_idx;
    //dumpStreamStats(strm);
}

void SipClient::onStreamDestroyed(pjsua_call_id callId, pjmedia_stream *strm,
                              unsigned stream_idx)
{
    qDebug() << "onStreamDestroyed" << callId << stream_idx;
    GET_INSTANCE_CID(callId)
    pjmedia_rtcp_stat stat{};
    const auto st = pjmedia_stream_get_stat(strm, &stat);
    if (PJ_SUCCESS != st) {
	    instance->errorHandler(tr("Cannot get stream stats"), status);
	    return;
    }
    emit instance->streamStatsReady(stat);
}

void SipClient::onBuddyState(pjsua_buddy_id buddyId)
{
    if (PJ_FALSE == pjsua_buddy_is_valid(buddyId)) {
        qWarning() << "Invalid buddy ID" << buddyId;
        return;
    }
    auto ptr = pjsua_buddy_get_user_data(buddyId);
    if (nullptr == ptr) {
        formatErrorMessage(tr("Cannot get SIP client instance for buddy ID %1").arg(buddyId));
        return;
    }\
    auto instance = reinterpret_cast<SipClient*>(ptr);
    emit instance->buddyStateReady(buddyId);
}

void SipClient::onPager(pjsua_call_id callId, const pj_str_t *from, const pj_str_t *to,
	     const pj_str_t *contact, const pj_str_t *mimeType, const pj_str_t *body)
{
	const auto src{PTR_TO_STR(from)};
	const auto dst{PTR_TO_STR(to)};
	const auto dstOrig{PTR_TO_STR(contact)};
	const auto contentType{PTR_TO_STR(mimeType)};
	const auto msg{PTR_TO_STR(body)};
	qDebug() << "Pager: call ID" << callId <<
		", from" << src <<
		", to" << dst <<
		", contact" << dstOrig <<
		", mimeType" << contentType <<
		", body" << msg;
}

void SipClient::onPagerStatus(pjsua_call_id callId, const pj_str_t *to, const pj_str_t *body,
		   void *user_data, pjsip_status_code status, const pj_str_t *reason)
{
	const auto dst{PTR_TO_STR(to)};
	const auto msg{PTR_TO_STR(body)};
	const auto motive{PTR_TO_STR(reason)};

	static std::array<char, MAX_ERROR_MSG_SIZE> statusMessage;
	pj_strerror(status, statusMessage.data(), statusMessage.size());

	qDebug() << "Pager status: call ID" << callId <<
		", to" << dst <<
		", body" << msg <<
		", status" << statusMessage.data() <<
		", reason" << motive;
}

void SipClient::onTyping(pjsua_call_id callId, const pj_str_t *from, const pj_str_t *to,
	      const pj_str_t *contact, pj_bool_t isTyping)
{
	const auto src{PTR_TO_STR(from)};
	const auto dst{PTR_TO_STR(to)};
	const auto dstOrig{PTR_TO_STR(contact)};
	qDebug() << "Typing: call ID" << callId <<
		", from" << src <<
		", to" << dst <<
		", contact" << dstOrig <<
		", typing" << isTyping;
}

void SipClient::pjsuaLogCallback(int level, const char *data, int len)
{
    qDebug() << QString("PJSUA [%1]").arg(level) << QString::fromStdString(std::string(data, len));
}

bool SipClient::init()
{
    //check PJSUA state
    const auto state = pjsua_get_state();
    if (PJSUA_STATE_RUNNING == state) {
        qDebug() << "PJSUA already running";
        return true;
    }
    qInfo() << "Init SIP client: current state" << state;

    //create PJSUA
    auto status = pjsua_create();
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Cannot create PJSUA"), status);
        return false;
    }

    //init PJSUA
    {
        pjsua_config cfg{};
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
	cfg.cb.on_pager = &onPager;
	cfg.cb.on_pager_status = &onPagerStatus;
	cfg.cb.on_typing = &onTyping;

        //configure STUN if any
	/*std::string stunSrv;
	if (!_settings->stunServer().isEmpty()) {
            cfg.stun_srv_cnt = 1;
            if (0 < _settings->stunPort()) {
                port = ":" + QString::number(_settings->stunPort());
            }
	    stunSrv = (_settings->stunServer() + port).toStdString();
	    pj_cstr(cfg.stun_srv, stunSrv.c_str());
            cfg.stun_map_use_stun2 = PJ_FALSE;
            cfg.stun_ignore_failure = PJ_TRUE;
            cfg.stun_try_ipv6 = PJ_FALSE;
            qInfo() << "STUN server configured" << srv;
        }*/

        //user agent contains app version and PJSIP version
        static const std::string pjsipVer(pj_get_version());
        std::string userAgent = (_settings->appName() + "/" +
                                 _settings->appVersion()).toStdString() +
                " (PJSIP/" + pjsipVer + ")";
        pj_cstr(&cfg.user_agent, userAgent.c_str());
	qInfo() << userAgent;

        pjsua_logging_config log_cfg{};
        pjsua_logging_config_default(&log_cfg);
        log_cfg.msg_logging = _settings->enableSipLog() ? PJ_TRUE : PJ_FALSE;
        log_cfg.level = DEFAULT_LOG_LEVEL;
        log_cfg.console_level = DEFAULT_CONSOLE_LOG_LEVEL;
        log_cfg.cb = &pjsuaLogCallback;

        pjsua_media_config media_cfg{};
        pjsua_media_config_default(&media_cfg);
        media_cfg.channel_count = 1;
        media_cfg.ec_options = PJMEDIA_ECHO_DEFAULT |
                PJMEDIA_ECHO_USE_NOISE_SUPPRESSOR |
                PJMEDIA_ECHO_AGGRESSIVENESS_DEFAULT;
        media_cfg.no_vad = _settings->enableVad() ? PJ_FALSE : PJ_TRUE;

        status = pjsua_init(&cfg, &log_cfg, &media_cfg);
        if (PJ_SUCCESS != status) {
            errorHandler(tr("Cannot init PJSUA"), status);
            return false;
        }
    }

    auto addTransport = [this](pjsip_transport_type_e type) {
        pjsua_transport_config cfg;
        pjsua_transport_config_default(&cfg);
        cfg.port = _settings->transportSourcePort();
        const auto status = pjsua_transport_create(type, &cfg, nullptr);
        if (PJ_SUCCESS != status) {
            errorHandler(tr("Error creating transport"), status);
            return false;
        }
        return true;
    };
    std::array<pjsip_transport_type_e, 3> transports{PJSIP_TRANSPORT_UDP, PJSIP_TRANSPORT_TCP, PJSIP_TRANSPORT_TLS};
    for (const auto transp: transports) {
        if (!addTransport(transp)) {
            return false;
        }
    }

    //start PJSUA
    status = pjsua_start();
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Cannot start PJSUA"), status);
        return false;
    }

    disableAudio();

    initAudioDevicesList();
    listAudioCodecs();

#ifdef ENABLE_VIDEO
    initVideoDevicesList();
    listVideoCodecs();
#endif

    disableTcpSwitch();

    qInfo() << "Init PJSUA library" << pjsua_get_state();
    return true;
}

void SipClient::release()
{
    const auto state = pjsua_get_state();
    if (PJSUA_STATE_RUNNING == state) {
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
    }
}

bool SipClient::registerAccount()
{
    const auto state = pjsua_get_state();
    if (PJSUA_STATE_RUNNING != state) {
        errorHandler(tr("PJSUA library not started (%1)").arg(state));
        return false;
    }

    const auto& domain = _settings->sipServer();
    if (domain.isEmpty()) {
        errorHandler(tr("Domain is empty"));
        return false;
    }
    const auto destPort = QString::number(_settings->sipPort());
    const auto& username = _settings->userName();
    if (username.isEmpty()) {
        errorHandler(tr("Username is empty"));
        return false;
    }
    auto authUsername = _settings->authUserName();
    if (authUsername.isEmpty()) {
        authUsername = username;
        _settings->setAuthUserName(username);
    }
    const auto& password = _settings->password();
    if (password.isEmpty()) {
        errorHandler(tr("Password is empty"));
        return false;
    }
    const auto sipTransport = SipClient::sipTransport(_settings->sipTransport());

    //unregister previous account if needed
    unregisterAccount();

    _serverDomain = domain + ":" + destPort + sipTransport;

    pjsua_acc_config cfg{};
    pjsua_acc_config_default(&cfg);
    auto id{"sip:" + username + "@" + _serverDomain};
    const auto& displayName = _settings->displayName();
    if (!displayName.isEmpty()) {
	id = "\"" + displayName + "\" <" + id + ">";
    }
    qInfo() << "ID URI" << id;
    auto tmpId = id.toStdString();
    pj_cstr(&cfg.id, tmpId.c_str());
    QString regUri = "sip:" + _serverDomain;
    qInfo() << "Reg URI" << regUri;
    auto tmpRegUri = regUri.toStdString();
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

    std::string proxyUri;
    if (_settings->proxyEnabled()) {
        auto proxyServer = "sip:" + _settings->proxyServer();
        if (0 < _settings->proxyPort()) {
            proxyServer += ":" + QString::number(_settings->proxyPort());
        }
        proxyServer += sipTransport;
	proxyUri = proxyServer.toStdString();
	const auto status{verifySipUri(proxyUri.c_str())};
	if (PJ_SUCCESS != status) {
		errorHandler(tr("Invalid outbound proxy URL"), status);
		return false;
	}
	qInfo() << "Outbound proxy URI" << proxyServer;
	pj_cstr(&cfg.proxy[cfg.proxy_cnt++], proxyUri.c_str());
        cfg.reg_use_proxy = PJSUA_REG_USE_OUTBOUND_PROXY | PJSUA_REG_USE_ACC_PROXY;
    }

    cfg.vid_cap_dev = PJMEDIA_VID_DEFAULT_CAPTURE_DEV;
    cfg.vid_rend_dev = PJMEDIA_VID_DEFAULT_RENDER_DEV;
    cfg.vid_in_auto_show = PJ_FALSE;
    cfg.vid_out_auto_transmit = PJ_FALSE;

    cfg.allow_sdp_nat_rewrite = _settings->allowSdpNatRewrite() ? PJ_TRUE : PJ_FALSE;
    cfg.allow_contact_rewrite = _settings->allowContactAndViaRewrite() ? PJ_TRUE : PJ_FALSE;
    cfg.allow_via_rewrite = cfg.allow_contact_rewrite;
    cfg.publish_enabled = _settings->publishEnabled() ? PJ_TRUE : PJ_FALSE;

    auto status = pjsua_acc_add(&cfg, PJ_TRUE, &_accId);
    if (PJ_SUCCESS != status) {
        errorHandler("Error adding account", status);
        return false;
    }

    status = pjsua_acc_set_user_data(_accId, this);
    if (PJ_SUCCESS != status) {
        errorHandler("Error setting user data for account", status);
        unregisterAccount();
        return false;
    }

    qInfo() << "Registration started";
    onRegState(_accId);
    return true;
}

void SipClient::manuallyRegister()
{
    qDebug() << "Manual registration";
    if (PJSUA_INVALID_ID != _accId) {
        const auto status = pjsua_acc_set_registration(_accId, PJ_TRUE);
        if (PJ_SUCCESS != status) {
            errorHandler("Cannot force registration", status);
        }
    } else {
        qWarning() << "Invalid acc ID";
    }
}

bool SipClient::makeCall(const QString &userId)
{
    qDebug() << "makeCall" << userId;

    if (userId.isEmpty()) {

        return false;
    }

    std::string uriBuffer;
    pj_str_t uriStr{};
    if (!callUri(&uriStr, userId, uriBuffer)) {
        return false;
    }

    //open audio device only when needed
    enableAudio();

#ifdef ENABLE_VIDEO
    pjsua_call_setting callSetting{};
    pjsua_call_setting_default(&callSetting);
    callSetting.vid_cnt = 1;
    pjsua_call_setting *callSettingPtr{&callSetting};
#else
    constexpr pjsua_call_setting *callSettingPtr{nullptr};
#endif

    pjsua_call_id callId{PJSUA_INVALID_ID};
    const auto status{pjsua_call_make_call(_accId, &uriStr, callSettingPtr,
					    nullptr, nullptr, &callId)};
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot make call", status);
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
    return true;
}

bool SipClient::sendDtmf(const QString &dtmf)
{
    const auto callId = _activeCallModel->currentCallId();
    if (PJSUA_INVALID_ID == callId) {
        qWarning() << "Cannot send DTMF without an active call";
        return false;
    }
    qDebug() << "send DTMF" << dtmf << "to" << callId;
    auto tmpDtmf = dtmf.toStdString();
    pj_str_t dtmfStr{};
    pj_cstr(&dtmfStr, tmpDtmf.c_str());
    const auto status = pjsua_call_dial_dtmf(callId, &dtmfStr);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot send DTMF", status);
        return false;
    }
    return true;
}

bool SipClient::answer(int callId, int statusCode)
{
    if (PJSUA_INVALID_ID == callId) {
        qCritical() << "Invalid call ID";
        return false;
    }

#ifdef ENABLE_VIDEO
    pjsua_call_setting callSetting{};
    pjsua_call_setting_default(&callSetting);
    callSetting.vid_cnt = 1;
    pjsua_call_setting *callSettingPtr{&callSetting};
#else
    constexpr pjsua_call_setting *callSettingPtr{nullptr};
#endif

    const auto status = pjsua_call_answer2(callId, callSettingPtr, statusCode, nullptr, nullptr);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot answer call", status);
        return false;
    }
    return true;
}

bool SipClient::hold(int callId)
{
    if (PJSUA_INVALID_ID == callId) {
        callId = _activeCallModel->currentCallId();
    }
    const auto status = pjsua_call_set_hold(callId, nullptr);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot put call on hold", status);
        return false;
    }
    _activeCallModel->setCallState(callId, ActiveCallModel::CallState::ON_HOLD);
    qDebug() << "Hold" << callId;
    return true;
}

bool SipClient::unhold(int callId)
{
    if (PJSUA_INVALID_ID == callId) {
        callId = _activeCallModel->currentCallId();
    }
    const auto status = pjsua_call_reinvite(callId, PJSUA_CALL_UNHOLD, nullptr);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot unhold call", status);
        return false;
    }
    _activeCallModel->setCallState(callId, ActiveCallModel::CallState::CONFIRMED);
    qDebug() << "Unhold" << callId;
    return true;
}

bool SipClient::hangup(int callId)
{
    if (PJSUA_INVALID_ID == callId) {
        qCritical() << "Invalid call ID";
        return false;
    }

    const pj_status_t status = pjsua_call_hangup(callId, 0, nullptr, nullptr);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot hangup call", status);
        return false;
    }
    return true;
}

bool SipClient::unsupervisedTransfer(const QString& phoneNumber, const QString& userName)
{
    if (phoneNumber.isEmpty()) {
        errorHandler("Cannot make transfer without a destination phone number");
        return false;
    }
    const int currentCallId = _activeCallModel->currentCallId();
    if (PJSUA_INVALID_ID == currentCallId) {
        errorHandler("Cannot make transfer without an active call");
        return false;
    }

    std::string uriBuffer;
    pj_str_t uriStr{};
    if (!callUri(&uriStr, phoneNumber, uriBuffer)) {
        return false;
    }
    const auto status = pjsua_call_xfer(currentCallId, &uriStr, nullptr);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot transfer call", status);
        return false;
    }
        _callHistoryModel->addContact(currentCallId, userName, phoneNumber,
                                      CallHistoryModel::CallStatus::TRANSFERRED);
        _callHistoryModel->updateCallStatus(currentCallId,
                                            CallHistoryModel::CallStatus::UNKNOWN, true);
    return true;
}

bool SipClient::swap(int callId)
{
    const auto &confCids = _activeCallModel->confirmedCallsId();
    if (confCids.isEmpty()) {
        qCritical() << "No active call(s) to swap";
        return false;
    }
    for (auto confCid: confCids) {
        if (!hold(confCid)) {
            return false;
        }
    }
    const bool rc = unhold(callId);
    if (rc) {
        _activeCallModel->setCurrentCallId(callId);
    }
    qDebug() << "Swap" << confCids << "with" << callId;
    return rc;
}

bool SipClient::merge(int callId)
{
    if (!unhold(callId)) {
        return false;
    }
    setupConferenceCall(callId);
    _activeCallModel->update();
    qDebug() << "Merge" << callId;
    return true;
}

bool SipClient::playDigit(const QString& digit)
{
    if (digit.isEmpty()) {
        return false;
    }

    initToneGenerator();

    if (nullptr == _toneGenPool) {
        qCritical() << "Generator pool is NULL";
        return false;
    }

    pjmedia_tone_digit toneDigit;
    toneDigit.digit = digit.toStdString().c_str()[0];
    toneDigit.on_msec = TONE_GEN_ON_MS;
    toneDigit.off_msec = TONE_GEN_OFF_MS;
    toneDigit.volume = 0;
    const auto status = pjmedia_tonegen_play_digits(_toneGenMediaPort, 1, &toneDigit, 0);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Play digit"), status);
        return false;
    }
    _toneGenTimer.start();
    return true;
}

bool SipClient::mute(bool start, int callId)
{
    if (PJSUA_INVALID_ID == callId) {
        callId = _activeCallModel->currentCallId();
    }
    const auto callConfPort = pjsua_call_get_conf_port(callId);
    if (PJSUA_INVALID_ID == callConfPort) {
        qCritical() << "Cannot get conference slot of call ID" << callId;
        return false;
    }
    pj_status_t status = PJ_SUCCESS;
    if (start) {
        status = pjsua_conf_disconnect(0, callConfPort);
    } else {
        status = pjsua_conf_connect(0, callConfPort);
    }
    if (PJ_SUCCESS != status) {
        const QString msg = start ? tr("Cannot mute call") : tr("Cannot unmute call");
        errorHandler(msg, status);
        return false;
    }
    qDebug() << "Mute" << start << callId;
    return true;
}

bool SipClient::setupConferenceCall(pjsua_call_id callId)
{
    const auto callCount = pjsua_call_get_count();
    if (2 > callCount) {
        qDebug() << "Nothing to do" << callCount;
        return true;
    }
    pjsua_conf_port_id confPortId = pjsua_call_get_conf_port(callId);
    if (PJSUA_INVALID_ID == confPortId) {
        errorHandler(tr("Cannot get current call conf port"));
        return false;
    }
    const QVector<int> confCalls = _activeCallModel->confirmedCallsId();
    for (auto otherCallId: confCalls) {
        if (callId == otherCallId) {
            continue;
        }
        const pjsua_conf_port_id otherCallConfPort = pjsua_call_get_conf_port(otherCallId);
        if (PJSUA_INVALID_ID == otherCallConfPort) {
            qWarning() << "Cannot get conference slot of call ID" << otherCallId;
            continue;
        }
        auto status = pjsua_conf_connect(confPortId, otherCallConfPort);
        if (PJ_SUCCESS != status) {
            errorHandler(tr("Cannot connect current conf slot to other"), status);
            return false;
        }
        status = pjsua_conf_connect(otherCallConfPort, confPortId);
        if (PJ_SUCCESS != status) {
            errorHandler(tr("Cannot connect other conf slot to current"), status);
            return false;
        }
    }
    qDebug() << "Conference call: confirmed calls" << confCalls.size();
    return true;
}

QString SipClient::formatErrorMessage(const QString &title, pj_status_t status)
{
    QString fullMsg{title};
    if (PJ_SUCCESS != status) {
        static std::array<char, MAX_ERROR_MSG_SIZE> message;
        pj_strerror(status, message.data(), message.size());
	fullMsg.append(":").append(message.data());
    }
    qWarning() << fullMsg;
    return fullMsg;
}

bool SipClient::unregisterAccount()
{
    if (PJSUA_INVALID_ID != _accId) {
        auto status = pjsua_acc_del(_accId);
        if (PJ_SUCCESS != status) {
            errorHandler("Error removing account", status);
            return false;
        }
        _accId = PJSUA_INVALID_ID;
	_serverDomain.clear();
	emit registrationStatusChanged(RegistrationStatus::Unregistered, tr("Not Registered"));
        qDebug() << "Account unregistered";
    } else {
        qDebug() << "No account to unregister";
    }
    return true;
}

bool SipClient::disableAudio()
{
    auto status = pjsua_set_null_snd_dev();
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Cannot set null audio device"), status);
        return false;
    }
    return true;
}

void SipClient::initAudioDevicesList()
{
    //refresh device list (needed when device changed notification is received)
    auto status = pjmedia_aud_dev_refresh();
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot refresh audio devices list", status);
    }

    const auto dev_count = static_cast<pjmedia_aud_dev_index>(pjmedia_aud_dev_count());
    QVector<AudioDevices::DeviceInfo> inputDevices;
    QVector<AudioDevices::DeviceInfo> outputDevices;
    for (pjmedia_aud_dev_index dev_idx = 0; dev_idx < dev_count; ++dev_idx) {
        pjmedia_aud_dev_info info{};
        const auto status = pjmedia_aud_dev_get_info(dev_idx, &info);
        if (PJ_SUCCESS != status) {
            errorHandler(tr("Cannot get audio device info"), status);
            break;
        }
        if (0 < info.input_count) {
            inputDevices.push_back({info.name, dev_idx});
        }
        if (0 < info.output_count) {
            outputDevices.push_back({info.name, dev_idx});
        }
    }
    if (inputDevices.isEmpty()) {
        errorHandler(tr("No input audio devices"));
    }

    _inputAudioDevices->init(inputDevices);
    const auto inDevInfo = Settings::inputAudioDeviceInfo();
    auto inDevModelIndex = _inputAudioDevices->deviceIndex(inDevInfo);
    if (AudioDevices::INVALID_MODEL_INDEX == inDevModelIndex) {
        qDebug() << "Invalid input device, using default device";
        inDevModelIndex = 0;
    }

    if (outputDevices.isEmpty()) {
        errorHandler(tr("No output audio devices"));
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

void SipClient::listAudioCodecs()
{
	std::array<pjsua_codec_info, MAX_CODECS> codecInfo;
	auto codecCount = static_cast<unsigned int>(codecInfo.size());

	//read first default priorities
	pj_status_t status = pjsua_enum_codecs(codecInfo.data(), &codecCount);
	if (PJ_SUCCESS != status) {
		errorHandler(tr("Cannot enum audio codecs"), status);
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
		errorHandler(tr("Cannot enum audio codecs"), status);
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

bool SipClient::setAudioCodecPriority(const QString &codecId, int priority)
{
	if ((0 > priority) || (priority > MAX_PRIORITY)) {
		qWarning() << "Invalid audio codec priority" << codecId << priority;
		return false;
	}
	const auto stdCodecId = codecId.toStdString();
	pj_str_t pjCodecId{};
	pj_cstr(&pjCodecId, stdCodecId.c_str());
	const auto status = pjsua_codec_set_priority(&pjCodecId, priority);
	if (PJ_SUCCESS != status) {
		errorHandler(tr("Cannot set audio codec priority"), status);
	}
	return PJ_SUCCESS == status;
}

#ifdef ENABLE_VIDEO
void SipClient::initVideoDevicesList()
{
    auto status = pjmedia_vid_dev_refresh();
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Cannot refresh video device list"), status);
    }
    const auto dev_count = static_cast<pjmedia_vid_dev_index>(pjmedia_vid_dev_count());
    QVector<VideoDevices::DeviceInfo> videoDevices;
    for (pjmedia_vid_dev_index dev_idx = 0; dev_idx < dev_count; ++dev_idx) {
        pjmedia_vid_dev_info info;
        status = pjmedia_vid_dev_get_info(dev_idx, &info);
        if (PJ_SUCCESS != status) {
            errorHandler(tr("Cannot get video device info"), status);
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
        errorHandler(tr("No video devices"));
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

void SipClient::listVideoCodecs()
{
    std::array<pjsua_codec_info, MAX_CODECS> codecInfo;
    unsigned int codecCount = static_cast<unsigned int>(codecInfo.size());
    auto status = pjsua_vid_enum_codecs(codecInfo.data(), &codecCount);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Cannot enum video codecs"), status);
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
        errorHandler(tr("Cannot enum video codecs"), status);
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

bool SipClient::setVideoCodecBitrate(const QString &codecId, int bitrate)
{
    const auto stdCodecId = codecId.toStdString();
    pj_str_t pjCodecId;
    pj_cstr(&pjCodecId, stdCodecId.c_str());
    pjmedia_vid_codec_param param;
    auto status = pjsua_vid_codec_get_param(&pjCodecId, &param);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Cannot set video codec priority"), status);
        return false;
    }
    param.enc_fmt.det.vid.avg_bps = bitrate;
    param.enc_fmt.det.vid.max_bps = bitrate;
    status = pjsua_vid_codec_set_param(&pjCodecId, &param);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Cannot set video codec priority"), status);
        return false;
    }
    return true;
}
#endif

bool SipClient::disableTcpSwitch()
{
    auto *cfg = pjsip_cfg();
    bool rc{};
    if (nullptr != cfg) {
	const auto disable = _settings->disableTcpSwitch();
	cfg->endpt.disable_tcp_switch = disable ? PJ_TRUE : PJ_FALSE;
        rc = true;
	qDebug() << "Disable TCP switch" << disable;
    } else {
        qWarning() << "Cannot get PJSIP config";
    }
    return rc;
}

QString SipClient::sipTransport(int type)
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

bool SipClient::callUri(pj_str_t *uri, const QString &userId, std::string &uriBuffer)
{
    if (nullptr == uri) {
	errorHandler(tr("Invalid output"), PJ_SUCCESS);
	return false;
    }
    if (_serverDomain.isEmpty()) {
	errorHandler(tr("No SIP server"), PJ_SUCCESS);
        return false;
    }

    const auto sipUri{"sip:" + userId + "@" + _serverDomain};

    uriBuffer = sipUri.toStdString();
    const char *uriPtr = uriBuffer.c_str();
    const auto status = verifySipUri(uriPtr);
    if (PJ_SUCCESS != status) {
	errorHandler(tr("URI verification failed"), status);
        return false;
    }
    pj_cstr(uri, uriPtr);

    return true;
}

pjsua_conf_port_id SipClient::callConfPort(pjsua_call_id callId)
{
    if (PJ_FALSE == pjsua_call_is_active(callId)) {
        qDebug() << "No active call";
        return PJSUA_INVALID_ID;
    }

    const auto confSlot = pjsua_call_get_conf_port(callId);
    if (PJSUA_INVALID_ID == confSlot) {
        qCritical() << "Cannot get conference slot of call ID" << callId;
    }
    return confSlot;
}

bool SipClient::enableAudio()
{
    const auto captureDevInfo = _inputAudioDevices->deviceInfo();
    if (!captureDevInfo.isValid()) {
        const auto msg = QString("Invalid input audio device index %1").arg(captureDevInfo.toString());
        errorHandler(msg);
        pjsua_set_null_snd_dev();
        return false;
    }
    const auto playbackDevInfo = _outputAudioDevices->deviceInfo();
    if (!playbackDevInfo.isValid()) {
        const auto msg = QString("Invalid output audio device index %1").arg(playbackDevInfo.toString());
        errorHandler(msg);
        pjsua_set_null_snd_dev();//for testing purposes only
        return false;
    }

    //slow operation: about 1 sec
    const auto status = pjsua_set_snd_dev(captureDevInfo.index, playbackDevInfo.index);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot set audio devices", status);
        return false;
    }

    qDebug() << "Finished to set audio devices: captureDev" << captureDevInfo.index <<
                ", playbackDev" << playbackDevInfo.index;
    return true;
}

void SipClient::hangupAll()
{
    pjsua_call_hangup_all();
}

bool SipClient::initRingTonePlayer(pjsua_call_id id, bool incoming)
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

    const auto tmpSoundFile = soundFileStr.toStdString();
    pj_str_t soundFile{};
    pj_cstr(&soundFile, tmpSoundFile.c_str());

    pjsua_player_id playerId = PJSUA_INVALID_ID;
    const auto status = pjsua_player_create(&soundFile, 0, &playerId);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot create player", status);
        return false;
    }
    _playerId[id] = playerId;
    return true;
}

bool SipClient::startPlayingRingTone(pjsua_call_id id, bool incoming)
{
    if (!initRingTonePlayer(id, incoming)) {
        qDebug() << "Cannot start playing ringtone for call" << id;
        return false;
    }
    if (0 == _playerId.count(id)) {
        qCritical() << "Cannot find a player ID for call" << id;
        return false;
    }
    const auto playerId = _playerId.at(id);
    pj_status_t status = pjsua_conf_connect(pjsua_player_get_conf_port(playerId), 0);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot play ring tone to output device", status);
        return false;
    }
    qInfo() << "Start playing ringtone";
    return true;
}

void SipClient::stopPlayingRingTone(pjsua_call_id id)
{
    if (0 == _playerId.count(id)) {
        //qWarning() << "Cannot find player ID for call" << id;
        return;
    }
    const auto playerId = _playerId.at(id);
    auto status = pjsua_conf_disconnect(pjsua_player_get_conf_port(playerId), 0);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot stop playing ring tone to output device", status);
        return;
    }
    qInfo() << "Stop playing ringtone";
    releaseRingTonePlayer(id);
}

bool SipClient::releaseRingTonePlayer(pjsua_call_id id)
{
    if (0 == _playerId.count(id)) {
        qCritical() << "Cannot find a player ID for call" << id;
        return false;
    }
    const auto playerId = _playerId.at(id);
    const auto status = pjsua_player_destroy(playerId);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot destroy ringtone player", status);
        return false;
    }
    _playerId.erase(id);
    qInfo() << "Release ringtone player";
    return true;
}

void SipClient::releaseRingTonePlayers()
{
    for(const auto &it: _playerId) {
        releaseRingTonePlayer(it.first);
    }
}

bool SipClient::initToneGenerator()
{
    if (nullptr != _toneGenPool) {
        return true;//nothing to do
    }
    _toneGenPool = pjsua_pool_create("toneGen", PJSUA_POOL_SIZE, PJSUA_POOL_SIZE);
    if (nullptr == _toneGenPool) {
        errorHandler(tr("Cannot allocate pool for tone generator"));
        return false;
    }
    enableAudio();
    auto status = pjmedia_tonegen_create(_toneGenPool, TONE_GEN_CLOCK_RATE_HZ,
                                         TONE_GEN_CHANNEL_COUNT,
                                         TONE_GEN_SAMPLES_PER_FRAME,
                                         TONE_GEN_BITS_PER_SAMPLE, 0, &_toneGenMediaPort);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Tone generator create"), status);
        return false;
    }
    status = pjsua_conf_add_port(_toneGenPool, _toneGenMediaPort, &_toneGenConfPort);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Tone generator add port"), status);
        return false;
    }
    status = pjsua_conf_adjust_rx_level(_toneGenConfPort, _settings->dialpadSoundVolume());
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Tone generator adjust rx level"), status);
        return false;
    }
    status = pjsua_conf_connect(_toneGenConfPort, 0);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Tone generator conf connect"), status);
        return false;
    }
    qInfo() << "Init tone generator";
    return true;
}

void SipClient::releaseToneGenerator()
{
    if ((nullptr != _toneGenMediaPort) && pjmedia_tonegen_is_busy(_toneGenMediaPort)) {
        qDebug() << "Tone gen is busy";
        return;
    }
    if (PJSUA_INVALID_ID != _toneGenConfPort) {
        disableAudio();
        const auto status = pjsua_conf_remove_port(_toneGenConfPort);
        if (PJ_SUCCESS != status) {
            errorHandler(tr("Tone generator conf remove"), status);
        }
        _toneGenConfPort = PJSUA_INVALID_ID;
    }
    if (nullptr != _toneGenMediaPort) {
        const auto status = pjmedia_port_destroy(_toneGenMediaPort);
        if (PJ_SUCCESS != status) {
            errorHandler(tr("Tone generator port destroy"), status);
        }
        _toneGenMediaPort = nullptr;
    }
    if (nullptr != _toneGenPool) {
        pj_pool_release(_toneGenPool);
        _toneGenPool = nullptr;
    }
    qInfo() << "Release tone generator";
}

bool SipClient::createRecorder(pjsua_call_id callId)
{
    if (PJSUA_INVALID_ID == callId) {
        qCritical() << "Invalid call ID";
        return false;
    }
    if ((0 != _recorderId.count(callId)) &&
            (PJSUA_INVALID_ID != _recorderId.at(callId))) {
        qWarning() << "Recorded already created for call ID " << callId;
        return false;
    }

    //generate recording file name
    const auto curDateTime = QDateTime::currentDateTime();
    const QString recFileName = _settings->recPath() + "/" +
            curDateTime.toString("MMMM_dd_yyyy-hh_mm_ss") + ".wav";
    pj_str_t recordFile{};
    pj_cstr(&recordFile, recFileName.toUtf8().data());

    //create recorder
    pjsua_recorder_id recId = PJSUA_INVALID_ID;
    const auto status = pjsua_recorder_create(&recordFile, 0, nullptr, 0, 0, &recId);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Cannot create recorder"), status);
        return false;
    }
    _recorderId[callId] = recId;
    qDebug() << "Created recorder " << recFileName;
    return true;
}

bool SipClient::startRecording(pjsua_call_id callId)
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
    const auto recConfPort = pjsua_recorder_get_conf_port(_recorderId.at(callId));
    if (PJSUA_INVALID_ID == recConfPort) {
        releaseRecorder(callId);
        errorHandler(tr("Cannot get recorder conf port"));
        return false;
    }
    const auto status = pjsua_conf_connect(callConfPort, recConfPort);
    if (PJ_SUCCESS != status) {
        releaseRecorder(callId);
        errorHandler(tr("Cannot start recording"), status);
        return false;
    }
    qDebug() << "Recording started for call ID " << callId;
    return true;
}

bool SipClient::stopRecording(pjsua_call_id callId)
{
    if (PJSUA_INVALID_ID == callId) {
        qCritical() << "Invalid call ID";
        return false;
    }
    if ((0 == _recorderId.count(callId)) ||
            (PJSUA_INVALID_ID == _recorderId.at(callId))) {
        qWarning() << "Invalid recorder ID";
        return true;
    }

    if (PJ_TRUE == pjsua_call_is_active(callId)) {
        //stop recording
        const auto callConfPort = pjsua_call_get_conf_port(callId);
        if (PJSUA_INVALID_ID == callConfPort) {
            releaseRecorder(callId);
            errorHandler(tr("Cannot get call conf port"));
            return false;
        }
        const auto recConfPort = pjsua_recorder_get_conf_port(_recorderId.at(callId));
        if (PJSUA_INVALID_ID == recConfPort) {
            releaseRecorder(callId);
            errorHandler(tr("Cannot get recorder conf port"));
            return false;
        }
        const auto status = pjsua_conf_disconnect(callConfPort, recConfPort);
        if (PJ_SUCCESS != status) {
            releaseRecorder(callId);
            errorHandler(tr("Cannot stop recording"), status);
            return false;
        }
    }
    qDebug() << "Recording stopped for call ID " << callId;
    return releaseRecorder(callId);
}

bool SipClient::releaseRecorder(pjsua_call_id callId)
{
    if ((0 == _recorderId.count(callId)) ||
            (PJSUA_INVALID_ID == _recorderId.at(callId))) {
        qWarning() << "Invalid recorder ID";
        return true;
    }
    const auto status = pjsua_recorder_destroy(_recorderId.at(callId));
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Cannot destroy recorder"), status);
        return false;
    }
    _recorderId.erase(callId);
    qDebug() << "Recorder has been released";
    return true;
}

void SipClient::processRegistrationStatus(pjsua_acc_info accInfo)
{
    auto registrationStatus{RegistrationStatus::Unregistered};
    switch (accInfo.status) {
    case PJSIP_SC_OK:
        registrationStatus = RegistrationStatus::Registered;
        break;
    case PJSIP_SC_TRYING:
        registrationStatus = RegistrationStatus::Trying;
        break;
    case PJSIP_SC_PROGRESS:
        registrationStatus = RegistrationStatus::InProgress;
        break;
    case PJSIP_SC_SERVICE_UNAVAILABLE:
        registrationStatus = RegistrationStatus::ServiceUnavailable;
        break;
    case PJSIP_SC_TEMPORARILY_UNAVAILABLE:
        registrationStatus = RegistrationStatus::TemporarilyUnavailable;
        break;
    default:;
    }
    const auto statusText = SipClient::toString(accInfo.status_text) + QString(" (%1)").arg(accInfo.status);
    qDebug() << "Reg status" << statusText;
    emit registrationStatusChanged(registrationStatus, statusText);
}

void SipClient::processIncomingCall(pjsua_call_id callId, pjsua_call_info callInfo)
{
    QString remoteInfo = toString(callInfo.remote_info);
    qDebug() << "Incoming call from" << remoteInfo;

    answer(callId, PJSIP_SC_RINGING);

    QString userName;
    QString userId;
    extractUserNameAndId(userName, userId, remoteInfo);
    emit incoming(callId, userName, userId);
}

void SipClient::processCallState(pjsua_call_id callId, pjsua_call_info callInfo)
{
    const QString stateText = toString(callInfo.state_text);
    const QString lastStatusText = toString(callInfo.last_status_text);
    qDebug() << "Call" << callId << ", state =" << stateText << "(" << callInfo.last_status << ")"
             << lastStatusText;

    if ((PJSIP_SC_BAD_REQUEST <= callInfo.last_status) &&
	    (PJSIP_SC_REQUEST_TERMINATED != callInfo.last_status) &&
	    (PJSIP_SC_REQUEST_TIMEOUT != callInfo.last_status)) {
        //show all SIP errors above Client Failure Responses
	emit errorMessage(lastStatusText.isEmpty() ? stateText : lastStatusText);
    }

    switch (callInfo.state) {
    case PJSIP_INV_STATE_NULL:
        break;
    case PJSIP_INV_STATE_CALLING: {
	    const QString remoteInfo = SipClient::toString(callInfo.remote_info);
	    QString userName;
	    QString userId;
	    SipClient::extractUserNameAndId(userName, userId, remoteInfo);
	    emit calling(callId, userName, userId);
    }
    break;
    case PJSIP_INV_STATE_INCOMING:
        break;
    case PJSIP_INV_STATE_EARLY:
        break;
    case PJSIP_INV_STATE_CONNECTING:
        break;
    case PJSIP_INV_STATE_CONFIRMED:
	connectCallToSoundDevices(callInfo.conf_slot);
        emit confirmed(callId);
        break;
    case PJSIP_INV_STATE_DISCONNECTED:
        emit disconnected(callId);
        break;
    default:
	qCritical() << "Unhandled call state" << callInfo.state;
    }
}

void SipClient::processCallMediaState(pjsua_call_id callId, pjsua_call_info callInfo)
{
    qDebug() << "Media state changed for call" << callId << ":" << SipClient::toString(callInfo.last_status_text)
	     << callInfo.last_status;
    if (PJSUA_CALL_MEDIA_ACTIVE == callInfo.media_status) {
	qInfo() << "Media active" << callInfo.media_cnt;
	bool hasVideo{};
	for (unsigned medIdx = 0; medIdx < callInfo.media_cnt; ++medIdx) {
	    if (isMediaActive(callInfo.media[medIdx])) {
                pjsua_stream_info streamInfo{};
                auto status = pjsua_call_get_stream_info(callId, medIdx, &streamInfo);
                if (PJ_SUCCESS == status) {
                    if (PJMEDIA_TYPE_AUDIO == streamInfo.type) {
                        GET_INSTANCE_CID(callId)
			instance->connectCallToSoundDevices(callInfo.conf_slot);
                        const auto &fmt = streamInfo.info.aud.fmt;
                        qInfo() << "Audio codec info: encoding" << toString(fmt.encoding_name)
                                << ", clock rate" << fmt.clock_rate << "Hz, channel count"
                                << fmt.channel_cnt;
#ifdef ENABLE_VIDEO
                    } else if (PJMEDIA_TYPE_VIDEO == streamInfo.type) {
                        const auto &fmt = streamInfo.info.vid.codec_info;
                        qInfo() << "Video codec info: encoding" << toString(fmt.encoding_name)
                                << ", encoding desc." << toString(fmt.encoding_desc)
                                << ", clock rate:" << fmt.clock_rate << "Hz";
			hasVideo = true;
#endif
                    }
                } else {
                    errorHandler("Cannot get stream info", status);
                }
            }
        }
#ifdef ENABLE_VIDEO
	manageVideo(hasVideo);
#endif
    } else if ((PJSUA_CALL_MEDIA_LOCAL_HOLD != callInfo.media_status) &&
	       (PJSUA_CALL_MEDIA_REMOTE_HOLD != callInfo.media_status)) {
        qWarning() << "Connection lost";
        emit registrationStatusChanged(RegistrationStatus::Unregistered, tr("Connection lost"));
        emit errorMessage(tr("You need an active Internet connection to make calls."));
    }
}

void SipClient::dumpStreamStats(pjmedia_rtcp_stat stat)
{
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

void SipClient::processBuddyState(pjsua_buddy_id buddyId)
{
    pjsua_buddy_info info{};
    const auto status = pjsua_buddy_get_info(buddyId, &info);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Cannot get buddy info"), status);
        return;
    }

    QString buddyState;
    switch (info.status) {
    case PJSUA_BUDDY_STATUS_UNKNOWN:
	buddyState = tr("Unknown");
	break;
    case PJSUA_BUDDY_STATUS_ONLINE:
	buddyState = tr("Online");
	break;
    case  PJSUA_BUDDY_STATUS_OFFLINE:
	buddyState = tr("Offline");
	break;
    }

    qDebug() << "Buddy state" << buddyId << buddyState;
    emit buddyStatusChanged(buddyId, buddyState);
}

void SipClient::extractUserNameAndId(QString& userName, QString& userId, const QString &info)
{
    const static QRegularExpression re("\"([\\w|\\s]*)\"\\s*<sip:([\\w|\\s]*)@.*");
    qDebug() << "Matching" << info;
    QRegularExpressionMatch match = re.match(info);
    userName.clear();
    userId.clear();
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
}

void SipClient::connectCallToSoundDevices(pjsua_conf_port_id confPortId)
{
    qDebug() << "Connect call conf port" << confPortId << "to sound devices";

    //ajust volume level
    setMicrophoneVolume(confPortId);
    setSpeakersVolume(confPortId);

    auto status{pjsua_conf_connect(confPortId, 0)};
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot connect conf slot to playback slot", status);
    }

    status = pjsua_conf_connect(0, confPortId);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot connect capture slot to conf slot", status);
    }
}

bool SipClient::setMicrophoneVolume(pjsua_call_id callId, bool mute)
{
    const auto confSlot = SipClient::callConfPort(callId);
    if (PJSUA_INVALID_ID == confSlot) {
        return false;
    }

    //microphone volume
    const float microphoneLevel = mute ? 0.0 : _settings->microphoneVolume();
    qInfo() << "Mic level" << microphoneLevel;
    const auto status = pjsua_conf_adjust_tx_level(confSlot, microphoneLevel);
    if (PJ_SUCCESS != status) {
	errorHandler("Cannot adjust tx level", status);
        return false;
    }
    return true;
}

bool SipClient::setSpeakersVolume(pjsua_call_id callId, bool mute)
{
    const auto confSlot = SipClient::callConfPort(callId);
    if (PJSUA_INVALID_ID == confSlot) {
        return false;
    }

    //speakers volume
    const float speakersLevel = mute ? 0.0 : _settings->speakersVolume();
    qInfo() << "Speakers level" << speakersLevel;
    const pj_status_t status = pjsua_conf_adjust_rx_level(confSlot, speakersLevel);
    if (PJ_SUCCESS != status) {
	errorHandler("Cannot adjust rx level", status);
        return false;
    }
    return true;
}

#ifdef ENABLE_VIDEO
bool SipClient::setVideoCodecPriority(const QString &codecId, int priority)
{
    if ((0 > priority) || (priority > MAX_PRIORITY)) {
        qWarning() << "Invalid video codec priority" << codecId << priority;
        return false;
    }
    const auto stdCodecId = codecId.toStdString();
    pj_str_t pjCodecId{};
    pj_cstr(&pjCodecId, stdCodecId.c_str());
    const auto status = pjsua_vid_codec_set_priority(&pjCodecId, priority);
    if (PJ_SUCCESS != status) {
        errorHandler("Cannot set video codec priority", status);
    }
    return PJ_SUCCESS == status;
}

void SipClient::manageVideo(bool enable)
{
    const auto currentCallId{_activeCallModel->currentCallId()};
    if (PJSUA_INVALID_ID == currentCallId) {
        qWarning() << "No active call";
        return;
    }

    const auto hasVideoStream{-1 < pjsua_call_get_vid_stream_idx(currentCallId)};
    if (!hasVideoStream && !enable) {
	qWarning() << "Video already disabled" << currentCallId;
	return;
    }
    if (hasVideoStream && enable) {
	qWarning() << "Video already enabled" << currentCallId;
        return;
    }

    const auto op{enable ? PJSUA_CALL_VID_STRM_START_TRANSMIT : PJSUA_CALL_VID_STRM_STOP_TRANSMIT};
    const auto status = pjsua_call_set_vid_strm(currentCallId, op, nullptr);
    if (status == PJ_SUCCESS) {
        qInfo() << "Start transmitting" << enable;
    } else {
        const auto msg = enable ? tr("Cannot start transmitting video stream") : tr("Cannot stop transmitting video stream");
	formatErrorMessage(msg, status);
    }
    enable ? initVideoWindow() : releaseVideoWindow();
}

void SipClient::initVideoWindow()
{
    const auto currentCallId = _activeCallModel->currentCallId();
    if (PJSUA_INVALID_ID == currentCallId) {
        return;
    }

    pjsua_call_info ci;
    auto status = pjsua_call_get_info(currentCallId, &ci);
    if (status != PJ_SUCCESS) {
        errorHandler(tr("Error get call info"), status);
        return;
    }
    for (unsigned i = 0; i < ci.media_cnt; ++i) {
        if ((ci.media[i].type == PJMEDIA_TYPE_VIDEO) &&
                (ci.media[i].dir & PJMEDIA_DIR_DECODING))
        {
            pjsua_vid_win_info wi;
            status = pjsua_vid_win_get_info(ci.media[i].stream.vid.win_in, &wi);
            if (status != PJ_SUCCESS) {
                errorHandler(tr("Error get vid win info"), status);
                return;
            }
            _videoWindow = QPointer(new QDialog());
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

void SipClient::releaseVideoWindow()
{
    _videoWindow.clear();
    releasePreviewWindow();
}

void SipClient::initPreviewWindow()
{
    const auto &devInfo = _videoDevices->deviceInfo();
    pjsua_vid_preview_param pre_param;
    pjsua_vid_preview_param_default(&pre_param);
    pre_param.rend_id = PJMEDIA_VID_DEFAULT_RENDER_DEV;
    pre_param.show = PJ_TRUE;
    pj_status_t status = pjsua_vid_preview_start(devInfo.index, &pre_param);
    if (status != PJ_SUCCESS) {
        errorHandler(tr("Error creating preview"), status);
        return;
    }
    auto wid = pjsua_vid_preview_get_win(devInfo.index);
    if (PJSUA_INVALID_ID != wid) {
        pjsua_vid_win_info wi;
        status = pjsua_vid_win_get_info(wid, &wi);
        if (status != PJ_SUCCESS) {
            errorHandler(tr("Cannot get window info"), status);
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
                _previewWindow = QPointer(preview);
            }
            preview->show();
            preview->raise();
        } else {
            qWarning() << "Cannot create preview widget";
        }
    }
}

void SipClient::releasePreviewWindow()
{
    _previewWindow.clear();
    const auto &devInfo = _videoDevices->deviceInfo();
    const pjsua_vid_win_id wid = pjsua_vid_preview_get_win(devInfo.index);
    if (wid != PJSUA_INVALID_ID) {
        pjsua_vid_win_set_show(wid, PJ_FALSE);
        pj_status_t status = pjsua_vid_preview_stop(devInfo.index);
        if (status != PJ_SUCCESS) {
            errorHandler(tr("Error releasing preview"), status);
        }
    }
}

void SipClient::setVideoWindowSize(pj_bool_t isNative, pjsua_vid_win_id wid, int width, int height)
{
    if (PJ_TRUE != isNative) {
        pjmedia_rect_size size;
        size.w = width;
        size.h = height;
        const auto status = pjsua_vid_win_set_size(wid, &size);
        if (status != PJ_SUCCESS) {
            errorHandler(tr("Cannot set window size"), status);
        }
    } else {
        qWarning() << "Window is native";
    }
}
#endif

int SipClient::addBuddy(const QString &userId)
{
    if (PJSUA_MAX_BUDDIES <= pjsua_get_buddy_count()) {
        errorHandler(tr("Maximum number of buddies exceeded"));
        return PJSUA_INVALID_ID;
    }
    pjsua_buddy_config buddyCfg{};
    pjsua_buddy_config_default(&buddyCfg);
    std::string uriBuffer;
    const bool rc = callUri(&buddyCfg.uri, userId, uriBuffer);
    if (!rc) {
        errorHandler(tr("Cannot buddy URI"));
        return PJSUA_INVALID_ID;
    }
    pjsua_buddy_id buddyId = PJSUA_INVALID_ID;
    buddyCfg.subscribe = PJ_TRUE;
    buddyCfg.user_data = nullptr;
    auto status = pjsua_buddy_add(&buddyCfg, &buddyId);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Cannot add buddy"), status);
        return PJSUA_INVALID_ID;
    }
    status = pjsua_buddy_set_user_data(buddyId, this);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Cannot set buddy user data"), status);
    }
    //must update the presence after setting the user data
    status = pjsua_buddy_update_pres(buddyId);
    if (PJ_SUCCESS != status) {
	errorHandler(tr("Cannot update buddy presence"), status);
    }
    qDebug() << "Added buddy" << userId << buddyId;
    return buddyId;
}

bool SipClient::removeBuddy(int buddyId)
{
    const auto status = pjsua_buddy_del(buddyId);
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Cannot del buddy"), status);
        return false;
    }
    return true;
}

bool SipClient::sendText(const QString& userId, const QString& txt)
{
	qDebug() << "sendText" << userId;
	if (PJSUA_INVALID_ID == _accId) {
		errorHandler(tr("No registered account"), PJ_SUCCESS);
		return false;
	}
	if (userId.isEmpty()) {
		errorHandler(tr("Empty user ID"), PJ_SUCCESS);
		return false;
	}
	if (txt.isEmpty()) {
		errorHandler(tr("Empty message"), PJ_SUCCESS);
		return false;
	}

	std::string uriBuffer;
	pj_str_t uriStr{};
	if (!callUri(&uriStr, userId, uriBuffer)) {
		return false;
	}

	const auto msg{txt.toStdString()};
	pj_str_t content{};
	pj_cstr(&content, msg.c_str());
	const auto status{pjsua_im_send(_accId, &uriStr, nullptr, &content, nullptr, &_accId)};
	if (PJ_SUCCESS != status) {
		errorHandler("Cannot send text", status);
		return false;
	}
	return true;
}

bool SipClient::sendTyping(const QString& userId, bool isTyping)
{
	qDebug() << "sendTyping" << userId << isTyping;
	if (PJSUA_INVALID_ID == _accId) {
		qCritical() << "No registered account";
		return false;
	}
	if (userId.isEmpty()) {
		qCritical() << "Empty user ID";
		return false;
	}

	std::string uriBuffer;
	pj_str_t uriStr{};
	if (!callUri(&uriStr, userId, uriBuffer)) {
		return false;
	}
	const auto status{pjsua_im_typing(_accId, &uriStr, isTyping ? PJ_TRUE : PJ_FALSE, nullptr)};
	if (PJ_SUCCESS != status) {
		errorHandler("Cannot send typing", status);
		return false;
	}
	return true;
}
