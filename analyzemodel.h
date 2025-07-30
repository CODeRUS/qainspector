#pragma once

#include <QAbstractListModel>

class AnalyzeModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit AnalyzeModel(QObject *parent = nullptr);

protected:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<QVariantMap> m_modelData;
};
