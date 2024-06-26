#pragma once

#include "pjsua.h"
#include <QTimer>
#include <QPointer>
#include <unordered_map>

class Softphone;
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

    ~SipClient() {
        release();
    }

    static SipClient* create(Softphone *softphone);
    bool init();
    void release();

    bool registerAccount();
    void manuallyRegister();
    bool unregisterAccount();

    bool makeCall(const QString &userId);
    bool sendDtmf(const QString &dtmf);

    bool answer(int callId, int statusCode = PJSIP_SC_OK);
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

#ifdef ENABLE_VIDEO
    bool setVideoCodecPriority(const QString &codecId, int priority);
    void releaseVideoWindow();
#endif

    int addBuddy(const QString &userId);
    bool removeBuddy(int buddyId);

    bool sendText(const QString& userId, const QString& txt);
    bool sendTyping(const QString& userId, bool isTyping);

signals:
    void errorMessage(const QString& msg);
    void registrationStatusChanged(RegistrationStatus registrationStatus, const QString& registrationStatusText);
    void incoming(int callId, const QString& userName, const QString& userId);
    void calling(int callId, const QString& userName, const QString& userId);
    void confirmed(int callId);
    void disconnected(int callId);
    void buddyStatusChanged(int buddyId, const QString& status);
    // private signals
    void registrationStatusReady(pjsua_acc_info accInfo);
    void incomingCallReady(pjsua_call_id callId, pjsua_call_info callInfo);
    void callStateReady(pjsua_call_id callId, pjsua_call_info callInfo);
    void callMediaStateReady(pjsua_call_id callId, pjsua_call_info callInfo);
    void streamStatsReady(pjmedia_rtcp_stat stat);
    void buddyStateReady(pjsua_buddy_id buddyId);

private:
    SipClient(QObject *parent);
    Q_DISABLE_COPY_MOVE(SipClient)

    enum { MAX_CODECS = 32, MAX_PRIORITY = 255, DEFAULT_BITRATE_KBPS = 256,
	   DEFAULT_LOG_LEVEL = 6, DEFAULT_CONSOLE_LOG_LEVEL = 6,
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

    static void onPager(pjsua_call_id callId, const pj_str_t *from, const pj_str_t *to,
			const pj_str_t *contact, const pj_str_t *mimeType, const pj_str_t *body);
    static void onPagerStatus(pjsua_call_id callId, const pj_str_t *to, const pj_str_t *body,
			void *user_data, pjsip_status_code status, const pj_str_t *reason);
    static void onTyping(pjsua_call_id callId, const pj_str_t *from, const pj_str_t *to,
			const pj_str_t *contact, pj_bool_t isTyping);

    static void pjsuaLogCallback(int level, const char *data, int len);

    bool callUri(pj_str_t *uri, const QString &userId, std::string &uriBuffer);
    void processRegistrationStatus(pjsua_acc_info accInfo);
    void processIncomingCall(pjsua_call_id callId, pjsua_call_info callInfo);
    void processCallState(pjsua_call_id callId, pjsua_call_info callInfo);
    void processCallMediaState(pjsua_call_id callId, pjsua_call_info callInfo);
    void dumpStreamStats(pjmedia_rtcp_stat stat);
    void processBuddyState(pjsua_buddy_id buddyId);

    static QString formatErrorMessage(const QString &title, pj_status_t status = PJ_SUCCESS);
    void errorHandler(const QString &title, pj_status_t status = PJ_SUCCESS) {
        auto const &msg = formatErrorMessage(title, status);
        emit errorMessage(msg);
    }
    void listAudioCodecs();
#ifdef ENABLE_VIDEO
    void initVideoDevicesList();
    void listVideoCodecs();
    bool setVideoCodecBitrate(const QString &codecId, int bitrate);
#endif

    bool disableTcpSwitch();
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

#ifdef ENABLE_VIDEO
    void manageVideo(bool enable);
    void initVideoWindow();
    void initPreviewWindow();
    void releasePreviewWindow();
    void setVideoWindowSize(pj_bool_t isNative, pjsua_vid_win_id wid, int width, int height);
#endif

    QPointer<Settings> _settings;
    QPointer<AudioDevices> _inputAudioDevices;
    QPointer<AudioDevices> _outputAudioDevices;
    QPointer<VideoDevices> _videoDevices;
    QPointer<AudioCodecs> _audioCodecs;
    QPointer<VideoCodecs> _videoCodecs;
    QPointer<RingTonesModel> _ringTonesModel;
    QPointer<CallHistoryModel> _callHistoryModel;
    QPointer<ActiveCallModel> _activeCallModel;

    QString _serverDomain;
    pjsua_acc_id _accId = PJSUA_INVALID_ID;
    std::unordered_map<pjsua_call_id, pjsua_player_id> _playerId;
    std::unordered_map<pjsua_call_id, pjsua_recorder_id> _recorderId;

    pj_pool_t* _toneGenPool = nullptr;
    pjmedia_port* _toneGenMediaPort = nullptr;
    pjsua_conf_port_id _toneGenConfPort = PJSUA_INVALID_ID;
    QTimer _toneGenTimer;

#ifdef ENABLE_VIDEO
    QPointer<QWidget> _previewWindow;
    QPointer<QWidget> _videoWindow;
#endif
};
