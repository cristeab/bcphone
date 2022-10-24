#pragma once

#include "pjsua.h"
#include <QObject>
#include <QTimer>
#include <unordered_map>

class Settings;
class AudioDevices;
class VideoDevices;
class AudioCodecs;
class VideoCodecs;
class RingTonesModel;
class CallHistoryModel;
class ActiveCallModel;

class SipClient : public QObject
{
    Q_OBJECT
public:
    enum class RegistrationStatus { Unregistered, Trying, InProgress, Registered,
                                    ServiceUnavailable, TemporarilyUnavailable };

    SipClient(Settings *settings,
              const RingTonesModel* ringTonesModel,
              CallHistoryModel* callHistoryModel,
              ActiveCallModel* activeCallModel,
              QObject *parent = nullptr);

    const AudioDevices* inputAudioDevices() const {
        return _inputAudioDevices;
    }
    const AudioDevices* outputAudioDevices() const {
        return _outputAudioDevices;
    }
    const VideoDevices* videoDevices() const {
        return _videoDevices;
    }
    const AudioCodecs* audioCodecs() const {
        return _audioCodecs;
    }
    const VideoCodecs* videoCodecs() const {
        return _videoCodecs;
    }

    bool init();
    void release();

    bool registerAccount();
    void manuallyRegister();
    bool unregisterAccount();

    bool makeCall(const QString &userId);
    bool sendDtmf(const QString &dtmf);

    bool answer(int callId);
    bool hangup(int callId);
    void hangupAll();

    bool hold(int callId);
    bool unhold(int callId);

    bool unsupervisedTransfer(const QString &phoneNumber, const QString& userName);
    //TODO: add supervised transfer
    bool swap(int callId);
    bool merge(int callId);

    bool playDigit(const QString& digit);

    bool mute(bool start, int callId);
    bool record(bool start, int callId) {
        auto cid = static_cast<pjsua_call_id>(callId);
        return start ? startRecording(cid) : stopRecording(cid);
    }

    bool setupConferenceCall(pjsua_call_id callId);

signals:
    void errorMessage(const QString& msg);
    void registrationStatusChanged();

private:
    enum { MAX_CODECS = 32, MAX_PRIORITY = 255, DEFAULT_BITRATE_KBPS = 256,
           DEFAULT_LOG_LEVEL = 10, DEFAULT_CONSOLE_LOG_LEVEL = 10,
           MAX_ERROR_MSG_SIZE = 1024, SIP_URI_SIZE = 900,
           PJSUA_POOL_SIZE = 512, TONE_GEN_CLOCK_RATE_HZ = 8000,
           TONE_GEN_CHANNEL_COUNT = 1, TONE_GEN_SAMPLES_PER_FRAME = 64,
           TONE_GEN_BITS_PER_SAMPLE = 16,
           TONE_GEN_ON_MS = 160, TONE_GEN_OFF_MS = 50, TONE_GEN_TIMEOUT_MS = 5000 };

    static void onRegState(pjsua_acc_id acc_id);
    static void onIncomingCall(pjsua_acc_id acc_id, pjsua_call_id call_id,
                               pjsip_rx_data *rdata);
    static void onCallState(pjsua_call_id call_id, pjsip_event *e);
    static void onCallMediaState(pjsua_call_id call_id);
    static void onStreamCreated(pjsua_call_id call_id, pjmedia_stream *strm,
                                unsigned stream_idx, pjmedia_port **p_port);
    static void onStreamDestroyed(pjsua_call_id call_id, pjmedia_stream *strm,
                                  unsigned stream_idx);
    static void onBuddyState(pjsua_buddy_id buddy_id);
    static void pjsuaLogCallback(int level, const char *data, int len);

    void errorHandler(const QString &title, pj_status_t status = PJ_SUCCESS);
    static QString toString(const pj_str_t& pjStr) {
        return QString::fromLocal8Bit(pjStr.ptr, static_cast<int>(pjStr.slen));
    }

    bool disableAudio();
    void initAudioDevicesList();
    void initVideoDevicesList();
    void listAudioCodecs();
    void listVideoCodecs();
    bool setAudioCodecPriority(const QString &codecId, int priority);
    bool setVideoCodecBitrate(const QString &codecId, int bitrate);

    static bool disableTcpSwitch(bool value);
    static QString sipTransport(int type);
    static pj_status_t verifySipUri(const char *url) {
        return (strlen(url) > SIP_URI_SIZE) ? PJSIP_EURITOOLONG : pjsua_verify_sip_url(url);
    }
    bool callUri(pj_str_t *uri, const QString &userId, std::string &uriBuffer);
    bool enableAudio();

    bool initRingTonePlayer(pjsua_call_id id, bool incoming);
    bool startPlayingRingTone(pjsua_call_id id, bool incoming);
    void stopPlayingRingTone(pjsua_call_id id);
    bool releaseRingTonePlayer(pjsua_call_id id);
    void releaseRingTonePlayers();

    bool initToneGenerator();
    void releaseToneGenerator();

    bool createRecorder(pjsua_call_id callId);
    bool startRecording(pjsua_call_id callId);
    bool stopRecording(pjsua_call_id callId);
    bool releaseRecorder(pjsua_call_id callId);

    void setRegistrationStatus(const pjsua_acc_info &info);

    Settings* _settings = nullptr;
    AudioDevices* _inputAudioDevices = nullptr;
    AudioDevices* _outputAudioDevices = nullptr;
    VideoDevices* _videoDevices = nullptr;
    AudioCodecs* _audioCodecs = nullptr;
    VideoCodecs* _videoCodecs = nullptr;
    const RingTonesModel* _ringTonesModel = nullptr;
    CallHistoryModel* _callHistoryModel = nullptr;
    ActiveCallModel* _activeCallModel = nullptr;

    pjsua_acc_id _accId = PJSUA_INVALID_ID;
    std::unordered_map<pjsua_call_id, pjsua_player_id> _playerId;
    std::unordered_map<pjsua_call_id, pjsua_recorder_id> _recorderId;

    pj_pool_t* _toneGenPool = nullptr;
    pjmedia_port* _toneGenMediaPort = nullptr;
    pjsua_conf_port_id _toneGenConfPort = PJSUA_INVALID_ID;
    QTimer _toneGenTimer;

    RegistrationStatus _registrationStatus = RegistrationStatus::Unregistered;
    QString _registrationStatusText;
};
