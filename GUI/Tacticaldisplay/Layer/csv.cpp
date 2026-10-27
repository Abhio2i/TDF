#include "csv.h"
#include "GUI/Tacticaldisplay/Layer/ILayerHost.h"
#include "GUI/Tacticaldisplay/Shape/mesh_entry.h"
#include <core/Hierarchy/Components/mesh.h>
#include <core/Hierarchy/Struct/vector.h>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QVector3D>
#include <QtMath>
#include <cmath>

// CHAITANYA JAIN CHANGE:
// Why: Extracted CSV export functionality from LayerPanel into a standalone module.

static QStringList parseCSVLineStr(const QString& line) {
    QStringList fields;
    QString currentField;
    bool inQuotes = false;
    for (int i = 0; i < line.length(); ++i) {
        QChar c = line[i];
        if (c == '"') {
            if (i + 1 < line.length() && line[i+1] == '"') {
                currentField += '"';
                i++;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            fields.append(currentField);
            currentField.clear();
        } else {
            currentField += c;
        }
    }
    fields.append(currentField);
    return fields;
}

static QList<QVector3D> parseWKTPoints(const QString& wkt) {
    QList<QVector3D> pts;
    int start = wkt.indexOf('(');
    int end = wkt.lastIndexOf(')');
    if (start != -1 && end != -1 && end > start) {
        QString coords = wkt.mid(start + 1, end - start - 1);
        coords.replace("(", "").replace(")", "");
        QStringList pairs = coords.split(',');
        for (const QString& pair : pairs) {
            QStringList xy = pair.trimmed().split(' ', Qt::SkipEmptyParts);
            if (xy.size() >= 2) {
                pts.append(QVector3D(xy[0].toDouble(), xy[1].toDouble(), 0));
            }
        }
    }
    return pts;
}

bool CSV::importLayer(ILayerHost* host, const QString& filePath, const QString& layerName)
{
    qDebug() << "===== CSV::importLayer CALLED =====";

    if (!host)
        return false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);

    QString headerLine = in.readLine();
    QStringList headers = parseCSVLineStr(headerLine);

    int wktIdx = headers.indexOf("Geometry_WKT");
    if (wktIdx == -1) wktIdx = headers.indexOf("WKT");
    if (wktIdx == -1) wktIdx = headers.indexOf("geom");
    if (wktIdx == -1) wktIdx = headers.indexOf("geometry");

    int nameIdx = headers.indexOf("Name");
    int typeIdx = headers.indexOf("Type");

    if (wktIdx == -1) {
        qDebug() << "No WKT column found!";
        return false;
    }

    while (!in.atEnd())
    {
        QString line = in.readLine();
        if (line.trimmed().isEmpty())
            continue;

        QStringList fields = parseCSVLineStr(line);
        if (fields.size() <= wktIdx)
            continue;

        QString wkt = fields[wktIdx].trimmed();
        QString wktUpper = wkt.toUpper();

        qDebug() << "WKT =" << wkt;

        if (wkt.isEmpty())
            continue;

        QString name =
            (nameIdx >= 0 && nameIdx < fields.size()) ? fields[nameIdx] : "";

        QString typeHint =
            (typeIdx >= 0 && typeIdx < fields.size()) ? fields[typeIdx] : "";

        QList<QVector3D> pts = parseWKTPoints(wkt);

        qDebug() << "Points parsed =" << pts.size();

        if (pts.isEmpty())
            continue;

        // ---------------- POINT ----------------
        if (wktUpper.contains("POINT"))
        {
            QVector3D position = pts[0];

            std::vector<QVector3D> offsets;
            offsets.push_back(QVector3D(0,0,0));

            host->registerImportedShape(
                layerName,
                "Point",
                name,
                position,
                offsets,
                QVector3D(1,1,1),
                true);
        }

        // ---------------- LINE ----------------
        else if (wktUpper.contains("LINESTRING"))
        {
            double avgLon = 0;
            double avgLat = 0;

            for (const auto &p : pts)
            {
                avgLon += p.x();
                avgLat += p.y();
            }

            avgLon /= pts.size();
            avgLat /= pts.size();

            QVector3D position(avgLon, avgLat, 0);

            std::vector<QVector3D> offsets;

            for (const auto &p : pts)
            {
                offsets.push_back(
                    QVector3D(
                        p.x() - avgLon,
                        p.y() - avgLat,
                        0));
            }

            host->registerImportedShape(
                layerName,
                "Line",
                name,
                position,
                offsets,
                QVector3D(1,1,1),
                false);
        }

        // ---------------- POLYGON + MULTIPOLYGON ----------------
        else if (wktUpper.contains("POLYGON"))
        {
            double avgLon = 0;
            double avgLat = 0;

            for (const auto &p : pts)
            {
                avgLon += p.x();
                avgLat += p.y();
            }

            avgLon /= pts.size();
            avgLat /= pts.size();

            QVector3D position(avgLon, avgLat, 0);

            if (typeHint == "Circle" || typeHint == "TempCircle")
            {
                double radius = qAbs(pts[0].x() - avgLon);

                host->registerImportedShape(
                    layerName,
                    "Circle",
                    name,
                    position,
                    std::vector<QVector3D>(),
                    QVector3D(radius, radius, 1),
                    true);
            }
            else
            {
                std::vector<QVector3D> offsets;

                for (const auto &p : pts)
                {
                    offsets.push_back(
                        QVector3D(
                            p.x() - avgLon,
                            p.y() - avgLat,
                            0));
                }

                host->registerImportedShape(
                    layerName,
                    "Polygon",
                    name,
                    position,
                    offsets,
                    QVector3D(1,1,1),
                    true);
            }
        }
        else
        {
            qDebug() << "Unsupported geometry:" << wktUpper.left(40);
        }
    }

    return true;
}

bool CSV::exportLayer(ILayerHost* host, const QString& layerName, const QStringList& shapeIds, const QString& filePath)
{
    if (!host) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);

    // CSV Header
    out << "Name,Type,Longitude,Latitude,Geometry_WKT\n";

    int exportedCount = 0;

    std::vector<MeshEntry> matches = host->getMeshEntriesForShapes(shapeIds);
    for (const MeshEntry& entry : matches) {
        // CHAITANYA JAIN CHANGE:
        // Why: Ignore non-GIS shapes (like Bitmaps, Texts) in export logic.
        if (getShapeType(entry) == "Unknown") continue;

        QString csvLine = createCSVLine(entry);
        if (!csvLine.isEmpty()) {
            out << csvLine << "\n";
            exportedCount++;
        }
    }

    file.close();

    // CREATE CSVT FILE FOR QGIS AUTO-DETECTION
    QFileInfo csvFileInfo(filePath);
    QString csvtPath = csvFileInfo.absolutePath() + "/" + csvFileInfo.completeBaseName() + ".csvt";

    QFile csvtFile(csvtPath);
    if (csvtFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream csvtOut(&csvtFile);
        csvtOut << "\"String\",\"String\",\"Real\",\"Real\",\"WKT\"\n";
        csvtFile.close();
    }

    // CREATE README WITH IMPORT INSTRUCTIONS
    QString readmePath = csvFileInfo.absolutePath() + "/" + csvFileInfo.completeBaseName() + "_IMPORT_INSTRUCTIONS.txt";
    QFile readmeFile(readmePath);
    if (readmeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream readme(&readmeFile);
        readme << "CSV + WKT Geometry Import Instructions for QGIS\n";
        readme << "=================================================\n\n";
        readme << "Layer: " << layerName << "\n";
        readme << "Total features: " << exportedCount << "\n\n";
        readme << "FILES CREATED:\n";
        readme << "  1. " << csvFileInfo.fileName() << " - CSV data file\n";
        readme << "  2. " << QFileInfo(csvtPath).fileName() << " - Column type definitions\n\n";
        readme << "IMPORTING IN QGIS:\n";
        readme << "==================\n\n";
        readme << "METHOD 1: Drag and Drop (Easiest)\n";
        readme << "  1. Simply drag the .csv file into QGIS\n";
        readme << "  2. QGIS will auto-detect geometry from WKT column (using .csvt file)\n";
        readme << "  3. All shapes should display on map\n\n";
        readme << "METHOD 2: Add Delimited Text Layer\n";
        readme << "  1. Layer → Add Layer → Add Delimited Text Layer\n";
        readme << "  2. Select: " << csvFileInfo.fileName() << "\n";
        readme << "  3. File Format: CSV\n";
        readme << "  4. Geometry Definition: Well Known Text (WKT)\n";
        readme << "  5. Geometry field: Geometry_WKT\n";
        readme << "  6. Geometry CRS: EPSG:4326 (WGS 84)\n";
        readme << "  7. Click Add\n\n";
        readme << "TROUBLESHOOTING:\n";
        readme << "================\n";
        readme << "If geometries don't show:\n";
        readme << "  - Verify the .csvt file is in the same folder as .csv\n";
        readme << "  - Manually select 'WKT' as geometry type in import dialog\n";
        readme << "  - Check that CRS is set to EPSG:4326\n\n";
        readme << "ALTERNATIVE: Use GeoJSON Instead\n";
        readme << "=================================\n";
        readme << "For easier import, export as GeoJSON format:\n";
        readme << "  - GeoJSON is automatically recognized by QGIS\n";
        readme << "  - No manual configuration needed\n";
        readme << "  - Just drag and drop - it works!\n";
        readmeFile.close();
    }

    return exportedCount > 0;
}

QString CSV::createCSVLine(const MeshEntry& entry)
{
    if (!entry.position || !entry.rotation || !entry.mesh) {
        return QString();
    }

    QStringList fields;

    // Name (escaped for CSV)
    QString name = entry.name;
    if (name.contains(',') || name.contains('"')) {
        name = "\"" + name.replace("\"", "\"\"") + "\"";
    }
    fields << name;

    // Type
    fields << getShapeType(entry);

    // Center coordinates
    QVector3D center = *entry.position;
    fields << QString::number(center.x(), 'f', 8);
    fields << QString::number(center.y(), 'f', 8);

    // Geometry as WKT (Well-Known Text)
    QString wkt = createWKTGeometry(entry);
    if (wkt.contains(',') || wkt.contains('"')) {
        wkt = "\"" + wkt.replace("\"", "\"\"") + "\"";
    }
    fields << wkt;

    return fields.join(",");
}

QString CSV::createWKTGeometry(const MeshEntry& entry)
{
    QString shapeName = entry.name;
    QVector3D center = *entry.position;

    float rotation = entry.rotation->z();
    float cosFwd = std::cos(rotation);
    float sinFwd = std::sin(rotation);

    if (shapeName.startsWith("TempCircle")) {
        // Polygon
        double radius = entry.size ? entry.size->x() : 0.001;
        const int numPoints = 36;

        QStringList points;
        for (int i = 0; i <= numPoints; i++) {
            double angle = (i * 360.0 / numPoints) * M_PI / 180.0;
            double lat = center.y() + radius * qSin(angle);
            double lon = center.x() + radius * qCos(angle);
            points << QString("%1 %2").arg(lon, 0, 'f', 8).arg(lat, 0, 'f', 8);
        }

        return QString("POLYGON((%1))").arg(points.join(","));

    } else if (shapeName.startsWith("TempPoint")) {
        // Point
        return QString("POINT(%1 %2)")
            .arg(center.x(), 0, 'f', 8)
            .arg(center.y(), 0, 'f', 8);

    } else if (shapeName.startsWith("TempPolyline") || shapeName.startsWith("Line")) {
        // LineString
        QStringList points;
        for (Vector* v : entry.mesh->polygen) {
            float worldX = v->x * cosFwd - v->y * sinFwd;
            float worldY = v->x * sinFwd + v->y * cosFwd;
            double lon = center.x() + worldX;
            double lat = center.y() + worldY;
            points << QString("%1 %2").arg(lon, 0, 'f', 8).arg(lat, 0, 'f', 8);
        }

        return QString("LINESTRING(%1)").arg(points.join(","));

    } else {
        // Polygon (Rectangle, Polygon)
        QStringList points;
        for (Vector* v : entry.mesh->polygen) {
            float worldX = v->x * cosFwd - v->y * sinFwd;
            float worldY = v->x * sinFwd + v->y * cosFwd;
            double lon = center.x() + worldX;
            double lat = center.y() + worldY;
            points << QString("%1 %2").arg(lon, 0, 'f', 8).arg(lat, 0, 'f', 8);
        }

        // Close the ring
        if (!entry.mesh->polygen.empty()) {
            Vector* first = entry.mesh->polygen[0];
            float worldX = first->x * cosFwd - first->y * sinFwd;
            float worldY = first->x * sinFwd + first->y * cosFwd;
            double lon = center.x() + worldX;
            double lat = center.y() + worldY;
            points << QString("%1 %2").arg(lon, 0, 'f', 8).arg(lat, 0, 'f', 8);
        }

        return QString("POLYGON((%1))").arg(points.join(","));
    }
}

QString CSV::getShapeType(const MeshEntry& entry)
{
    QString shapeName = entry.name;
    if (shapeName.startsWith("TempCircle") || shapeName.startsWith("Circle")) return "Circle";
    if (shapeName.startsWith("TempRectangle") || shapeName.startsWith("Rectangle")) return "Rectangle";
    if (shapeName.startsWith("TempPolygon") || shapeName.startsWith("Polygon")) return "Polygon";
    if (shapeName.startsWith("TempPolyline") || shapeName.startsWith("Polyline") || shapeName.startsWith("Line")) return "Line";
    if (shapeName.startsWith("TempPoint") || shapeName.startsWith("Point")) return "Point";

    return "Unknown";
}