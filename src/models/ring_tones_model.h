#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>
#include <QVector>

class RingTonesModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    enum RingTonesRoles {
        Name = Qt::UserRole+1
    };
    struct RingTonesInfo {
        QString fileTitle;
        QString filePath;
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int,QByteArray> roleNames() const;

    explicit RingTonesModel(QObject *parent = nullptr);
    bool initDefaultRingTones();
    void init(const QVector<RingTonesInfo> &ringTones);
    QString filePath(int index) const;
    void setFilePath(int index, const QString &filePath);
private:
    bool isValidIndex(int index) const {
        return ((index >= 0) && (index < _ringTones.count()));
    }
    QVector<RingTonesInfo> _ringTones;
};
