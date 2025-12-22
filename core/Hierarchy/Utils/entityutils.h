#ifndef ENTITYUTILS_H
#define ENTITYUTILS_H

#include <QString>
#include <QList>
#include "core/Hierarchy/Struct/constants.h"
#include <QStringList>
#include <cmath>
const double G_ACCELERATION = 9.8;
#define EARTH_RADIUS 6371000.0 // in meters

QJsonObject toParm(float value,QString unit);
float valueFromParm(const QJsonObject& parm);
double toRadians(double degree);
QString entityTypeToString(Constants::EntityType type);
Constants::EntityType stringToEntityType(const QString& str);
QStringList entityTypeOptions();

QString formationTypeToString(Constants::FormationType type);
Constants::FormationType stringToFormationType(const QString& str);
QStringList formationTypeOptions();
double distanceBetween(double lat1, double lon1, double lat2, double lon2);
#endif // ENTITYUTILS_H

