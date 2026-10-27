#ifndef ILAYER_FORMAT_H
#define ILAYER_FORMAT_H

#include <QString>
#include <QStringList>
#include "common/types.h"

class ILayerFormat
{
public:
    virtual ~ILayerFormat() = default;

    virtual QString id() const = 0;
    virtual QString filter() const = 0;
    virtual QStringList extensions() const = 0;
    virtual bool canExport() const { return true; }
    virtual bool canImport() const { return true; }

    virtual bool exportShapes(const QList<ShapeData>& shapes,
                              const QString& filePath) const = 0;
    virtual QList<ShapeData> importShapes(const QString& filePath) const = 0;
};

#endif // ILAYER_FORMAT_H
