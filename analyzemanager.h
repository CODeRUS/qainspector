#pragma once

#include <QObject>

class AnalyzeManager : public QObject
{
    Q_OBJECT
public:
    explicit AnalyzeManager(QObject *parent = nullptr);

signals:
};
