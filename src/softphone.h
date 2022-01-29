#pragma once

#include "qmlhelpers.h"
#include "settings.h"
#include "models/audio_devices.h"
#include "models/ring_tones_model.h"
#include "models/video_devices.h"
#include "models/contacts_model.h"
#include "models/call_history_model.h"
#include "models/audio_codecs.h"
#include "models/video_codecs.h"
#include "models/active_call_model.h"
#include "models/presence_model.h"
#include "models/playback_model.h"
#include "pjsua.h"
#include <QObject>
#include <QTimer>
#include <QString>
#include <QMap>

class Softphone : public QObject {
	Q_OBJECT

public:
    enum CallState { NONE, OUTGOING, INCOMING, ACTIVE };
    Q_ENUM(CallState)
    enum RegistrationStatus { UNREGISTERED, IN_PROGRESS, REGISTERED };
    Q_ENUM(RegistrationStatus)

private:
    QML_READABLE_PROPERTY(RegistrationStatus, registrationStatus, setRegistrationStatus, RegistrationStatus::UNREGISTERED)
    QML_READABLE_PROPERTY(QString, registrationText, setRegistrationText, "")

    QML_READABLE_PROPERTY(bool, showBusy, setShowBusy, false)
    QML_READABLE_PROPERTY(bool, loggedOut, setLoggedOut, false)

    QML_WRITABLE_PROPERTY(QString, dialogMessage, setDialogMessage, "")
    QML_READABLE_PROPERTY(bool, dialogError, setDialogError, false)
    QML_WRITABLE_PROPERTY(bool, dialogRetry, setDialogRetry, false)

    QML_WRITABLE_PROPERTY(QString, dialedText, setDialedText, "")
    QML_WRITABLE_PROPERTY(QString, currentUserId, setCurrentUserId, "")
    QML_WRITABLE_PROPERTY(bool, activeCall, setActiveCall, false)
    QML_READABLE_PROPERTY(bool, confirmedCall, setConfirmedCall, false)

    QML_WRITABLE_PROPERTY(bool, muteMicrophone, setMuteMicrophone, true)
    QML_WRITABLE_PROPERTY(bool, record, setRecord, false)
    QML_WRITABLE_PROPERTY(bool, holdCall, setHoldCall, false)

    QML_READABLE_PROPERTY(bool, hasVideo, setHasVideo, false)
    QML_WRITABLE_PROPERTY(bool, enableVideo, setEnableVideo, false)

    QML_WRITABLE_PROPERTY(int, winPosX, setWinPosX, 0)
    QML_WRITABLE_PROPERTY(int, winPosY, setWinPosY, 0)
    QML_WRITABLE_PROPERTY(int, winWidth, setWinWidth, 0)

    QML_CONSTANT_PROPERTY_PTR(Settings, settings)
    QML_CONSTANT_PROPERTY_PTR(AudioDevices, inputAudioDevices)
    QML_CONSTANT_PROPERTY_PTR(AudioDevices, outputAudioDevices)
    QML_CONSTANT_PROPERTY_PTR(RingTonesModel, ringTonesModel)
    QML_CONSTANT_PROPERTY_PTR(VideoDevices, videoDevices)
    QML_CONSTANT_PROPERTY_PTR(AudioCodecs, audioCodecs)
    QML_CONSTANT_PROPERTY_PTR(VideoCodecs, videoCodecs)
    QML_CONSTANT_PROPERTY_PTR(ContactsModel, contactsModel)
    QML_CONSTANT_PROPERTY_PTR(CallHistoryModel, callHistoryModel)
    QML_CONSTANT_PROPERTY_PTR(ActiveCallModel, activeCallModel)
    QML_CONSTANT_PROPERTY_PTR(PresenceModel, presenceModel)
    QML_CONSTANT_PROPERTY_PTR(PlaybackModel, playbackModel)

    QML_WRITABLE_PROPERTY(quint32, currentUserElapsedSec, setCurrentUserElapsedSec, 0)

    QML_CONSTANT_PROPERTY(int, invalidCallId, PJSUA_INVALID_ID)

    QML_WRITABLE_PROPERTY(bool, blindTransfer, setBlindTransfer, false)
    QML_WRITABLE_PROPERTY(QString, blindTransferUserName, setBlindTransferUserName, "")

    QML_WRITABLE_PROPERTY(bool, conference, setConference, false)

public:
    Softphone();
    ~Softphone();

    void setMainForm(QObject *mainForm) { _mainForm = mainForm; }

    Q_INVOKABLE bool registerAccount();
    Q_INVOKABLE bool makeCall(const QString &userId);
    Q_INVOKABLE bool answer(int callId);
    Q_INVOKABLE bool hangup(int callId);
    Q_INVOKABLE void hangupAll();
    Q_INVOKABLE bool unsupervisedTransfer(const QString &phoneNumber);
    Q_INVOKABLE bool holdAndAnswer(int callId);
    Q_INVOKABLE bool swap(int callId);
    Q_INVOKABLE bool merge(int callId);
    Q_INVOKABLE bool sendDtmf(const QString &dtmf);
    Q_INVOKABLE void manuallyRegister();
    Q_INVOKABLE bool playDigit(const QString& digit);

    bool hold(bool value, int callId);
    bool mute(bool value, int callId);
    bool rec(bool value, int callId);
    QString convertNumber(const QString &num);
    bool registered() const { return RegistrationStatus::REGISTERED == _registrationStatus; }
    void release();

    static bool callUri(pj_str_t *uri, const QString &userId, std::string &uriBuffer);
    static void errorHandler(const QString &title, pj_status_t status = PJ_SUCCESS,
                             bool emitSignal = false);

signals:
    void disconnected(int callId);
    void confirmed(int callId);
    void calling(int callId, const QString &userId, const QString &userName);
    void incoming(int callCount, int callId, const QString &userId,
                  const QString &userName, bool isConf);
    void phoneStateChanged();
    void audioDevicesChanged();

private:
    Q_DISABLE_COPY_MOVE(Softphone)

    enum PhoneState { UNKNOWN = 0x00, PARKING_CALL = 0x01, RECORDING_CALL = 0x02,
                            MERGE_CALLS = 0x04 };
    enum SipErrorCodes { BadRequest = 400, RequestTimeout = 408, RequestTerminated = 487 };
    enum { ERROR_COUNT_MAX = 3, DEFAULT_BITRATE_KBPS = 256 };
    enum { TONE_GENERATOR_TIMEOUT_MS = 5000, TONE_GENERATOR_ON_MS = 160, TONE_GENERATOR_OFF_MS = 50 };

    bool init();
    bool unregisterAccount();
    void initAudioDevicesList();
    void listAudioCodecs();
    void initVideoDevicesList();
    void listVideoCodecs();

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
    static pj_status_t onFileEnd(pjmedia_port* media_port, void* args);

    void onConfirmed(int callId);
    void onCalling(int callId, const QString &userId, const QString &userName);
    void onIncoming(int callCount, int callId, const QString &userId,
                    const QString &userName);
    void onDisconnected(int callId);

    bool initRingTonePlayer(pjsua_call_id id, bool incoming);
    bool startPlayingRingTone(pjsua_call_id id, bool incoming);
    void stopPlayingRingTone(pjsua_call_id id);
    bool releaseRingTonePlayer(pjsua_call_id id);
    void releaseRingTonePlayers();

    bool createRecorder(pjsua_call_id callId);
    bool startRecording(pjsua_call_id callId);
    bool stopRecording(pjsua_call_id callId);
    bool releaseRecorder(pjsua_call_id callId);

    bool createPlaybackPlayer(pjsua_call_id callId);
    bool startPlayback(pjsua_call_id callId);
    bool stopPlayback(pjsua_call_id callId);
    bool releasePlaybackPlayer(pjsua_call_id callId);

    static void dumpStreamStats(pjmedia_stream *strm);
    bool setMicrophoneVolume(pjsua_conf_port_id portId, bool mute = false);
    bool setSpeakersVolume(pjsua_conf_port_id portId, bool mute = false);
    static bool disableTcpSwitch(bool value = false);
    void connectCallToSoundDevices(pjsua_conf_port_id confPortId);
    void setupConferenceCall(pjsua_call_id callId);
    void startCurrentUserTimer();
    void stopCurrentUserTimer();
    static std::tuple<QString,QString> extractUserNameAndId(const QString &info);

    bool setAudioCodecPriority(const QString &codecId, int priority);
    bool setVideoCodecPriority(const QString &codecId, int priority);
    bool setVideoCodecBitrate(const QString &codecId, int bitrate);

    void onMicrophoneVolumeChanged();
    void onSpeakersVolumeChanged();

    bool recordingCall() const {
        return PhoneState::RECORDING_CALL == (_phoneState & PhoneState::RECORDING_CALL);
    }
    void setRecordingCall(bool on) {
        if (on) {
            _phoneState |= PhoneState::RECORDING_CALL;
        } else {
            _phoneState &= !PhoneState::RECORDING_CALL;
        }
        emit phoneStateChanged();
    }

    void raiseWindow();

    static QString toString(const pj_str_t &pjStr) {
        return QString::fromLocal8Bit(pjStr.ptr, static_cast<int>(pjStr.slen));
    }
    static bool isMediaActive(const pjsua_call_media_info &media) {
        return ((PJMEDIA_TYPE_AUDIO == media.type) ||
                (PJMEDIA_TYPE_VIDEO == media.type)) &&
                (PJSUA_CALL_MEDIA_NONE != media.status) &&
                (PJSUA_CALL_MEDIA_ERROR != media.status);
    }

    void onEnableVideo();

    void initVideoWindow();
    void releaseVideoWindow();
    void initPreviewWindow();
    void releasePreviewWindow();
    void setVideoWindowSize(pj_bool_t isNative, pjsua_vid_win_id wid, int width, int height);

    static pj_status_t verifySipUri(const char *url) {
        return (strlen(url) > 900) ? PJSIP_EURITOOLONG : pjsua_verify_sip_url(url);
    }

    bool enableAudio();
    bool disableAudio(bool force = false);

    bool initToneGenerator();
    void releaseToneGenerator();

    static QString sipTransport(int type);

    void errorDialog(const QString& msg) {
        setDialogError(true);
        setDialogMessage(msg);
    }

    QObject *_mainForm = nullptr;
    QTimer _currentUserTimer;
    static const QString _notAvailable;
    bool _pjsuaStarted = false;
    pjsua_acc_id _accId = PJSUA_INVALID_ID;
    QHash<pjsua_call_id, pjsua_player_id> _playerId;
    QHash<pjsua_call_id, pjsua_recorder_id> _recId;
    QHash<pjsua_call_id, pjsua_player_id> _playbackPlayerId;
    uint8_t _phoneState = PhoneState::UNKNOWN;
    bool _manualHangup = false;
    bool _audioEnabled = false;
    pj_pool_t* _toneGenPool = nullptr;
    pjmedia_port* _toneGenMediaPort = nullptr;
    pjsua_conf_port_id _toneGenConfPort = PJSUA_INVALID_ID;
    QTimer _toneGenTimer;
    std::unique_ptr<QWidget> _previewWindow;
    std::unique_ptr<QWidget> _videoWindow;
};
