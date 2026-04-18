#ifndef SENSORPROFILE_H
#define SENSORPROFILE_H

#include "./component.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include <QObject>
#include <QJsonObject>
class Hierarchy;
class SensorProfile: public QObject, public Component
{
    Q_OBJECT
public:
    SensorProfile(Hierarchy* h);
    ComponentType Typo() const override { return ComponentType::SensorProfile; }

    bool Active;
    std::unordered_map<std::string, Sensor*> *sensors;
    Sensor* getSensor(const std::string& id) const;
    // Add a map to store custom parameters
    QJsonObject customParameters; // Store custom parameters as key-value pairs
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;


    void renameSubComponent(std::string ID, QString newName) override;
};

#endif // SENSORPROFILE_H
