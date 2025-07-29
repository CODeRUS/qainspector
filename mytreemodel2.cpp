// Copyright (c) 2019-2020 Open Mobile Platform LLC.
#include "mytreemodel2.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>

#include <QClipboard>
#include <QGuiApplication>

#include <QJsonValue>

MyTreeModel2::MyTreeModel2(QObject* parent)
    : QAbstractItemModel(parent)
{
    m_headers.append("classname");
    m_headers.append("objectName");
    m_headers.append("objectId");
    m_headers.append("mainTextProperty");
    m_headers.append("abs_x");
    m_headers.append("abs_y");
    m_headers.append("enabled");
    m_headers.append("visible");
    m_headers.append("width");
    m_headers.append("height");

    QJsonObject rootObject;
    rootObject.insert("classname", QJsonValue::fromVariant("Classname"));
    rootObject.insert("abs_x", QJsonValue::fromVariant("abx"));
    rootObject.insert("abs_y", QJsonValue::fromVariant("aby"));
    rootObject.insert("mainTextProperty", QJsonValue::fromVariant("Text"));
    rootObject.insert("objectName", QJsonValue::fromVariant("ObjectName"));
    rootObject.insert("objectId", QJsonValue::fromVariant("ObjectId"));
    rootObject.insert("enabled", QJsonValue::fromVariant("ena"));
    rootObject.insert("visible", QJsonValue::fromVariant("vis"));
    rootObject.insert("width", QJsonValue::fromVariant("wid"));
    rootObject.insert("height", QJsonValue::fromVariant("hei"));

    m_rootItem = new TreeItem2(rootObject);
}

void MyTreeModel2::fillModel(const QJsonObject& object)
{
    beginResetModel();

    m_rootItem->genocide();

    QJsonObject data = object;
    const QJsonArray childsData = data.take(QStringLiteral("children")).toArray();

    TreeItem2* firstItem = new TreeItem2(data, m_rootItem);
    m_rootItem->appendChild(firstItem);

    QList<TreeItem2*> childs = processChilds(childsData, firstItem);
    for (TreeItem2* child : childs)
    {
        firstItem->appendChild(child);
    }

    endResetModel();
}

void MyTreeModel2::loadDump(const QString& dump)
{
    qDebug() << Q_FUNC_INFO << dump.length();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(dump.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError)
    {
        fillModel(doc.object());
    }
    else
    {
        qWarning() << Q_FUNC_INFO << error.errorString();
    }
}

QList<TreeItem2*> MyTreeModel2::processChilds(const QJsonArray& data, TreeItem2* parentItem)
{
    QList<TreeItem2*> childs;
    for (const QJsonValue& value : data)
    {
        QJsonObject childData = value.toObject();
        const QJsonArray childsData = childData.take(QStringLiteral("children")).toArray();
        TreeItem2* child = new TreeItem2(childData, parentItem);
        QList<TreeItem2*> itemChilds = processChilds(childsData, child);
        for (TreeItem2* itemChild : itemChilds)
        {
            child->appendChild(itemChild);
        }
        childs.append(child);
    }
    return childs;
}

QVariant MyTreeModel2::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    if (role > m_headers.count())
        return QVariant();

    TreeItem2* item = static_cast<TreeItem2*>(index.internalPointer());

    return item->data(m_headers[index.column()]).toString();
}

Qt::ItemFlags MyTreeModel2::flags(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }

    return QAbstractItemModel::flags(index);
}

QVariant MyTreeModel2::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        return m_rootItem->data(m_headers[section]);
    }

    return QVariant();
}

QModelIndex MyTreeModel2::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
    {
        return QModelIndex();
    }

    TreeItem2* parentItem;

    if (parent.isValid())
    {
        parentItem = static_cast<TreeItem2*>(parent.internalPointer());
    }
    else
    {
        parentItem = m_rootItem;
    }

    TreeItem2* childItem = parentItem->child(row);
    if (childItem)
    {
        return createIndex(row, column, childItem);
    }

    return QModelIndex();
}

QModelIndex MyTreeModel2::parent(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return QModelIndex();
    }

    TreeItem2* childItem = static_cast<TreeItem2*>(index.internalPointer());
    TreeItem2* parentItem = childItem->parent();

    if (parentItem == m_rootItem)
    {
        createIndex(0, 0, parentItem);
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int MyTreeModel2::rowCount(const QModelIndex& parent) const
{
    TreeItem2* parentItem = nullptr;
    if (!parent.isValid())
    {
        if (!m_rootItem)
        {
            return 0;
        }
        parentItem = m_rootItem;
    }
    else
    {
        if (parent.column() > 0)
        {
            return 0;
        }
        parentItem = static_cast<TreeItem2*>(parent.internalPointer());
    }
    if (!parentItem)
    {
        return 0;
    }

    return parentItem->childCount();
}

int MyTreeModel2::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return static_cast<TreeItem2*>(parent.internalPointer())->columnCount();
    }
    return m_headers.count();
}

QRect MyTreeModel2::getRect(const QModelIndex& index)
{
    QRect rect;
    if (!index.isValid())
    {
        return rect;
    }

    TreeItem2* item = static_cast<TreeItem2*>(index.internalPointer());
    rect.setX(item->data("abs_x").toInt());
    rect.setY(item->data("abs_y").toInt());
    rect.setWidth(item->data("width").toInt());
    rect.setHeight(item->data("height").toInt());

    return rect;
}

QJsonObject MyTreeModel2::getData(const QModelIndex& index)
{
    if (!index.isValid())
    {
        return QJsonObject();
    }

    TreeItem2* item = static_cast<TreeItem2*>(index.internalPointer());

    return item->data();
}

QVariantMap MyTreeModel2::getDataVariant(const QModelIndex &index)
{
    return getData(index).toVariantMap();
}

void MyTreeModel2::copyToClipboard(const QModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }

    TreeItem2* item = static_cast<TreeItem2*>(index.internalPointer());

    qGuiApp->clipboard()->setText(item->data("id").toString());
}

QStringList MyTreeModel2::headers() const
{
    return m_headers;
}

QVariantList MyTreeModel2::getChildrenIndexes(TreeItem2* node)
{
    QVariantList indexes;
    TreeItem2* parent = node ? node : m_rootItem;

    for (int i = 0; i != parent->childCount(); ++i)
    {
        TreeItem2* child = parent->child(i);
        indexes.push_back(createIndex(i, 0, reinterpret_cast<quintptr>(child)));

        indexes.append(getChildrenIndexes(child));
    }

    return indexes;
}

QModelIndex MyTreeModel2::searchIndex(const QString& key,
                                      const QVariant& value,
                                      bool partialSearch,
                                      const QModelIndex& currentIndex,
                                      TreeItem2* node)
{
    static bool currentFound = false;
    if (!node)
    {
        currentFound = false;
    }
    if (!currentIndex.isValid())
    {
        currentFound = true;
    }
    TreeItem2* parent = node ? node : m_rootItem;

    QModelIndex index;
    for (int i = 0; i != parent->childCount(); ++i)
    {
        TreeItem2* child = parent->child(i);
        QModelIndex newIndex = createIndex(i, 0, reinterpret_cast<quintptr>(child));

        const bool match =
            child->data(key) == value || (partialSearch && value.metaType() == QMetaType(QMetaType::QString) &&
                                          child->data(key).toString().contains(value.toString()));
        if (match)
            newIndex = createIndex(i, m_headers.indexOf(key), reinterpret_cast<quintptr>(child));

        if (!currentFound && newIndex == currentIndex)
        {
            currentFound = true;
        }
        if (newIndex != currentIndex && match)
        {
            if (currentFound)
            {
                index = newIndex;
                break;
            }
        }

        QModelIndex childIndex = searchIndex(key, value, partialSearch, currentIndex, child);
        if (childIndex.internalPointer())
        {
            index = childIndex;
            break;
        }
    }

    if (!node && currentIndex.isValid() && !index.isValid())
    {
        index = searchIndex(key, value, partialSearch, currentIndex, m_rootItem->child(0));
    }

    return index;
}

QModelIndex MyTreeModel2::searchIndex(SearchType key,
                                      const QVariant& value,
                                      bool partialSearch,
                                      const QModelIndex& currentIndex,
                                      TreeItem2* node)
{
    const QStringList keys {
        QStringLiteral("classname"),
        QStringLiteral("mainTextProperty"),
        QStringLiteral("objectName"),
        QStringLiteral("objectId"),
    };
    const QString sKey = keys[static_cast<std::underlying_type<SearchType>::type>(key)];
    qDebug() << Q_FUNC_INFO << sKey << currentIndex << node;
    return searchIndex(sKey, value, partialSearch, currentIndex, node);
}

QModelIndex MyTreeModel2::searchByCoordinates(qreal posx, qreal posy, TreeItem2* node)
{
    if (!node)
    {
        qDebug() << Q_FUNC_INFO << posx << posy;
    }

    TreeItem2* parent = node ? node : m_rootItem;
    QModelIndex childIndex;

    for (int i = 0; i != parent->childCount(); ++i)
    {
        TreeItem2* child = parent->child(i);

        bool canProcess = true;

        const QVariant activeVariant = child->data(QStringLiteral("active"));
        const bool active = activeVariant.isValid() ? activeVariant.toBool() : true;
        if (!active)
        {
            canProcess = false;
        }

        const QVariant visibleVariant = child->data(QStringLiteral("visible"));
        const bool visible = visibleVariant.isValid() ? visibleVariant.toBool() : true;
        if (!visible)
        {
            canProcess = false;
        }

        const QVariant enabledVariant = child->data(QStringLiteral("enabled"));
        const bool enabled = enabledVariant.isValid() ? enabledVariant.toBool() : true;
        if (!enabled)
        {
            canProcess = false;
        }

        const QVariant opacityVariant = child->data(QStringLiteral("opacity"));
        const float opacity = opacityVariant.isValid() ? opacityVariant.toFloat() : 1.0f;
        if (opacity == 0.0f)
        {
            canProcess = false;
        }

        const QString classname = child->data(QStringLiteral("classname")).toString();
        const int itemx = child->data(QStringLiteral("abs_x")).toInt();
        const int itemy = child->data(QStringLiteral("abs_y")).toInt();
        const int itemw = child->data(QStringLiteral("width")).toInt();
        const int itemh = child->data(QStringLiteral("height")).toInt();

        if (canProcess &&
            !classname.endsWith(QLatin1String("Loader")) &&
            classname != QLatin1String("DeclarativeTouchBlocker") &&
            classname != QLatin1String("QQuickItem") &&
            classname != QLatin1String("RotatingItem") &&
            classname != QLatin1String("QQuickShaderEffect") &&
            classname != QLatin1String("QQuickOverlay") &&
            classname != QLatin1String("QQuickRectangle") &&
            classname != QLatin1String("QQuickMouseArea") &&
            classname != QLatin1String("InformationManager") &&
            classname != QLatin1String("QQuickShaderEffectSource") &&
            classname != QLatin1String("HwcImage") &&
            !classname.endsWith(QLatin1String("Gradient")) &&
            !classname.endsWith(QLatin1String("DropArea")) &&
            !classname.endsWith(QLatin1String("Effect")) &&
            posx >= itemx &&
            posx <= (itemx + itemw) && posy >= itemy && posy <= (itemy + itemh))
        {
            childIndex = createIndex(i, 0, reinterpret_cast<quintptr>(child));
        } else

        if (canProcess &&
            classname.endsWith(QLatin1String("DropArea"))
        ) {
            continue;
        }

        QModelIndex someIndex = searchByCoordinates(posx, posy, child);
        if (someIndex.isValid())
        {
            childIndex = someIndex;
        }
    }

    return childIndex;
}

QModelIndex MyTreeModel2::searchByCoordinates(const QPointF& pos, TreeItem2* node)
{
    return searchByCoordinates(pos.x(), pos.y(), node);
}

TreeItem2::TreeItem2(const QJsonObject& data, TreeItem2* parent)
    : m_data(data)
    , m_parent(parent)
{
}

TreeItem2::~TreeItem2()
{
    qDeleteAll(m_childs);
}

void TreeItem2::appendChild(TreeItem2* child)
{
    m_childs.append(child);
}

TreeItem2* TreeItem2::child(int index)
{
    if (index < 0 || index >= m_childs.count())
    {
        return nullptr;
    }

    return m_childs.value(index);
}

QVector<TreeItem2*> TreeItem2::childs()
{
    return m_childs;
}

int TreeItem2::childCount() const
{
    return m_childs.count();
}

void TreeItem2::genocide()
{
    qDeleteAll(m_childs);
    m_childs.clear();
}

QVariant TreeItem2::data(const QString& roleName) const
{
    return m_data.value(roleName).toVariant();
}

QJsonObject TreeItem2::data() const
{
    return m_data;
}

int TreeItem2::columnCount() const
{
    return m_data.count();
}

int TreeItem2::row()
{
    if (!m_parent)
    {
        return 0;
    }

    return m_parent->row(this);
}

int TreeItem2::row(TreeItem2* child)
{
    return m_childs.indexOf(child);
}

TreeItem2* TreeItem2::parent()
{
    return m_parent;
}
