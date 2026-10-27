#include "flatgeobuf.h"
#include "geojson.h"
#include "GUI/Tacticaldisplay/Layer/ILayerHost.h"
#include <QProcess>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>

// CHAITANYA JAIN CHANGE:
// Why: Extracted FlatGeobuf export functionality from LayerPanel into a standalone module.

#include <qgsvectorlayer.h>
#include <qgsfeature.h>
#include <qgsgeometry.h>

bool FlatGeobuf::importLayer(ILayerHost* host, const QString& filePath, const QString& layerName)
{
    if (!host) return false;

    QgsVectorLayer *layer = new QgsVectorLayer(filePath, "temp", "ogr");
    if (!layer->isValid()) {
        delete layer;
        return false;
    }

    QgsFeatureIterator it = layer->getFeatures();
    QgsFeature feat;
    while (it.nextFeature(feat)) {
        QgsGeometry geom = feat.geometry();
        if (geom.isNull()) continue;

        QString featName;
        QVariant nameAttr = feat.attribute("name");
        if (!nameAttr.isValid()) nameAttr = feat.attribute("NAME");
        if (!nameAttr.isValid()) nameAttr = feat.attribute("id");
        if (nameAttr.isValid()) featName = nameAttr.toString();

        Qgis::GeometryType geomType = geom.type();
        if (geomType == Qgis::GeometryType::Point) {
            QgsPointXY pt = geom.asPoint();
            QVector3D position(pt.x(), pt.y(), 0);
            std::vector<QVector3D> offsets = { QVector3D(0, 0, 0) };
            host->registerImportedShape(layerName, "Point", featName, position, offsets, QVector3D(1, 1, 1), true);

        } else if (geomType == Qgis::GeometryType::Line) {
            QgsPolylineXY line = geom.asPolyline();
            if (line.isEmpty()) continue;

            double avgLon = 0, avgLat = 0;
            for (const QgsPointXY &p : line) {
                avgLon += p.x();
                avgLat += p.y();
            }
            avgLon /= line.size();
            avgLat /= line.size();

            QVector3D position(avgLon, avgLat, 0);
            std::vector<QVector3D> offsets;
            for (const QgsPointXY &p : line) {
                offsets.push_back(QVector3D(p.x() - avgLon, p.y() - avgLat, 0));
            }
            host->registerImportedShape(layerName, "Line", featName, position, offsets, QVector3D(1, 1, 1), false);

        } else if (geomType == Qgis::GeometryType::Polygon) {
            QgsPolygonXY poly = geom.asPolygon();
            for (const QgsPolylineXY &ring : poly) {
                if (ring.isEmpty()) continue;

                double avgLon = 0, avgLat = 0;
                for (const QgsPointXY &p : ring) {
                    avgLon += p.x();
                    avgLat += p.y();
                }
                avgLon /= ring.size();
                avgLat /= ring.size();

                QVector3D position(avgLon, avgLat, 0);

                if (featName.startsWith("Circle", Qt::CaseInsensitive) || featName.startsWith("TempCircle", Qt::CaseInsensitive)) {
                    double radius = 0.001;
                    if (!ring.isEmpty()) {
                        radius = std::sqrt(std::pow(ring[0].x() - avgLon, 2) + std::pow(ring[0].y() - avgLat, 2));
                    }
                    QVector3D size(radius, radius, 1);
                    host->registerImportedShape(layerName, "Circle", featName, position, std::vector<QVector3D>(), size, true);
                } else {
                    std::vector<QVector3D> offsets;
                    for (const QgsPointXY &p : ring) {
                        offsets.push_back(QVector3D(p.x() - avgLon, p.y() - avgLat, 0));
                    }
                    host->registerImportedShape(layerName, "Polygon", featName, position, offsets, QVector3D(1, 1, 1), true);
                }
            }
        }
    }

    delete layer;
    return true;
}

bool FlatGeobuf::exportLayer(ILayerHost* host, const QString& layerName, const QStringList& shapeIds, const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    QString geoJsonPath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "_temp.geojson";

    // First export as GeoJSON using the extracted GeoJSON module
    bool success = GeoJSON::exportLayer(host, layerName, shapeIds, geoJsonPath);

    if (!success) {
        return false;
    }

    // Try to use ogr2ogr if available (GDAL tool)
    QProcess process;
    QStringList arguments;
    arguments << "-f" << "FlatGeobuf" << filePath << geoJsonPath;

    process.start("ogr2ogr", arguments);
    bool conversionSuccess = process.waitForFinished(30000); // 30 second timeout

    if (conversionSuccess && process.exitCode() == 0) {
        // Clean up temporary GeoJSON
        QFile::remove(geoJsonPath);
        return true;
    } else {
        // Create instruction file
        QString instructionPath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "_README.txt";
        QFile readmeFile(instructionPath);
        if (readmeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream readme(&readmeFile);
            readme << "FlatGeobuf Export Instructions\n";
            readme << "================================\n\n";
            readme << "Layer: " << layerName << "\n\n";
            readme << "FlatGeobuf requires GDAL/OGR for conversion.\n\n";
            readme << "GeoJSON file created: " << QFileInfo(geoJsonPath).fileName() << "\n\n";
            readme << "To convert to FlatGeobuf:\n";
            readme << "1. Install GDAL: https://gdal.org/download.html\n";
            readme << "2. Run: ogr2ogr -f FlatGeobuf output.fgb " << QFileInfo(geoJsonPath).fileName() << "\n\n";
            readme << "Or use QGIS:\n";
            readme << "1. Load the .geojson file\n";
            readme << "2. Right-click layer > Export > Save Features As\n";
            readme << "3. Select 'FlatGeobuf' format\n";
            readmeFile.close();
        }
        return true; // Return true since GeoJSON was created
    }
}
