#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QQmlEngine>

class ChatList;

class MessagesModel : public QAbstractListModel
{
	Q_OBJECT
	QML_ANONYMOUS

    public:
	enum MessageRoles {
		Id = Qt::UserRole+1,
		DateRole,
		TimeRole,
		StatusRole,
		DirectionRole,
		LocalityRole,
		TypeRole,
		MessageRole,
		MmsAttachmentsContentSize,
		MmsAttachmentsContentType,
		MmsAttachmentsFileName,
		AccountCode,
		MessageId,
		SrcLabel,
		SrcExtension,
		SrcAuth,
		DstOriginalLabel,
		DstOriginalExtension,
		DstOriginalAuth,
		DstLabel,
		DstExtension,
		DstAuth
	};

	enum class Status { SUCCESS, BLOCKED };
	Q_ENUM(Status)
	enum class Direction { INBOUND, OUTBOUND, UNKNOWN };
	Q_ENUM(Direction)
	enum class Locality { INTERNAL, EXTERNAL, UNKNOWN };
	Q_ENUM(Locality)
	enum class Type { SMS, MMS, UNKNOWN };
	Q_ENUM(Type)
	struct MmsAttachment {
		uint32_t contentSize{};
		QString contentType;
		QString fileName;
	};
	struct AdditionalInfo {
		QString label;
		QString extension;
		QString auth;
	};
	struct Message {
		QString id;
		QDateTime timestamp;
		Status status{Status::BLOCKED};
		Direction direction{Direction::UNKNOWN};
		Locality locality{Locality::UNKNOWN};
		Type type{Type::SMS};
		QString message;
		QList<MmsAttachment> mmsAttachments;
		QString accountCode;
		QString messageId;
		AdditionalInfo srcAdditionalInfo;
		AdditionalInfo dstOriginalAdditionalInfo;
		AdditionalInfo dstAdditionalInfo;
	};

	explicit MessagesModel(QObject *parent = nullptr);
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QHash<int,QByteArray> roleNames() const override;

	void setChatList(ChatList *chatList) { _chatList = chatList; }
	void clear();
	void append(const Message &msg);
	void sortTimestamp() {
		emit layoutAboutToBeChanged();
		std::sort(_messages.begin(), _messages.end(), [](const Message& left, const Message& right) {
			return left.timestamp < right.timestamp;
		});
		emit layoutChanged();
	}
	void updateChatList();

    private:
	bool isValidIndex(int index) const {
		return ((index >= 0) && (index < _messages.count()));
	}
	QList<Message> _messages;
	ChatList *_chatList{nullptr};
};
