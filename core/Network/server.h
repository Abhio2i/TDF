#ifndef SERVER_H
#define SERVER_H

#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <string>
//class NetworkManager;

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
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

    //NetworkManager* networkManager = nullptr;
private:
    void acceptLoop();                    // Accept client connections
    void clientHandlerThread(SOCKET sock); // Per-client communication

    SOCKET serverSocket;
    std::atomic<bool> running;

    std::thread acceptThread;
    std::mutex stateMutex;
    std::unordered_map<SOCKET, std::string> clients;
    std::mutex clientMutex;
};

#endif


