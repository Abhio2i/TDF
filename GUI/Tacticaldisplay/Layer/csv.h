#ifndef CSV_H
#define CSV_H

// CHAITANYA JAIN CHANGE:
// Why: Extracted CSV export functionality from LayerPanel into a standalone module.

#include <QString>
#include <QStringList>

class ILayerHost;
struct MeshEntry;

class CSV
{
public:
    static bool importLayer(ILayerHost* host, const QString& filePath, const QString& layerName);
    static bool exportLayer(ILayerHost* host, const QString& layerName, const QStringList& shapeIds, const QString& filePath);

private:
    static QString createCSVLine(const MeshEntry& entry);
    static QString createWKTGeometry(const MeshEntry& entry);
    static QString getShapeType(const MeshEntry& entry);
};

#endif // CSV_H