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

    std::unordered_map<std::string, Folder*> Folders;
    std::unordered_map<std::string, Entity*> Entities;

    Folder* addFolder(std::string name);
    void addFolderWithObject(Folder *folder);
    void removeFolder(std::string name);

    Entity* addEntity(std::string name);
    void addEntityWithObject(Entity *entity);
    void removeEntity(std::string name);

    QJsonObject toJson();
    void fromJson(const QJsonObject& obj);

};

#endif // PROFILECATEGAORY_H
