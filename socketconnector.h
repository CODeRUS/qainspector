// Copyright (c) 2019-2020 Open Mobile Platform LLC.
#ifndef SOCKETCONNECTOR_H
#define SOCKETCONNECTOR_H

#include <QElapsedTimer>
#include <QObject>
#include <QPoint>
#include <QVector>

class QTcpSocket;
class SocketConnector : public QObject
{
    Q_OBJECT
public:
    explicit SocketConnector(QObject* parent = nullptr);

    Q_PROPERTY(bool connected READ isConnected WRITE setConnected NOTIFY connectedChanged)
    bool isConnected() const;
    void setConnected(bool connected);

    Q_PROPERTY(QString hostname MEMBER m_hostName NOTIFY hostnameChanged)
    Q_PROPERTY(QString port MEMBER m_hostPort NOTIFY portChanged)
    Q_PROPERTY(QString applicationName MEMBER m_applicationName NOTIFY applicationNameChanged)

public slots:
    QString getDumpTree(const QString &filter = {});
    QByteArray getGrabWindow();

    void mousePressed(const QPoint &p);
    void mouseReleased(const QPoint &p);
    void mouseMoved(const QPoint &p);

signals:
    void connectedChanged(bool connected);
    void hostnameChanged();
    void portChanged();
    void applicationNameChanged();

    void imageData(const QString &b64);

private:
    QTcpSocket* m_socket;
    QString m_hostName;
    QString m_hostPort;
    QString m_applicationName;

    QVector<QPoint> m_points;
    QElapsedTimer m_timer;
};

#endif // SOCKETCONNECTOR_H
