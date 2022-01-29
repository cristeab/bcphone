#include "generic_codecs.h"

GenericCodecs::GenericCodecs(QObject *parent) : QAbstractTableModel(parent)
{
}

QVariant GenericCodecs::data(const QModelIndex &index, int role) const
{
    QVariant out;
    const auto row = index.row();
    const auto col = index.column();
    switch (role) {
    case Qt::DisplayRole:
        [[fallthrough]];
    case Qt::EditRole:
        if (isValidIndex(row)) {
            switch (col) {
            case CODEC_ID:
                out = _codecInfo.at(row).formattedCodecId;
                break;
            case PRIORITY:
                out = _codecInfo.at(row).priority;
                break;
            default:
                qWarning() << "Unknown column index" << col;
            }
        }
        break;
    default:
        ;
    }
    return out;
}

QHash<int, QByteArray> GenericCodecs::roleNames() const
{
    return { { Qt::DisplayRole, "display" },
        { Qt::EditRole, "edit" } };
}

bool GenericCodecs::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole) {
        if (!checkIndex(index)) {
            return false;
        }
        const auto row = index.row();
        if ((PRIORITY == index.column()) && isValidIndex(row)) {
            const auto newPriority = value.toInt();
            if (newPriority != _codecInfo[row].priority) {
                qDebug() << "Changing codec priority" << _codecInfo[row].codecId << newPriority;
                const auto oldPriority = _codecInfo[row].priority;
                _codecInfo[row].priority = newPriority;
                emit codecPriorityChanged(_codecInfo[row].codecId, newPriority,
                                          oldPriority);
                emit dataChanged(index, index);
            } else {
                //qDebug() << "Codec priority is unchanged" << _codecInfo[row].codecId;
            }
        }
        return true;
    }
    return false;
}

QVariant GenericCodecs::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case CODEC_ID:
            return QString("Codec ID");
        case PRIORITY:
            return QString("Priority");
        }
    }
    return QVariant();
}

Qt::ItemFlags GenericCodecs::flags(const QModelIndex &index) const
{
    Qt::ItemFlags out = QAbstractTableModel::flags(index);
    if (PRIORITY == index.column()) {
        out |= Qt::ItemIsEditable;
    }
    return out;
}

void GenericCodecs::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();
    std::sort(_codecInfo.begin(), _codecInfo.end(), [column, order](const CodecInfo &left,
              const CodecInfo &right) {
        bool out = false;
        switch (column) {
        case CODEC_ID:
            out = left.codecId > right.codecId;
            break;
        case PRIORITY:
            out = left.priority > right.priority;
            break;
        }

        return (Qt::DescendingOrder == order) ? out : !out;
    });
    emit layoutChanged();
}

void GenericCodecs::setCodecPriority(const QString &codecId, int priority)
{
    for (int i = 0; i < _codecInfo.size(); ++i) {
        if (_codecInfo.at(i).codecId == codecId) {
            _codecInfo[i].priority = priority;
            break;
        }
    }
}

bool GenericCodecs::setChecked(int row, bool checked)
{
    bool rc = false;
    if (isValidIndex(row) && (_codecInfo[row].checked != checked)) {
        _codecInfo[row].checked = checked;
        rc = true;
    }
    return rc;
}

void GenericCodecs::restoreAudioCodecDefaultPriorities()
{
    for (auto &ci: _codecInfo) {
        const auto oldPriority = ci.priority;
        ci.priority = ci.defaultPriority;
        emit codecPriorityChanged(ci.codecId, ci.priority, oldPriority);
    }
    sort(PRIORITY, Qt::DescendingOrder);
}

int GenericCodecs::defaultPriority(int row)
{
    int priority = 0;
    if (isValidIndex(row)) {
        priority = _codecInfo[row].defaultPriority;
        if (_codecInfo[row].checked && (0 == priority)) {
            priority += 1;
        }
    }
    return priority;
}
