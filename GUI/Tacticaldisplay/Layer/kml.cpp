#include "kml.h"
#include "GUI/Tacticaldisplay/Layer/ILayerHost.h"
#include "GUI/Tacticaldisplay/Shape/mesh_entry.h"
#include <core/Hierarchy/Components/mesh.h>
#include <core/Hierarchy/Struct/vector.h>
#include <QFile>
#include <QTextStream>
#include <QColor>
#include <QVector3D>
#include <QtMath>
#include <cmath>

// CHAITANYA JAIN CHANGE:
// Why: Extracted KML export functionality from LayerPanel into a standalone module.

#include <QXmlStreamReader>

bool KML::importLayer(ILayerHost* host, const QString& filePath, const QString& layerName)
{
    if (!host) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (xml.name().toString() == "Placemark") {
                QString name;
                QString geomType;
                QList<QVector3D> pts;

                while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString() == "Placemark")) {
                    if (xml.atEnd()) break;
                    xml.readNext();
                    if (xml.tokenType() == QXmlStreamReader::StartElement) {
                        QString tag = xml.name().toString();
                        if (tag == "name") {
                            name = xml.readElementText();
                        } else if (tag == "Point" || tag == "LineString" || tag == "Polygon") {
                            geomType = tag;
                        } else if (tag == "coordinates") {
                            QString coordsStr = xml.readElementText();
                            QStringList coordsList = coordsStr.replace('\n', ' ').replace('\t', ' ').split(' ', Qt::SkipEmptyParts);
                            for (const QString& coord : coordsList) {
                                QStringList parts = coord.split(',');
                                if (parts.size() >= 2) {
                                    pts.append(QVector3D(parts[0].toDouble(), parts[1].toDouble(), 0));
                                }
                            }
                        }
                    }
                }

                if (pts.isEmpty()) continue;

                if (geomType == "Point") {
                    QVector3D position = pts[0];
                    std::vector<QVector3D> offsets = { QVector3D(0, 0, 0) };
                    host->registerImportedShape(layerName, "Point", name, position, offsets, QVector3D(1, 1, 1), true);

                } else if (geomType == "LineString") {
                    double avgLon = 0, avgLat = 0;
                    for (const auto& p : pts) { avgLon += p.x(); avgLat += p.y(); }
                    avgLon /= pts.size(); avgLat /= pts.size();

                    QVector3D position(avgLon, avgLat, 0);
                    std::vector<QVector3D> offsets;
                    for (const auto& p : pts) offsets.push_back(QVector3D(p.x() - avgLon, p.y() - avgLat, 0));
                    host->registerImportedShape(layerName, "Line", name, position, offsets, QVector3D(1, 1, 1), false);

                } else if (geomType == "Polygon") {
                    double avgLon = 0, avgLat = 0;
                    for (const auto& p : pts) { avgLon += p.x(); avgLat += p.y(); }
                    avgLon /= pts.size(); avgLat /= pts.size();

                    QVector3D position(avgLon, avgLat, 0);
                    std::vector<QVector3D> offsets;
                    for (const auto& p : pts) offsets.push_back(QVector3D(p.x() - avgLon, p.y() - avgLat, 0));
                    host->registerImportedShape(layerName, "Polygon", name, position, offsets, QVector3D(1, 1, 1), true);
                }
            }
        }
    }

    return !xml.hasError();
}

bool KML::exportLayer(ILayerHost* host, const QString& layerName, const QStringList& shapeIds, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream out(&file);
    // KML Header
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n";
    out << "  <Document>\n";
    out << "    <name>" << layerName << "</name>\n";

    if (host) {
        for (const MeshEntry& entry : host->getMeshEntriesForShapes(shapeIds)) {
            // CHAITANYA JAIN CHANGE:
            // Why: Ignore non-GIS shapes (like Bitmaps, Texts) in KML export.
            if (getShapeType(entry) == "Unknown") continue;

            QString kmlPlacemark = createKMLPlacemark(entry, entry.name);
            if (!kmlPlacemark.isEmpty()) {
                out << kmlPlacemark;
            }
        }
    }

    out << "  </Document>\n";
    out << "</kml>\n";

    file.close();
    return true;
}

QString KML::createKMLPlacemark(const MeshEntry& entry, const QString& shapeId)
{
    QString kml;
    kml += "    <Placemark>\n";
    kml += "      <name>" + entry.name + "</name>\n";
    kml += "      <description>Type: " + getShapeType(entry) + ", ID: " + shapeId + "</description>\n";

    // Style with color
    if (entry.mesh && entry.mesh->color) {
        QColor c = *entry.mesh->color;
        // KML color format is AABBGGRR (alpha, blue, green, red)
        QString kmlColor = QString("%1%2%3%4")
                               .arg(c.alpha(), 2, 16, QChar('0'))
                               .arg(c.blue(), 2, 16, QChar('0'))
                               .arg(c.green(), 2, 16, QChar('0'))
                               .arg(c.red(), 2, 16, QChar('0'));

        kml += "      <Style>\n";
        kml += "        <LineStyle>\n";
        kml += "          <color>" + kmlColor + "</color>\n";
        kml += "          <width>2</width>\n";
        kml += "        </LineStyle>\n";
        kml += "        <PolyStyle>\n";
        // Make polygon semi-transparent (alpha = 0x7f)
        QString fillColor = QString("7f%1%2%3")
                                .arg(c.blue(), 2, 16, QChar('0'))
                                .arg(c.green(), 2, 16, QChar('0'))
                                .arg(c.red(), 2, 16, QChar('0'));
        kml += "          <color>" + fillColor + "</color>\n";
        kml += "        </PolyStyle>\n";
        kml += "      </Style>\n";
    }

    QString geometry = getGeometryAsKML(entry);
    if (!geometry.isEmpty()) {
        kml += geometry;
    }

    kml += "    </Placemark>\n";
    return kml;
}

QString KML::getGeometryAsKML(const MeshEntry& entry)
{
    QString kml;

    if (!entry.position || !entry.rotation || !entry.mesh) {
        return kml;
    }

    QString shapeName = entry.name;

    QVector3D center = *entry.position;

    float rotation = entry.rotation->z();
    float cosFwd = std::cos(rotation);
    float sinFwd = std::sin(rotation);

    if (shapeName.startsWith("TempCircle"))
    {
        kml += "      <Polygon>\n";
        kml += "        <outerBoundaryIs>\n";
        kml += "          <LinearRing>\n";
        kml += "            <coordinates>\n";

        double radius = entry.size ? entry.size->x() : 0.001;

        // Latitude correction so circle stays circular
        double latRad = qDegreesToRadians(center.y());
        double cosLat = std::cos(latRad);

        if (std::abs(cosLat) < 0.000001)
            cosLat = 0.000001;

        double radiusLat = radius;
        double radiusLon = radius / cosLat;

        const int numPoints = 72;

        for (int i = 0; i <= numPoints; ++i)
        {
            double angle = (2.0 * M_PI * i) / numPoints;

            double lon = center.x() + radiusLon * std::cos(angle);
            double lat = center.y() + radiusLat * std::sin(angle);

            kml += "              "
                   + QString::number(lon, 'f', 8)
                   + ","
                   + QString::number(lat, 'f', 8)
                   + ",0\n";
        }

        kml += "            </coordinates>\n";
        kml += "          </LinearRing>\n";
        kml += "        </outerBoundaryIs>\n";
        kml += "      </Polygon>\n";
    }
    else if (shapeName.startsWith("TempPoint"))
    {
        kml += "      <Point>\n";
        kml += "        <coordinates>"
               + QString::number(center.x(),'f',8)
               + ","
               + QString::number(center.y(),'f',8)
               + ",0</coordinates>\n";
        kml += "      </Point>\n";
    }
    else if (shapeName.startsWith("TempPolyline") || shapeName.startsWith("Line"))
    {
        kml += "      <LineString>\n";
        kml += "        <coordinates>\n";

        for (Vector *v : entry.mesh->polygen)
        {
            float worldX = v->x * cosFwd - v->y * sinFwd;
            float worldY = v->x * sinFwd + v->y * cosFwd;

            double lon = center.x() + worldX;
            double lat = center.y() + worldY;

            kml += "          "
                   + QString::number(lon,'f',8)
                   + ","
                   + QString::number(lat,'f',8)
                   + ",0\n";
        }

        kml += "        </coordinates>\n";
        kml += "      </LineString>\n";
    }
    else if (shapeName.startsWith("TempRectangle") ||
             shapeName.startsWith("TempPolygon"))
    {
        kml += "      <Polygon>\n";
        kml += "        <outerBoundaryIs>\n";
        kml += "          <LinearRing>\n";
        kml += "            <coordinates>\n";

        for (Vector *v : entry.mesh->polygen)
        {
            float worldX = v->x * cosFwd - v->y * sinFwd;
            float worldY = v->x * sinFwd + v->y * cosFwd;

            double lon = center.x() + worldX;
            double lat = center.y() + worldY;

            kml += "              "
                   + QString::number(lon,'f',8)
                   + ","
                   + QString::number(lat,'f',8)
                   + ",0\n";
        }

        if (!entry.mesh->polygen.empty())
        {
            Vector *first = entry.mesh->polygen.front();

            float worldX = first->x * cosFwd - first->y * sinFwd;
            float worldY = first->x * sinFwd + first->y * cosFwd;

            double lon = center.x() + worldX;
            double lat = center.y() + worldY;

            kml += "              "
                   + QString::number(lon,'f',8)
                   + ","
                   + QString::number(lat,'f',8)
                   + ",0\n";
        }

        kml += "            </coordinates>\n";
        kml += "          </LinearRing>\n";
        kml += "        </outerBoundaryIs>\n";
        kml += "      </Polygon>\n";
    }

    return kml;
}
QString KML::getShapeType(const MeshEntry& entry)
{
    QString shapeName = entry.name;

    if (shapeName.startsWith("Circle") || shapeName.startsWith("TempCircle")) return "Circle";
    if (shapeName.startsWith("Rectangle") || shapeName.startsWith("TempRectangle")) return "Rectangle";
    if (shapeName.startsWith("Polygon") || shapeName.startsWith("TempPolygon")) return "Polygon";
    if (shapeName.startsWith("Polyline") || shapeName.startsWith("TempPolyline") || shapeName.startsWith("Line")) return "Line";
    if (shapeName.startsWith("Point") || shapeName.startsWith("TempPoint")) return "Point";

    return "Unknown";
}
