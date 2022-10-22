#include "sip_client.h"
#include "settings.h"
#include "models/audio_devices.h"
#include "models/audio_codecs.h"
#include "models/video_codecs.h"
#include <QDebug>

SipClient::SipClient(Settings *settings,
          QObject *parent) :
    _settings(settings),
    _inputAudioDevices(new AudioDevices(parent)),
    _outputAudioDevices(new AudioDevices(parent)),
    _videoDevices(new VideoDevices(parent)),
    _audioCodecs(new AudioCodecs(parent)),
    _videoCodecs(new VideoCodecs(parent)),
    QObject{parent}
{}

bool SipClient::init()
{
    //check PJSUA state
    const auto state = pjsua_get_state();
    if (PJSUA_STATE_RUNNING == state) {
        qDebug() << "PJSUA already running";
        return true;
    }

    //create PJSUA
    auto status = pjsua_create();
    if (PJ_SUCCESS != status) {
        errorHandler(tr("Cannot create PJSUA"), status);
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
        log_cfg.cb = &pjsuaLogCallback;

        pjsua_media_config media_cfg;
        pjsua_media_config_default(&media_cfg);
        media_cfg.channel_count = 1;
        media_cfg.ec_options = PJMEDIA_ECHO_DEFAULT |
                PJMEDIA_ECHO_USE_NOISE_SUPPRESSOR |
                PJMEDIA_ECHO_AGGRESSIVENESS_DEFAULT;
        media_cfg.no_vad = PJ_FALSE;

        status = pjsua_init(&cfg, &log_cfg, &media_cfg);
        if (PJ_SUCCESS != status) {
            errorHandler(tr("Cannot init PJSUA"), status);
            return false;
        }
    }

    auto addTransport = [this](pjsip_transport_type_e type) {
        pjsua_transport_config cfg;
        pjsua_transport_config_default(&cfg);
        cfg.port = 0;//any available source port
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
    initVideoDevicesList();

    listAudioCodecs();
    listVideoCodecs();

    disableTcpSwitch(false);

    qInfo() << "Init PJSUA library";
    return true;
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
    qCritical() << fullError;
    emit errorMessage(fullError);
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

bool SipClient::disableTcpSwitch(bool value)
{
    pjsip_cfg_t *cfg = pjsip_cfg();
    bool rc{};
    if (nullptr != cfg) {
        cfg->endpt.disable_tcp_switch = value ? PJ_TRUE : PJ_FALSE;
        rc = true;
        qDebug() << "Disable TCP switch" << value;
    } else {
        qWarning() << "Cannot get PJSIP config";
    }
    return rc;
}
