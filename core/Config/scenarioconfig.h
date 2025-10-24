#ifndef SCENARIOCONFIG_H
#define SCENARIOCONFIG_H
#include <QObject>

class ScenarioConfig: public QObject
{
    Q_OBJECT
public:
    ScenarioConfig();
    std::string Name;

    void toJson();
    void fromJson();
};

#endif // SCENARIOCONFIG_H
