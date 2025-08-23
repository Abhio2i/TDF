#ifndef CLIENT_H
#define CLIENT_H

#include <thread>
#include <mutex>
#include <string>
#include <functional>
#include <atomic>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#endif

// Platform-specific definitions
#ifdef _WIN32
#define SOCKET_TYPE SOCKET
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#define SOCKET_ERROR_VALUE SOCKET_ERROR
#define CLOSE_SOCKET closesocket
#define SHUTDOWN_BOTH SD_BOTH
#else
#define SOCKET_TYPE int
#define INVALID_SOCKET_VALUE -1
#define SOCKET_ERROR_VALUE -1
#define CLOSE_SOCKET close
#define SHUTDOWN_BOTH SHUT_RDWR
#endif

class Client {
public:
    Client();
    virtual ~Client();

    bool connectToServer(const std::string& ip, int port);
    void start();
    void stop();
    void sendMessage(const std::string& msg);
    void processJsonMessage(const std::string& messageStr);
    bool isRunning() const;

    // Callbacks
    std::function<void(const std::string&)> onMessageReceived;
    std::function<void()> onConnected;
    std::function<void()> onDisconnected;
    std::function<void(const std::string&)> onError;

private:
    void receiveThreadFunc();

    SOCKET_TYPE clientSocket;
    std::thread recvThread;
    std::atomic<bool> running;
    mutable std::mutex socketMutex;
};

#endif // CLIENT_H
