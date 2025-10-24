#ifndef NETWORKTRANSPORT_H
#define NETWORKTRANSPORT_H

#include "qwebsocket.h"
#include "qwebsocketserver.h"
#include <QUdpSocket>
#include <QHostAddress>
#include <QObject>

class NetworkTransport: public QObject
{
    Q_OBJECT

public:
    NetworkTransport();
    ~NetworkTransport();
    void init(const QString& ip, int port);
    void start(bool server = false);
    void sendMessage(QString message);
    void sendUDPMessage(const QString &message);
    void sendBinaryMessage(QByteArray byteMessage);
    bool isServer();

private slots:
    void readyUDPRead();
    void NewConnection();
    void Connected();
    void Disconnected();
    void ErrorOccurred(const QList<QSslError> &errors);
    void ReceivedMessage(QString message);
    void BinaryMessage(QByteArray byteMessage);

signals:
    void onNewConnection();
    void onConnect();
    void onDisconnect();
    void onErrorOccurred(QString error);
    void onReceivedMessage(QString message);
    void onBinaryMessage(QByteArray byteMessage);
private:
    QWebSocket *m_webSocket = nullptr;
    QWebSocketServer *m_server = nullptr;
    QList<QWebSocket *> m_clients;
    unsigned int port =3000;
    bool Server = false;
    QString address = "localhost";
    QUdpSocket *udpSocket;
    const quint16 DESTINATION_PORT = 1234;
    // Use QHostAddress::LocalHost for same-machine testing
    const QHostAddress DESTINATION_ADDRESS = QHostAddress::LocalHost;
};

#endif // NETWORKTRANSPORT_H
