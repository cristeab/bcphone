#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QQmlEngine>

class ChatList : public QAbstractListModel
{
	Q_OBJECT
	QML_ANONYMOUS

    public:
	enum ChatRoles {
		Label = Qt::UserRole+1,
		Extension,
		Count
	};
	struct ChatInfo {
		QString label;
		QString extension;
		uint32_t count{};
	};
	using ChatInfoList = QList<ChatInfo>;

	explicit ChatList(QObject *parent = nullptr);
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QHash<int,QByteArray> roleNames() const override;

	void setChatInfo(const ChatInfoList &data) {
		emit layoutAboutToBeChanged();
		_chats = data;
		emit layoutChanged();
	}
	void clear() {
		emit layoutAboutToBeChanged();
		_chats.clear();
		emit layoutChanged();
	}

	void addChat(const ChatInfo& chatInfo) {
		bool found{false};
		for (int i = 0; i < _chats.count(); ++i) {
			if (chatInfo.label == _chats.at(i).label) {
				found = true;
				break;
			}
		}

		if (!found) {
			emit layoutAboutToBeChanged();
			_chats << chatInfo;
			emit layoutChanged();
		}
	}

    private:
	bool isValidIndex(int index) const {
		return (index >= 0) && (index < _chats.count());
	}
	ChatInfoList _chats;
};
