#include "video_devices.h"
#include "settings.h"
#include <QDebug>

QVariant VideoDevices::data(const QModelIndex &index, int role) const
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

void VideoDevices::init(const QVector<DeviceInfo> &videoDevs)
{
    emit layoutAboutToBeChanged();
    _devs = videoDevs;
    emit layoutChanged();
}

const VideoDevices::DeviceInfo& VideoDevices::deviceInfo() const
{
    if (nullptr != _settings) {
        return VideoDevices::deviceInfoFromIndex(_settings->videoModelIndex());
    } else {
        qCritical() << "Cannot get settings pointer";
    }
    static const DeviceInfo emptyInfo;
    return emptyInfo;
}
