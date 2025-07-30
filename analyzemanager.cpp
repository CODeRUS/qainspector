#include "analyzemanager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

AnalyzeManager::AnalyzeManager(QObject *parent)
    : QObject{parent}
{}

void AnalyzeManager::analyzeDataAdded(const QString &location)
{
    qDebug() << Q_FUNC_INFO << location;

    QFile pointFile(location + "/point.txt");
    if (!pointFile.open(QIODevice::ReadOnly))
    {
        qWarning() << Q_FUNC_INFO << "Failed to open point file:" << pointFile.fileName();
        return;
    }

    QByteArray pointData = pointFile.readAll();
    pointFile.close();

    if (pointData.isEmpty())
    {
        qWarning() << Q_FUNC_INFO << "Point data is empty in file:" << pointFile.fileName();
        return;
    }

    QStringList pointLines = QString::fromUtf8(pointData).split(',', Qt::SkipEmptyParts);
    if (pointLines.size() < 2)
    {
        qWarning() << Q_FUNC_INFO << "Invalid point data in file:" << pointFile.fileName();
        return;
    }

    QPoint point(pointLines[0].toInt(), pointLines[1].toInt());

    emit dataAdded(point, location);
}

void AnalyzeManager::load()
{
    const auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QDir dirPath(dir);
    if (!dirPath.exists())
    {
        qWarning() << Q_FUNC_INFO << "Data directory does not exist:" << dir;
        return;
    }

    const QStringList files = dirPath.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &file : files)
    {
        const QString location = dirPath.absoluteFilePath(file);
        analyzeDataAdded(location);
    }
}

void AnalyzeManager::remove(const QString &location)
{
    qDebug() << Q_FUNC_INFO << location;

    QDir dir(location);
    if (!dir.exists())
    {
        qWarning() << Q_FUNC_INFO << "Directory does not exist:" << location;
        return;
    }

    if (!dir.removeRecursively())
    {
        qWarning() << Q_FUNC_INFO << "Failed to remove directory:" << location;
    }
}
