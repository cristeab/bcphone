#include "settings.h"
#include "softphone.h"
#include "config.h"
#include <QSettings>
#include <QVector>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QQmlEngine>

#define SIP_SERVER "SIP SERVER"
#define SIP_PORT "SIP PORT"
#define USER_NAME "USER NAME"
#define PASSWORD "PASSWORD"
#define ACCOUNT_NAME "ACCOUNT NAME"
#define SIP_TRANSPORT "SIP TRANSPORT"
#define MEDIA_TRANSPORT "MEDIA TRANSPORT"

#define INPUT_AUDIO_MODEL_NAME "INPUT AUDIO MODEL NAME"
#define INPUT_AUDIO_MODEL_INDEX "INPUT AUDIO MODEL INDEX"

#define OUTPUT_AUDIO_MODEL_NAME "OUTPUT AUDIO MODEL NAME"
#define OUTPUT_AUDIO_MODEL_INDEX "OUTPUT AUDIO MODEL INDEX"

#define VIDEO_MODEL_NAME "VIDEO MODEL NAME"
#define VIDEO_MODEL_INDEX "VIDEO MODEL INDEX"

#define INBOUND_RING_TONES_MODEL_INDEX "INBOUND RING TONES MODEL INDEX"
#define OUTBOUND_RING_TONES_MODEL_INDEX "OUTBOUND RING TONES MODEL INDEX"

#define MICROPHONE_VOLUME "MICROPHONE VOLUME"
#define SPEAKERS_VOLUME "SPEAKERS VOLUME"

#define CALL_HISTORY "CALL HISTORY"
#define CH_CONTACT_ID "CH CONTACT ID"
#define CH_USER_NAME "CH USER NAME"
#define CH_PHONE_NUMBER "CH PHONE NUMBER"
#define CH_DATE_TIME "CH DATE TIME"
#define CH_DATE_TIME_FORMAT "yyyy-MM-dd HH:mm:ss"
#define CH_CALL_STATUS "CH CALL STATUS"

#define CONTACTS_LIST "CONTACTS LIST"
#define CL_CONTACT_ID "CL CONTACT ID"
#define CL_FIRST_NAME "CL FIRST NAME"
#define CL_LAST_NAME "CL LAST NAME"
#define CL_EMAIL "CL EMAIL"
#define CL_PHONE_NUMBER "CL PHONE NUMBER"
#define CL_MOBILE_NUMBER "CL MOBILE NUMBER"
#define CL_ADDRESS "CL ADDRESS"
#define CL_STATE "CL STATE"
#define CL_CITY "CL CITY"
#define CL_ZIP "CL ZIP"
#define CL_COMMENT "CL COMMENT"

#define REC_PATH "REC PATH"

#define AUDIO_CODEC_INFO "audioCodecInfo"
#define AUDIO_CODEC_ID "audioCodecId"
#define AUDIO_CODEC_PRIORITY "audioCodecPriority"

#define VIDEO_CODEC_INFO "videoCodecInfo"
#define VIDEO_CODEC_ID "videoCodecId"
#define VIDEO_CODEC_PRIORITY "videoCodecPriority"

#define BUDDY_LIST "buddyList"
#define BUDDY_USER_ID "buddyUserId"

#define SETTINGS_FOLDER ".BCphone"
#define REC_FOLDER "BCphoneRecordings"

//#define STUN_SERVER "STUN SERVER"
//#define STUN_PORT "STUN PORT"

#define PROXY_AUTH_ID "PROXY AUTH ID"
#define PROXY_ENABLED "PROXY ENABLED"
#define PROXY_SERVER "PROXY SERVER"
#define PROXY_PORT "PROXY PORT"

Settings::Settings(QObject *parent) : QObject (parent)
{
    qmlRegisterInterface<Settings>("Settings", 1);

    load();
}

const QString& Settings::writablePath()
{
    static QString path;
    if (path.isEmpty()) {
        path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)+"/" SETTINGS_FOLDER;
        bool rc = QDir(path).exists();
        if (!rc) {
            rc = QDir().mkpath(path);
            if (!rc) {
                qCritical() << "Cannot create" << path;
                path.clear();
            }
        }
    }
    return path;
}

void Settings::clear()
{
    setSipServer("sip.ringcentral.com");
    setSipPort(5060);
    setUserName("");
    setPassword("");
    setAccountName("");
    setSipTransport(SipTransport::Tls);
    setMediaTransport(MediaTransport::Srtp);

    setInputAudioModelIndex(0);
    setOutputAudioModelIndex(0);
    setInboundRingTonesModelIndex(0);
    setOutboundRingTonesModelIndex(1);
    setVideoModelIndex(0);

    setMicrophoneVolume(1);
    setSpeakersVolume(1);
    setDialpadSoundVolume(0.75);

    setRecPath("");

    //setStunServer("stun.zoiper.com");
    //setStunPort(3478);

    setAuthUserName("");
    setProxyEnabled(false);
    setProxyServer("sip60.ringcentral.com");
    setProxyPort(5096);

    Settings::uninstallClear();
}

void Settings::uninstallClear()
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.clear();
    settings.setFallbacksEnabled(false);
    settings.sync();
    const auto path = writablePath();
    if (!path.isEmpty()) {
        QDir(path).removeRecursively();
    }
}

void Settings::load()
{
    QSettings settings(ORG_NAME, APP_NAME);
    qDebug() << "Reading settings from" << settings.fileName();

    setSipServer(settings.value(SIP_SERVER, _sipServer).toString());
    setSipPort(settings.value(SIP_PORT, _sipPort).toInt());
    setUserName(settings.value(USER_NAME, _userName).toString());
    setPassword(settings.value(PASSWORD, _password).toString());
    setAccountName(settings.value(ACCOUNT_NAME, _accountName).toString());
    setSipTransport(settings.value(SIP_TRANSPORT, _sipTransport).toInt());
    setMediaTransport(settings.value(MEDIA_TRANSPORT, _mediaTransport).toInt());

    //indices for audio and video devices are loaded and saved separately

    setInboundRingTonesModelIndex(settings.value(INBOUND_RING_TONES_MODEL_INDEX,
                                                 _inboundRingTonesModelIndex).toInt());
    setOutboundRingTonesModelIndex(settings.value(OUTBOUND_RING_TONES_MODEL_INDEX,
                                                  _outboundRingTonesModelIndex).toInt());

    setMicrophoneVolume(settings.value(MICROPHONE_VOLUME, _microphoneVolume).toDouble());
    setSpeakersVolume(settings.value(SPEAKERS_VOLUME, _speakersVolume).toDouble());

    setRecPath(settings.value(REC_PATH, _recPath).toString());
    if (_recPath.isEmpty()) {
        setRecPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)+"/" REC_FOLDER);
    }
    if (!QDir(_recPath).exists()) {
        QDir().mkpath(_recPath);
    }

    //setStunServer(settings.value(STUN_SERVER, _stunServer).toString());
    //setStunPort(settings.value(STUN_PORT, _stunPort).toInt());

    setAuthUserName(settings.value(PROXY_AUTH_ID, _authUserName).toString());
    setProxyEnabled(settings.value(PROXY_ENABLED, _proxyEnabled).toBool());
    setProxyServer(settings.value(PROXY_SERVER, _proxyServer).toString());
    setProxyPort(settings.value(PROXY_PORT, _proxyPort).toInt());
}

void Settings::save()
{
    QSettings settings(ORG_NAME, APP_NAME);
    qDebug() << "Save settings to" << settings.fileName();

    settings.setValue(SIP_SERVER, _sipServer);
    settings.setValue(SIP_PORT, _sipPort);
    settings.setValue(USER_NAME, _userName);
    settings.setValue(PASSWORD, _password);
    settings.setValue(ACCOUNT_NAME, _accountName);
    settings.setValue(SIP_TRANSPORT, _sipTransport);
    settings.setValue(MEDIA_TRANSPORT, _mediaTransport);

    settings.setValue(INBOUND_RING_TONES_MODEL_INDEX, _inboundRingTonesModelIndex);
    settings.setValue(OUTBOUND_RING_TONES_MODEL_INDEX, _outboundRingTonesModelIndex);

    settings.setValue(MICROPHONE_VOLUME, _microphoneVolume);
    settings.setValue(SPEAKERS_VOLUME, _speakersVolume);

    settings.setValue(REC_PATH, _recPath);

    //settings.setValue(STUN_SERVER, _stunServer);
    //settings.setValue(STUN_PORT, _stunPort);

    settings.setValue(PROXY_AUTH_ID, _authUserName);
    settings.setValue(PROXY_ENABLED, _proxyEnabled);
    settings.setValue(PROXY_SERVER, _proxyServer);
    settings.setValue(PROXY_PORT, _proxyPort);

    settings.setFallbacksEnabled(false);
    settings.sync();
}

AudioDevices::DeviceInfo Settings::inputAudioDeviceInfo()
{
    QSettings settings(ORG_NAME, APP_NAME);
    return { settings.value(INPUT_AUDIO_MODEL_NAME).toString(),
                settings.value(INPUT_AUDIO_MODEL_INDEX,
                               PJMEDIA_AUD_INVALID_DEV).toInt() };
}

void Settings::saveInputAudioDeviceInfo(const AudioDevices::DeviceInfo &devInfo)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.setValue(INPUT_AUDIO_MODEL_NAME, devInfo.name);
    settings.setValue(INPUT_AUDIO_MODEL_INDEX, devInfo.index);
}

AudioDevices::DeviceInfo Settings::outputAudioDeviceInfo()
{
    QSettings settings(ORG_NAME, APP_NAME);
    return { settings.value(OUTPUT_AUDIO_MODEL_NAME).toString(),
                settings.value(OUTPUT_AUDIO_MODEL_INDEX,
                               PJMEDIA_AUD_INVALID_DEV).toInt() };
}

void Settings::saveOutputAudioDeviceInfo(const AudioDevices::DeviceInfo &devInfo)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.setValue(OUTPUT_AUDIO_MODEL_NAME, devInfo.name);
    settings.setValue(OUTPUT_AUDIO_MODEL_INDEX, devInfo.index);
}

VideoDevices::DeviceInfo Settings::videoDeviceInfo()
{
    QSettings settings(ORG_NAME, APP_NAME);
    return { settings.value(VIDEO_MODEL_NAME).toString(),
                settings.value(VIDEO_MODEL_INDEX,
                               PJMEDIA_VID_INVALID_DEV).toInt() };
}

void Settings::saveVideoDeviceInfo(const VideoDevices::DeviceInfo &devInfo)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.setValue(VIDEO_MODEL_NAME, devInfo.name);
    settings.setValue(VIDEO_MODEL_INDEX, devInfo.index);
}

QVector<CallHistoryModel::CallHistoryInfo> Settings::callHistoryInfo()
{
    QVector<CallHistoryModel::CallHistoryInfo> history;
    QSettings settings(ORG_NAME, APP_NAME);
    const auto size = settings.beginReadArray(CALL_HISTORY);
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        CallHistoryModel::CallHistoryInfo item;
        item.contactId = settings.value(CH_CONTACT_ID).toInt();
        item.userName = settings.value(CH_USER_NAME).toString();
        item.phoneNumber = settings.value(CH_PHONE_NUMBER).toString();
        item.dateTime = QDateTime::fromString(settings.value(CH_DATE_TIME).toString(),
                                              CH_DATE_TIME_FORMAT);
        item.callStatus = static_cast<CallHistoryModel::CallStatus>(settings.value(CH_CALL_STATUS).toInt());
        history.append(item);
    }
    settings.endArray();
    return history;
}

void Settings::saveCallHistoryInfo(const QVector<CallHistoryModel::CallHistoryInfo> &historyInfo)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.beginWriteArray(CALL_HISTORY);
    for (int i = 0; i < historyInfo.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(CH_CONTACT_ID, historyInfo.at(i).contactId);
        settings.setValue(CH_USER_NAME, historyInfo.at(i).userName);
        settings.setValue(CH_PHONE_NUMBER, historyInfo.at(i).phoneNumber);
        settings.setValue(CH_DATE_TIME, historyInfo.at(i).dateTime.toString(CH_DATE_TIME_FORMAT));
        settings.setValue(CH_CALL_STATUS, static_cast<int>(historyInfo.at(i).callStatus));
    }
    settings.endArray();
}

QVector<ContactsModel::ContactInfo> Settings::contactsInfo()
{
    QVector<ContactsModel::ContactInfo> contacts;
    QSettings settings(ORG_NAME, APP_NAME);
    const auto size = settings.beginReadArray(CONTACTS_LIST);
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        ContactsModel::ContactInfo item;
        item.id = settings.value(CL_CONTACT_ID).toInt();
        item.firstName = settings.value(CL_FIRST_NAME).toString();
        item.lastName = settings.value(CL_LAST_NAME).toString();
        item.email = settings.value(CL_EMAIL).toString();
        item.phoneNumber = settings.value(CL_PHONE_NUMBER).toString();
        item.mobileNumber = settings.value(CL_MOBILE_NUMBER).toString();
        item.address = settings.value(CL_ADDRESS).toString();
        item.state = settings.value(CL_STATE).toString();
        item.city = settings.value(CL_CITY).toString();
        item.zip = settings.value(CL_ZIP).toString();
        item.comment = settings.value(CL_COMMENT).toString();
        contacts.append(item);
    }
    settings.endArray();
    return contacts;
}

void Settings::saveContactsInfo(const QVector<ContactsModel::ContactInfo> &contactsInfo)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.beginWriteArray(CONTACTS_LIST);
    for (int i = 0; i < contactsInfo.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(CL_CONTACT_ID, contactsInfo.at(i).id);
        settings.setValue(CL_FIRST_NAME, contactsInfo.at(i).firstName);
        settings.setValue(CL_LAST_NAME, contactsInfo.at(i).lastName);
        settings.setValue(CL_EMAIL, contactsInfo.at(i).email);
        settings.setValue(CL_PHONE_NUMBER, contactsInfo.at(i).phoneNumber);
        settings.setValue(CL_MOBILE_NUMBER, contactsInfo.at(i).mobileNumber);
        settings.setValue(CL_ADDRESS, contactsInfo.at(i).address);
        settings.setValue(CL_STATE, contactsInfo.at(i).state);
        settings.setValue(CL_CITY, contactsInfo.at(i).city);
        settings.setValue(CL_ZIP, contactsInfo.at(i).zip);
        settings.setValue(CL_COMMENT, contactsInfo.at(i).comment);
    }
    settings.endArray();
}

QList<GenericCodecs::CodecInfo> Settings::audioCodecInfo()
{
    QList<GenericCodecs::CodecInfo> codecInfo;
    QSettings settings(ORG_NAME, APP_NAME);
    const auto size = settings.beginReadArray(AUDIO_CODEC_INFO);
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        GenericCodecs::CodecInfo item;
        item.codecId = settings.value(AUDIO_CODEC_ID).toString();
        item.priority = settings.value(AUDIO_CODEC_PRIORITY).toInt();
        codecInfo.append(item);
    }
    settings.endArray();
    return codecInfo;
}

void Settings::saveAudioCodecInfo(const QList<GenericCodecs::CodecInfo> &codecInfo)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.beginWriteArray(AUDIO_CODEC_INFO);
    for (int i = 0; i < codecInfo.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(AUDIO_CODEC_ID, codecInfo.at(i).codecId);
        settings.setValue(AUDIO_CODEC_PRIORITY, codecInfo.at(i).priority);
    }
    settings.endArray();
}

QStringList Settings::buddyList()
{
    QStringList buddies;
    QSettings settings(ORG_NAME, APP_NAME);
    const auto size = settings.beginReadArray(BUDDY_LIST);
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        buddies << settings.value(BUDDY_USER_ID).toString();
    }
    settings.endArray();
    return buddies;
}

void Settings::setBuddyList(const QList<PresenceModel::PresenceInfo> &buddies)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.beginWriteArray(BUDDY_LIST);
    for (int i = 0; i < buddies.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(BUDDY_USER_ID, buddies.at(i).phoneNumber);
    }
    settings.endArray();
}

QList<GenericCodecs::CodecInfo> Settings::videoCodecInfo()
{
    QList<GenericCodecs::CodecInfo> codecInfo;
    QSettings settings(ORG_NAME, APP_NAME);
    const auto size = settings.beginReadArray(VIDEO_CODEC_INFO);
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        GenericCodecs::CodecInfo item;
        item.codecId = settings.value(VIDEO_CODEC_ID).toString();
        item.priority = settings.value(VIDEO_CODEC_PRIORITY).toInt();
        codecInfo.append(item);
    }
    settings.endArray();
    return codecInfo;
}

void Settings::saveVideoCodecInfo(const QList<GenericCodecs::CodecInfo> &codecInfo)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.beginWriteArray(VIDEO_CODEC_INFO);
    for (int i = 0; i < codecInfo.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(VIDEO_CODEC_ID, codecInfo.at(i).codecId);
        settings.setValue(VIDEO_CODEC_PRIORITY, codecInfo.at(i).priority);
    }
    settings.endArray();
}
