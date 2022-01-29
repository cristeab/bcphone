#include "video_devices.h"
#include "softphone.h"
#include <QQmlEngine>
#include <QDebug>

VideoDevices::VideoDevices(QObject *parent) : GenericDevices(parent)
{
    qmlRegisterInterface<VideoDevices>("VideoDevices", 1);
}

int VideoDevices::rowCount(const QModelIndex &/*parent*/) const
{
    return _devs.size();
}

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
