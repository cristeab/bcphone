#include "audio_devices.h"
#include "softphone.h"
#include "settings.h"
#include <QDebug>

AudioDevices::AudioDevices(QObject *parent) : GenericDevices(parent)
{
}

int AudioDevices::rowCount(const QModelIndex &/*parent*/) const
{
    return _devs.size();
}

QVariant AudioDevices::data(const QModelIndex &index, int role) const
{
    if (!isValidIndex(index.row())) {
        qCritical() << "invalid model index";
        return QVariant();
    }
    QVariant out;
    switch (role) {
    case Name:
        out = _devs.at(index.row()).name;
        break;
    case Index:
        out = _devs.at(index.row()).index;
        break;
    default:
        qCritical() << "unknown role" << role;
    }
    return out;
}

void AudioDevices::init(const QVector<DeviceInfo> &audioDevs)
{
    emit layoutAboutToBeChanged();
    _devs = audioDevs;
    emit layoutChanged();
}

const AudioDevices::DeviceInfo& AudioDevices::deviceInfo() const
{
    if (nullptr != _settings) {
        return AudioDevices::deviceInfoFromIndex(_settings->inputAudioModelIndex());
    } else {
        qCritical() << "Cannot get settings pointer";
    }
    static const DeviceInfo emptyInfo;
    return emptyInfo;
}
