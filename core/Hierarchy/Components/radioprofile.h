#ifndef RADIOPROFILE_H
#define RADIOPROFILE_H

#include "./component.h"
#include <QObject>
#include <QJsonObject>
class Hierarchy;
class Radio;
class RadioProfile: public QObject, public Component
{
    Q_OBJECT
public:
    RadioProfile(Hierarchy* h);
    ComponentType Typo() const override { return ComponentType::RadioProfile; }

    bool Active;
    std::unordered_map<std::string, Radio*> *radios                    ;

    // Add a map to store custom parameters
    QJsonObject customParameters; // Store custom parameters as key-value pairs
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QString data3 = "") override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
    void renameSubComponent(std::string ID, QString newName) override;

};

#endif // RADIOPROFILE_H
