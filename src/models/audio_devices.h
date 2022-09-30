#pragma once

#include "generic_devices.h"
#include <QQmlEngine>
#include <pjmedia/audiodev.h>

class AudioDevices : public GenericDevices<pjmedia_aud_dev_index, PJMEDIA_AUD_INVALID_DEV> {
    Q_OBJECT
    QML_ANONYMOUS

public:
    explicit AudioDevices(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void init(const QVector<DeviceInfo> &audioDevs);
    const DeviceInfo& deviceInfo() const;
private:
    Q_DISABLE_COPY_MOVE(AudioDevices)
};
