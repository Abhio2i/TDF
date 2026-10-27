#include "gml.h"
#include "GUI/Tacticaldisplay/Layer/ILayerHost.h"
#include "GUI/Tacticaldisplay/Shape/mesh_entry.h"

#include <core/Hierarchy/Components/mesh.h>
#include <core/Hierarchy/Struct/vector.h>

#include <QFile>
#include <QTextStream>
#include <QVector3D>
#include <QXmlStreamReader>
#include <QStringList>
#include <QtMath>

#include <cmath>

// ============================================================================
// CHAITANYA JAIN CHANGE:
// Why:
// Reworked GML import parsing to prevent stretched/scattered geometries.
//
// Old behavior:
// - Assumed every GML coordinate pair was always longitude, latitude.
// - Assumed every posList always contained exactly 2 dimensions.
// - Mixed geometry state while parsing nested GML feature elements.
// - Did not safely handle gml:pos.
// - Did not correctly preserve separate geometry boundaries.
// - Could misread QGIS EPSG:4326 axis order.
//
// New behavior:
// - Supports gml:coordinates, gml:pos and gml:posList.
// - Handles srsDimension="2" and srsDimension="3".
// - Detects EPSG:4326 latitude/longitude axis order conservatively.
// - Keeps all format parsing inside GML.
// - Sends only normalized geographic coordinates to ILayerHost.
// - No CanvasWidget dependency.
// - No LayerImportFeature dependency.
// ============================================================================

namespace
{

QString normalizedText(QString text)
{
    text.replace('\n', ' ');
    text.replace('\r', ' ');
    text.replace('\t', ' ');
    return text.simplified();
}

bool isFiniteCoordinate(double x, double y)
{
    return std::isfinite(x) && std::isfinite(y);
}

bool looksLikeGeographic(double x, double y)
{
    return qAbs(x) <= 180.0 && qAbs(y) <= 90.0;
}

// CHAITANYA JAIN CHANGE:
// Why:
// GML 3.x with EPSG:4326 commonly follows official latitude/longitude
// axis ordering, unlike GeoJSON which is always longitude/latitude.
//
// IMPORTANT:
// This is used only when the SRS explicitly identifies EPSG:4326
// in a form that normally follows official EPSG axis order.
bool shouldSwapAxisOrder(const QString& srsName)
{
    QString srs = srsName.trimmed().toLower();

    if (srs.isEmpty()) {
        return false;
    }

    // CRS84 explicitly means longitude, latitude.
    if (srs.contains("crs84")) {
        return false;
    }

    // Common URN / HTTP GML EPSG:4326 forms use official EPSG axis order.
    if (srs.contains("urn:ogc:def:crs:epsg") &&
        srs.contains("4326")) {
        return true;
    }

    if (srs.contains("opengis.net/def/crs/epsg") &&
        srs.contains("4326")) {
        return true;
    }

    // Plain "EPSG:4326" is intentionally not swapped automatically.
    // Many real-world files use it as traditional X/Y = lon/lat.
    return false;
}

QVector3D makeGeographicPoint(double first,
                              double second,
                              bool swapAxis)
{
    double lon = first;
    double lat = second;

    if (swapAxis) {
        lon = second;
        lat = first;
    }

    return QVector3D(lon, lat, 0.0);
}

void appendCoordinatesText(const QString& text,
                           QList<QVector3D>& points,
                           bool swapAxis)
{
    const QString clean = normalizedText(text);
    const QStringList tuples =
        clean.split(' ', Qt::SkipEmptyParts);

    for (const QString& tuple : tuples) {
        const QStringList values =
            tuple.split(',', Qt::SkipEmptyParts);

        if (values.size() < 2) {
            continue;
        }

        bool okFirst = false;
        bool okSecond = false;

        const double first = values[0].toDouble(&okFirst);
        const double second = values[1].toDouble(&okSecond);

        if (!okFirst || !okSecond ||
            !isFiniteCoordinate(first, second)) {
            continue;
        }

        points.append(
            makeGeographicPoint(first, second, swapAxis)
            );
    }
}

void appendPosText(const QString& text,
                   QList<QVector3D>& points,
                   bool swapAxis)
{
    const QStringList values =
        normalizedText(text).split(' ', Qt::SkipEmptyParts);

    if (values.size() < 2) {
        return;
    }

    bool okFirst = false;
    bool okSecond = false;

    const double first = values[0].toDouble(&okFirst);
    const double second = values[1].toDouble(&okSecond);

    if (!okFirst || !okSecond ||
        !isFiniteCoordinate(first, second)) {
        return;
    }

    points.append(
        makeGeographicPoint(first, second, swapAxis)
        );
}

void appendPosListText(const QString& text,
                       QList<QVector3D>& points,
                       int dimension,
                       bool swapAxis)
{
    const QStringList values =
        normalizedText(text).split(' ', Qt::SkipEmptyParts);

    if (dimension < 2) {
        dimension = 2;
    }

    for (int i = 0;
         i + 1 < values.size();
         i += dimension) {

        bool okFirst = false;
        bool okSecond = false;

        const double first = values[i].toDouble(&okFirst);
        const double second = values[i + 1].toDouble(&okSecond);

        if (!okFirst || !okSecond ||
            !isFiniteCoordinate(first, second)) {
            continue;
        }

        points.append(
            makeGeographicPoint(first, second, swapAxis)
            );
    }
}

bool registerGeometry(ILayerHost* host,
                      const QString& layerName,
                      const QString& geometryType,
                      const QString& featureName,
                      const QList<QVector3D>& points)
{
    if (!host || points.isEmpty() || geometryType.isEmpty()) {
        return false;
    }

    if (geometryType == "Point") {
        const QVector3D position = points.first();

        if (!isFiniteCoordinate(position.x(), position.y())) {
            return false;
        }

        std::vector<QVector3D> offsets;
        offsets.emplace_back(0.0f, 0.0f, 0.0f);

        host->registerImportedShape(
            layerName,
            "Point",
            featureName,
            position,
            offsets,
            QVector3D(1, 1, 1),
            true
            );

        return true;
    }

    double sumLon = 0.0;
    double sumLat = 0.0;
    int validCount = 0;

    for (const QVector3D& point : points) {
        if (!isFiniteCoordinate(point.x(), point.y())) {
            continue;
        }

        sumLon += point.x();
        sumLat += point.y();
        ++validCount;
    }

    if (validCount == 0) {
        return false;
    }

    const double centerLon = sumLon / validCount;
    const double centerLat = sumLat / validCount;

    QVector3D position(centerLon, centerLat, 0.0);

    std::vector<QVector3D> offsets;
    offsets.reserve(static_cast<size_t>(validCount));

    for (const QVector3D& point : points) {
        if (!isFiniteCoordinate(point.x(), point.y())) {
            continue;
        }

        offsets.emplace_back(
            point.x() - centerLon,
            point.y() - centerLat,
            0.0
            );
    }

    if (geometryType == "LineString") {
        if (offsets.size() < 2) {
            return false;
        }

        host->registerImportedShape(
            layerName,
            "Line",
            featureName,
            position,
            offsets,
            QVector3D(1, 1, 1),
            false
            );

        return true;
    }

    if (geometryType == "Polygon") {
        if (offsets.size() < 3) {
            return false;
        }

        // CHAITANYA JAIN CHANGE:
        // Why:
        // Avoid storing the duplicated closing vertex twice.
        // CanvasWidget closes imported polygons through closePath=true.
        if (offsets.size() > 3) {
            const QVector3D& first = offsets.front();
            const QVector3D& last = offsets.back();

            const double epsilon = 1e-10;

            if (qAbs(first.x() - last.x()) < epsilon &&
                qAbs(first.y() - last.y()) < epsilon) {
                offsets.pop_back();
            }
        }

        host->registerImportedShape(
            layerName,
            "Polygon",
            featureName,
            position,
            offsets,
            QVector3D(1, 1, 1),
            true
            );

        return true;
    }

    return false;
}

} // anonymous namespace


bool GML::importLayer(ILayerHost* host,
                      const QString& filePath,
                      const QString& layerName)
{
    if (!host) {
        return false;
    }

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QXmlStreamReader xml(&file);

    QString currentFeatureName;
    QString currentGeometryType;
    QString currentSrsName;

    QList<QVector3D> currentPoints;

    bool insideGeometry = false;
    bool importedAnyGeometry = false;

    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType token =
            xml.readNext();

        if (token == QXmlStreamReader::StartElement) {

            const QString tag =
                xml.name().toString();

            // ------------------------------------------------------------
            // Feature name
            // ------------------------------------------------------------
            if (tag == "name" && !insideGeometry) {
                currentFeatureName =
                    xml.readElementText().trimmed();

                continue;
            }

            // ------------------------------------------------------------
            // Geometry start
            // ------------------------------------------------------------
            if (tag == "Point" ||
                tag == "LineString" ||
                tag == "Polygon") {

                currentGeometryType = tag;
                currentPoints.clear();
                insideGeometry = true;

                currentSrsName =
                    xml.attributes()
                        .value("srsName")
                        .toString();

                continue;
            }

            if (!insideGeometry) {
                continue;
            }

            // ------------------------------------------------------------
            // Coordinate SRS can also exist on nested coordinate elements
            // ------------------------------------------------------------
            const QString elementSrs =
                xml.attributes()
                    .value("srsName")
                    .toString();

            if (!elementSrs.isEmpty()) {
                currentSrsName = elementSrs;
            }

            const bool swapAxis =
                shouldSwapAxisOrder(currentSrsName);

            // ------------------------------------------------------------
            // GML 2.x coordinates
            // Example:
            // <gml:coordinates>lon,lat lon,lat</gml:coordinates>
            // ------------------------------------------------------------
            if (tag == "coordinates") {

                const QString coordinateText =
                    xml.readElementText();

                appendCoordinatesText(
                    coordinateText,
                    currentPoints,
                    swapAxis
                    );

                continue;
            }

            // ------------------------------------------------------------
            // GML 3.x single position
            // Example:
            // <gml:pos>lat lon</gml:pos>
            // ------------------------------------------------------------
            if (tag == "pos") {

                const QString positionText =
                    xml.readElementText();

                appendPosText(
                    positionText,
                    currentPoints,
                    swapAxis
                    );

                continue;
            }

            // ------------------------------------------------------------
            // GML 3.x position list
            // Supports both:
            // srsDimension="2"
            // srsDimension="3"
            // ------------------------------------------------------------
            if (tag == "posList") {

                int dimension = 2;

                const QString dimensionText =
                    xml.attributes()
                        .value("srsDimension")
                        .toString();

                if (!dimensionText.isEmpty()) {
                    bool ok = false;
                    const int parsedDimension =
                        dimensionText.toInt(&ok);

                    if (ok && parsedDimension >= 2) {
                        dimension = parsedDimension;
                    }
                }

                const QString positionListText =
                    xml.readElementText();

                appendPosListText(
                    positionListText,
                    currentPoints,
                    dimension,
                    swapAxis
                    );

                continue;
            }

        } else if (token == QXmlStreamReader::EndElement) {

            const QString tag =
                xml.name().toString();

            if (insideGeometry &&
                tag == currentGeometryType) {

                // CHAITANYA JAIN CHANGE:
                // Why:
                // Register exactly once when one complete GML geometry
                // has finished parsing. This prevents geometry state
                // from leaking into the next feature.

                if (registerGeometry(
                        host,
                        layerName,
                        currentGeometryType,
                        currentFeatureName,
                        currentPoints)) {

                    importedAnyGeometry = true;
                }

                currentGeometryType.clear();
                currentSrsName.clear();
                currentPoints.clear();
                insideGeometry = false;
            }
        }
    }

    const bool parseSuccess = !xml.hasError();

    file.close();

    return parseSuccess && importedAnyGeometry;
}


bool GML::exportLayer(ILayerHost* host,
                      const QString& layerName,
                      const QStringList& shapeIds,
                      const QString& filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly |
                   QIODevice::Text |
                   QIODevice::Truncate)) {
        return false;
    }

    QTextStream out(&file);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

    out << "<gml:FeatureCollection\n";
    out << "  xmlns:gml=\"http://www.opengis.net/gml\"\n";
    out << "  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n";

    out << "  <gml:name>"
        << layerName.toHtmlEscaped()
        << "</gml:name>\n";

    if (host) {
        for (const MeshEntry& entry :
             host->getMeshEntriesForShapes(shapeIds)) {

            // CHAITANYA JAIN CHANGE:
            // Why:
            // Export only supported GIS shapes.
            if (getShapeType(entry) == "Unknown") {
                continue;
            }

            const QString gmlFeature =
                createGMLFeature(entry, entry.name);

            if (!gmlFeature.isEmpty()) {
                out << gmlFeature;
            }
        }
    }

    out << "</gml:FeatureCollection>\n";

    file.close();

    return true;
}


QString GML::createGMLFeature(const MeshEntry& entry,
                              const QString& shapeId)
{
    const QString shapeType = getShapeType(entry);

    if (shapeType == "Unknown") {
        return QString();
    }

    const QString geometry =
        getGeometryAsGML(entry);

    if (geometry.isEmpty()) {
        return QString();
    }

    QString gml;

    gml += "  <gml:featureMember>\n";
    gml += "    <Feature>\n";

    gml += "      <gml:name>";
    gml += entry.name.toHtmlEscaped();
    gml += "</gml:name>\n";

    gml += "      <gml:description>";
    gml += QString("Type: %1, ID: %2")
               .arg(shapeType.toHtmlEscaped(),
                    shapeId.toHtmlEscaped());
    gml += "</gml:description>\n";

    gml += geometry;

    gml += "    </Feature>\n";
    gml += "  </gml:featureMember>\n";

    return gml;
}


QString GML::getGeometryAsGML(const MeshEntry& entry)
{
    QString gml;

    if (!entry.position ||
        !entry.rotation ||
        !entry.mesh) {
        return gml;
    }

    const QString shapeName = entry.name;
    const QVector3D center = *entry.position;

    const float rotation = entry.rotation->z();
    const float cosFwd = std::cos(rotation);
    const float sinFwd = std::sin(rotation);

    // --------------------------------------------------------------------
    // Circle
    // --------------------------------------------------------------------
    if (shapeName.startsWith("TempCircle") ||
        shapeName.startsWith("Circle")) {

        const double radius =
            entry.size ? entry.size->x() : 0.001;

        if (!std::isfinite(radius) || radius <= 0.0) {
            return QString();
        }

        QStringList coords;

        // More points = smoother circle
        const int numPoints = 72;

        // Longitude correction based on latitude
        double cosLat = std::cos(qDegreesToRadians(center.y()));
        if (std::abs(cosLat) < 0.01)
            cosLat = 0.01;

        for (int i = 0; i <= numPoints; ++i) {

            const double angle =
                (2.0 * M_PI * i) / numPoints;

            // Circle in local coordinates
            const double dx = radius * std::cos(angle);
            const double dy = radius * std::sin(angle);

            // Compensate longitude distortion
            const double lon =
                center.x() + (dx / cosLat);

            const double lat =
                center.y() + dy;

            coords.append(
                QString::number(lon, 'f', 10) +
                "," +
                QString::number(lat, 'f', 10)
                );
        }

        gml += "      <gml:Polygon srsName=\"CRS:84\">\n";
        gml += "        <gml:outerBoundaryIs>\n";
        gml += "          <gml:LinearRing>\n";
        gml += "            <gml:coordinates>\n";
        gml += "              " + coords.join(" ") + "\n";
        gml += "            </gml:coordinates>\n";
        gml += "          </gml:LinearRing>\n";
        gml += "        </gml:outerBoundaryIs>\n";
        gml += "      </gml:Polygon>\n";

        return gml;
    }

    // --------------------------------------------------------------------
    // Polyline
    // --------------------------------------------------------------------
    if (shapeName.startsWith("TempPolyline") ||
        shapeName.startsWith("Polyline") ||
        shapeName.startsWith("Line")) {

        if (entry.mesh->polygen.size() < 2) {
            return QString();
        }

        QStringList coords;

        for (Vector* vertex : entry.mesh->polygen) {

            if (!vertex) {
                continue;
            }

            const double worldX =
                vertex->x * cosFwd -
                vertex->y * sinFwd;

            const double worldY =
                vertex->x * sinFwd +
                vertex->y * cosFwd;

            const double lon =
                center.x() + worldX;

            const double lat =
                center.y() + worldY;

            coords.append(
                QString::number(lon, 'f', 10) +
                "," +
                QString::number(lat, 'f', 10)
                );
        }

        if (coords.size() < 2) {
            return QString();
        }

        gml += "      <gml:LineString srsName=\"CRS:84\">\n";
        gml += "        <gml:coordinates>\n";
        gml += "          " + coords.join(" ") + "\n";
        gml += "        </gml:coordinates>\n";
        gml += "      </gml:LineString>\n";

        return gml;
    }

    // --------------------------------------------------------------------
    // Rectangle / Polygon
    // --------------------------------------------------------------------
    if (shapeName.startsWith("TempRectangle") ||
        shapeName.startsWith("Rectangle") ||
        shapeName.startsWith("TempPolygon") ||
        shapeName.startsWith("Polygon")) {

        if (entry.mesh->polygen.size() < 3) {
            return QString();
        }

        QStringList coords;

        for (Vector* vertex : entry.mesh->polygen) {

            if (!vertex) {
                continue;
            }

            const double worldX =
                vertex->x * cosFwd -
                vertex->y * sinFwd;

            const double worldY =
                vertex->x * sinFwd +
                vertex->y * cosFwd;

            const double lon =
                center.x() + worldX;

            const double lat =
                center.y() + worldY;

            coords.append(
                QString::number(lon, 'f', 10) +
                "," +
                QString::number(lat, 'f', 10)
                );
        }

        if (coords.size() < 3) {
            return QString();
        }

        // Close exactly once.
        if (coords.first() != coords.last()) {
            coords.append(coords.first());
        }

        gml += "      <gml:Polygon srsName=\"CRS:84\">\n";
        gml += "        <gml:outerBoundaryIs>\n";
        gml += "          <gml:LinearRing>\n";
        gml += "            <gml:coordinates>\n";
        gml += "              " + coords.join(" ") + "\n";
        gml += "            </gml:coordinates>\n";
        gml += "          </gml:LinearRing>\n";
        gml += "        </gml:outerBoundaryIs>\n";
        gml += "      </gml:Polygon>\n";

        return gml;
    }

    return QString();
}


QString GML::getShapeType(const MeshEntry& entry)
{
    const QString shapeName = entry.name;

    // CHAITANYA JAIN CHANGE:
    // Why:
    // Imported and native interactive shapes use Temp* prefixes.
    // The old GML implementation ignored these names during export.

    if (shapeName.startsWith("TempCircle") ||
        shapeName.startsWith("Circle")) {
        return "Circle";
    }

    if (shapeName.startsWith("TempRectangle") ||
        shapeName.startsWith("Rectangle")) {
        return "Rectangle";
    }

    if (shapeName.startsWith("TempPolygon") ||
        shapeName.startsWith("Polygon")) {
        return "Polygon";
    }

    if (shapeName.startsWith("TempPolyline") ||
        shapeName.startsWith("Polyline") ||
        shapeName.startsWith("Line")) {
        return "Line";
    }

    if (shapeName.startsWith("TempPoint") ||
        shapeName.startsWith("Point")) {
        return "Point";
    }

    return "Unknown";
}
