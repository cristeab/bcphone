#pragma once

#include "config.h"
#include "qmlhelpers.h"
#include "models/audio_devices.h"
#include "models/video_devices.h"
#include "models/call_history_model.h"
#include "models/contacts_model.h"
#include "models/generic_codecs.h"
#include "models/presence_model.h"
#include <QObject>
#include <QString>

class Settings : public QObject {
    Q_OBJECT
    QML_ANONYMOUS

public:
    enum SipTransport { Udp, Tcp, Tls };
    enum MediaTransport { Rtp, Srtp };

private:
    //enum { StunPortUdpAndTcp = 3478, StunPortTls = 5349 };
    enum { SIP_PORT =  5060, PROXY_PORT = 5096,
           INVALID_INDEX = -1,
           INBOUND_RING_TONE_INDEX = 0, OUTBOUND_RING_TONE_INDEX = 1,
           TRANSPORT_DEFAULT_PORT = 0 };
    static constexpr double DIALPAD_SOUND_VOLUME = 0.75;
    static constexpr double MICROPHONE_VOLUME = 1.0;
    static constexpr double SPEAKERS_VOLUME = 1.0;
    static constexpr bool PROXY_ENABLED = false;
    static constexpr bool ENABLE_SIP_LOG = true;
    static constexpr bool ENABLE_VAD = true;
    static constexpr bool DISABLE_TCP_SWITCH = false;

    QML_CONSTANT_PROPERTY(QString, orgName, ORG_NAME)
    QML_CONSTANT_PROPERTY(QString, appName, APP_NAME)
    QML_CONSTANT_PROPERTY(QString, appVersion, APP_VERSION)

    QML_WRITABLE_PROPERTY(QString, sipServer, setSipServer, "")
    QML_WRITABLE_PROPERTY_POD(int, sipPort, setSipPort, SIP_PORT)
    QML_WRITABLE_PROPERTY(QString, userName, setUserName, "")
    QML_WRITABLE_PROPERTY(QString, password, setPassword, "")
    QML_WRITABLE_PROPERTY(QString, displayName, setDisplayName, "")
    QML_WRITABLE_PROPERTY(QString, authUserName, setAuthUserName, "")
    QML_WRITABLE_PROPERTY_POD(int, sipTransport, setSipTransport, SipTransport::Udp)
    QML_WRITABLE_PROPERTY_POD(int, mediaTransport, setMediaTransport, MediaTransport::Rtp)

    QML_WRITABLE_PROPERTY_POD(int, inputAudioModelIndex, setInputAudioModelIndex, INVALID_INDEX)
    QML_WRITABLE_PROPERTY_POD(int, outputAudioModelIndex, setOutputAudioModelIndex, INVALID_INDEX)
    QML_WRITABLE_PROPERTY_POD(int, videoModelIndex, setVideoModelIndex, INVALID_INDEX)

    QML_WRITABLE_PROPERTY_POD(int, inboundRingTonesModelIndex, setInboundRingTonesModelIndex, INBOUND_RING_TONE_INDEX)
    QML_WRITABLE_PROPERTY_POD(int, outboundRingTonesModelIndex, setOutboundRingTonesModelIndex, OUTBOUND_RING_TONE_INDEX)

    QML_WRITABLE_PROPERTY_FLOAT(qreal, microphoneVolume, setMicrophoneVolume, MICROPHONE_VOLUME)
    QML_WRITABLE_PROPERTY_FLOAT(qreal, speakersVolume, setSpeakersVolume, SPEAKERS_VOLUME)
    QML_WRITABLE_PROPERTY_FLOAT(qreal, dialpadSoundVolume, setDialpadSoundVolume, DIALPAD_SOUND_VOLUME)

    QML_WRITABLE_PROPERTY(QString, recPath, setRecPath, "")

    //QML_WRITABLE_PROPERTY(QString, stunServer, setStunServer, "stun.zoiper.com")
    //QML_WRITABLE_PROPERTY(int, stunPort, setStunPort, 3478)

    QML_WRITABLE_PROPERTY_POD(bool, proxyEnabled, setProxyEnabled, PROXY_ENABLED)
    QML_WRITABLE_PROPERTY(QString, proxyServer, setProxyServer, "")
    QML_WRITABLE_PROPERTY_POD(int, proxyPort, setProxyPort, PROXY_PORT)

    QML_WRITABLE_PROPERTY_POD(bool, enableSipLog, setEnableSipLog, ENABLE_SIP_LOG)
    QML_WRITABLE_PROPERTY_POD(bool, enableVad, setEnableVad, ENABLE_VAD)
    QML_WRITABLE_PROPERTY_POD(uint32_t, transportSourcePort, setTransportSourcePort, TRANSPORT_DEFAULT_PORT)
    QML_WRITABLE_PROPERTY_POD(bool, disableTcpSwitch, setDisableTcpSwitch, DISABLE_TCP_SWITCH)

    QML_WRITABLE_PROPERTY_POD(bool, allowSdpNatRewrite, setAllowSdpNatRewrite, true)
    QML_WRITABLE_PROPERTY_POD(bool, allowContactAndViaRewrite, setAllowContactAndViaRewrite, true)
    QML_WRITABLE_PROPERTY_POD(bool, publishEnabled, setPublishEnabled, true)

public:

    explicit Settings(QObject *parent = nullptr);

    static const QString& writablePath();

    bool canRegister() const {
        return !_sipServer.isEmpty() && !_userName.isEmpty() && !_password.isEmpty();
    }

    static AudioDevices::DeviceInfo inputAudioDeviceInfo();
    static void saveInputAudioDeviceInfo(const AudioDevices::DeviceInfo &devInfo);

    static AudioDevices::DeviceInfo outputAudioDeviceInfo();
    static void saveOutputAudioDeviceInfo(const AudioDevices::DeviceInfo &devInfo);

    static VideoDevices::DeviceInfo videoDeviceInfo();
    static void saveVideoDeviceInfo(const VideoDevices::DeviceInfo &devInfo);

    static QVector<CallHistoryModel::CallHistoryInfo> callHistoryInfo();
    static void saveCallHistoryInfo(const QVector<CallHistoryModel::CallHistoryInfo> &historyInfo);

    static QVector<ContactsModel::ContactInfo> contactsInfo();
    static void saveContactsInfo(const QVector<ContactsModel::ContactInfo> &contactsInfo);

    static QList<GenericCodecs::CodecInfo> audioCodecInfo();
    static void saveAudioCodecInfo(const QList<GenericCodecs::CodecInfo> &codecInfo);

    static QList<GenericCodecs::CodecInfo> videoCodecInfo();
    static void saveVideoCodecInfo(const QList<GenericCodecs::CodecInfo> &codecInfo);

    Q_INVOKABLE void save();
    Q_INVOKABLE void clear();
    static void uninstallClear();

private:
    Q_DISABLE_COPY_MOVE(Settings)
    void load();
};
