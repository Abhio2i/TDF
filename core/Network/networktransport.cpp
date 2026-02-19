//Author::Aman Negi
#include "networktransport.h"

NetworkTransport::NetworkTransport(){
    udpSocket = nullptr;
}

NetworkTransport::~NetworkTransport(){

}

/// @brief Handle incoming UDP datagrams and forward them as binary messages
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
        //qDebug() << "Content:" << QString::fromUtf8(datagram);
        //emit onReceivedMessage( QString::fromUtf8(datagram));
        emit onBinaryMessage(datagram);

    }
}

/// @brief Send a text message as a UDP datagram to all connected clients
/// @param message The UTF-8 encoded text message to transmit
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

/// @brief Send a binary datagram (UDP) to all connected clients
/// @param byteMessage Binary data to send (already serialized or marshalled)
void NetworkTransport::sendBinaryUDPMessage(const QByteArray &byteMessage)
{
    if (!udpSocket)
    {
        qWarning() << "UDP socket not initialized. Cannot send binary message.";
        return;
    }

    // Iterate through connected WebSocket clients (used for their peer info)
    for (QWebSocket *client : m_clients)
    {
        // CRITICAL CHECK: Ensure valid client with proper connection state
        if (client && (client->state() == QAbstractSocket::ConnectedState))
        {
            // Write the binary datagram to the destination (UDP)
            qint64 bytesSent = udpSocket->writeDatagram(
                byteMessage,
                client->peerAddress(),
                DESTINATION_PORT
                );

            // Logging and error handling
            if (bytesSent == -1)
            {
                qWarning() << "Error sending binary datagram:"
                           << udpSocket->errorString();
            }
            else
            {
                qDebug() << "Sent binary message (" << bytesSent << "bytes) to"
                         << client->peerAddress().toString() << ":" << DESTINATION_PORT;
            }
        }
        else
        {
            qWarning() << "Skipping client: WebSocket not connected or invalid";
        }
    }
}

bool NetworkTransport::isServer(){
    return Server;
}

/// @brief Initialize and start the network transport in either server or client mode
/// @param server If true, starts a WebSocket server; if false, starts a WebSocket client
void NetworkTransport::start(bool server){
    if(m_server||m_webSocket)return;
    // now we are in the correct thread
    udpSocket = new QUdpSocket(this);
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

/// @brief Configure the network transport with target IP address and port
/// @param ip IP address of the remote host or server
/// @param por Port number to use for the WebSocket connection
void NetworkTransport::init(const QString& ip, int por){
    address = ip;
    port = por;
}

/// @brief Send a text message over WebSocket to the server or all connected clients
/// @param message The UTF-8 encoded text message to transmit
void NetworkTransport::sendMessage(const QString &message){
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

/// @brief Send a binary message over WebSocket to the server or all connected clients
/// @param byteMessage Binary data to transmit
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
                qDebug() << "Sent message (" << byteMessage.size() << "bytes) to"
                         << DESTINATION_ADDRESS.toString() << ":" << DESTINATION_PORT;
                client->sendBinaryMessage(byteMessage);
            }
        }
    }
}

/// @brief Handle a new incoming WebSocket client connection
void NetworkTransport::NewConnection(){
    // The server has a new connection, accept it and get the client socket
    QWebSocket *pSocket = m_server->nextPendingConnection();
    ;
    qDebug() << "Client connected from:" << pSocket->peerAddress().toString();
    // 1. Connect signal to handle incoming text messages from the client
    // We connect to a generic slot like 'processTextMessage'
    connect(pSocket, &QWebSocket::textMessageReceived, this, &NetworkTransport::ReceivedMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &NetworkTransport::BinaryMessage);
    // 2. Connect signal to know when the client disconnects
    // We connect to a slot like 'socketDisconnected' which can clean up resources
    connect(pSocket, &QWebSocket::disconnected, this, &NetworkTransport::Disconnected);

    emit onNewConnection(pSocket);//chnaged by Aman
    // If you were tracking multiple clients, you would add pSocket to a list here,
    m_clients.append(pSocket);
}

/// @brief Handle successful WebSocket connection
void NetworkTransport::Connected(){
    emit onConnect();
}

/// @brief Handle WebSocket disconnection event
void NetworkTransport::Disconnected(){
    emit onDisconnect();
}

/// @brief Handle and report WebSocket SSL errors
/// @param errors List of SSL errors reported by the WebSocket connection
void NetworkTransport::ErrorOccurred(const QList<QSslError> &errors){
    qWarning() << "WebSocket SSL Errors Occurred:";
    // Loop through the list to print all errors
    for (const QSslError &error : errors) {
        qWarning() << " - Error String:" << error.errorString();
        emit onErrorOccurred("Error String:" + error.errorString());
    }
}

/// @brief Handle an incoming WebSocket text message
/// @param message The received UTF-8 encoded text message
void NetworkTransport::ReceivedMessage(QString message){
    qDebug()<<"Recive Text:" << message;
    emit onReceivedMessage(message);
}

/// @brief Handle an incoming WebSocket binary message
/// @param byteMessage The received binary data
void NetworkTransport::BinaryMessage(QByteArray byteMessage){
    qDebug()<<"Recive byte:" << byteMessage;
    emit onBinaryMessage(byteMessage);
}
