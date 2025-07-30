// Copyright (c) 2019-2020 Open Mobile Platform LLC.
#include "socketconnector.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTcpSocket>
#include <QStandardPaths>
#include <QDir>

SocketConnector::SocketConnector(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_manager(new AnalyzeManager(this))
{
    connect(m_socket,
            &QTcpSocket::connected,
            [this]()
            {
                QJsonObject json;
                json.insert(QStringLiteral("cmd"), QJsonValue(QStringLiteral("action")));
                json.insert(QStringLiteral("action"), QJsonValue(QStringLiteral("initialize")));
                json.insert(QStringLiteral("params"),
                            QJsonValue::fromVariant(QStringList({m_applicationName})));
                const QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);

                m_socket->write(data);
                m_socket->write("\n", 1);
                m_socket->waitForBytesWritten();

                m_socket->waitForReadyRead(500);
                qDebug().noquote() << m_socket->readAll();

                qWarning() << Q_FUNC_INFO << "connected";
                emit connectedChanged(true);
            });
    connect(m_socket,
            &QTcpSocket::disconnected,
            [this]()
            {
                qWarning() << Q_FUNC_INFO << "disconnected";
                emit connectedChanged(false);
            });
}

bool SocketConnector::isConnected() const
{
    return m_socket->state() == QTcpSocket::ConnectedState;
}

void SocketConnector::setConnected(bool connected)
{
    qDebug() << Q_FUNC_INFO << m_hostName << m_hostPort;
    const bool lastConnected = isConnected();

    if (!connected && lastConnected)
    {
        m_socket->close();
    }
    else if (connected && !lastConnected)
    {
        m_socket->connectToHost(m_hostName, m_hostPort.toUShort());
        qDebug() << Q_FUNC_INFO << "Connecting:" << m_socket->waitForConnected(3000);
    }

    qDebug() << Q_FUNC_INFO << "Set connect:" << connected << "Connected:" << isConnected();
}

QString SocketConnector::getDumpTree(const QString &filter)
{
    qDebug() << filter;

    QJsonDocument filterDoc = QJsonDocument::fromJson(filter.toUtf8());

    QJsonObject json
    {
        { "cmd", "action" },
        { "action", "execute" },
        { "params", {
            { "app:dumpTreeFilter",  QJsonArray{{ filterDoc.array() }} }
        }}
    };
    const QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);

    qDebug().noquote() << QJsonDocument(json).toJson();

    m_socket->write(data);
    m_socket->write("\n", 1);
    m_socket->waitForBytesWritten();

    QByteArray replyData;
    QJsonDocument replyDoc;
    QJsonParseError error;
    error.error = QJsonParseError::UnterminatedObject;

    replyDoc = QJsonDocument::fromJson(replyData, &error);

    while (error.error != QJsonParseError::NoError)
    {
        if (!m_socket->waitForReadyRead(-1))
        {
            qWarning() << Q_FUNC_INFO << "Timeout" << error.error << error.errorString();
            qDebug().noquote() << replyData;
            return QString();
        }
        replyData.append(m_socket->readAll());
        replyDoc = QJsonDocument::fromJson(replyData, &error);
    }

    QJsonObject replyObject = replyDoc.object();
    if (replyObject.contains(QStringLiteral("status")) &&
        replyObject.value(QStringLiteral("status")).toInt() == 0)
    {
        QByteArray data = qUncompress(QByteArray::fromBase64(
            replyObject.value(QStringLiteral("value")).toString().toLatin1()));
        return QString::fromUtf8(data);
    }
    return QString();
}

QByteArray SocketConnector::getGrabWindow()
{
    QJsonObject json;
    json.insert(QStringLiteral("cmd"), QJsonValue(QStringLiteral("action")));
    json.insert(QStringLiteral("action"), QJsonValue(QStringLiteral("getScreenshot")));
    json.insert(QStringLiteral("params"), QJsonValue(QString()));
    const QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);

    m_socket->write(data);
    m_socket->write("\n", 1);
    m_socket->waitForBytesWritten();

    QByteArray replyData;
    QJsonDocument replyDoc;
    QJsonParseError error;
    error.error = QJsonParseError::UnterminatedObject;

    replyDoc = QJsonDocument::fromJson(replyData, &error);

    while (error.error != QJsonParseError::NoError)
    {
        if (!m_socket->waitForReadyRead(-1))
        {
            qWarning() << Q_FUNC_INFO << "Timeout" << error.error << error.errorString();
            qDebug().noquote() << replyData;
            return {};
        }
        replyData.append(m_socket->readAll());
        replyDoc = QJsonDocument::fromJson(replyData, &error);
    }

    QJsonObject replyObject = replyDoc.object();
    if (replyObject.contains(QStringLiteral("status")) &&
        replyObject.value(QStringLiteral("status")).toInt() == 0)
    {
        const auto b64 = replyObject.value(QStringLiteral("value")).toString();
        emit imageData(b64);
        const QByteArray img =
            QByteArray::fromBase64(b64.toUtf8());
        qDebug() << Q_FUNC_INFO << img.size();
        return img;
    }
    return {};
}

void SocketConnector::startAnalyze()
{
    QJsonObject json
    {
        { "cmd", "action" },
        { "action", "startAnalyze" },
        { "params", "" }
    };
    const auto data = QJsonDocument(json).toJson(QJsonDocument::Compact);

    m_socket->write(data);
    m_socket->write("\n", 1);
    m_socket->waitForBytesWritten();

    connect(m_socket, &QTcpSocket::readyRead, this, &SocketConnector::onDataAvailable, Qt::UniqueConnection);
}

void SocketConnector::stopAnalyze()
{
    disconnect(m_socket, &QTcpSocket::readyRead, this, &SocketConnector::onDataAvailable);

    QJsonObject json
    {
        { "cmd", "action" },
        { "action", "stopAnalyze" },
        { "params", "" }
    };
    const auto data = QJsonDocument(json).toJson(QJsonDocument::Compact);

    m_socket->write(data);
    m_socket->write("\n", 1);
    m_socket->waitForBytesWritten();
}

void SocketConnector::onDataAvailable()
{
    disconnect(m_socket, &QTcpSocket::readyRead, this, &SocketConnector::onDataAvailable);

    QString location;

    QByteArray buf;

    qDebug() << Q_FUNC_INFO << m_socket->bytesAvailable();
    while (m_socket->bytesAvailable() > 0 || m_socket->waitForReadyRead(1000)) {
        const auto data = m_socket->readLine();
        qDebug() << "read line:" << data.size();
        qDebug().noquote() << data;

        if (data.startsWith("pressed:")) {
            const auto msecs = QDateTime::currentMSecsSinceEpoch();
            const auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            const auto current = QString::number(msecs);

            QDir dirPath(dir);
            dirPath.mkpath(current);

            location = dirPath.absoluteFilePath(current);

            qDebug() << "Created location:" << location;

            QString pointStr = data.mid(9).trimmed();
            QJsonObject json {
                { "x", pointStr.section(',', 0, 0).toInt() },
                { "y", pointStr.section(',', 1, 1).toInt() },
            };
            QFile pointFile(location + "/point.json");
            if (pointFile.open(QIODevice::WriteOnly)) {
                pointFile.write(QJsonDocument(json).toJson(QJsonDocument::Compact));
                pointFile.close();
            } else {
                qWarning() << Q_FUNC_INFO << "Failed to open file for writing:" << pointFile.fileName();
            }
        } else if (data.startsWith("dump start:")) {
            buf.clear();
        } else if (data.startsWith("dump end")) {
            qDebug() << Q_FUNC_INFO << "Dump end, size:" << buf.size();
            QFile dumpFile(location + "/dump.json");
            if (dumpFile.open(QIODevice::WriteOnly)) {
                dumpFile.write(qUncompress(buf));
                dumpFile.close();
            } else {
                qWarning() << Q_FUNC_INFO << "Failed to open file for writing:" << dumpFile.fileName();
            }
            buf.clear();
        } else if (data.startsWith("screen start:")) {
            buf.clear();
        } else if (data.startsWith("screen end")) {
            qDebug() << Q_FUNC_INFO << "Screen end, size:" << buf.size();
            QFile screenFile(location + "/screenshot.png");
            if (screenFile.open(QIODevice::WriteOnly)) {
                screenFile.write(qUncompress(buf));
                screenFile.close();
            } else {
                qWarning() << Q_FUNC_INFO << "Failed to open file for writing:" << screenFile.fileName();
            }
            buf.clear();
        } else {
            buf.append(data);
        }
    }

    m_manager->analyzeDataAdded(location);

    connect(m_socket, &QTcpSocket::readyRead, this, &SocketConnector::onDataAvailable, Qt::UniqueConnection);
}

void SocketConnector::analyzeData(const QByteArray &data)
{
    qDebug() << Q_FUNC_INFO << data.size();
    if (data.startsWith("pressed:")) {

    }
    // qDebug().noquote() << data;
}

void SocketConnector::mousePressed(const QPoint &p)
{
    m_points = {p};
    m_timer.start();
}

void SocketConnector::mouseReleased(const QPoint &p)
{
    qint64 elapsed = m_timer.elapsed();

    QByteArray data;

    if (m_points.size() == 1) {
        QString action = QStringLiteral("app:click");
        if (elapsed > 700) {
            action = QStringLiteral("app:pressAndHold");
        }
        QJsonObject json;
        json.insert(QStringLiteral("cmd"), QJsonValue(QStringLiteral("action")));
        json.insert(QStringLiteral("action"), QJsonValue(QStringLiteral("execute")));
        json.insert(
            QStringLiteral("params"),
            QJsonValue::fromVariant(QVariantList{action, QVariantList{p.x(), p.y()}}));
        data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    } else {
        QPoint fp = m_points.first();

        QJsonObject json;
        json.insert(QStringLiteral("cmd"), QJsonValue(QStringLiteral("action")));
        json.insert(QStringLiteral("action"), QJsonValue(QStringLiteral("execute")));
        json.insert(
            QStringLiteral("params"),
            QJsonValue::fromVariant(QVariantList{QStringLiteral("app:move"), QVariantList{fp.x(), fp.y(), p.x(), p.y()}}));
        data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    }

    if (data.isEmpty()) {
        return;
    }

    m_socket->write(data);
    m_socket->write("\n", 1);
    m_socket->waitForBytesWritten();

    m_socket->waitForReadyRead(5000);
    m_socket->readAll();
}

void SocketConnector::mouseMoved(const QPoint &p)
{
    m_points.append(p);
}

AnalyzeManager *SocketConnector::manager()
{
    return m_manager;
}
