// Copyright (c) 2019-2020 Open Mobile Platform LLC.
#include "socketconnector.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>

SocketConnector::SocketConnector(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
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
                m_socket->readAll();

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

QString SocketConnector::getDumpTree()
{
    QJsonObject json;
    json.insert(QStringLiteral("cmd"), QJsonValue(QStringLiteral("action")));
    json.insert(QStringLiteral("action"), QJsonValue(QStringLiteral("execute")));
    json.insert(
        QStringLiteral("params"),
        QJsonValue::fromVariant(QVariantList{QStringLiteral("app:dumpTree"), QVariantList{}}));
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
        const QByteArray img =
            QByteArray::fromBase64(replyObject.value(QStringLiteral("value")).toString().toUtf8());
        qDebug() << Q_FUNC_INFO << img.size();
        return img;
    }
    return {};
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
