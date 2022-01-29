#pragma once

#include "qmlhelpers.h"
#include <QAbstractListModel>
#include <QList>
#include <QUrl>

class PlaybackModel : public QAbstractListModel
{
    Q_OBJECT
    QML_READABLE_PROPERTY(bool, isValid, setIsValid, false)

public:
    enum PlaybackRoles {
        FilePath = Qt::UserRole+1,
        Selected
    };
    struct PlaybackInfo {
        QUrl filePath;
        bool selected = false;
        bool operator==(const PlaybackInfo& other) const {
            return this->filePath == other.filePath;
        }
    };
    explicit PlaybackModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int,QByteArray> roleNames() const override;

    QList<QString> selectedPlaybackList() const;
    Q_INVOKABLE void append(const QUrl& filePath);
    Q_INVOKABLE void removeSelected();
    Q_INVOKABLE void setSelected(int index, bool value);
    Q_INVOKABLE QString filePath(int index) const;

signals:
    void errorDialog(const QString& msg);

private:
    bool isValidIndex(int index) const {
        return ((index >= 0) && (index < _playback.count()));
    }
    void updateIsValid();
    QList<PlaybackInfo> _playback;
};
