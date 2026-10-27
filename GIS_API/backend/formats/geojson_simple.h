#ifndef GEOJSON_SIMPLE_H
#define GEOJSON_SIMPLE_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include "common/types.h"

class GeoJsonSimple
{
public:
    static QList<ShapeData> importShapes(const QString& filePath);
    static bool exportShapes(const QStringList& shapeIds, const QString& filePath);
};

#endif // GEOJSON_SIMPLE_H
