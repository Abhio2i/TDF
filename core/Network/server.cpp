#include "server.h"
#include <iostream>
#include <qDebug>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
//#include <QDebug>
#include <QString>
Server::Server() : serverSocket(INVALID_SOCKET), running(false) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

Server::~Server() {
    stop();
    WSACleanup();
}

void Server::consoleInputHandler() {
    std::string input;
    while (true) {
        std::getline(std::cin, input);

        if (input.empty()) continue;

        if (input.rfind("@", 0) == 0) {
            // Format: @192.168.0.158 Hello there!
            size_t spacePos = input.find(' ');
            if (spacePos != std::string::npos) {
                std::string ip = input.substr(1, spacePos - 1);
                std::string msg = input.substr(spacePos + 1);
                sendToClient(ip, msg);
            } else {
                std::cout << "[Usage] @<IP> <message>\n";
            }
        } else {
            broadcast(input, "Server");  // Broadcast to all clients
        }
    }
}


std::string getLocalIP() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) return "127.0.0.1";
    struct hostent* host = gethostbyname(hostname);
    if (host == nullptr) return "127.0.0.1";

    struct in_addr addr;
    memcpy(&addr, host->h_addr_list[0], sizeof(struct in_addr));
    return inet_ntoa(addr);
}
bool Server::start(int port) {
    std::lock_guard<std::mutex> lock(stateMutex);
    qDebug() << "[Server] start() called on port:" << port;

    if (running) {
        if (onError) onError("Server is already running.");
        return false;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        if (onError) onError("Failed to create socket. Error: " + std::to_string(WSAGetLastError()));
        return false;
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        if (onError) onError("setsockopt(SO_REUSEADDR) failed. Error: " + std::to_string(WSAGetLastError()));
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
        return false;
    }

    linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 0;
    setsockopt(serverSocket, SOL_SOCKET, SO_LINGER, (char*)&so_linger, sizeof(so_linger));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        if (onError) onError("Bind failed. Error: " + std::to_string(WSAGetLastError()));
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
        return false;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        if (onError) onError("Listen failed. Error: " + std::to_string(WSAGetLastError()));
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
        return false;
    }

    running = true;
    if (onStart) onStart();

    qDebug() << "[Server] Server successfully started.";

    acceptThread = std::thread(&Server::acceptLoop, this);
    acceptThread.detach();

    return true;

    if (!running && onError) {
        onError("Unknown failure: Server did not start.");
    } else if (!running) {
        std::cerr << "[Server] Unknown failure without error callback.\n";
    }
    return false;

}




void Server::acceptLoop() {
    while (running.load()) {
        sockaddr_in clientAddr{};
        int clientSize = sizeof(clientAddr);
        SOCKET clientSock = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
        if (clientSock == INVALID_SOCKET) {
            if (running && onError) {
                onError("Accept failed. Error: " + std::to_string(WSAGetLastError()));
            }
            continue;
        }

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);

        {
            std::lock_guard<std::mutex> lock(clientMutex);
            clients[clientSock] = std::string(ipStr);  // ✅ SOCKET -> IP
        }


        if (onClientConnected) onClientConnected(ipStr);

        std::thread(&Server::clientHandlerThread, this, clientSock).detach();
    }
}


void Server::stop() {
    if (!running.load()) return;

    running = false;

    {
        std::lock_guard<std::mutex> lock(stateMutex);
        if (serverSocket != INVALID_SOCKET) {
            shutdown(serverSocket, SD_BOTH);
            closesocket(serverSocket);
            serverSocket = INVALID_SOCKET;
        }
    }

    {
        std::lock_guard<std::mutex> lock(clientMutex);
        for (auto& [sock, ip] : clients) {
            shutdown(sock, SD_BOTH);
            closesocket(sock);
        }
        clients.clear();
    }

    if (onStop) onStop();
}

void Server::clientHandlerThread(SOCKET clientSock) {
    char buffer[1024];
    std::string clientIP;

    {
        std::lock_guard<std::mutex> lock(clientMutex);
        for (const auto& [sock, ip] : clients) {
            if (sock == clientSock) {
                clientIP = ip;
                break;
            }
        }
    }

    std::string leftover;

    while (running.load()) {
        memset(buffer, 0, sizeof(buffer));

        int bytesReceived = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            if (onClientDisconnected) onClientDisconnected(clientIP);

            closesocket(clientSock);
            {
                std::lock_guard<std::mutex> lock(clientMutex);
                clients.erase(clientSock);
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

            qDebug() << "[Server] Received from client"
                     << QString::fromStdString(clientIP) << ":"
                     << QString::fromStdString(message);

            if (onMessageReceived)
                onMessageReceived(clientIP, message);

            // 🔽 Handle JSON Message
            QString jsonPart = QString::fromStdString(message);
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(jsonPart.toUtf8(), &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "[Server] JSON parse error:" << parseError.errorString();
                continue;
            }

            if (!doc.isObject()) {
                qDebug() << "[Server] Received JSON is not an object.";
                continue;
            }

            QJsonObject obj = doc.object();
            QString role = obj.value("role").toString();
            QString type = obj.value("type").toString();

            if (role == "add" && type == "entity") {
                QString id = obj.value("id").toString();
                QString name = obj.value("name").toString();
                QString parentID = obj.value("parentID").toString();

                // qDebug() << "[Server] Parsed entity from client:";
                // qDebug() << "  Name:" << name;
                // qDebug() << "  ID:" << id;
                // qDebug() << "  ParentID:" << parentID;
                qDebug() << "[Server] Parsed entity:" << name << ", ID:" << id << ", ParentID:" << parentID;
                    // Optional: Hook into logic
                    // if (networkManager) networkManager->entityAdded(parentID, id, name);
            }
        }
    }
}



void Server::broadcast(const std::string& message, const std::string& sender) {
    std::lock_guard<std::mutex> lock(clientMutex);

    if (!message.empty() && message[0] == '@') {
        size_t spacePos = message.find(' ');
        if (spacePos != std::string::npos) {
            std::string targetIP = message.substr(1, spacePos - 1);
            std::string realMsg = message.substr(spacePos + 1);
            sendToClient(targetIP, "[Private from " + sender + "]: " + realMsg + "\n");
        }
    } else {
        std::string fullMsg = /*"[From " + sender + "]: " +*/ message + "\n";
        for (const auto& [sock, ip] : clients) {
            send(sock, fullMsg.c_str(), static_cast<int>(fullMsg.length()), 0);
        }
    }
}




void Server::sendToClient(const std::string& ip, const std::string& message) {
    std::lock_guard<std::mutex> lock(clientMutex);

    for (const auto& [sock, storedIP] : clients) {
        if (storedIP == ip) {
            std::string jsonMessage = R"({"sender":"Server","message":")" + message + R"("})";

            int result = send(sock, jsonMessage.c_str(), static_cast<int>(jsonMessage.length()), 0);
            if (result == SOCKET_ERROR) {
                std::cerr << "[Server] Failed to send message to " << ip
                          << ". Error code: " << WSAGetLastError() << "\n";
            } else {
                std::cout << "[Server] Sent to " << ip << ": " << message << "\n";
            }
            return;
        }
    }

    std::cerr << "[Server] IP not found: " << ip << "\n";
}




