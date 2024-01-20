#include "chat_list_proxy.h"
#include "chat_list.h"

ChatListProxy::ChatListProxy(QObject *parent) : QSortFilterProxyModel(parent),
	  _chatList(new ChatList(this))
{
	setSourceModel(_chatList);
	setFilterRole(Qt::DisplayRole);  // Set the role to use for filtering (e.g., DisplayRole)
	setFilterCaseSensitivity(Qt::CaseInsensitive);  // Set the case sensitivity of the filter
}

bool ChatListProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
	QString itemData = sourceModel()->data(index).toString();

	// Filter based on a search string
	const auto searchString = filterRegularExpression().pattern();
	return itemData.contains(searchString, Qt::CaseInsensitive);
}

void ChatListProxy::addEmptyChat(const QString& label, const QString& extension)
{
	if (nullptr != _chatList) {
		ChatList::ChatInfo info;
		info.label = label;
		info.extension = extension;
		info.count = 0;
		_chatList->addChat(info);
	}
}
