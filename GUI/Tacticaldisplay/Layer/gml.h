#ifndef GML_H
#define GML_H

// CHAITANYA JAIN CHANGE:
// Why: Extracted GML export functionality from LayerPanel into a standalone module.

#include <QString>
#include <QStringList>

class ILayerHost;
struct MeshEntry;

class GML
{
public:
    static bool importLayer(ILayerHost* host, const QString& filePath, const QString& layerName);
    static bool exportLayer(ILayerHost* host, const QString& layerName, const QStringList& shapeIds, const QString& filePath);

private:
    static QString createGMLFeature(const MeshEntry& entry, const QString& shapeId);
    static QString getGeometryAsGML(const MeshEntry& entry);
    static QString getShapeType(const MeshEntry& entry);
};

#endif // GML_H