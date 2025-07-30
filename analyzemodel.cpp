#include "analyzemodel.h"

AnalyzeModel::AnalyzeModel(QObject *parent)
    : QAbstractListModel{parent}
{

}

int AnalyzeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0; // No children for non-root indices
    }
    return m_modelData.count(); // Return the number of items in the model
}

QVariant AnalyzeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_modelData.count()) {
        return {}; // Invalid index
    }

    const QVariantMap &item = m_modelData.at(index.row());
    switch (role) {
        case Qt::DisplayRole:
            return item.value("display"); // Assuming 'display' is a key in the map
        case Qt::UserRole:
            return item.value("userData"); // Assuming 'userData' is another key in the map
        default:
            return {}; // Unhandled role
    }
}

QHash<int, QByteArray> AnalyzeModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "display"; // Role for display data
    roles[Qt::UserRole] = "userData"; // Role for user-specific data
    return roles; // Return the defined roles
}
