#include "socketconnector.h"

#include <QFile>
#include <QJsonDocument>
#include <QTcpSocket>

SocketConnector::SocketConnector(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, [this](){
        QJsonObject json;
        json.insert(QStringLiteral("cmd"), QJsonValue(QStringLiteral("action")));
        json.insert(QStringLiteral("action"), QJsonValue(QStringLiteral("initialize")));
        json.insert(QStringLiteral("params"), QJsonValue::fromVariant(QStringList( { m_applicationName })));
        const QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);

        m_socket->write(data);
        m_socket->write(QByteArrayLiteral("\n"));
        m_socket->waitForBytesWritten();

        m_socket->waitForReadyRead(500);
        m_socket->readAll();

        qWarning() << Q_FUNC_INFO << "connected";
        emit connectedChanged(true);
    });
    connect(m_socket, &QTcpSocket::disconnected, [this](){
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

    if (!connected && lastConnected) {
        m_socket->close();
    } else if (connected && !lastConnected) {
        m_socket->connectToHost(m_hostName, m_hostPort.toUShort());
        qDebug() << Q_FUNC_INFO << "Connecting:" << m_socket->waitForConnected(3000);
    }

    qDebug() << Q_FUNC_INFO << "Set connect:" << connected << "Connected:" << isConnected();
}

void SocketConnector::getDumpPage(QJSValue callback)
{
    QJsonObject json;

    json.insert(QStringLiteral("cmd"), QJsonValue(QStringLiteral("action")));
    json.insert(QStringLiteral("action"), QJsonValue(QStringLiteral("execute")));
    json.insert(QStringLiteral("params"), QJsonValue::fromVariant(QVariantList{QStringLiteral("app:dumpCurrentPage"), QVariantList{}} ));
    const QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);

    m_socket->write(data);
    m_socket->write(QByteArrayLiteral("\n"));
    m_socket->waitForBytesWritten();

    QByteArray replyData;
    QJsonDocument replyDoc;
    QJsonParseError error;
    error.error = QJsonParseError::UnterminatedObject;

    while (error.error != QJsonParseError::NoError) {
        if (!m_socket->waitForReadyRead(10000)) {
            qWarning() << Q_FUNC_INFO << "Timeout" << error.error << error.errorString();
            return;
        }
        const QString readData = m_socket->readLine();
        replyData.append(readData);
        replyDoc = QJsonDocument::fromJson(replyData, &error);
    }
    QJsonObject replyObject = replyDoc.object();

    const QString dump = replyObject.value(QStringLiteral("value")).toString();

    if (replyObject.contains(QStringLiteral("status")) && replyObject.value(QStringLiteral("status")).toInt() == 0) {
        if (callback.isCallable()) {
            callback.call({ QJSValue(dump) });
        }
    }
}

void SocketConnector::getDumpTree(QJSValue callback)
{
    QJsonObject json;
    json.insert(QStringLiteral("cmd"), QJsonValue(QStringLiteral("action")));
    json.insert(QStringLiteral("action"), QJsonValue(QStringLiteral("execute")));
    json.insert(QStringLiteral("params"), QJsonValue::fromVariant( QVariantList{QStringLiteral("app:dumpTree"), QVariantList{}} ));
    const QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);

    m_socket->write(data);
    m_socket->write(QByteArrayLiteral("\n"));
    m_socket->waitForBytesWritten();

    QByteArray replyData;
    QJsonDocument replyDoc;
    QJsonParseError error;
    error.error = QJsonParseError::UnterminatedObject;

    replyDoc = QJsonDocument::fromJson(replyData, &error);

    while (error.error != QJsonParseError::NoError) {
        if (!m_socket->waitForReadyRead(10000)) {
            qWarning() << Q_FUNC_INFO << "Timeout" << error.error << error.errorString();
            return;
        }
        replyData.append(m_socket->readAll());
        replyDoc = QJsonDocument::fromJson(replyData, &error);
    }

    QJsonObject replyObject = replyDoc.object();
    if (replyObject.contains(QStringLiteral("status")) && replyObject.value(QStringLiteral("status")).toInt() == 0) {
        if (callback.isCallable()) {
            callback.call({ QJSValue(replyObject.value(QStringLiteral("value")).toString()) });
        }
    }
}

void SocketConnector::getGrabWindow(QJSValue callback)
{

    QJsonObject json;
    json.insert(QStringLiteral("cmd"), QJsonValue(QStringLiteral("action")));
    json.insert(QStringLiteral("action"), QJsonValue(QStringLiteral("getScreenshot")));
    json.insert(QStringLiteral("params"), QJsonValue(QString()));
    const QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);

    m_socket->write(data);
    m_socket->write(QByteArrayLiteral("\n"));
    m_socket->waitForBytesWritten();

    QByteArray replyData;
    QJsonDocument replyDoc;
    QJsonParseError error;
    error.error = QJsonParseError::UnterminatedObject;

    replyDoc = QJsonDocument::fromJson(replyData, &error);

    while (error.error != QJsonParseError::NoError) {
        if (!m_socket->waitForReadyRead(10000)) {
            qWarning() << Q_FUNC_INFO << "Timeout" << error.error << error.errorString();
            return;
        }
        replyData.append(m_socket->readAll());
        replyDoc = QJsonDocument::fromJson(replyData, &error);
    }

    QJsonObject replyObject = replyDoc.object();
    if (replyObject.contains(QStringLiteral("status")) && replyObject.value(QStringLiteral("status")).toInt() == 0) {

        QFile file("dump.png");
        if (file.open(QFile::WriteOnly)) {
            const QByteArray data = QByteArray::fromBase64(replyObject.value(QStringLiteral("value")).toString().toUtf8());
            qDebug() << Q_FUNC_INFO << file.write(data);
            file.close();
        }

        if (callback.isCallable()) {
            callback.call({ QJSValue(!data.isEmpty()) });
        }
    }
}
