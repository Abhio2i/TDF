#include "client.h"
#include <iostream>
#include <QJsonParseError>

#ifdef _WIN32
#define _WIN32_WINNT 0x0600 // Windows Vista or later for inet_pton
#endif

Client::Client() : clientSocket(INVALID_SOCKET_VALUE), running(false) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        if (onError) onError("WSAStartup failed. Error: " + std::to_string(WSAGetLastError()));
    }
#endif
}

Client::~Client() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool Client::connectToServer(const std::string& ip, int port) {
    std::lock_guard<std::mutex> lock(socketMutex);
    if (running.load()) {
        if (onError) onError("Client is already running.");
        return false;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET_VALUE) {
        if (onError) onError("Failed to create socket. Error: " + std::to_string(
#ifdef _WIN32
                        WSAGetLastError()
#else
                        errno
#endif
                        ));
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    // IP address conversion
    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
#ifdef _WIN32
        serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
        if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
#else
        if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
#endif
            if (onError) onError("Invalid IP address format.");
            CLOSE_SOCKET(clientSocket);
            clientSocket = INVALID_SOCKET_VALUE;
            return false;
        }
    }

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR_VALUE) {
        if (onError) onError("Failed to connect to server. Error: " + std::to_string(
#ifdef _WIN32
                        WSAGetLastError()
#else
                        errno
#endif
                        ));
        CLOSE_SOCKET(clientSocket);
        clientSocket = INVALID_SOCKET_VALUE;
        return false;
    }

    running = true;
    if (onConnected) onConnected();

    recvThread = std::thread(&Client::receiveThreadFunc, this);
    return true;
}

void Client::start() {
    // Not used in current implementation, kept for compatibility
}

void Client::stop() {
    if (!running.load()) return;

    running = false;

    {
        std::lock_guard<std::mutex> lock(socketMutex);
        if (clientSocket != INVALID_SOCKET_VALUE) {
            shutdown(clientSocket, SHUTDOWN_BOTH);
            CLOSE_SOCKET(clientSocket);
            clientSocket = INVALID_SOCKET_VALUE;
        }
    }

    if (recvThread.joinable()) {
        recvThread.join();
    }

    if (onDisconnected) onDisconnected();
}

bool Client::isRunning() const {
    return running.load();
}

void Client::sendMessage(const std::string& msg) {
    std::lock_guard<std::mutex> lock(socketMutex);
    if (running.load() && clientSocket != INVALID_SOCKET_VALUE) {
        std::string messageWithNewline = msg + "\n";
        int result = send(clientSocket, messageWithNewline.c_str(), static_cast<int>(messageWithNewline.size()), 0);
        if (result == SOCKET_ERROR_VALUE) {
            if (onError) onError("Failed to send message. Error: " + std::to_string(
#ifdef _WIN32
                            WSAGetLastError()
#else
                            errno
#endif
                            ));
        }
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
            {
                std::lock_guard<std::mutex> lock(socketMutex);
                if (clientSocket != INVALID_SOCKET_VALUE) {
                    shutdown(clientSocket, SHUTDOWN_BOTH);
                    CLOSE_SOCKET(clientSocket);
                    clientSocket = INVALID_SOCKET_VALUE;
                }
            }
            break;
        }

        buffer[bytesReceived] = '\0';
        leftover += buffer;

        size_t pos;
        while ((pos = leftover.find('\n')) != std::string::npos) {
            std::string message = leftover.substr(0, pos);
            leftover.erase(0, pos + 1);

            if (message.empty()) continue;

            if (onMessageReceived) {
                onMessageReceived(message);
            }
            qDebug() << "[Client] Received from server:" << QString::fromStdString(message);
            processJsonMessage(message);
        }
    }
}

void Client::processJsonMessage(const std::string& messageStr) {
    QString raw = QString::fromStdString(messageStr).trimmed();
    int jsonStart = raw.indexOf('{');
    if (jsonStart == -1) {
        qDebug() << "[Client] No JSON found in message.";
        return;
    }

    QString jsonString = raw.mid(jsonStart);
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
