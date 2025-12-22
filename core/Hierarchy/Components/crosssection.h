#ifndef CROSSSECTION_H
#define CROSSSECTION_H
#include "./component.h"
#include <QObject>
#include <QJsonObject>

class CrossSection: public QObject, public Component
{
    Q_OBJECT
public:
    struct data{
        float uniformedValue = 100.00;
        float modulationValue = 0.00;
    };

    CrossSection();
    ComponentType Typo() const override { return ComponentType::CrossSection; }
    data Radar;
    data Visual;
    data Infrared;
    data Sonar;
    data Laser;

    // Add a map to store custom parameters
    QJsonObject customParameters; // Store custom parameters as key-value pairs
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QString data3 = "") override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
};

#endif // CROSSSECTION_H
