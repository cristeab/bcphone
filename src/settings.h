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

public:
    enum SipTransport { Udp, Tcp, Tls };
    enum MediaTransport { Rtp, Srtp };
    //enum { StunPortUdpAndTcp = 3478, StunPortTls = 5349 };

private:
    QML_CONSTANT_PROPERTY(QString, orgName, ORG_NAME)
    QML_CONSTANT_PROPERTY(QString, appName, APP_NAME)
    QML_CONSTANT_PROPERTY(QString, appVersion, APP_VERSION)

    QML_WRITABLE_PROPERTY(QString, sipServer, setSipServer, "sip.ringcentral.com")
    QML_WRITABLE_PROPERTY(int, sipPort, setSipPort, 5060)
    QML_WRITABLE_PROPERTY(QString, userName, setUserName, "")
    QML_WRITABLE_PROPERTY(QString, password, setPassword, "")
    QML_WRITABLE_PROPERTY(QString, accountName, setAccountName, "")
    QML_WRITABLE_PROPERTY(int, sipTransport, setSipTransport, SipTransport::Tls)
    QML_WRITABLE_PROPERTY(int, mediaTransport, setMediaTransport, MediaTransport::Srtp)

    QML_WRITABLE_PROPERTY(int, inputAudioModelIndex, setInputAudioModelIndex, -1)
    QML_WRITABLE_PROPERTY(int, outputAudioModelIndex, setOutputAudioModelIndex, -1)
    QML_WRITABLE_PROPERTY(int, videoModelIndex, setVideoModelIndex, -1)

    QML_WRITABLE_PROPERTY(int, inboundRingTonesModelIndex, setInboundRingTonesModelIndex, 0)
    QML_WRITABLE_PROPERTY(int, outboundRingTonesModelIndex, setOutboundRingTonesModelIndex, 1)

    QML_WRITABLE_PROPERTY_FLOAT(qreal, microphoneVolume, setMicrophoneVolume, 2.0)
    QML_WRITABLE_PROPERTY_FLOAT(qreal, speakersVolume, setSpeakersVolume, 2.0)
    QML_WRITABLE_PROPERTY_FLOAT(qreal, dialpadSoundVolume, setDialpadSoundVolume, 0.4)

    QML_WRITABLE_PROPERTY(QString, recPath, setRecPath, "")

    //QML_WRITABLE_PROPERTY(QString, stunServer, setStunServer, "stun.zoiper.com")
    //QML_WRITABLE_PROPERTY(int, stunPort, setStunPort, 3478)

    QML_WRITABLE_PROPERTY(QString, authUserName, setAuthUserName, "")
    QML_WRITABLE_PROPERTY(bool, proxyEnabled, setProxyEnabled, false)
    QML_WRITABLE_PROPERTY(QString, proxyServer, setProxyServer, "sip60.ringcentral.com")
    QML_WRITABLE_PROPERTY(int, proxyPort, setProxyPort, 5096)

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

    static QStringList buddyList();
    static void setBuddyList(const QList<PresenceModel::PresenceInfo> &buddies);

    Q_INVOKABLE void save();
    void clear();
    static void uninstallClear();

private:
    Q_DISABLE_COPY_MOVE(Settings)
    void load();
};
