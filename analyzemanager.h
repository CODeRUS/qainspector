#pragma once

#include <QObject>
#include <QPoint>

class AnalyzeManager : public QObject
{
    Q_OBJECT
public:
    explicit AnalyzeManager(QObject *parent = nullptr);

    void analyzeDataAdded(const QString &location);

    Q_INVOKABLE void load();
    Q_INVOKABLE void remove(const QString &location);

signals:
    void dataAdded(const QPoint &point, const QString &location);
};
