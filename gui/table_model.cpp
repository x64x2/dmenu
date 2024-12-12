#include "table_model.hpp"

int laiin::TableModel::rowCount(const QModelIndex &) const 
{
    return 200;
}

int laiin::TableModel::columnCount(const QModelIndex &) const 
{
    return 200;
}

QVariant laiin::TableModel::data(const QModelIndex &index, int role) const 
{
    switch (role) {
        case Qt::DisplayRole:
            return QString("%1, %2").arg(index.column()).arg(index.row());
        default:
            break;
    }

    return QVariant();
}

QHash<int, QByteArray> laiin::TableModel::roleNames() const 
{
    return { {Qt::DisplayRole, "display"} };
}
