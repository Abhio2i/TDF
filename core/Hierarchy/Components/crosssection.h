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

    data Radar;
    data Visual;
    data Infrared;
    data Sonar;
    data Laser;

    // Add a map to store custom parameters
    QJsonObject customParameters; // Store custom parameters as key-value pairs

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
};

#endif // CROSSSECTION_H
