#include "chat_list.h"

ChatList::ChatList(QObject *parent) : QAbstractListModel(parent)
{
}

int ChatList::rowCount(const QModelIndex &parent) const
{
	return _chats.count();
}

QVariant ChatList::data(const QModelIndex &index, int role) const
{
	if (!isValidIndex(index.row())) {
		qCritical() << "invalid model index";
		return QVariant();
	}
	QVariant out;
	const auto &chat = _chats.at(index.row());
	switch (role) {
	case Label:
		out = chat.label;
		break;
	case Extension:
		out = chat.extension;
		break;
	case Count:
		out = chat.count;
		break;
	case Qt::DisplayRole:
		// used to filter the model
		out = chat.label;
		break;
	default:
		qCritical() << "unknown role" << role;
	}
	return out;
}

QHash<int,QByteArray> ChatList::roleNames() const
{
	static const auto roles = QHash<int, QByteArray> {
		{ Label, "label" },
		{ Extension, "extension" },
		{ Count, "count" }
	};
	return roles;
}
