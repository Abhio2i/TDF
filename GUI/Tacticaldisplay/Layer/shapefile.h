#ifndef SHAPEFILE_H
#define SHAPEFILE_H

// CHAITANYA JAIN CHANGE:
// Why: Extracted Shapefile export functionality from LayerPanel into a standalone module.

#include <QString>
#include <QStringList>
#include <QVector>
#include <QByteArray>
#include <QDataStream>

class ILayerHost;
struct MeshEntry;

class Shapefile
{
public:
    static bool importLayer(ILayerHost* host, const QString& filePath, const QString& layerName);
    static bool exportLayer(ILayerHost* host, const QString& layerName, const QStringList& shapeIds, const QString& filePath);

private:
    static bool exportShapesByType(const QVector<const MeshEntry*>& shapes, const QString& filePath, const QString& layerName, int shapeType);
    static void writeShapefileHeader(QDataStream& stream, qint32 fileLength, int shapeType, double minX, double minY, double maxX, double maxY);
    static QByteArray createShapefileRecord(const MeshEntry* entry, int shapeType, double& minX, double& minY, double& maxX, double& maxY);
    static bool writeDBF(const QString& filePath, const QVector<const MeshEntry*>& shapes);
    static void writeDBFFieldDescriptor(QDataStream& stream, const QString& name, char type, int length);
    static QString getShapeType(const MeshEntry& entry);
};

#endif // SHAPEFILE_H