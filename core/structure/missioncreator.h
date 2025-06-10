#ifndef MISSIONCREATOR_H
#define MISSIONCREATOR_H
#include <QObject>
#include <core/structure/folder.h>
#include <core/Components/mission.h>

class MissionCreator: public QObject
{
    Q_OBJECT
public:
    MissionCreator();
    std::unordered_map<std::string, Folder> Folders;
    std::unordered_map<std::string, Mission> missionList;

    void createMission();
    void deleteMission();
    void saveMissionAsFile();
    void addMissionFromFile();
};

#endif // MISSIONCREATOR_H
