#include "client.h"
#include <iostream>
#include <qDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

Client::Client() : clientSocket(INVALID_SOCKET), running(false) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        if (onError) onError("WSAStartup failed.");
    }
}

Client::~Client() {
    stop();
    WSACleanup();
}

bool Client::connectToServer(const std::string& ip, int port) {
    std::lock_guard<std::mutex> lock(socketMutex);
    if (running.load()) {
        if (onError) onError("Client is already running.");
        return false;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        if (onError) onError("Failed to create socket.");
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);
    serverAddr.sin_port = htons(port);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        if (onError) onError("Failed to connect to server.");
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return false;
    }

    running = true;
    if (onConnected) onConnected();

    // Run receive logic on detached background thread to avoid blocking UI
    recvThread = std::thread(&Client::receiveThreadFunc, this);
    recvThread.detach();

    return true;
}

void Client::start() {
    // Not used in this version
}

void Client::stop() {
    if (!running.load()) return;

    running = false;

    std::lock_guard<std::mutex> lock(socketMutex);
    if (clientSocket != INVALID_SOCKET) {
        shutdown(clientSocket, SD_BOTH);
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }

    // We can't join a detached thread, but it will exit automatically due to running=false
    if (onDisconnected) onDisconnected();
}

bool Client::isRunning() const {
    return running.load();
}

void Client::sendMessage(const std::string& msg) {
    std::lock_guard<std::mutex> lock(socketMutex);
    if (running.load() && clientSocket != INVALID_SOCKET) {
        std::string messageWithNewline = msg + "\n";  // Ensure newline delimiter
        send(clientSocket, messageWithNewline.c_str(), static_cast<int>(messageWithNewline.size()), 0);
    }
}


void Client::receiveThreadFunc() {
    char buffer[1024];
    std::string leftover;

    while (running.load()) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived <= 0) {
            running = false;
            if (onDisconnected) onDisconnected();
            break;
        }

        buffer[bytesReceived] = '\0';
        leftover += buffer;

        size_t pos;
        while ((pos = leftover.find('\n')) != std::string::npos) {
            std::string message = leftover.substr(0, pos);
            leftover.erase(0, pos + 1);

            if (onMessageReceived) {
                onMessageReceived(message);  // Existing callback
            }
            qDebug() << "[Client] Received from server:" << QString::fromStdString(message);
            //  emit messageReceived(QString::fromStdString(message));  // Only if you define this signal in a QObject class

            //processJsonMessage(message);  // ✅ Handle JSON data
        }
    }
}

void Client::processJsonMessage(const std::string& messageStr) {
    QString raw = QString::fromStdString(messageStr).trimmed();

    // Find the first '{' to locate start of JSON
    int jsonStart = raw.indexOf('{');
    if (jsonStart == -1) {
        qDebug() << "[Client] No JSON found in message.";
        return;
    }

    QString jsonString = raw.mid(jsonStart);  // Extract from '{' onward

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "[Client] JSON parse error:" << error.errorString();
        return;
    }

    if (!doc.isObject()) {
        qDebug() << "[Client] Message is not a JSON object.";
        return;
    }

    QJsonObject obj = doc.object();
    QString role = obj.value("role").toString();
    QString type = obj.value("type").toString();
    QString id = obj.value("id").toString();
    QString name = obj.value("name").toString();
    QString parentID = obj.value("parentID").toString();

    qDebug() << "[Client] Parsed entity:" << name << ", ID:" << id << ", ParentID:" << parentID;
}


