#include "analyzemanager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStandardPaths>

AnalyzeManager::AnalyzeManager(QObject *parent)
    : QObject{parent}
{}

void AnalyzeManager::analyzeDataAdded(const QString &location)
{
    qDebug() << Q_FUNC_INFO << location;

    QFile pointFile(location + "/point.json");
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

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(pointData, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning() << Q_FUNC_INFO << "Failed to parse point data:" << parseError.errorString();
        return;
    }
    if (!doc.isObject())
    {
        qWarning() << Q_FUNC_INFO << "Point data is not an object:" << pointData;
        return;
    }
    QVariantMap point = doc.object().toVariantMap();
    point.insert("location", location);
    if (!point.contains("id"))
    {
        point.insert("id", "");
    }

    emit dataAdded(point);
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

void AnalyzeManager::refine(const QString &location, const QString &id)
{
    qDebug() << Q_FUNC_INFO << location << id;

    QFile pointFile(location + "/point.json");
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

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(pointData, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning() << Q_FUNC_INFO << "Failed to parse point data:" << parseError.errorString();
        return;
    }

    QJsonObject pointObject = doc.object();
    pointObject.insert("id", id);

    QFile outFile(location + "/point.json");
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qWarning() << Q_FUNC_INFO << "Failed to open output file:" << outFile.fileName();
        return;
    }

    outFile.write(QJsonDocument(pointObject).toJson());


}
