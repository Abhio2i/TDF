#ifndef GEOJSON_H
#define GEOJSON_H

// CHAITANYA JAIN CHANGE:
// Why: Extracted GeoJSON export functionality from LayerPanel into a standalone module.

#include <QString>
#include <QStringList>
#include <QJsonObject>

class ILayerHost;
struct MeshEntry;

class GeoJSON
{
public:
    static bool importLayer(ILayerHost* host, const QString& filePath, const QString& layerName);
    static bool exportLayer(ILayerHost* host, const QString& layerName, const QStringList& shapeIds, const QString& filePath);

private:
    static QJsonObject createGeoJSONFeature(const MeshEntry& entry, const QString& shapeId);
    static QJsonObject getGeometryAsGeoJSON(const MeshEntry& entry);
    static QString getShapeType(const MeshEntry& entry);
};

#endif // GEOJSON_H