#include "AppLogModel.h"

#include <QDateTime>

AppLogModel::AppLogModel(QObject *parent) : QAbstractListModel(parent)
{
}

int AppLogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_entries.size();
}

QVariant AppLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }

    const AppLogEntry &entry = m_entries.at(index.row());
    switch (role) {
    case TimeRole:
        return entry.time;
    case SourceRole:
        return entry.source;
    case DirectionRole:
        return entry.direction;
    case TextRole:
        return entry.text;
    case DecodedRole:
        return entry.decoded;
    default:
        return {};
    }
}

QHash<int, QByteArray> AppLogModel::roleNames() const
{
    return {
        {TimeRole, "time"},
        {SourceRole, "source"},
        {DirectionRole, "direction"},
        {TextRole, "text"},
        {DecodedRole, "decoded"}
    };
}

int AppLogModel::count() const
{
    return m_entries.size();
}

void AppLogModel::clear()
{
    if (m_entries.isEmpty()) {
        return;
    }
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

void AppLogModel::append(const QString &source,
                         const QString &direction,
                         const QString &text,
                         const QString &decoded)
{
    if (m_entries.size() >= m_maxEntries) {
        const int removeCount = qMin(100, m_entries.size());
        beginRemoveRows(QModelIndex(), 0, removeCount - 1);
        m_entries.erase(m_entries.begin(), m_entries.begin() + removeCount);
        endRemoveRows();
    }

    const int row = m_entries.size();
    beginInsertRows(QModelIndex(), row, row);
    m_entries.append({QDateTime::currentDateTime().toString("HH:mm:ss.zzz"),
                      source,
                      direction,
                      text,
                      decoded});
    endInsertRows();
    emit countChanged();
}
