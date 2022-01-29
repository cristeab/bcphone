#include "playback_model.h"
#include "settings.h"
#include <QQmlEngine>

PlaybackModel::PlaybackModel(QObject *parent)
    : QAbstractListModel{parent}
{
    qmlRegisterInterface<PlaybackModel>("PlaybackModel", 1);
    _playback = Settings::playbackInfo();
    updateIsValid();
}


int PlaybackModel::rowCount(const QModelIndex& parent) const
{
    return _playback.size();
}

QVariant PlaybackModel::data(const QModelIndex &index, int role) const
{
    QVariant out;
    if (!isValidIndex(index.row())) {
        qCritical() << "Invalid model index";
        return out;
    }

    const auto row = index.row();
    switch (role) {
    case FilePath:
        out = _playback.at(row).filePath.toLocalFile();
        break;
    case Selected:
        out = _playback.at(row).selected;
        break;
    default:
        qCritical() << "unknown role" << role;
    }
    return out;
}

QHash<int,QByteArray> PlaybackModel::roleNames() const
{
    static const auto roles = QHash<int, QByteArray> {
        { FilePath, "filePath" },
        { Selected, "selected" }
    };
     return roles;
}

void PlaybackModel::append(const QUrl& filePath)
{
    qInfo() << "Append" << filePath;
    //make sure that there is no duplicate
    for (const auto& it: _playback) {
        if (it.filePath == filePath) {
            qWarning() << "Already in the list";
            emit errorDialog(tr("Cannot add playback file: already in the list"));
            return;
        }
    }
    emit layoutAboutToBeChanged();
    _playback << PlaybackInfo{filePath, false};
    emit layoutChanged();
    Settings::savePlaybackInfo(_playback);
}

void PlaybackModel::removeSelected()
{
    emit layoutAboutToBeChanged();
    auto it = _playback.begin();
    while (it != _playback.end()) {
        if (it->selected) {
            qDebug() << "Removing" << it->filePath;
            it = _playback.erase(it);
        } else {
            ++it;
        }
    }
    emit layoutChanged();
    Settings::savePlaybackInfo(_playback);
    updateIsValid();
}

QList<QString> PlaybackModel::selectedPlaybackList() const
{
    QList<QString> sel;
    for (const auto& it: _playback) {
        if (it.selected) {
            sel << it.filePath.toLocalFile();
        }
    }
    return sel;
}

void PlaybackModel::setSelected(int index, bool value)
{
    if (isValidIndex(index)) {
        _playback[index].selected = value;
        Settings::savePlaybackInfo(_playback);
        updateIsValid();
    }
}

QString PlaybackModel::filePath(int index) const
{
    for (int i = 0; i < _playback.size(); ++i) {
        if (i == index) {
            return _playback.at(i).filePath.toString();
        }
    }
    return "";
}

void PlaybackModel::updateIsValid()
{
    bool valid = false;
    for (const auto& it: _playback) {
        if (it.selected) {
            valid = true;
            break;
        }
    }
    setIsValid(valid);
}
