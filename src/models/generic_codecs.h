#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QQmlEngine>

class GenericCodecs : public QAbstractTableModel
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    struct CodecInfo {
        QString codecId;
        QString formattedCodecId;
        int priority = 0;
        bool checked = true;
        int defaultPriority = -1;
    };

    GenericCodecs(QObject *parent = nullptr);
    virtual ~GenericCodecs() = default;
    virtual void init() = 0;
    int rowCount(const QModelIndex & = QModelIndex()) const override {
        return _codecInfo.size();
    }
    int columnCount(const QModelIndex & = QModelIndex()) const override {
        return COLUMN_COUNT;
    }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    void setCodecPriority(const QString &codecId, int priority);

    virtual void setCodecsInfo(const QList<CodecInfo> &info) = 0;

    Q_INVOKABLE bool isEnabled(int row) {
        return isValidIndex(row) ? 0 < _codecInfo.at(row).priority : false;
    }
    Q_INVOKABLE bool setChecked(int row, bool checked);
    Q_INVOKABLE bool isChecked(int row) {
        return isValidIndex(row) ? _codecInfo.at(row).checked : false;
    }
    Q_INVOKABLE void restoreAudioCodecDefaultPriorities();
    Q_INVOKABLE int defaultPriority(int row);

    const QList<CodecInfo>& codecInfo() const { return _codecInfo; }

signals:
    void codecPriorityChanged(const QString &codecId, int newPriority, int oldPriority);

protected:
    enum { CODEC_ID = 0, PRIORITY, COLUMN_COUNT };
    bool isValidIndex(int index) const {
        return ((index >= 0) && (index < _codecInfo.count()));
    }
    QList<CodecInfo> _codecInfo;
};
