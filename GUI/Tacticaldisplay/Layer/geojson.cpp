#include "geojson.h"
#include "GUI/Tacticaldisplay/Layer/ILayerHost.h"
#include "GUI/Tacticaldisplay/Shape/mesh_entry.h"

#include <core/Hierarchy/Components/mesh.h>
#include <core/Hierarchy/Struct/vector.h>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>
#include <QFile>
#include <QVector3D>
#include <QString>
#include <QDebug>
#include <QtMath>

#include <limits>
#include <cmath>


// ============================================================================
// CHAITANYA JAIN CHANGE:
//
// GeoJSON module supports:
//
// - Circle
// - Rectangle
// - Polygon
// - Polyline / Line
// - Point
// - Text
// - Built-in Bitmap:
//      Hospital
//      School
//      Forest Area
//
// IMPORTANT:
//
// GeoJSON has no native Text or Bitmap geometry.
//
// Therefore:
//
// Text:
//      geometry.type = "Point"
//      properties.type = "Text"
//      properties.text = "..."
//
// Bitmap:
//      geometry.type = "Point"
//      properties.type = "Bitmap"
//      properties.bitmapType = "Hospital" / "School" / "Forest Area"
//
// We DO NOT store the Qt resource path in GeoJSON.
//
// During import:
//
// "Hospital"
//      -> registerImportedBitmap(..., "Hospital", ...)
//      -> ShapeEventHandler::getBitmapImagePath("Hospital")
//      -> ":/icons/images/hospital.png"
//
// This keeps exported GeoJSON portable.
// ============================================================================


// ============================================================================
// Import Layer
// ============================================================================

bool GeoJSON::importLayer(
    ILayerHost* host,
    const QString& filePath,
    const QString& layerName)
{
    if (!host) {
        qWarning() << "GeoJSON::importLayer: host is null";
        return false;
    }

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "GeoJSON::importLayer: Could not open file:"
                   << filePath;
        return false;
    }

    QJsonParseError parseError;

    const QJsonDocument doc =
        QJsonDocument::fromJson(
            file.readAll(),
            &parseError
            );

    file.close();


    if (parseError.error != QJsonParseError::NoError) {

        qWarning()
        << "GeoJSON parse error:"
        << parseError.errorString();

        return false;
    }


    if (doc.isNull() || !doc.isObject()) {

        qWarning()
        << "GeoJSON::importLayer: Invalid root object";

        return false;
    }


    const QJsonObject root =
        doc.object();


    if (!root.contains("features") ||
        !root["features"].isArray()) {

        qWarning()
        << "GeoJSON::importLayer: features array missing";

        return false;
    }


    const QJsonArray features =
        root["features"].toArray();


    qDebug()
        << "Importing GeoJSON features:"
        << features.size();


    // =====================================================================
    // Loop through all features
    // =====================================================================

    for (const QJsonValue& value : features)
    {
        if (!value.isObject())
            continue;


        const QJsonObject feature =
            value.toObject();


        // -----------------------------------------------------------------
        // Geometry
        // -----------------------------------------------------------------

        if (!feature.contains("geometry") ||
            !feature["geometry"].isObject()) {

            continue;
        }


        const QJsonObject geometry =
            feature["geometry"].toObject();


        const QString geometryType =
            geometry["type"].toString();


        const QJsonValue coordinatesValue =
            geometry["coordinates"];


        // -----------------------------------------------------------------
        // Properties
        // -----------------------------------------------------------------

        QJsonObject properties;


        if (feature.contains("properties") &&
            feature["properties"].isObject())
        {
            properties =
                feature["properties"].toObject();
        }


        // -----------------------------------------------------------------
        // Feature Name
        // -----------------------------------------------------------------

        QString featureName;


        if (properties.contains("name")) {

            featureName =
                properties["name"].toString();
        }
        else if (properties.contains("Name")) {

            featureName =
                properties["Name"].toString();
        }
        else if (properties.contains("id")) {

            featureName =
                properties["id"].toString();
        }


        // -----------------------------------------------------------------
        // TDF Shape Type
        // -----------------------------------------------------------------

        const QString shapeTypeHint =
            properties["type"].toString();


        // -----------------------------------------------------------------
        // Circle Radius
        // -----------------------------------------------------------------

        double circleRadius =
            properties["radius"].toDouble(0.0);


        // -----------------------------------------------------------------
        // Bitmap Type
        //
        // Expected values:
        //
        // Hospital
        // School
        // Forest Area
        // -----------------------------------------------------------------

        const QString bitmapType =
            properties["bitmapType"].toString();


        // -----------------------------------------------------------------
        // Rotation
        // -----------------------------------------------------------------

        const float rotation =
            static_cast<float>(
                properties["rotation"]
                    .toDouble(
                        properties["rotationZ"]
                            .toDouble(0.0)
                        )
                );


        // -----------------------------------------------------------------
        // Size
        // -----------------------------------------------------------------

        const QVector3D importedSize(

            static_cast<float>(
                properties["sizeX"]
                    .toDouble(1.0)
                ),

            static_cast<float>(
                properties["sizeY"]
                    .toDouble(1.0)
                ),

            static_cast<float>(
                properties["sizeZ"]
                    .toDouble(1.0)
                )
            );


        qDebug()
            << "Import Feature:"
            << featureName
            << "Geometry:"
            << geometryType
            << "TDF Type:"
            << shapeTypeHint
            << "Bitmap Type:"
            << bitmapType;


        // =================================================================
        // POINT GEOMETRY
        //
        // Used by:
        //
        // - Point
        // - Text
        // - Bitmap
        // =================================================================

        if (geometryType.compare(
                "Point",
                Qt::CaseInsensitive) == 0)
        {
            if (!coordinatesValue.isArray())
                continue;


            const QJsonArray coordinates =
                coordinatesValue.toArray();


            if (coordinates.size() < 2)
                continue;


            const double longitude =
                coordinates[0].toDouble();


            const double latitude =
                coordinates[1].toDouble();


            const QVector3D position(
                longitude,
                latitude,
                0
                );


            // =============================================================
            // TEXT
            // =============================================================

            if (shapeTypeHint.compare(
                    "Text",
                    Qt::CaseInsensitive) == 0)
            {
                const QString text =
                    properties["text"].toString();


                qDebug()
                    << "Importing Text:"
                    << featureName
                    << "Text:"
                    << text;


                host->registerImportedText(
                    layerName,
                    featureName,
                    position,
                    text
                    );


                continue;
            }


            // =============================================================
            // BUILT-IN BITMAP
            //
            // GeoJSON contains:
            //
            // "type": "Bitmap"
            // "bitmapType": "Hospital"
            //
            // We pass bitmapType instead of Qt resource path.
            // =============================================================

            if (shapeTypeHint.compare(
                    "Bitmap",
                    Qt::CaseInsensitive) == 0)
            {
                if (bitmapType.isEmpty()) {

                    qWarning()
                    << "Bitmap feature has no bitmapType:"
                    << featureName;

                    continue;
                }


                // Only allow known built-in bitmap types.
                if (bitmapType.compare(
                        "Hospital",
                        Qt::CaseInsensitive) != 0 &&

                        bitmapType.compare(
                            "School",
                            Qt::CaseInsensitive) != 0 &&

                        bitmapType.compare(
                            "Forest Area",
                            Qt::CaseInsensitive) != 0)
                {
                    qWarning()
                    << "Unknown bitmapType:"
                    << bitmapType;

                    continue;
                }


                qDebug()
                    << "Importing Bitmap:"
                    << featureName
                    << "Bitmap Type:"
                    << bitmapType
                    << "Position:"
                    << position;


                host->registerImportedBitmap(
                    layerName,
                    featureName,
                    position,
                    bitmapType,
                    importedSize,
                    rotation
                    );


                continue;
            }


            // =============================================================
            // NORMAL POINT
            // =============================================================

            std::vector<QVector3D> offsets =
            {
                QVector3D(0, 0, 0)
        };


        host->registerImportedShape(
            layerName,
            "Point",
            featureName,
            position,
            offsets,
            importedSize,
            true
            );


        continue;
    }


    // =================================================================
    // LINE STRING
    // =================================================================

    else if (geometryType.compare(
                 "LineString",
                 Qt::CaseInsensitive) == 0)
    {
        if (!coordinatesValue.isArray())
            continue;


        const QJsonArray coordinates =
            coordinatesValue.toArray();


        if (coordinates.isEmpty())
            continue;


        double averageLongitude = 0.0;
        double averageLatitude  = 0.0;

        int validCount = 0;


        // -------------------------------------------------------------
        // Calculate center
        // -------------------------------------------------------------

        for (const QJsonValue& pointValue : coordinates)
        {
            if (!pointValue.isArray())
                continue;


            const QJsonArray point =
                pointValue.toArray();


            if (point.size() < 2)
                continue;


            averageLongitude +=
                point[0].toDouble();


            averageLatitude +=
                point[1].toDouble();


            validCount++;
        }


        if (validCount == 0)
            continue;


        averageLongitude /=
            validCount;


        averageLatitude /=
            validCount;


        const QVector3D position(
            averageLongitude,
            averageLatitude,
            0
            );


        // -------------------------------------------------------------
        // Create offsets
        // -------------------------------------------------------------

        std::vector<QVector3D> offsets;


        for (const QJsonValue& pointValue : coordinates)
        {
            if (!pointValue.isArray())
                continue;


            const QJsonArray point =
                pointValue.toArray();


            if (point.size() < 2)
                continue;


            offsets.push_back(

                QVector3D(

                        point[0].toDouble()
                        - averageLongitude,

                    point[1].toDouble()
                        - averageLatitude,

                    0
                    )
                );
        }


        if (offsets.size() < 2)
            continue;


        host->registerImportedShape(
            layerName,
            "Line",
            featureName,
            position,
            offsets,
            importedSize,
            false
            );


        continue;
    }


    // =================================================================
    // POLYGON
    //
    // Can represent:
    //
    // - Polygon
    // - Rectangle
    // - Circle
    // =================================================================

    else if (geometryType.compare(
                 "Polygon",
                 Qt::CaseInsensitive) == 0)
    {
        if (!coordinatesValue.isArray())
            continue;


        const QJsonArray polygonCoordinates =
            coordinatesValue.toArray();


        if (polygonCoordinates.isEmpty())
            continue;


        // Currently outer ring only.
        if (!polygonCoordinates[0].isArray())
            continue;


        const QJsonArray ring =
            polygonCoordinates[0].toArray();


        if (ring.isEmpty())
            continue;


        // =============================================================
        // CIRCLE
        // =============================================================

        if (shapeTypeHint.compare(
                "Circle",
                Qt::CaseInsensitive) == 0 ||

                shapeTypeHint.compare(
                    "TempCircle",
                    Qt::CaseInsensitive) == 0)
        {
            double minLongitude =
                std::numeric_limits<double>::max();


            double maxLongitude =
                std::numeric_limits<double>::lowest();


            double minLatitude =
                std::numeric_limits<double>::max();


            double maxLatitude =
                std::numeric_limits<double>::lowest();


            int validCount = 0;


            for (const QJsonValue& pointValue : ring)
            {
                if (!pointValue.isArray())
                    continue;


                const QJsonArray point =
                    pointValue.toArray();


                if (point.size() < 2)
                    continue;


                const double longitude =
                    point[0].toDouble();


                const double latitude =
                    point[1].toDouble();


                minLongitude =
                    std::min(
                        minLongitude,
                        longitude
                        );


                maxLongitude =
                    std::max(
                        maxLongitude,
                        longitude
                        );


                minLatitude =
                    std::min(
                        minLatitude,
                        latitude
                        );


                maxLatitude =
                    std::max(
                        maxLatitude,
                        latitude
                        );


                validCount++;
            }


            if (validCount == 0)
                continue;


            const QVector3D position(

                    (minLongitude +
                     maxLongitude) / 2.0,

                (minLatitude +
                 maxLatitude) / 2.0,

                0
                );


            // If radius is missing,
            // calculate it from bounding box.

            if (circleRadius <= 0.0)
            {
                circleRadius =
                    (maxLongitude -
                     minLongitude) / 2.0;
            }


            const QVector3D circleSize(

                circleRadius,

                circleRadius,

                1
                );


            host->registerImportedShape(
                layerName,
                "Circle",
                featureName,
                position,
                std::vector<QVector3D>(),
                circleSize,
                true
                );


            continue;
        }


        // =============================================================
        // POLYGON / RECTANGLE
        // =============================================================

        int vertexCount =
            ring.size();


        // -------------------------------------------------------------
        // Ignore duplicate closing point
        // -------------------------------------------------------------

        if (vertexCount > 1)
        {
            const QJsonArray firstPoint =
                ring.first().toArray();


            const QJsonArray lastPoint =
                ring.last().toArray();


            if (firstPoint.size() >= 2 &&
                lastPoint.size() >= 2)
            {
                const double epsilon =
                    1e-12;


                const bool sameLongitude =
                    qAbs(
                        firstPoint[0].toDouble() -
                        lastPoint[0].toDouble()
                        ) < epsilon;


                const bool sameLatitude =
                    qAbs(
                        firstPoint[1].toDouble() -
                        lastPoint[1].toDouble()
                        ) < epsilon;


                if (sameLongitude &&
                    sameLatitude)
                {
                    vertexCount--;
                }
            }
        }


        if (vertexCount < 3)
            continue;


        double averageLongitude = 0.0;
        double averageLatitude  = 0.0;

        int validCount = 0;


        // -------------------------------------------------------------
        // Calculate center
        // -------------------------------------------------------------

        for (int i = 0;
             i < vertexCount;
             ++i)
        {
            if (!ring[i].isArray())
                continue;


            const QJsonArray point =
                ring[i].toArray();


            if (point.size() < 2)
                continue;


            averageLongitude +=
                point[0].toDouble();


            averageLatitude +=
                point[1].toDouble();


            validCount++;
        }


        if (validCount == 0)
            continue;


        averageLongitude /=
            validCount;


        averageLatitude /=
            validCount;


        const QVector3D position(

            averageLongitude,

            averageLatitude,

            0
            );


        // -------------------------------------------------------------
        // Create offsets
        // -------------------------------------------------------------

        std::vector<QVector3D> offsets;


        for (int i = 0;
             i < vertexCount;
             ++i)
        {
            if (!ring[i].isArray())
                continue;


            const QJsonArray point =
                ring[i].toArray();


            if (point.size() < 2)
                continue;


            offsets.push_back(

                QVector3D(

                        point[0].toDouble()
                        - averageLongitude,

                    point[1].toDouble()
                        - averageLatitude,

                    0
                    )
                );
        }


        if (offsets.size() < 3)
            continue;


        QString importType =
            "Polygon";


        if (shapeTypeHint.compare(
                "Rectangle",
                Qt::CaseInsensitive) == 0)
        {
            importType =
                "Rectangle";
        }


        host->registerImportedShape(
            layerName,
            importType,
            featureName,
            position,
            offsets,
            importedSize,
            true
            );


        continue;
    }


    // =================================================================
    // Unsupported geometry
    // =================================================================

    else
    {
        qWarning()
        << "Unsupported GeoJSON geometry:"
        << geometryType;
    }
}


qDebug()
    << "GeoJSON import finished";


return true;
}


// ============================================================================
// Export Layer
// ============================================================================

bool GeoJSON::exportLayer(
    ILayerHost* host,
    const QString& layerName,
    const QStringList& shapeIds,
    const QString& filePath)
{
    Q_UNUSED(layerName);


    if (!host) {

        qWarning()
        << "GeoJSON::exportLayer: host is null";

        return false;
    }


    QJsonObject rootObject;


    rootObject["type"] =
        "FeatureCollection";


    // =====================================================================
    // CRS
    // =====================================================================

    QJsonObject crs;


    QJsonObject crsProperties;


    crsProperties["name"] =
        "EPSG:4326";


    crs["type"] =
        "name";


    crs["properties"] =
        crsProperties;


    rootObject["crs"] =
        crs;


    // =====================================================================
    // Features
    // =====================================================================

    QJsonArray features;


    const auto entries =
        host->getMeshEntriesForShapes(
            shapeIds
            );


    for (const MeshEntry& entry : entries)
    {
        const QString shapeType =
            getShapeType(entry);


        if (shapeType == "Unknown")
        {
            qDebug()
            << "Skipping unknown shape:"
            << entry.name;

            continue;
        }


        const QJsonObject feature =
            createGeoJSONFeature(
                entry,
                entry.name
                );


        if (!feature.isEmpty())
        {
            features.append(
                feature
                );
        }
    }


    rootObject["features"] =
        features;


    // =====================================================================
    // Write File
    // =====================================================================

    QFile file(filePath);


    if (!file.open(
            QIODevice::WriteOnly |
            QIODevice::Truncate))
    {
        qWarning()
        << "Could not write GeoJSON:"
        << filePath;

        return false;
    }


    const QJsonDocument document(
        rootObject
        );


    file.write(
        document.toJson(
            QJsonDocument::Indented
            )
        );


    file.close();


    qDebug()
        << "GeoJSON exported:"
        << filePath
        << "Features:"
        << features.size();


    return true;
}


// ============================================================================
// Create GeoJSON Feature
// ============================================================================

QJsonObject GeoJSON::createGeoJSONFeature(
    const MeshEntry& entry,
    const QString& shapeId)
{
    const QString shapeType =
        getShapeType(entry);


    if (shapeType == "Unknown")
    {
        return QJsonObject();
    }


    QJsonObject feature;


    feature["type"] =
        "Feature";


    // =====================================================================
    // Properties
    // =====================================================================

    QJsonObject properties;


    properties["name"] =
        entry.name;


    properties["id"] =
        shapeId;


    properties["type"] =
        shapeType;


    // =====================================================================
    // Circle
    // =====================================================================

    if (shapeType == "Circle")
    {
        properties["radius"] =
            entry.size
                ? entry.size->x()
                : 0.001;
    }


    // =====================================================================
    // Text
    // =====================================================================

    if (shapeType == "Text")
    {
        properties["text"] =
            entry.text;
    }


    // =====================================================================
    // Built-in Bitmap
    //
    // Do NOT save Qt resource path.
    //
    // Save logical bitmapType instead.
    // =====================================================================

    if (shapeType == "Bitmap")
    {
        QString bitmapType;


        if (entry.bitmapPath ==
            ":/icons/images/hospital.png")
        {
            bitmapType =
                "Hospital";
        }

        else if (entry.bitmapPath ==
                 ":/icons/images/school.png")
        {
            bitmapType =
                "School";
        }

        else if (entry.bitmapPath ==
                 ":/icons/images/forest-area.png")
        {
            bitmapType =
                "Forest Area";
        }


        if (bitmapType.isEmpty())
        {
            qWarning()
            << "Unknown built-in bitmap path:"
            << entry.bitmapPath;

            return QJsonObject();
        }


        properties["bitmapType"] =
            bitmapType;
    }


    // =====================================================================
    // Common Size
    // =====================================================================

    if (entry.size)
    {
        properties["sizeX"] =
            entry.size->x();


        properties["sizeY"] =
            entry.size->y();


        properties["sizeZ"] =
            entry.size->z();
    }


    // =====================================================================
    // Rotation
    // =====================================================================

    if (entry.rotation)
    {
        // Use actual Z Euler rotation in degrees.
        properties["rotation"] =
            entry.rotation
                ->toEulerAngles()
                .z();
    }


    // =====================================================================
    // Style
    // =====================================================================

    if (entry.mesh)
    {
        if (entry.mesh->color)
        {
            properties["fill"] =
                entry.mesh
                    ->color
                    ->name();


            properties["stroke"] =
                entry.mesh
                    ->color
                    ->name();
        }


        properties["lineWidth"] =
            entry.mesh->lineWidth;
    }


    feature["properties"] =
        properties;


    // =====================================================================
    // Geometry
    // =====================================================================

    const QJsonObject geometry =
        getGeometryAsGeoJSON(
            entry
            );


    if (geometry.isEmpty())
    {
        return QJsonObject();
    }


    feature["geometry"] =
        geometry;


    return feature;
}


// ============================================================================
// Get Geometry As GeoJSON
// ============================================================================

QJsonObject GeoJSON::getGeometryAsGeoJSON(
    const MeshEntry& entry)
{
    QJsonObject geometry;


    if (!entry.position)
    {
        return geometry;
    }


    const QString shapeType =
        getShapeType(entry);


    if (shapeType == "Unknown")
    {
        return geometry;
    }


    const QVector3D center =
        *entry.position;


    // =====================================================================
    // Rotation
    //
    // Mesh polygon rotation is handled in radians in the existing TDF
    // geometry logic.
    // =====================================================================

    float rotation =
        0.0f;


    if (entry.rotation)
    {
        rotation =
            entry.rotation->z();
    }


    const float cosFwd =
        std::cos(rotation);


    const float sinFwd =
        std::sin(rotation);


    // =====================================================================
    // Circle
    // =====================================================================

    if (shapeType == "Circle")
    {
        geometry["type"] =
            "Polygon";


        const double radius =
            entry.size
                ? entry.size->x()
                : 0.001;


        if (!std::isfinite(radius) ||
            radius <= 0.0)
        {
            return QJsonObject();
        }


        const int numberOfPoints =
            36;


        QJsonArray ring;


        for (int i = 0;
             i <= numberOfPoints;
             ++i)
        {
            const double angle =

                (i * 360.0 /
                 numberOfPoints)

                * M_PI / 180.0;


            const double longitude =

                center.x() +

                    radius *
                    std::cos(angle);


            const double latitude =

                center.y() +

                    radius *
                    std::sin(angle);


            QJsonArray point;


            point.append(
                longitude
                );


            point.append(
                latitude
                );


            ring.append(
                point
                );
        }


        QJsonArray coordinates;


        coordinates.append(
            ring
            );


        geometry["coordinates"] =
            coordinates;


        return geometry;
    }


    // =====================================================================
    // Rectangle / Polygon
    // =====================================================================

    if (shapeType == "Rectangle" ||
        shapeType == "Polygon")
    {
        if (!entry.mesh ||
            entry.mesh->polygen.empty())
        {
            return QJsonObject();
        }


        QJsonArray ring;


        for (Vector* vertex :
             entry.mesh->polygen)
        {
            if (!vertex)
                continue;


            const double worldX =

                    vertex->x *
                    cosFwd

                -

                    vertex->y *
                    sinFwd;


            const double worldY =

                    vertex->x *
                    sinFwd

                +

                    vertex->y *
                    cosFwd;


            const double longitude =

                center.x() +
                worldX;


            const double latitude =

                center.y() +
                worldY;


            QJsonArray point;


            point.append(
                longitude
                );


            point.append(
                latitude
                );


            ring.append(
                point
                );
        }


        if (ring.size() < 3)
        {
            return QJsonObject();
        }


        // -----------------------------------------------------------------
        // Close Polygon
        // -----------------------------------------------------------------

        const QJsonArray firstPoint =
            ring.first().toArray();


        const QJsonArray lastPoint =
            ring.last().toArray();


        bool alreadyClosed =
            false;


        if (firstPoint.size() >= 2 &&
            lastPoint.size() >= 2)
        {
            const double epsilon =
                1e-12;


            alreadyClosed =

                    qAbs(
                        firstPoint[0].toDouble() -
                        lastPoint[0].toDouble()
                        ) < epsilon

                &&

                    qAbs(
                        firstPoint[1].toDouble() -
                        lastPoint[1].toDouble()
                        ) < epsilon;
        }


        if (!alreadyClosed)
        {
            ring.append(
                firstPoint
                );
        }


        QJsonArray coordinates;


        coordinates.append(
            ring
            );


        geometry["type"] =
            "Polygon";


        geometry["coordinates"] =
            coordinates;


        return geometry;
    }


    // =====================================================================
    // Line / Polyline
    // =====================================================================

    if (shapeType == "Line")
    {
        if (!entry.mesh ||
            entry.mesh->polygen.empty())
        {
            return QJsonObject();
        }


        QJsonArray coordinates;


        for (Vector* vertex :
             entry.mesh->polygen)
        {
            if (!vertex)
                continue;


            const double worldX =

                    vertex->x *
                    cosFwd

                -

                    vertex->y *
                    sinFwd;


            const double worldY =

                    vertex->x *
                    sinFwd

                +

                    vertex->y *
                    cosFwd;


            const double longitude =

                center.x() +
                worldX;


            const double latitude =

                center.y() +
                worldY;


            QJsonArray point;


            point.append(
                longitude
                );


            point.append(
                latitude
                );


            coordinates.append(
                point
                );
        }


        if (coordinates.size() < 2)
        {
            return QJsonObject();
        }


        geometry["type"] =
            "LineString";


        geometry["coordinates"] =
            coordinates;


        return geometry;
    }


    // =====================================================================
    // Point / Text / Bitmap
    //
    // Text and Bitmap use Point as their GeoJSON geometry.
    // =====================================================================

    if (shapeType == "Point" ||
        shapeType == "Text" ||
        shapeType == "Bitmap")
    {
        QJsonArray coordinates;


        coordinates.append(
            center.x()
            );


        coordinates.append(
            center.y()
            );


        geometry["type"] =
            "Point";


        geometry["coordinates"] =
            coordinates;


        return geometry;
    }


    return QJsonObject();
}


// ============================================================================
// Get Shape Type
// ============================================================================

QString GeoJSON::getShapeType(const MeshEntry& entry)
{
    // =====================================================================
    // PRIMARY SOURCE: Internal Shape Type
    //
    // entry.name      = User-visible name, e.g. "Area"
    // entry.shapeType = Internal type, e.g. "Circle"
    // =====================================================================

    if (!entry.shapeType.isEmpty())
    {
        if (entry.shapeType.compare("Circle", Qt::CaseInsensitive) == 0)
            return "Circle";

        if (entry.shapeType.compare("Rectangle", Qt::CaseInsensitive) == 0)
            return "Rectangle";

        if (entry.shapeType.compare("Polygon", Qt::CaseInsensitive) == 0)
            return "Polygon";

        if (entry.shapeType.compare("Polyline", Qt::CaseInsensitive) == 0 ||
            entry.shapeType.compare("Line", Qt::CaseInsensitive) == 0)
            return "Line";

        if (entry.shapeType.compare("Point", Qt::CaseInsensitive) == 0)
            return "Point";

        if (entry.shapeType.compare("Text", Qt::CaseInsensitive) == 0)
            return "Text";

        if (entry.shapeType.compare("Bitmap", Qt::CaseInsensitive) == 0)
            return "Bitmap";
    }


    // =====================================================================
    // BACKWARD COMPATIBILITY
    //
    // Old MeshEntry objects may not have shapeType set.
    // For those objects, detect type using old logic.
    // =====================================================================

    const QString shapeName = entry.name;


    // =====================================================================
    // Built-in Bitmap
    // =====================================================================

    if (entry.bitmapPath ==
            ":/icons/images/hospital.png" ||
        entry.bitmapPath ==
            ":/icons/images/school.png" ||
        entry.bitmapPath ==
            ":/icons/images/forest-area.png")
    {
        return "Bitmap";
    }


    // =====================================================================
    // Text
    // =====================================================================

    if (shapeName.startsWith("TempText", Qt::CaseInsensitive) ||
        shapeName.startsWith("Text", Qt::CaseInsensitive))
    {
        return "Text";
    }


    // =====================================================================
    // Circle
    // =====================================================================

    if (shapeName.startsWith("TempCircle", Qt::CaseInsensitive) ||
        shapeName.startsWith("Circle", Qt::CaseInsensitive))
    {
        return "Circle";
    }


    // =====================================================================
    // Rectangle
    // =====================================================================

    if (shapeName.startsWith("TempRectangle", Qt::CaseInsensitive) ||
        shapeName.startsWith("Rectangle", Qt::CaseInsensitive))
    {
        return "Rectangle";
    }


    // =====================================================================
    // Polygon
    // =====================================================================

    if (shapeName.startsWith("TempPolygon", Qt::CaseInsensitive) ||
        shapeName.startsWith("Polygon", Qt::CaseInsensitive))
    {
        return "Polygon";
    }


    // =====================================================================
    // Line / Polyline
    // =====================================================================

    if (shapeName.startsWith("TempPolyline", Qt::CaseInsensitive) ||
        shapeName.startsWith("Polyline", Qt::CaseInsensitive) ||
        shapeName.startsWith("Line", Qt::CaseInsensitive))
    {
        return "Line";
    }


    // =====================================================================
    // Point
    // =====================================================================

    if (shapeName.startsWith("TempPoint", Qt::CaseInsensitive) ||
        shapeName.startsWith("Point", Qt::CaseInsensitive))
    {
        return "Point";
    }


    return "Unknown";
}