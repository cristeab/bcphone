#pragma once

#include "generic_devices.h"
#include <QQmlEngine>
#include <pjmedia/audiodev.h>

class AudioDevices : public GenericDevices<pjmedia_aud_dev_index, PJMEDIA_AUD_INVALID_DEV> {
    Q_OBJECT
    QML_ANONYMOUS

public:
    explicit AudioDevices(const Settings *settings, QObject *parent = nullptr) :
        GenericDevices(settings, parent) {}
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void init(const QVector<DeviceInfo> &audioDevs);
    const DeviceInfo& deviceInfo() const;
private:
    Q_DISABLE_COPY_MOVE(AudioDevices)
};
