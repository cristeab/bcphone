#include "messages_proxy_model.h"
#include "messages_model.h"

MessagesProxyModel::MessagesProxyModel(QObject *parent) : QSortFilterProxyModel(parent),
	  _messagesModel(new MessagesModel(this))
{
	setSourceModel(_messagesModel);
	setFilterRole(Qt::DisplayRole);  // Set the role to use for filtering (e.g., DisplayRole)
	setFilterCaseSensitivity(Qt::CaseInsensitive);  // Set the case sensitivity of the filter
}

bool MessagesProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
	QString itemData = sourceModel()->data(index).toString();

	// Filter based on a search string
	const auto searchString = filterRegularExpression().pattern();
	return itemData.contains(searchString, Qt::CaseSensitive);
}

void MessagesProxyModel::appendOutgoing(const QString& srcExtension,
					const QString& dstExtension,
					const QString& message)
{
	if (nullptr != _messagesModel) {
		MessagesModel::Message msg;
		msg.timestamp = QDateTime::currentDateTimeUtc();
		msg.srcAdditionalInfo.extension = srcExtension;
		msg.dstAdditionalInfo.extension = dstExtension;
		msg.message = message;
		msg.direction = MessagesModel::Direction::OUTBOUND;
		msg.type = MessagesModel::Type::SMS;
		_messagesModel->append(msg);
		qDebug() << "Append outgoing message" << message;
	} else {
		qWarning() << "Cannot append outgoing message";
	}
}
