#ifndef ENTITYUTILS_H
#define ENTITYUTILS_H

#include <QString>
#include <QList>
#include "core/Hierarchy/Struct/constants.h"
#include <QStringList>
QString entityTypeToString(Constants::EntityType type);
Constants::EntityType stringToEntityType(const QString& str);
QStringList entityTypeOptions();

QString formationTypeToString(Constants::FormationType type);
Constants::FormationType stringToFormationType(const QString& str);
QStringList formationTypeOptions();

#endif // ENTITYUTILS_H

