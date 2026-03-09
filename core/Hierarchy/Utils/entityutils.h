#ifndef ENTITYUTILS_H
#define ENTITYUTILS_H

#include <QString>
#include <QList>
#include "core/Hierarchy/Struct/constants.h"
#include <QStringList>
#include <cmath>
struct ECEF { double x, y, z; };
struct FlatXYZ { double x, y, z; };
struct GeoPos { double lat, lon, alt; };
const double G_ACCELERATION = 9.8f;
const double FTtoKM = 1/3281.0f;
const double KMtoFT = 3281.0f;
const double FTminToKMs = 1/196900.0f;//ft/min to km/s
#define EARTH_RADIUS 6371000.0 // in meters

QJsonObject toParm(float value,QString unit,float min = 0.0f,float max = 0.0f, QString description = "");
float valueFromParm(const QJsonObject& parm);
double toRadians(double degree);
QString entityTypeToString(Constants::EntityType type);
Constants::EntityType stringToEntityType(const QString& str);
QStringList entityTypeOptions();

QString formationTypeToString(Constants::FormationType type);
Constants::FormationType stringToFormationType(const QString& str);
QStringList formationTypeOptions();
double distanceBetween(double lat1, double lon1, double lat2, double lon2);
FlatXYZ geoToFlatXYZ(double lat, double lon, double alt);
GeoPos flatXYZToGeo(double x, double y, double z);
ECEF geoToXYZ(double lat, double lon, double alt);
GeoPos xyzToGeo(double x, double y, double z);

std::tuple<double, double> calculateNewLatLong(double lat1, double lon1, double heading, double DISTANCE_KM);
double convertToClockwise360(double lon180);
#endif // ENTITYUTILS_H

