#include "settings.h"
#include "config.h"
#include <QSettings>
#include <QVector>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

#define SETTINGS_FOLDER ".BCphone"
#define REC_FOLDER "BCphoneRecordings"
#define CH_DATE_TIME_FORMAT "yyyy-MM-dd HH:mm:ss"

// macros to set/get values from/to settings
#define XSTR(a) STR_HELPER(a)
#define STR_HELPER(a) #a

#define GET_SETTING(name) settings.value(XSTR(name), _ ## name)
#define SET_SETTING(name) settings.setValue(XSTR(name), _ ## name)

Settings::Settings(QObject *parent) : QObject (parent)
{
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
    setSipServer("");
    setSipPort(SIP_PORT);
    setUserName("");
    setPassword("");
    setAccountName("");
    setSipTransport(SipTransport::Udp);
    setMediaTransport(MediaTransport::Rtp);

    setInputAudioModelIndex(INVALID_INDEX);
    setOutputAudioModelIndex(INVALID_INDEX);
    setInboundRingTonesModelIndex(INBOUND_RING_TONE_INDEX);
    setOutboundRingTonesModelIndex(OUTBOUND_RING_TONE_INDEX);
    setVideoModelIndex(INVALID_INDEX);

    setMicrophoneVolume(MICROPHONE_VOLUME);
    setSpeakersVolume(SPEAKERS_VOLUME);
    setDialpadSoundVolume(DIALPAD_SOUND_VOLUME);

    setRecPath("");

    //setStunServer("stun.zoiper.com");
    //setStunPort(3478);

    setAuthUserName("");
    setProxyEnabled(PROXY_ENABLED);
    setProxyServer("");
    setProxyPort(PROXY_PORT);

    setEnableSipLog(ENABLE_SIP_LOG);
    setEnableVad(ENABLE_VAD);
    setTransportSourcePort(TRANSPORT_DEFAULT_PORT);
    setDisableTcpSwitch(DISABLE_TCP_SWITCH);

    Settings::uninstallClear();
}

void Settings::uninstallClear()
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.clear();
    const auto path = writablePath();
    if (!path.isEmpty()) {
        QDir(path).removeRecursively();
    }
}

void Settings::load()
{
    QSettings settings(ORG_NAME, APP_NAME);
    qDebug() << "Reading settings from" << settings.fileName();

    setSipServer(GET_SETTING(sipServer).toString());
    setSipPort(GET_SETTING(sipPort).toInt());
    setUserName(GET_SETTING(userName).toString());
    setPassword(GET_SETTING(password).toString());
    setAccountName(GET_SETTING(accountName).toString());
    setSipTransport(GET_SETTING(sipTransport).toInt());
    setMediaTransport(GET_SETTING(mediaTransport).toInt());

    //indices for audio and video devices are loaded and saved separately

    setInboundRingTonesModelIndex(GET_SETTING(inboundRingTonesModelIndex).toInt());
    setOutboundRingTonesModelIndex(GET_SETTING(outboundRingTonesModelIndex).toInt());

    setMicrophoneVolume(GET_SETTING(microphoneVolume).toDouble());
    setSpeakersVolume(GET_SETTING(speakersVolume).toDouble());

    setRecPath(GET_SETTING(recPath).toString());
    if (_recPath.isEmpty()) {
        setRecPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)+"/" REC_FOLDER);
    }
    if (!QDir(_recPath).exists()) {
        QDir().mkpath(_recPath);
    }

    //setStunServer(settings.value(STUN_SERVER, _stunServer).toString());
    //setStunPort(settings.value(STUN_PORT, _stunPort).toInt());

    setAuthUserName(GET_SETTING(authUserName).toString());
    setProxyEnabled(GET_SETTING(proxyEnabled).toBool());
    setProxyServer(GET_SETTING(proxyServer).toString());
    setProxyPort(GET_SETTING(proxyPort).toInt());

    setEnableSipLog(GET_SETTING(enableSipLog).toBool());
    setEnableVad(GET_SETTING(enableVad).toBool());
    setTransportSourcePort(GET_SETTING(transportSourcePort).toInt());
    setDisableTcpSwitch(GET_SETTING(disableTcpSwitch).toBool());
}

void Settings::save()
{
    QSettings settings(ORG_NAME, APP_NAME);
    qDebug() << "Save settings to" << settings.fileName();

    SET_SETTING(sipServer);
    SET_SETTING(sipPort);
    SET_SETTING(userName);
    SET_SETTING(password);
    SET_SETTING(accountName);
    SET_SETTING(sipTransport);
    SET_SETTING(mediaTransport);

    SET_SETTING(inboundRingTonesModelIndex);
    SET_SETTING(outboundRingTonesModelIndex);

    SET_SETTING(microphoneVolume);
    SET_SETTING(speakersVolume);

    SET_SETTING(recPath);

    //settings.setValue(STUN_SERVER, _stunServer);
    //settings.setValue(STUN_PORT, _stunPort);

    SET_SETTING(authUserName);
    SET_SETTING(proxyEnabled);
    SET_SETTING(proxyServer);
    SET_SETTING(proxyPort);

    SET_SETTING(enableSipLog);
    SET_SETTING(enableVad);
    SET_SETTING(transportSourcePort);
    SET_SETTING(disableTcpSwitch);
}

AudioDevices::DeviceInfo Settings::inputAudioDeviceInfo()
{
    QSettings settings(ORG_NAME, APP_NAME);
    return { settings.value(XSTR(inputAudioModelName)).toString(),
             settings.value(XSTR(inputAudioModelIndex), PJMEDIA_AUD_INVALID_DEV).toInt() };
}

void Settings::saveInputAudioDeviceInfo(const AudioDevices::DeviceInfo &devInfo)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.setValue(XSTR(inputAudioModelName), devInfo.name);
    settings.setValue(XSTR(inputAudioModelIndex), devInfo.index);
}

AudioDevices::DeviceInfo Settings::outputAudioDeviceInfo()
{
    QSettings settings(ORG_NAME, APP_NAME);
    return { settings.value(XSTR(outputAudioModelName)).toString(),
             settings.value(XSTR(outputAudioModelIndex), PJMEDIA_AUD_INVALID_DEV).toInt() };
}

void Settings::saveOutputAudioDeviceInfo(const AudioDevices::DeviceInfo &devInfo)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.setValue(XSTR(outputAudioModelName), devInfo.name);
    settings.setValue(XSTR(outputAudioModelIndex), devInfo.index);
}

VideoDevices::DeviceInfo Settings::videoDeviceInfo()
{
    QSettings settings(ORG_NAME, APP_NAME);
    return { settings.value(XSTR(videoModelName)).toString(),
             settings.value(XSTR(videoModelIndex), PJMEDIA_VID_INVALID_DEV).toInt() };
}

void Settings::saveVideoDeviceInfo(const VideoDevices::DeviceInfo &devInfo)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.setValue(XSTR(videoModelName), devInfo.name);
    settings.setValue(XSTR(videoModelIndex), devInfo.index);
}

QVector<CallHistoryModel::CallHistoryInfo> Settings::callHistoryInfo()
{
    QVector<CallHistoryModel::CallHistoryInfo> history;
    QSettings settings(ORG_NAME, APP_NAME);
    const auto size = settings.beginReadArray(XSTR(callHistory));
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        CallHistoryModel::CallHistoryInfo item;
        item.contactId = settings.value(XSTR(contactId)).toInt();
        item.userName = settings.value(XSTR(userName)).toString();
        item.phoneNumber = settings.value(XSTR(phoneNumber)).toString();
        item.dateTime = QDateTime::fromString(settings.value(XSTR(dateTime)).toString(),
                                              CH_DATE_TIME_FORMAT);
        item.callStatus = static_cast<CallHistoryModel::CallStatus>(settings.value(XSTR(callStatus)).toInt());
        history.append(item);
    }
    settings.endArray();
    return history;
}

void Settings::saveCallHistoryInfo(const QVector<CallHistoryModel::CallHistoryInfo> &historyInfo)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.beginWriteArray(XSTR(callHistory));
    for (int i = 0; i < historyInfo.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(XSTR(contactId), historyInfo.at(i).contactId);
        settings.setValue(XSTR(userName), historyInfo.at(i).userName);
        settings.setValue(XSTR(phoneNumber), historyInfo.at(i).phoneNumber);
        settings.setValue(XSTR(dateTime), historyInfo.at(i).dateTime.toString(CH_DATE_TIME_FORMAT));
        settings.setValue(XSTR(callStatus), static_cast<int>(historyInfo.at(i).callStatus));
    }
    settings.endArray();
}

QVector<ContactsModel::ContactInfo> Settings::contactsInfo()
{
    QVector<ContactsModel::ContactInfo> contacts;
    QSettings settings(ORG_NAME, APP_NAME);
    const auto size = settings.beginReadArray(XSTR(contactList));
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        ContactsModel::ContactInfo item;
        item.id = settings.value(XSTR(contactId)).toInt();
        item.firstName = settings.value(XSTR(firstName)).toString();
        item.lastName = settings.value(XSTR(lastName)).toString();
        item.email = settings.value(XSTR(contactEmail)).toString();
        item.phoneNumber = settings.value(XSTR(phoneNumber)).toString();
        item.mobileNumber = settings.value(XSTR(mobileNumber)).toString();
        item.address = settings.value(XSTR(contactAddress)).toString();
        item.state = settings.value(XSTR(contactState)).toString();
        item.city = settings.value(XSTR(contactCity)).toString();
        item.zip = settings.value(XSTR(contactZip)).toString();
        item.comment = settings.value(XSTR(comment)).toString();
        contacts.append(item);
    }
    settings.endArray();
    return contacts;
}

void Settings::saveContactsInfo(const QVector<ContactsModel::ContactInfo> &contactsInfo)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.beginWriteArray(XSTR(contactList));
    for (int i = 0; i < contactsInfo.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(XSTR(contactId), contactsInfo.at(i).id);
        settings.setValue(XSTR(firstName), contactsInfo.at(i).firstName);
        settings.setValue(XSTR(lastName), contactsInfo.at(i).lastName);
        settings.setValue(XSTR(contactEmail), contactsInfo.at(i).email);
        settings.setValue(XSTR(phoneNumber), contactsInfo.at(i).phoneNumber);
        settings.setValue(XSTR(mobileNumber), contactsInfo.at(i).mobileNumber);
        settings.setValue(XSTR(contactAddress), contactsInfo.at(i).address);
        settings.setValue(XSTR(contactState), contactsInfo.at(i).state);
        settings.setValue(XSTR(contactCity), contactsInfo.at(i).city);
        settings.setValue(XSTR(contactZip), contactsInfo.at(i).zip);
        settings.setValue(XSTR(comment), contactsInfo.at(i).comment);
    }
    settings.endArray();
}

QList<GenericCodecs::CodecInfo> Settings::audioCodecInfo()
{
    QList<GenericCodecs::CodecInfo> codecInfo;
    QSettings settings(ORG_NAME, APP_NAME);
    const auto size = settings.beginReadArray(XSTR(audioCodecInfo));
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        GenericCodecs::CodecInfo item;
        item.codecId = settings.value(XSTR(audioCodecId)).toString();
        item.priority = settings.value(XSTR(audioCodecPriority)).toInt();
        codecInfo.append(item);
    }
    settings.endArray();
    return codecInfo;
}

void Settings::saveAudioCodecInfo(const QList<GenericCodecs::CodecInfo> &codecInfo)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.beginWriteArray(XSTR(audioCodecInfo));
    for (int i = 0; i < codecInfo.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(XSTR(audioCodecId), codecInfo.at(i).codecId);
        settings.setValue(XSTR(audioCodecPriority), codecInfo.at(i).priority);
    }
    settings.endArray();
}

QStringList Settings::buddyList()
{
    QStringList buddies;
    QSettings settings(ORG_NAME, APP_NAME);
    const auto size = settings.beginReadArray(XSTR(buddyList));
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        buddies << settings.value(XSTR(buddyUserId)).toString();
    }
    settings.endArray();
    return buddies;
}

void Settings::setBuddyList(const QList<PresenceModel::PresenceInfo> &buddies)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.beginWriteArray(XSTR(buddyList));
    for (int i = 0; i < buddies.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(XSTR(buddyUserId), buddies.at(i).phoneNumber);
    }
    settings.endArray();
}

QList<GenericCodecs::CodecInfo> Settings::videoCodecInfo()
{
    QList<GenericCodecs::CodecInfo> codecInfo;
    QSettings settings(ORG_NAME, APP_NAME);
    const auto size = settings.beginReadArray(XSTR(videoCodecInfo));
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        GenericCodecs::CodecInfo item;
        item.codecId = settings.value(XSTR(videoCodecId)).toString();
        item.priority = settings.value(XSTR(videoCodecPriority)).toInt();
        codecInfo.append(item);
    }
    settings.endArray();
    return codecInfo;
}

void Settings::saveVideoCodecInfo(const QList<GenericCodecs::CodecInfo> &codecInfo)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.beginWriteArray(XSTR(videoCodecInfo));
    for (int i = 0; i < codecInfo.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(XSTR(videoCodecId), codecInfo.at(i).codecId);
        settings.setValue(XSTR(videoCodecPriority), codecInfo.at(i).priority);
    }
    settings.endArray();
}
