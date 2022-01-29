#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QDebug>

class Settings;

template<typename IndexType = int, int INVALID_DEVICE_INDEX = -1>
class GenericDevices : public QAbstractListModel
{
public:
    enum { INVALID_MODEL_INDEX = -1 };
    enum DeviceRoles {
        Name = Qt::UserRole + 1,
        Index
    };
    struct DeviceInfo {
        QString name;
        IndexType index = INVALID_DEVICE_INDEX;
        bool isValid() const { return !name.isEmpty() && (INVALID_DEVICE_INDEX != index); }
        QString toString() const { return name + ": " + QString::number(index); }
    };
    GenericDevices(QObject *parent) : QAbstractListModel(parent) {}
    virtual ~GenericDevices() = default;
    QHash<int,QByteArray> roleNames() const {
        static const auto roles = QHash<int, QByteArray> {
            { Name, "name" },
            { Index, "index" }
        };
        return roles;
    }
    int deviceIndex(const DeviceInfo &deviceInfo) const {
        if (deviceInfo.isValid()) {
            for (int i = 0; i < _devs.size(); ++i) {
                if (_devs.at(i).isValid() && deviceInfo.isValid() &&
                        (deviceInfo.name == _devs.at(i).name) &&
                        (deviceInfo.index == _devs.at(i).index)) {
                    return i;
                }
            }
        }
        qWarning() << "Cannot find device index" << deviceInfo.name << deviceInfo.index;
        return INVALID_MODEL_INDEX;
    }
    const DeviceInfo& deviceInfoFromIndex(int modelIndex) const {
        if (isValidIndex(modelIndex)) {
            return _devs.at(modelIndex);
        }
        qCritical() << "Invalid model index" << modelIndex;
        static const DeviceInfo emptyInfo;
        return emptyInfo;
    }
    void setSettings(const Settings *settings) { _settings = settings; }

protected:
    Q_DISABLE_COPY_MOVE(GenericDevices)
    bool isValidIndex(int index) const {
        return ((index >= 0) && (index < _devs.count()));
    }
    QVector<DeviceInfo> _devs;
    const Settings *_settings = nullptr;
};
