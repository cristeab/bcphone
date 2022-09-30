#include "ring_tones_model.h"
#include "settings.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>

RingTonesModel::RingTonesModel(QObject *parent) : QAbstractListModel(parent)
{
}

int RingTonesModel::rowCount(const QModelIndex& /*parent*/) const
{
    return _ringTones.size();
}

QVariant RingTonesModel::data(const QModelIndex &index, int role) const
{
    if (!isValidIndex(index.row())) {
        qCritical() << "invalid model index";
        return QVariant();
    }
    QVariant out;
    switch (role) {
    case Name:
        out = _ringTones.at(index.row()).fileTitle;
        break;
    default:
        qCritical() << "unknown role" << role;
    }
    return out;
}

QHash<int,QByteArray> RingTonesModel::roleNames() const
{
   static const auto roles = QHash<int, QByteArray> {
        { Name, "name" }
    };
    return roles;
}

void RingTonesModel::init(const QVector<RingTonesInfo> &ringTones)
{
    initDefaultRingTones();
    emit layoutAboutToBeChanged();
    _ringTones += ringTones;
    emit layoutChanged();
}

QString RingTonesModel::filePath(int index) const
{
    if (!isValidIndex(index)) {
        qCritical() << "invalid model index" << index;
        return QString();
    }
    return _ringTones.at(index).filePath;
}

void RingTonesModel::setFilePath(int index, const QString &filePath)
{
    index += 1;//skip default ringtone
    if (isValidIndex(index)) {
        _ringTones[index].filePath = filePath;
    } else {
        qCritical() << "invalid model index";
    }
}

bool RingTonesModel::initDefaultRingTones()
{
    const QStringList audioFiles{ "discord-ringtone.wav", "phone-calling.wav",
                                  "telephone-ring.wav", "discord-call-ringtone.wav",
                                  "default-ringtone.wav"};
    emit layoutAboutToBeChanged();
    _ringTones.clear();
    for (const auto &a: audioFiles) {
        QFile inFile(QString(":/audio/%1").arg(a));
        if (!inFile.open(QIODevice::ReadOnly)) {
            qCritical() << "Cannot open" << inFile.fileName() << "for reading" <<
                           inFile.errorString();
            continue;
        }

        QString basename = QFileInfo(inFile.fileName()).fileName();
        const QString filePath = Settings::writablePath()+"/"+basename;

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qCritical() << "Could not open" << file.fileName() << "for writing" <<
                           file.errorString();
            continue;
        }
        file.write(inFile.readAll());

        RingTonesInfo info;
        info.fileTitle = basename.replace('-', ' ').remove(".wav");
        info.filePath = filePath;
        _ringTones.append(info);
    }
    emit layoutChanged();

    return true;
}
