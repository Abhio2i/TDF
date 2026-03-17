#ifndef WAYPOINTS_H
#define WAYPOINTS_H
#include <QObject>
#include <QJsonObject>
#include "./vector.h"
#include "./geocords.h"

class Waypoints: public QObject
{
    Q_OBJECT
public:
    Waypoints();
    Geocords *geocord = nullptr;
    Vector *position = nullptr;
    double speed = 0;
    bool sensor = true;
    bool formation = true;

    QJsonObject toJson()const;
    void fromJson(const QJsonObject& obj);
};

#endif // WAYPOINTS_H
