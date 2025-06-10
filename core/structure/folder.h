#ifndef FOLDER_H
#define FOLDER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <core/structure/entity.h>
#include <core/Components/formation.h>

class Hierarchy;
class Folder: public QObject
{
    Q_OBJECT
public:
    Folder(Hierarchy* h);
    ~Folder();
    std::string Name;
    bool Active;
    std::string ID;
    std::string parentID;

    std::unordered_map<std::string, Folder*> Folders;
    std::unordered_map<std::string, Entity*> Entities;
    Mission *mission;
    Formation *formation;

    Folder* addFolder(std::string name);
    void addFolderWithObject(Folder *folder);
    void removeFolder(std::string name);

    Entity* addEntity(std::string name);
    void addEntityWithObject(Entity *entity);
    void removeEntity(std::string name);

    QJsonObject toJson();
    void fromJson(const QJsonObject& obj);

};

#endif // FOLDER_H
