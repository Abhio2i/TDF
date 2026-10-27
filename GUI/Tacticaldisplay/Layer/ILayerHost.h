#ifndef ILAYERHOST_H
#define ILAYERHOST_H

// CHAITANYA JAIN CHANGE:
// Why: Created this abstract interface class to decouple LayerPanel from CanvasWidget.

#include <QString>
#include <QStringList>
#include <vector>
#include <QPointF>
#include "GUI/Tacticaldisplay/Shape/mesh_entry.h"

struct ViewportInfo {
    QPointF centerGeo;
    double degPerPixLon = 0.001;
    double degPerPixLat = 0.001;
};

class ILayerHost
{
public:
    virtual ~ILayerHost() = default;

    // Delete shape/raster from canvas
    virtual bool deleteObjectFromCanvas(const QString& shapeId) = 0;

    // Add raster mesh entry to canvas
    virtual void addRasterMesh(const MeshEntry& entry) = 0;
    virtual void addImportedMesh(const MeshEntry& entry) = 0;

    // Fetch geometry/properties for exporting
    virtual std::vector<MeshEntry> getMeshEntriesForShapes(const QStringList& shapeIds) = 0;

    // Fetch viewport scale information for unreferenced rasters
    virtual ViewportInfo getViewportInfo() const = 0;

    // Zoom viewport to fit bounding coordinates
    virtual void fitMapBounds(double minLon, double minLat, double maxLon, double maxLat) = 0;

    // Register a shape parsed by a format-specific plugin without exposing CanvasWidget internals
    virtual void registerImportedShape(const QString& layerName,
                                       const QString& shapeType,
                                       const QString& shapeName,
                                       const QVector3D& position,
                                       const std::vector<QVector3D>& polygenOffsets,
                                       const QVector3D& size,
                                       bool closePath) = 0;
    virtual void registerImportedText(
        const QString& layerName,
        const QString& featureName,
        const QVector3D& position,
        const QString& text) = 0;

    virtual void registerImportedBitmap(
        const QString& layerName,
        const QString& featureName,
        const QVector3D& position,
        const QString& bitmapPath,
        const QVector3D& size,
        float rotation) = 0;
};

#endif // ILAYERHOST_H