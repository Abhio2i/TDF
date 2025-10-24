#ifndef MISSION_H
#define MISSION_H
#include "./component.h"
#include <QObject>
#include <core/Hierarchy/Struct/constants.h>
#include <core/Hierarchy/Struct/task.h>

class Mission : public QObject, public Component
{
    Q_OBJECT
public:
    Mission();
    ComponentType Typo() const override { return ComponentType::Mission; }
    std::string Name;
    bool Active;
    std::string ID;
    Constants::EntityType type;
    std::unordered_map<std::string, Task> *taskGroup;

    QJsonObject toJson()const override;
    void fromJson(const QJsonObject &obj) override;
};

#endif // MISSION_H
