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
    Geocords *geocord;
    Vector *position;

    QJsonObject toJson()const;
    void fromJson(const QJsonObject& obj);
};

#endif // WAYPOINTS_H
