#pragma once

#include "generic_devices.h"
#include <QQmlEngine>
#include <pjmedia/videodev.h>

class VideoDevices : public GenericDevices<pjmedia_vid_dev_index, PJMEDIA_VID_INVALID_DEV> {
    Q_OBJECT
    QML_ANONYMOUS

public:
    explicit VideoDevices(const Settings *settings, QObject *parent = nullptr) :
        GenericDevices(settings, parent) {}
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void init(const QVector<DeviceInfo> &videoDevs);
    const DeviceInfo& deviceInfo() const;
private:
    Q_DISABLE_COPY_MOVE(VideoDevices)
};
