#ifndef MISSIONEXCUTER_H
#define MISSIONEXCUTER_H
#include <QObject>
#include <core/Hierarchy/entity.h>
#include <core/Hierarchy/Struct/task.h>

class MissionExcuter: public QObject
{
    Q_OBJECT
public:
    MissionExcuter();
    bool Active;
    std::string ID;
    std::string Name;
    bool Start;
    bool Running;
    bool Pause;
    bool Done;
    Entity *entity;

    std::string missionTable;
    std::unordered_map<std::string, Task> *taskGroup;

    void start();
    void pause();
    void resume();
    void complete();
    void excuteTask();


    void toJson();
    void fromJson();
};

#endif // MISSIONEXCUTER_H
