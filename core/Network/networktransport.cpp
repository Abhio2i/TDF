#include "networktransport.h"

NetworkTransport::NetworkTransport(){
    udpSocket = new QUdpSocket(this);
}

NetworkTransport::~NetworkTransport(){

}

void NetworkTransport::readyUDPRead()
{
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());

        QHostAddress senderAddress;
        quint16 senderPort;

        // Read the datagram and get the sender info
        udpSocket->readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderPort);

        qDebug() << "Message from:" << senderAddress.toString() << ":" << senderPort;
        qDebug() << "Content:" << QString::fromUtf8(datagram);
        emit onReceivedMessage( QString::fromUtf8(datagram));
    }
}

void NetworkTransport::sendUDPMessage(const QString &message)
{
    QByteArray datagram = message.toUtf8();

    for (QWebSocket *client : m_clients)
    {
        // CRITICAL CHECK: Ensure the socket is connected before sending data
        if (client && (client->state() == QAbstractSocket::ConnectedState))
        {
            qint64 bytesSent = udpSocket->writeDatagram(
                datagram,
                client->peerAddress(),
                DESTINATION_PORT
                );

            if (bytesSent == -1) {
                qDebug() << "Error sending datagram:" << udpSocket->errorString();
            } else {
                qDebug() << "Sent message (" << bytesSent << "bytes) to"
                         << DESTINATION_ADDRESS.toString() << ":" << DESTINATION_PORT;
            }

        }
    }

}


bool NetworkTransport::isServer(){
    return Server;
}

void NetworkTransport::start(bool server){
    if(m_server||m_webSocket)return;
    if(server){
        m_server = new QWebSocketServer(QStringLiteral("My Server"), QWebSocketServer::NonSecureMode    );

        if (m_server->listen(QHostAddress::Any, port)) {
            qDebug() << "WebSocket server listening on port "<<port;
            // Connect the server to a slot to handle new incoming connections
            connect(m_server, &QWebSocketServer::newConnection, this, &NetworkTransport::NewConnection); // Assuming onNewConnection is added to mainwindow.h
            Server = server;
        } else {
            qWarning() << "Could not start server.";
        }
    }else{
        m_webSocket = new QWebSocket();

        // Connect the client to slots to monitor its state
        connect(m_webSocket, &QWebSocket::sslErrors, this, &NetworkTransport::ErrorOccurred);
        connect(m_webSocket, &QWebSocket::textMessageReceived, this, &NetworkTransport::ReceivedMessage);
        connect(m_webSocket, &QWebSocket::binaryMessageReceived, this, &NetworkTransport::BinaryMessage);
        connect(m_webSocket, &QWebSocket::connected, this, &NetworkTransport::Connected);     // Assuming onConnected is added to mainwindow.h
        connect(m_webSocket, &QWebSocket::disconnected, this, &NetworkTransport::Disconnected); // Assuming onDisconnected is added to mainwindow.h

        QString urlString = QString("ws://%1:%2").arg(address).arg(port);
        m_webSocket->open(QUrl(urlString));

        // Bind to the port
        if (udpSocket->bind(QHostAddress::Any, DESTINATION_PORT,QUdpSocket::ShareAddress)) {
            qDebug() << "UDP Receiver listening on port" << DESTINATION_PORT;
            // Connect readyRead signal to a slot
            connect(udpSocket, &QUdpSocket::readyRead, this, &NetworkTransport::readyUDPRead);
        } else {
            qDebug() << "Failed to bind socket:" << udpSocket->errorString();
        }
    }
}

void NetworkTransport::init(const QString& ip, int por){
    address = ip;
    port = por;
}

void NetworkTransport::sendMessage(QString message){
    if(m_webSocket){
        // CRITICAL: Check if the socket is in the ConnectedState
        if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
            m_webSocket->sendTextMessage(message);
        } else {
            // Optional: Debugging message if not connected
            qWarning() << "Cannot send message: WebSocket is not connected (Current state:"
                       << m_webSocket->state() << ")";
        }
    }
    if(m_server){
        for (QWebSocket *client : m_clients)
        {
            // CRITICAL CHECK: Ensure the socket is connected before sending data
            if (client && (client->state() == QAbstractSocket::ConnectedState))
            {
                client->sendTextMessage(message);
            }
        }
    }
}

void NetworkTransport::sendBinaryMessage(QByteArray byteMessage){
    if(m_webSocket){
        // CRITICAL: Check if the socket is in the ConnectedState
        if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
            m_webSocket->sendBinaryMessage(byteMessage);
        } else {
            // Optional: Debugging message if not connected
            qWarning() << "Cannot send message: WebSocket is not connected (Current state:"
                       << m_webSocket->state() << ")";
        }
    }
    if(m_server){
        for (QWebSocket *client : m_clients)
        {
            // CRITICAL CHECK: Ensure the socket is connected before sending data
            if (client && (client->state() == QAbstractSocket::ConnectedState))
            {
                client->sendBinaryMessage(byteMessage);
            }
        }
    }
}

void NetworkTransport::NewConnection(){
    // The server has a new connection, accept it and get the client socket
    QWebSocket *pSocket = m_server->nextPendingConnection();

    qDebug() << "Client connected from:" << pSocket->peerAddress().toString();
    // 1. Connect signal to handle incoming text messages from the client
    // We connect to a generic slot like 'processTextMessage'
    connect(pSocket, &QWebSocket::textMessageReceived, this, &NetworkTransport::ReceivedMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &NetworkTransport::BinaryMessage);
    // 2. Connect signal to know when the client disconnects
    // We connect to a slot like 'socketDisconnected' which can clean up resources
    connect(pSocket, &QWebSocket::disconnected, this, &NetworkTransport::Disconnected);

    emit onNewConnection();
    // If you were tracking multiple clients, you would add pSocket to a list here,
    m_clients.append(pSocket);
}


void NetworkTransport::Connected(){
    emit onConnect();
}

void NetworkTransport::Disconnected(){
    emit onDisconnect();
}

void NetworkTransport::ErrorOccurred(const QList<QSslError> &errors){
    qWarning() << "WebSocket SSL Errors Occurred:";
    // Loop through the list to print all errors
    for (const QSslError &error : errors) {
        qWarning() << " - Error String:" << error.errorString();
        emit onErrorOccurred("Error String:" + error.errorString());
    }
}

void NetworkTransport::ReceivedMessage(QString message){
    qDebug()<<"Recive Text:" << message;
    emit onReceivedMessage(message);
}

void NetworkTransport::BinaryMessage(QByteArray byteMessage){
    qDebug()<<"Recive byte:" << byteMessage;
    emit onBinaryMessage(byteMessage);
}
