#include "geocords.h"

Geocords::Geocords(double latitude, double longitude, double altitude,double Heading, QObject* parent):
    QObject(parent),latitude(latitude),longitude(longitude),altitude(altitude),Heading(Heading)
{

}


QJsonObject Geocords::toJson(){
    QJsonObject obj;
    obj["type"] = "geocord";
    obj["latitude"] = latitude;
    obj["longitude"] = longitude;
    obj["altitude"] = altitude;
    obj["heading"] = Heading;
    return obj;
}

void Geocords::fromJson(const QJsonObject &obj)
{
    if (obj.contains("latitude"))
        latitude = obj["latitude"].toVariant().toDouble();
    if (obj.contains("longitude"))
        longitude = obj["longitude"].toVariant().toDouble();
    if (obj.contains("altitude"))
        altitude = obj["altitude"].toVariant().toDouble();
    if (obj.contains("heading"))
        Heading = obj["heading"].toVariant().toDouble();
}
