#ifndef ENTITYUTILS_H
#define ENTITYUTILS_H

#include <QString>
#include <QList>
#include "core/Hierarchy/Struct/constants.h"
QString entityTypeToString(Constants::EntityType type);
Constants::EntityType stringToEntityType(const QString& str);
QList<QString> entityTypeOptions();

#endif // ENTITYUTILS_H

