#pragma once

#include "pjsua.h"
#include <QTimer>
#include <QPointer>
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

    AudioDevices* inputAudioDevices() const {
        return _inputAudioDevices;
    }
    AudioDevices* outputAudioDevices() const {
        return _outputAudioDevices;
    }
    VideoDevices* videoDevices() const {
        return _videoDevices;
    }
    AudioCodecs* audioCodecs() const {
        return _audioCodecs;
    }
    VideoCodecs* videoCodecs() const {
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

    bool enableAudio();
    bool disableAudio();
    bool setAudioCodecPriority(const QString &codecId, int priority);
    void initAudioDevicesList();

    bool setMicrophoneVolume(pjsua_call_id callId, bool mute = false);
    bool setSpeakersVolume(pjsua_call_id callId, bool mute = false);

    bool startPlayingRingTone(pjsua_call_id id, bool incoming);
    void stopPlayingRingTone(pjsua_call_id id);

    bool setVideoCodecPriority(const QString &codecId, int priority);
    void releaseVideoWindow();

    static bool callUri(pj_str_t *uri, const QString &userId, std::string &uriBuffer);

signals:
    void errorMessage(const QString& msg);
    void registrationStatusChanged(RegistrationStatus registrationStatus, const QString& registrationStatusText);
    void incoming(int callId, const QString& userName, const QString& userId);
    void calling(int callId, const QString& userName, const QString& userId);
    void confirmed(int callId);
    void disconnected(int callId);
    void buddyStatusChanged(int buddyId, const QString& status);

private:
    Q_DISABLE_COPY_MOVE(SipClient)

    enum { MAX_CODECS = 32, MAX_PRIORITY = 255, DEFAULT_BITRATE_KBPS = 256,
           DEFAULT_LOG_LEVEL = 10, DEFAULT_CONSOLE_LOG_LEVEL = 10,
           MAX_ERROR_MSG_SIZE = 1024, SIP_URI_SIZE = 900,
           PJSUA_POOL_SIZE = 512, TONE_GEN_CLOCK_RATE_HZ = 8000,
           TONE_GEN_CHANNEL_COUNT = 1, TONE_GEN_SAMPLES_PER_FRAME = 64,
           TONE_GEN_BITS_PER_SAMPLE = 16,
           TONE_GEN_ON_MS = 160, TONE_GEN_OFF_MS = 50, TONE_GEN_TIMEOUT_MS = 5000 };

    static void onRegState(pjsua_acc_id accId);
    static void onIncomingCall(pjsua_acc_id accId, pjsua_call_id callId, pjsip_rx_data *rdata);
    static void onCallState(pjsua_call_id callId, pjsip_event *e);
    static void onCallMediaState(pjsua_call_id callId);
    static void onStreamCreated(pjsua_call_id callId, pjmedia_stream *strm,
                                unsigned streamIdx, pjmedia_port **pPort);
    static void onStreamDestroyed(pjsua_call_id callId, pjmedia_stream *strm,
                                  unsigned streamIdx);
    static void onBuddyState(pjsua_buddy_id buddyId);
    static void pjsuaLogCallback(int level, const char *data, int len);

    void processRegistrationStatus(const pjsua_acc_info &info);
    void processIncomingCall(pjsua_call_id callId, const pjsua_call_info &info);
    void processCallState(pjsua_call_id callId, const pjsua_call_info &info);
    void processCallMediaState(pjsua_call_id callId, const pjsua_call_info &info);
    void dumpStreamStats(pjmedia_stream *strm);
    void processBuddyState(pjsua_buddy_id buddyId);

    void errorHandler(const QString &title, pj_status_t status = PJ_SUCCESS);
    void initVideoDevicesList();
    void listAudioCodecs();
    void listVideoCodecs();
    bool setVideoCodecBitrate(const QString &codecId, int bitrate);

    static bool disableTcpSwitch(bool value);
    static QString sipTransport(int type);
    static pj_status_t verifySipUri(const char *url) {
        return (strlen(url) > SIP_URI_SIZE) ? PJSIP_EURITOOLONG : pjsua_verify_sip_url(url);
    }
    static QString toString(const pj_str_t& pjStr) {
        return QString::fromLocal8Bit(pjStr.ptr, static_cast<int>(pjStr.slen));
    }
    static void extractUserNameAndId(QString& userName, QString& userId, const QString &info);
    static bool isMediaActive(const pjsua_call_media_info &media) {
        return ((PJMEDIA_TYPE_AUDIO == media.type) ||
                (PJMEDIA_TYPE_VIDEO == media.type)) &&
                (PJSUA_CALL_MEDIA_NONE != media.status) &&
                (PJSUA_CALL_MEDIA_ERROR != media.status);
    }

    static pjsua_conf_port_id callConfPort(pjsua_call_id callId);

    bool initRingTonePlayer(pjsua_call_id id, bool incoming);
    bool releaseRingTonePlayer(pjsua_call_id id);
    void releaseRingTonePlayers();

    bool initToneGenerator();
    void releaseToneGenerator();

    bool createRecorder(pjsua_call_id callId);
    bool startRecording(pjsua_call_id callId);
    bool stopRecording(pjsua_call_id callId);
    bool releaseRecorder(pjsua_call_id callId);

    void connectCallToSoundDevices(pjsua_conf_port_id confPortId);

    void manageVideo(bool enable);
    void initVideoWindow();
    void initPreviewWindow();
    void releasePreviewWindow();
    void setVideoWindowSize(pj_bool_t isNative, pjsua_vid_win_id wid, int width, int height);

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

    QPointer<QWidget> _previewWindow;
    QPointer<QWidget> _videoWindow;
};
