
#include "networkmanager.h"
#include <iostream>
#include <thread>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QJsonObject>>
#include <QJsonDocument>
#include <QFile>
#include <QDebug>

std::unique_ptr<Server> NetworkManager::ser = nullptr;
std::unique_ptr<Client> NetworkManager::cli = nullptr;
std::atomic<bool> NetworkManager::serverRunning{false};
std::atomic<bool> NetworkManager::clientRunning{false};

// Static control for one-time input thread
static std::once_flag inputThreadStarted;

NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {}

QString getLocalIP() {
    for (const QHostAddress& addr : QNetworkInterface::allAddresses()) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && addr != QHostAddress::LocalHost)
            return addr.toString();
    }
    return QHostAddress(QHostAddress::LocalHost).toString();
}
bool NetworkManager::initServer(int port) {
    qDebug() << "[NetworkManager] initServer called";

    if (!ser) {
        ser = std::make_unique<Server>();

        ser->onStart = [port]() {
            std::cout << "[Server] Listening on port " << port << "\n";
            serverRunning = true;
        };
        ser->onStop = []() {
            std::cout << "[Server] Stopped.\n";
            serverRunning = false;
        };
        ser->onError = [](const std::string& err) {
            std::cerr << "[Server] Error: " << err << "\n";
        };

        // ✅ Welcome message logic placed inside the callback
        ser->onClientConnected = [this](const std::string& ip) {
            std::cout << "[Server] Connected: " << ip << "\n";

            if (NetworkManager::ser && serverRunning) {
                std::string welcomeMsg = "Welcome from server!";
                emit this->getCurrentJsonData();
                //  NetworkManager::ser->sendToClient(ip, welcomeMsg);  // only to this client

                // Broadcast to all clients including this one
                // NetworkManager::ser->broadcast("hello to all", "SERVER");

                qDebug() << "[Server] Sent welcome message to" << QString::fromStdString(ip);
            }
        };



        ser->onClientDisconnected = [](const std::string& ip) {
            std::cout << "[Server] Disconnected: " << ip << "\n";
        };
        ser->onMessageReceived = [](const std::string& ip, const std::string& msg) {
            std::cout << "[" << ip << "] " << msg << "\n";

        };

        qDebug() << "[NetworkManager] Server object created successfully.";
        return true;
    }

    qDebug() << "[NetworkManager] Server object already exists.";
    return false;
}


bool NetworkManager::startServer(int port) {
    if (serverRunning) {
        std::cerr << "[Server] Already running!\n";
        return false;
    }

    qDebug() << "[NetworkManager] Attempting to start server on port" << port;

    if (!ser || !ser->start(port)) {
        std::cerr << "[Server] Start failed. Attempting restart...\n";
        ser.reset();
        initServer(port);

        if (!ser || !ser->start(port)) {
            std::cerr << "[Server] Restart attempt failed.\n";
            return false;
        }
    }

    serverRunning = true;

    // Start input thread only once
    std::call_once(inputThreadStarted, []() {
        std::thread([] {
            if (NetworkManager::ser)
                NetworkManager::ser->consoleInputHandler();
        }).detach();
    });


    return true;
}

Q_INVOKABLE void NetworkManager::sendServerMessage(const QString& message) {
    if (ser && serverRunning && !message.trimmed().isEmpty()) {
        ser->broadcast(message.toStdString(), "SERVER");
    }
}

bool NetworkManager::stopServer() {
    if (ser) {
        ser->stop();
        return true;
    }
    return false;
}

bool NetworkManager::initClient(const QString& ip, int port) {
    qDebug() << "[NetworkManager] initClient called with IP:" << ip << "Port:" << port;

    if (!cli) {
        cli = std::make_unique<Client>();

        cli->onConnected = []() {
            qDebug() << "[Client] Connected.";
            clientRunning = true;
        };
        cli->onDisconnected = []() {
            qDebug() << "[Client] Disconnected.";
            clientRunning = false;
        };
        cli->onError = [](const std::string& err) {
            qWarning() << "[Client] Error:" << QString::fromStdString(err);
        };
        cli->onMessageReceived = [this](const std::string& msg) {
            std::cout << "[Server] " << msg << "\n";
            qDebug() << "[Client] Message received:" << QString::fromStdString(msg);

            QByteArray byteArray = QByteArray::fromStdString(msg);
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(byteArray, &parseError);

            if (doc.isNull() || !doc.isObject()) {
                qDebug() << "[Client] Failed to parse JSON:" << parseError.errorString();

            }else{
                qDebug() << "done";
                QJsonObject obj = doc.object();
                QString role = obj.value("role").toString();
                QString type = obj.value("type").toString();
                qDebug() <<"work2";
                if (role == "init" && type == "data") {
                    emit initData(obj); // Only pass expected 3 arguments

                }else
                    if(role == "add" && type == "entitys") {
                        QString parentID = obj.value("parentID").toString();
                        QString name = obj.value("name").toString();
                        qDebug() <<"work";
                        emit addEntity(parentID, name, false);  // Only pass expected 3 arguments

                    }else
                        if(role == "add" && type == "entityJson") {
                            QString parentID = obj.value("parentID").toString();
                            QString name = obj.value("name").toString();
                            qDebug() <<"work entityjson";
                            emit addEntityFromJson(parentID, obj, false);  // Only pass expected 3 arguments
                        }else
                            if(role == "add" && type == "component") {
                                QString Id = obj.value("Id").toString();
                                QString name = obj.value("name").toString();
                                qDebug() <<"work component";
                                emit addComponent(Id, name);  // Only pass expected 3 arguments
                            }else
                                if(role == "remove" && type == "entity") {
                                    QString Id = obj.value("Id").toString();
                                    QString parentId = obj.value("parentId").toString();
                                    bool profile = obj.value("Profile").toBool();
                                    qDebug() <<"work remove entity";
                                    emit removeEntity(parentId,Id, profile);  // Only pass expected 3 arguments
                                }
            }
        };

    }

    clientIp = ip;
    clientPort = port;
    return true;
}


bool NetworkManager::startClient() {
    qDebug() << "[NetworkManager] Attempting to start client to"
             << clientIp << ":" << clientPort;

    if (clientRunning) {
        qWarning() << "[Client] Already running!";
        return false;
    }

    if (!cli) {
        qWarning() << "[Client] Client not initialized.";
        return false;
    }

    // Attempt to connect
    if (!cli->connectToServer(clientIp.toStdString(), clientPort)) {
        qWarning() << "[Client] Connection failed.";
        return false;
    }

    // Wait a short moment for the connection flag to be updated (if needed)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (clientRunning) {
        //cli->sendMessage("Hello from Qt client!\n");
        qDebug() << "[Client] Sent test message to server.";
    } else {
        qWarning() << "[Client] Connected flag not set yet.";
    }

    // Start input thread for client
    std::call_once(inputThreadStarted, []() {
        std::thread([] {
            std::string line;
            while (std::getline(std::cin, line)) {
                if (!line.empty()) {
                    if (NetworkManager::ser && NetworkManager::serverRunning) {
                        NetworkManager::ser->broadcast(line, "SERVER");
                    } else if (NetworkManager::cli && NetworkManager::clientRunning) {
                        NetworkManager::cli->sendMessage(line);
                    } else {
                        std::cerr << "[InputThread] No valid client/server running.\n";
                    }
                }
            }
        }).detach();
    });

    return true;
}


bool NetworkManager::stopClient() {
    qDebug() << "[NetworkManager] Stopping client...";
    if (cli) {
        cli->stop();
        return true;
    }
    return false;
}

void NetworkManager::sendClientMessage(const QString& message) {
    if (cli && clientRunning && !message.trimmed().isEmpty()) {
        cli->sendMessage(message.toStdString());
    }
}

void NetworkManager::getJsonData(const QJsonObject& obj){
    QJsonObject copy = obj;
    copy["role"] = "init";
    copy["type"] = "data";
    sendJson(copy);
}

void NetworkManager::sendJson(const QJsonObject& obj) {
    QJsonDocument doc(obj);
    QString msg = doc.toJson(QJsonDocument::Compact);
    std::string stdMsg = msg.toStdString();
    std::cout << "Log: " << stdMsg << std::endl;
    // qDebug() << "[NetworkManager JSON] Sending:" << stdMsg;
    QByteArray byteArray = QByteArray::fromStdString(stdMsg);
    QJsonParseError parseError;
    QJsonDocument docs = QJsonDocument::fromJson(byteArray, &parseError);

    if (docs.isNull() || !docs.isObject()) {
        qDebug() << "[Client] Failed to parse JSON:" << parseError.errorString();

    }else{
        qDebug() << "successfull";
    }
    if (serverRunning && ser) {
        ser->broadcast(stdMsg, "Server");
    } else if (clientRunning && cli) {
        //cli->sendMessage(stdMsg + "\n");  // Add newline to separate messages
    }
}




// Stub implementations for signals/serialization
// void NetworkManager::toJson() {}
// void NetworkManager::fromJson() {}
// void NetworkManager::profileAddedPointer(ProfileCategaory*) {}
// void NetworkManager::folderAddedPointer(QString, Folder*) {}
// void NetworkManager::entityAddedPointer(QString, Entity*) {}
// void NetworkManager::profileAdded(QString, QString) {}
// void NetworkManager::folderAdded(QString, QString, QString) {}
// void NetworkManager::entityAdded(QString, QString, QString) {}
// void NetworkManager::componentAdded(QString, QString) {}
// void NetworkManager::profileRemoved(QString) {}
// void NetworkManager::folderRemoved(QString) {}
// void NetworkManager::entityRemoved(QString) {}
// void NetworkManager::componentRemoved(QString, QString) {}
// void NetworkManager::profileRenamed(QString, QString) {}
// void NetworkManager::folderRenamed(QString, QString) {}
// void NetworkManager::entityRenamed(QString, QString) {}
// void NetworkManager::entityMeshAdded(QString, Entity*) {}
// void NetworkManager::entityMeshRemoved(QString) {}
// void NetworkManager::entityPhysicsAdded(QString, Entity*) {}
// void NetworkManager::entityPhysicsRemoved(QString) {}
// void NetworkManager::entityUpdate(QString) {}


// Serialization
void NetworkManager::toJson() {
    QJsonObject obj;
    obj["status"] = "placeholder";

    QJsonDocument doc(obj);
    QFile file("network_state.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}
void NetworkManager::UpdateClient(){
    emit this->getCurrentJsonData();
}
void NetworkManager::fromJson() {
    QFile file("network_state.json");
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject obj = doc.object();
    qDebug() << "Loaded state:" << QJsonDocument(obj).toJson(QJsonDocument::Indented);
}

// Pointer-based (stubs)
void NetworkManager::profileAddedPointer(ProfileCategaory*) {}
void NetworkManager::folderAddedPointer(QString parentID, Folder*) {}
void NetworkManager::entityAddedPointer(QString parentID, Entity* entity) {
    QJsonObject msg = entity->toJson();
    msg["role"] = "add";
    msg["type"] = "entityJson";
    msg["parentID"] = parentID;
    //sendJson(msg);
}

void NetworkManager::profileAdded(QString ID, QString profileName) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "profile";
    msg["id"] = ID;
    msg["name"] = profileName;
    //sendJson(msg);
    emit this->getCurrentJsonData();
}

void NetworkManager::folderAdded(QString parentID, QString ID, QString folderName) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "folder";
    msg["id"] = ID;
    msg["parentID"] = parentID;
    msg["name"] = folderName;
    //sendJson(msg);
    emit this->getCurrentJsonData();
}

void NetworkManager::entityAdded(QString parentID, QString ID, QString entityName) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "entity";
    msg["id"] = ID;
    msg["parentID"] = parentID;
    msg["name"] = entityName;
    //sendJson(msg);
    emit this->getCurrentJsonData();
}

void NetworkManager::componentAdded(QString Id, QString componentName) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "component";
    msg["Id"] = Id;
    msg["name"] = componentName;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

void NetworkManager::profileRemoved(QString ID) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "profile";
    msg["id"] = ID;
    //sendJson(msg);
    emit this->getCurrentJsonData();
}

void NetworkManager::folderRemoved(QString ID) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "folder";
    msg["id"] = ID;
    //sendJson(msg);
    emit this->getCurrentJsonData();
}

void NetworkManager::entityRemoved(QString parentId,QString ID,bool Profile) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "entity";
    msg["Id"] = ID;
    msg["parentId"] = parentId;
    msg["Profile"] = Profile;
    sendJson(msg);
    emit this->getCurrentJsonData();
}

void NetworkManager::componentRemoved(QString parentID, QString componentName) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "component";
    msg["parentID"] = parentID;
    msg["name"] = componentName;
    //sendJson(msg);
    //emit this->getCurrentJsonData();
}

void NetworkManager::profileRenamed(QString ID, QString name) {
    QJsonObject msg;
    msg["role"] = "rename";
    msg["type"] = "profile";
    msg["id"] = ID;
    msg["newName"] = name;
    //sendJson(msg);
    //emit this->getCurrentJsonData();
}

void NetworkManager::folderRenamed(QString ID, QString name) {
    QJsonObject msg;
    msg["role"] = "rename";
    msg["type"] = "folder";
    msg["id"] = ID;
    msg["newName"] = name;
    //sendJson(msg);
    //emit this->getCurrentJsonData();
}

void NetworkManager::entityRenamed(QString ID, QString name) {
    QJsonObject msg;
    msg["role"] = "rename";
    msg["type"] = "entity";
    msg["id"] = ID;
    msg["newName"] = name;
    //sendJson(msg);
    //emit this->getCurrentJsonData();
}

void NetworkManager::entityMeshAdded(QString ID, Entity*) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "mesh";
    msg["entityID"] = ID;
    //sendJson(msg);
}

void NetworkManager::entityMeshRemoved(QString ID) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "mesh";
    msg["entityID"] = ID;
    //sendJson(msg);
}

void NetworkManager::entityPhysicsAdded(QString ID, Entity*) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "physics";
    msg["entityID"] = ID;
    //sendJson(msg);
}

void NetworkManager::entityPhysicsRemoved(QString ID) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "physics";
    msg["entityID"] = ID;
    //sendJson(msg);
}

void NetworkManager::entityUpdate(QString ID) {
    QJsonObject msg;
    msg["role"] = "update";
    msg["type"] = "entity";
    msg["id"] = ID;
    //sendJson(msg);
    //emit this->getCurrentJsonData();
}

