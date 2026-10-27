#ifndef FLATGEOBUF_H
#define FLATGEOBUF_H

// CHAITANYA JAIN CHANGE:
// Why: Extracted FlatGeobuf export functionality from LayerPanel into a standalone module.

#include <QString>
#include <QStringList>

class ILayerHost;

class FlatGeobuf
{
public:
    static bool importLayer(ILayerHost* host, const QString& filePath, const QString& layerName);
    static bool exportLayer(ILayerHost* host, const QString& layerName, const QStringList& shapeIds, const QString& filePath);
};

#endif // FLATGEOBUF_H