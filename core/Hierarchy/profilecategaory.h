#ifndef PROFILECATEGAORY_H
#define PROFILECATEGAORY_H

#include <QObject>
#include "./folder.h"
//#include <core/structure/hierarchy.h>

class Hierarchy;
class ProfileCategaory: public QObject
{
    Q_OBJECT
public:
    ProfileCategaory(Hierarchy* h);
    ~ProfileCategaory();
    std::string Name;
    bool Active;
    std::string ID;
    Constants::EntityType type;
    std::unordered_map<std::string, Folder*> Folders;
    std::unordered_map<std::string, Entity*> Entities;

    Folder* addFolder(std::string name, std::string iD = "");
    void addFolderWithObject(Folder *folder);
    void removeFolder(std::string name);

    Entity* addEntity(std::string name, std::string iD = "");
    void addEntityWithObject(Entity *entity);
    void removeEntity(std::string name);
    void setProfileType(Constants::EntityType Type);
    QJsonObject toJson();
    void fromJson(const QJsonObject& obj);


};

#endif // PROFILECATEGAORY_H
