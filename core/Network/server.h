#ifndef SERVER_H
#define SERVER_H

#include <thread>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h> // Added for addrinfo, getaddrinfo, freeaddrinfo
#include <unistd.h>
#include <errno.h>
#endif

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
// #include <qDebug>

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

class Server {
public:
    Server();
    ~Server();

    bool start(int port);
    void stop();
    void broadcast(const std::string& message, const std::string& sender);
    void sendToClient(const std::string& ip, const std::string& message);
    void consoleInputHandler();

    // Callbacks
    std::function<void()> onStart;
    std::function<void()> onStop;
    std::function<void(const std::string&)> onError;
    std::function<void(const std::string&)> onClientConnected;
    std::function<void(const std::string&)> onClientDisconnected;
    std::function<void(const std::string&, const std::string&)> onMessageReceived;

private:
    void acceptLoop();
    void clientHandlerThread(SOCKET_TYPE sock);

    SOCKET_TYPE serverSocket;
    std::atomic<bool> running;
    std::thread acceptThread;
    std::mutex stateMutex;
    std::unordered_map<SOCKET_TYPE, std::string> clients;
    std::mutex clientMutex;
};

#endif // SERVER_H
