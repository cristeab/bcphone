#pragma once

#include <QSortFilterProxyModel>

class MessagesModel;

class MessagesProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT
    public:
	explicit MessagesProxyModel(QObject *parent = nullptr);
	MessagesModel* messagesModel() { return _messagesModel; }
	Q_INVOKABLE void appendOutgoing(const QString& srcExtension, const QString& dstExtension, const QString& message);
    protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    private:
	MessagesModel* _messagesModel{nullptr};
};
