#ifndef APPLOGMODEL_H
#define APPLOGMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>

struct AppLogEntry
{
    QString time;
    QString source;
    QString direction;
    QString text;
    QString decoded;
};

class AppLogModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        TimeRole = Qt::UserRole + 1,
        SourceRole,
        DirectionRole,
        TextRole,
        DecodedRole
    };

    explicit AppLogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;

    Q_INVOKABLE void clear();
    Q_INVOKABLE void append(const QString &source,
                            const QString &direction,
                            const QString &text,
                            const QString &decoded = QString());

signals:
    void countChanged();

private:
    QVector<AppLogEntry> m_entries;
    int m_maxEntries = 1500;
};

#endif // APPLOGMODEL_H
