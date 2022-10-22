#pragma once

#include "pjsua.h"
#include <QObject>

class Settings;
class AudioDevices;
class VideoDevices;
class AudioCodecs;
class VideoCodecs;

class SipClient : public QObject
{
    Q_OBJECT
public:
    explicit SipClient(Settings *settings, QObject *parent = nullptr);

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

    bool makeCall(const QString &userId);
    bool sendDtmf(const QString &dtmf);

    bool answer(int callId);
    bool hangup(int callId);

    bool hold(int callId);
    bool unsupervisedTransfer(const QString &phoneNumber);
    //TODO: add supervised transfer
    bool swap(int callId);
    bool merge(int callId);

    bool playDigit(const QString& digit);

    bool mute(bool value, int callId);
    bool record(bool value, int callId);

signals:
    void errorMessage(const QString& msg);

private:
    enum { MAX_CODECS = 32, MAX_PRIORITY = 255, DEFAULT_BITRATE_KBPS = 256 };

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

    Settings* _settings = nullptr;
    AudioDevices* _inputAudioDevices = nullptr;
    AudioDevices* _outputAudioDevices = nullptr;
    VideoDevices* _videoDevices = nullptr;
    AudioCodecs* _audioCodecs = nullptr;
    VideoCodecs* _videoCodecs = nullptr;
};
