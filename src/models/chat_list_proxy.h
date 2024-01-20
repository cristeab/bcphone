#pragma once

#include <QSortFilterProxyModel>

class ChatList;

class ChatListProxy : public QSortFilterProxyModel
{
	Q_OBJECT
    public:
	explicit ChatListProxy(QObject *parent = nullptr);
	ChatList* chatList() { return _chatList; }
	Q_INVOKABLE void addEmptyChat(const QString& label, const QString &extension);
    protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    private:
	ChatList* _chatList{nullptr};
};
