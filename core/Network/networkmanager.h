#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <unordered_map>
#include <string>
#include <atomic>

#include "core/Hierarchy/profilecategaory.h"
#include "core/Network/networktransport.h"
#include <core/Hierarchy/hierarchy.h> // <-- Add or confirm this line
// or whatever the correct path is, e.g., #include "hierarchy.h"
class NetworkManager : public QObject {
    Q_OBJECT

public:
    explicit NetworkManager(QObject* parent = nullptr);

    // Server controls
    Q_INVOKABLE bool startServer(int port);
    // Q_INVOKABLE void sendServerMessage(const QString& msg);
    // Q_INVOKABLE void sendClientMessage(const QString& message);

    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    void init(const QString& ip, int port);
    bool stopServer();

    // Client controls
    bool initClient(const QString& ip, int port); // << updated
    bool startClient();                           // << now uses stored IP/port
    bool stopClient();

    void onMessaageRecevied(QString message);
    void onConnect();
    void onNewConnction();
    // Global network access
    NetworkTransport* networkTransport;
    std::unordered_map<std::string, std::string>* connectedClients;

    // Serialization
    void toJson();
    void fromJson();

    void UpdateClient();
    // Entity signals - pointer-based
    void profileAddedPointer(ProfileCategaory* profile);
    void folderAddedPointer(QString parentID, Folder* folder);
    void entityAddedPointer(QString parentID, Entity* entity);

    // Entity signals - value-based
    void profileAdded(QString ID, QString profileName);
    void folderAdded(QString parentID, QString ID, QString folderName);
    void entityAdded(QString parentID, QString ID, QString entityName);
    void componentAdded(QString Id, QString componentName);

    void profileRemoved(QString ID);
    void folderRemoved(QString ID);
    void entityRemoved(QString parentId,QString ID,bool Profile);
    void componentRemoved(QString parentID, QString componentName);

    void profileRenamed(QString Id, QString name);
    void folderRenamed(QString Id, QString name);
    void entityRenamed(QString Id, QString name);

    void entityMeshAdded(QString ID, Entity* entity);
    void entityMeshRemoved(QString ID);
    void entityPhysicsAdded(QString ID, Entity* entity);
    void entityPhysicsRemoved(QString ID);
    void entityComponentsUpdate(QString ID, QString componentName, QJsonObject delta);

    void entityUpdate(QString ID);
    void getJsonData(const QJsonObject& obj);
signals:
    void addFolder(QString parentId,QString ID,QString FolderName,bool Profile);
    void addEntity(QString parentId,QString ID,QString EntityName,bool Profile);
    void addEntityFromJson(QString parentId,QJsonObject obj,bool Profile);
    void addComponent(QString Id,QString ComponentName);
    void removeEntity(QString parentId,QString ID,bool Profile);
    void removeFolder(QString ID);
    void entityComponentUpdate(QString ID, QString name, QJsonObject delta);
    void getCurrentJsonData();
    void initData(const QJsonObject& obj);
    void updateScene(float deltaTime);
private:
    // static std::unique_ptr<Server> ser;
    // static std::unique_ptr<Client> cli;
    static std::atomic<bool> serverRunning;
    static std::atomic<bool> clientRunning;

    // Client configuration (newly added)
    QString clientIp;
    int clientPort;
    void sendJson(const QJsonObject& obj);
    NetworkTransport* network;
    Hierarchy* hierarchy = nullptr;
};

#endif
    // NETWORKMANAGER_H
// NETWORKMANAGER_H
// NETWORKMANAGER_H
