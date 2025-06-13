#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H
#include "core/Hierarchy/profilecategaory.h"
#include <QObject>
#include <core/Network/netwoktransport.h>

class NetworkManager: public QObject
{
    Q_OBJECT
public:
    NetworkManager();

    NetwokTransport *networkTransport;
    std::unordered_map<std::string,  std::string> *connectedClients;

    void toJson();
    void fromJson();

public:
    void profileAddedPointer(ProfileCategaory* profile);
    void folderAddedPointer(QString parentID,Folder* folder);
    void entityAddedPointer(QString parentID,Entity* entity);

    void profileAdded(QString ID, QString profileName);
    void folderAdded(QString parentID,QString ID,QString folderName);
    void entityAdded(QString parentID,QString ID,QString entityName);
    void componentAdded(QString parentID,QString componentName);

    void profileRemoved(QString ID);
    void folderRemoved(QString ID);
    void entityRemoved(QString ID);
    void componentRemoved(QString parentID,QString componentName);

    void profileRenamed(QString Id, QString name);
    void folderRenamed(QString Id, QString name);
    void entityRenamed(QString Id, QString name);

    void entityMeshAdded(QString ID,Entity* entity);
    void entityMeshRemoved(QString ID);

    void entityPhysicsAdded(QString ID,Entity* entity);
    void entityPhysicsRemoved(QString ID);


    void entityUpdate(QString ID);
};

#endif // NETWORKMANAGER_H
