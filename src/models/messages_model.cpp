#include "messages_model.h"
#include "chat_list.h"

MessagesModel::MessagesModel(QObject *parent)
{
	qmlRegisterType<MessagesModel>("MessagesModel", 1, 0, "MessagesModel");
}

int MessagesModel::rowCount(const QModelIndex& /*parent*/) const
{
	return _messages.size();
}

QVariant MessagesModel::data(const QModelIndex &index, int role) const
{
	if (!isValidIndex(index.row())) {
		qCritical() << "invalid model index";
		return QVariant();
	}
	QVariant out;
	const auto &msg = _messages.at(index.row());
	switch (role) {
	case Id:
		out = msg.id;
		break;
	case DateRole:
		out = msg.timestamp.toString("yyyy-MM-dd");
		break;
	case TimeRole:
		out = msg.timestamp.toString("HH:mm:ss");
		break;
	case StatusRole:
		out = static_cast<int>(msg.status);
		break;
	case DirectionRole:
		out = static_cast<int>(msg.direction);
		break;
	case LocalityRole:
		out = static_cast<int>(msg.locality);
		break;
	case TypeRole:
		out = static_cast<int>(msg.type);
		break;
	case MessageRole:
		out = msg.message;
		break;
	case MmsAttachmentsContentSize: {
		QList<QVariant> sizes;
		for (const auto& it: msg.mmsAttachments) {
			sizes << it.contentSize;
		}
		out = sizes;
	}
	break;
	case MmsAttachmentsContentType: {
		QList<QVariant> types;
		for (const auto& it: msg.mmsAttachments) {
			types << it.contentType;
		}
		out = types;
	}
	break;
	case MmsAttachmentsFileName: {
		QList<QVariant> fns;
		for (const auto& it: msg.mmsAttachments) {
			fns << it.fileName;
		}
		out = fns;
	}
	break;
	case AccountCode:
		out = msg.accountCode;
		break;
	case MessageId:
		out = msg.messageId;
		break;
	case SrcLabel:
		out = msg.srcAdditionalInfo.label;
		break;
	case SrcExtension:
		out = msg.srcAdditionalInfo.extension;
		break;
	case SrcAuth:
		out = msg.srcAdditionalInfo.auth;
		break;
	case DstOriginalLabel:
		out = msg.dstOriginalAdditionalInfo.label;
		break;
	case DstOriginalExtension:
		out = msg.dstOriginalAdditionalInfo.extension;
		break;
	case DstOriginalAuth:
		out = msg.dstOriginalAdditionalInfo.auth;
		break;
	case DstLabel:
		out = msg.dstAdditionalInfo.label;
		break;
	case DstExtension:
		out = msg.dstAdditionalInfo.extension;
		break;
	case DstAuth:
		out = msg.dstAdditionalInfo.auth;
		break;
	case Qt::DisplayRole:
		out = msg.srcAdditionalInfo.extension + msg.dstAdditionalInfo.extension;
		break;
	default:
		qCritical() << "unknown role" << role;
	}
	return out;
}

QHash<int,QByteArray> MessagesModel::roleNames() const
{
	static const auto roles = QHash<int, QByteArray> {
		{ Id, "id" },
		{ DateRole, "dateRole" },
		{ TimeRole, "timeRole" },
		{ StatusRole, "statusRole" },
		{ DirectionRole, "directionRole" },
		{ LocalityRole, "localityRole" },
		{ TypeRole, "typeRole" },
		{ MessageRole, "messageRole" },
		{ MmsAttachmentsContentSize, "mmsAttachmentsContentSize" },
		{ MmsAttachmentsContentType, "mmsAttachmentsContentType" },
		{ MmsAttachmentsFileName, "mmsAttachmentsFileName" },
		{ AccountCode, "accountCode" },
		{ MessageId, "messageId" },
		{ SrcLabel, "srcLabel" },
		{ SrcExtension, "srcExtension" },
		{ SrcAuth, "srcAuth" },
		{ DstOriginalLabel, "dstOriginalLabel" },
		{ DstOriginalExtension, "dstOriginalExtension" },
		{ DstOriginalAuth, "dstOriginalAuth" },
		{ DstLabel, "dstLabel" },
		{ DstExtension, "dstExtension" },
		{ DstAuth, "dstAuth" }
	};
	return roles;
}

void MessagesModel::clear()
{
	emit layoutAboutToBeChanged();
	_messages.clear();
	emit layoutChanged();
	if (nullptr != _chatList) {
		_chatList->clear();
	}
}

void MessagesModel::append(const Message &msg)
{
	emit layoutAboutToBeChanged();
	_messages << msg;
	emit layoutChanged();
}

void MessagesModel::updateChatList()
{
	QStringList conversationList;
	QStringList extensionList;
	QHash<QString, uint32_t> chatCount;
	for (const auto& msg: _messages) {
		switch (msg.direction) {
		case Direction::INBOUND:
			if (!msg.srcAdditionalInfo.label.isEmpty()) {
				if (!conversationList.contains(msg.srcAdditionalInfo.label)) {
					conversationList << msg.srcAdditionalInfo.label;
					extensionList << msg.srcAdditionalInfo.extension;
				}
				chatCount[msg.srcAdditionalInfo.label] += 1U;
			} else {
				qWarning() << "Empty src label" << msg.message;
			}
			break;
		case Direction::OUTBOUND:
			if (!msg.dstAdditionalInfo.label.isEmpty()) {
				if (!conversationList.contains(msg.dstAdditionalInfo.label)) {
					conversationList << msg.dstAdditionalInfo.label;
					extensionList << msg.dstAdditionalInfo.extension;
				}
				chatCount[msg.dstAdditionalInfo.label] += 1U;
			} else {
				qWarning() << "Empty dst label" << msg.message;
			}
			break;
		case Direction::UNKNOWN:
			//should not happen
			qWarning() << "Unknown direction" << msg.message;
			break;
		default:
			;
		}
	}
	if (nullptr != _chatList) {
		ChatList::ChatInfoList chatInfoList;
		for (int i = 0; i < conversationList.size(); ++i) {
			ChatList::ChatInfo chatInfo{};
			chatInfo.label = conversationList.at(i);
			chatInfo.extension = extensionList.at(i);
			chatInfo.count = chatCount.value(chatInfo.label);
			chatInfoList << chatInfo;
		}
		_chatList->setChatInfo(chatInfoList);
	}
}
