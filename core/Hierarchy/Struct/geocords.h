#ifndef GEOCORDS_H
#define GEOCORDS_H
#include <QObject>
#include "./Vector.h"
#include <QJsonObject>
class Geocords: public QObject
{
    Q_OBJECT
public:
    explicit Geocords(double latitude = 0.0f, double longitude = 0.0f, double altitude = 0.0f,double Heading = 0.0f, QObject* parent = nullptr);

    double latitude;
    double longitude;
    double altitude;
    double Heading;

    Vector toWorld(Geocords *cordinate);
    Geocords fromWorld(Vector position);
    double distance(Geocords *cordinate1, Geocords *cordinate2);

    QJsonObject toJson();
    void fromJson(const QJsonObject &obj);
};

#endif // GEOCORDS_H
