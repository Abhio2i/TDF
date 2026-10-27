#ifndef KML_H
#define KML_H

// CHAITANYA JAIN CHANGE:
// Why: Extracted KML export functionality from LayerPanel into a standalone module.

#include <QString>
#include <QStringList>

class ILayerHost;
struct MeshEntry;

class KML
{
public:
    static bool importLayer(ILayerHost* host, const QString& filePath, const QString& layerName);
    static bool exportLayer(ILayerHost* host, const QString& layerName, const QStringList& shapeIds, const QString& filePath);

private:
    static QString createKMLPlacemark(const MeshEntry& entry, const QString& shapeId);
    static QString getGeometryAsKML(const MeshEntry& entry);
    static QString getShapeType(const MeshEntry& entry);
};

#endif // KML_H