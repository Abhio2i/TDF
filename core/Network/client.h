#ifndef CLIENT_H
#define CLIENT_H

#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <string>
#include <functional>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

class Client{

public:
    Client();
    ~Client();

    bool connectToServer(const std::string& ip, int port);
    void start();
    void stop();

    void sendMessage(const std::string& msg);
    void processJsonMessage(const std::string& messageStr);
    bool isRunning() const;

    std::function<void(const std::string&)> onMessageReceived;
    std::function<void()> onConnected;
    std::function<void()> onDisconnected;
    std::function<void(const std::string&)> onError;

    // signals:
    //     void messageReceived(const QString& message);
private:
    void receiveThreadFunc();

    SOCKET clientSocket;
    std::thread recvThread;
    std::atomic<bool> running; // atomic for lightweight access
    mutable std::mutex socketMutex; // only protects socket ops
};

#endif
    // CLIENT_H
