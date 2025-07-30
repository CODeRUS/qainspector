// Copyright (c) 2019-2020 Open Mobile Platform LLC.
#pragma once

#include <QAbstractItemModel>
#include <QJsonObject>
#include <QRect>

class TreeItem2
{
public:
    TreeItem2(const QJsonObject &data, TreeItem2 *parent = nullptr);
    ~TreeItem2();

    void appendChild(TreeItem2 *child);
    TreeItem2 *child(int index);
    QVector<TreeItem2 *> childs();
    int childCount() const;

    void genocide();

    QVariant data(const QString &roleName) const;
    QJsonObject data() const;
    int columnCount() const;

    int row();
    int row(TreeItem2 *child);

    TreeItem2 *parent();

private:

    QVector<TreeItem2*> m_childs;
    QJsonObject m_data;
    TreeItem2 *m_parent = nullptr;

};

class MyTreeModel2 : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit MyTreeModel2(QObject *parent = nullptr);

    enum class SearchType {
        ClassName,
        Text,
        ObjectName
    };

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE QModelIndex rootIndex() const;

    Q_INVOKABLE QRect getRect(const QModelIndex &index);
    Q_INVOKABLE QJsonObject getData(const QModelIndex &index);
    Q_INVOKABLE QVariantMap getDataVariant(const QModelIndex &index);
    Q_INVOKABLE void copyToClipboard(const QModelIndex &index);

    Q_INVOKABLE QStringList headers() const;

public slots:
    void fillModel(const QJsonObject &object);
    void loadDump(const QString &dump);
    void loadFile(const QString &location);

    QVariantList getChildrenIndexes(TreeItem2 *node = nullptr);
    QModelIndex searchIndex(const QString &key, const QVariant &value, bool partialSearch, const QModelIndex &currentIndex, TreeItem2 *node = nullptr);
    QModelIndex searchIndex(SearchType key, const QVariant &value, bool partialSearch, const QModelIndex &currentIndex, TreeItem2 *node = nullptr);
    QModelIndex searchByCoordinates(qreal posx, qreal posy, TreeItem2 *node = nullptr);
    QModelIndex searchByCoordinates(const QPointF &pos, TreeItem2 *node = nullptr);

private:
    QList<TreeItem2*> processChilds(const QJsonArray &data, TreeItem2 *parentItem);

    QStringList m_headers;
    TreeItem2 *m_rootItem = nullptr;
};
