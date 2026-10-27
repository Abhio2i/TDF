/******************************************************************************************
 * Description:
 * This file implements ScriptEngineGIS, which provides script-driven GIS functionality.
 * It handles drawing of points, lines, polygons, circles, rectangles, bitmaps, distance
 * measurement, offline geo-verification (India region), auto-zoom, screenshots and logging.
 *
 * UPDATED: migrated from the old Widget/GISlib/ShapesFeature stack to the new
 * GIS backend: ShapeApi, LayerApi, MapApi (singletons wrapping ShapeManager, LayerManager,
 * GISEngine). MapWidget is now the only widget-level dependency left, and it is only used
 * for pixel-level operations (screenshots, forced repaint, interactive draw mode).
 *
 * UPDATED (param order): all AngelScript-facing geo APIs that took (lon, lat) now take
 * (lat, lon) instead, e.g. moveShape(shapeName, lat, lon). Internal calls into ShapeApi/
 * MapWidget/QPointF still use whatever order those backends expect (QPointF is (x=lon,
 * y=lat)); only the public parameter order as seen by AngelScript scripts has changed.
 *
 * Author - Amjad
 ******************************************************************************************/

#include "scriptenginegis.h"
#include "scriptengine.h"
#include "qtimer.h"
#include <angelscript.h>
#include <angelscript/add_on/scriptstdstring/scriptstdstring.h>
#include <QMetaObject>
#include <QDebug>
#include <string>
#include <QObject>
#include <QVector3D>
#include <QPointF>
#include <cstdlib>
#include <cmath>
#include <angelscript/add_on/scriptarray/scriptarray.h>
#include "core/GlobalRegistry.h"
#include "core/Simulation/simulation.h"
#include "core/structure/runtime.h"
#include "core/Debug/console.h"
#include <fstream>
#include <QDir>
#include <QPixmap>
#include <QDateTime>
#include <QCoreApplication>
#include <QThread>
#include <QRectF>
#include <QtMath>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>

// ===== New GIS backend =====
#include <GIS_API/frontend/widgets/map_widget.h>
#include <GIS_API/backend/api/shape_api.h>
#include <GIS_API/backend/api/layer_api.h>
#include <GIS_API/backend/api/map_api.h>
#include <GIS_API/backend/formats/layer_format_registry.h>
#include <GIS_API/backend/formats/builtin_layer_formats.h>
#include "backend/api/measure_api.h"

// NOTE: AirbaseApi / ShapeHistoryApi headers were not supplied for this migration.
// Their usage below matches the exact call pattern already used elsewhere in the
// codebase (ui_action_api.cpp, editor.cpp): AirbaseApi::instance()->initialize()/
// toggleVisibility()/isVisible(), ShapeHistoryApi::instance()->startPreview()/
// restoreHistory()/hidePreview(). If the include paths differ, adjust these two
// includes accordingly.
#include <GIS_API/backend/api/airbase_api.h>
#include <GIS_API/backend/api/shape_history_api.h>

// Helper: Convert latitude & longitude to formatted string
static QString geoToString(double lat, double lon)
{
    return QString("Lat:%1 Lon:%2")
    .arg(lat, 0, 'f', 6)
        .arg(lon, 0, 'f', 6);
}

// Helper: Expand bounding box using lat/lon
static void updateBounds(QRectF& r, double lat, double lon)
{
    if (r.isNull())
        r = QRectF(QPointF(lon, lat), QSizeF(0, 0));
    else
        r = r.united(QRectF(QPointF(lon, lat), QSizeF(0, 0)));
}

// Approximate meters -> degrees conversion (equatorial approximation, good
// enough for the auto-zoom bounds / demo shapes used by AutoScript).
static const double METERS_PER_DEGREE = 111000.0;

// Haversine distance in meters between two lat/lon points.
static double haversineMeters(double lat1, double lon1, double lat2, double lon2)
{
    const double R = 6371000.0; // Earth radius in meters
    const double dLat = qDegreesToRadians(lat2 - lat1);
    const double dLon = qDegreesToRadians(lon2 - lon1);
    const double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
                     std::cos(qDegreesToRadians(lat1)) * std::cos(qDegreesToRadians(lat2)) *
                         std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return R * c;
}

// Built-in bitmap name -> icon path (mirrors MapWidget's bitmapIconForName)
static QString builtinBitmapIconPath(const QString& name)
{
    if (name.compare(QStringLiteral("Hospital"), Qt::CaseInsensitive) == 0)
        return QStringLiteral(":/icons/images/hospital.png");
    if (name.compare(QStringLiteral("School"), Qt::CaseInsensitive) == 0)
        return QStringLiteral(":/icons/images/school.png");
    if (name.compare(QStringLiteral("Forest Area"), Qt::CaseInsensitive) == 0)
        return QStringLiteral(":/icons/images/forest-area.png");
    return QString();
}

// Inject the MapWidget dependency (replaces set()/setGIS())
void ScriptEngineGIS::setMapWidget(MapWidget* map)
{
    gisMap = map;
}

QMap<QString, CityInfo> ScriptEngineGIS::s_cityDatabase;
bool ScriptEngineGIS::s_cityDatabaseLoaded = false;


void ScriptEngineGIS::loadCityDatabase(const QString& jsonPath)
{
    if (s_cityDatabaseLoaded)
        return;

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[GIS] Could not load city database from:" << jsonPath;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[GIS] Failed to parse city JSON:" << parseError.errorString();
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "[GIS] City JSON root is not an object";
        return;
    }

    QJsonObject rootObj = doc.object();
    if (!rootObj.contains("indian_cities") || !rootObj["indian_cities"].isArray()) {
        qWarning() << "[GIS] 'indian_cities' key missing or not an array";
        return;
    }

    QJsonArray citiesArray = rootObj["indian_cities"].toArray();
    int loadedCount = 0;

    for (const QJsonValue& val : citiesArray) {
        if (!val.isObject())
            continue;

        QJsonObject cityObj = val.toObject();

        CityInfo info;
        info.name = cityObj["name"].toString();
        info.lat  = cityObj["lat"].toDouble();
        info.lon  = cityObj["lon"].toDouble();

        if (info.name.isEmpty())
            continue;

        s_cityDatabase.insert(info.name.toLower(), info);
        loadedCount++;
    }

    s_cityDatabaseLoaded = true;

    qDebug() << "[GIS] City database loaded:" << loadedCount << "entries from JSON";
}

// Return active city location string
QString ScriptEngineGIS::cityLocationString() const
{
    if (!cityActive) return "City : NOT SET";

    return QString("City : %1 | Lat : %2 | Lon : %3")
        .arg(activeCity)
        .arg(cityLat, 0, 'f', 6)
        .arg(cityLon, 0, 'f', 6);
}

// ================= CITY CONTEXT =================
void ScriptEngineGIS::useCity(const std::string& cityName)
{
    if (!s_cityDatabaseLoaded) {
        loadCityDatabase();
    }

    QString name = QString::fromStdString(cityName).trimmed().toLower();

    if (s_cityDatabase.contains(name)) {
        CityInfo c = s_cityDatabase.value(name);

        activeCity = c.name;
        cityLat = c.lat;
        cityLon = c.lon;
        cityActive = true;

        if (logFn) {
            logFn(
                ReportCategory::SYSTEM,
                "City Selected",
                activeCity,
                geoToString(cityLat, cityLon),
                "",
                "City context set",
                "SUCCESS",
                "",
                ""
                );
        }
        return;
    }

    cityActive = false;

    if (logFn) {
        logFn(
            ReportCategory::SYSTEM,
            "City Selected",
            QString::fromStdString(cityName),
            "",
            "",
            "Unknown city",
            "FAILED",
            "",
            ""
            );
    }
}

// ================= GEO VERIFICATION (OFFLINE - INDIA REGION) =================
// Simple India bounding box (defence-safe offline check)
bool ScriptEngineGIS::isLatLonInIndia(double lat, double lon) const
{
    // India approximate bounding box
    const double MIN_LAT = 6.5;
    const double MAX_LAT = 37.5;
    const double MIN_LON = 68.0;
    const double MAX_LON = 97.5;

    return (lat >= MIN_LAT && lat <= MAX_LAT &&
            lon >= MIN_LON && lon <= MAX_LON);
}

// Returns human-readable verification line
QString ScriptEngineGIS::geoVerificationLine(double lat, double lon) const
{
    if (isLatLonInIndia(lat, lon)) {
        return "Geo Verification : VERIFIED (India Region)";
    }
    return "Geo Verification : FAILED (Outside India Region)";
}

// ================= NAME -> ID RESOLUTION =================
QString ScriptEngineGIS::findShapeIdByName(const QString& name) const
{
    if (name.isEmpty())
        return QString();

    const QJsonArray shapes = ShapeApi::instance()->getAllShapes();
    for (const QJsonValue& v : shapes) {
        const QJsonObject obj = v.toObject();
        if (obj.value(QStringLiteral("text")).toString() == name)
            return obj.value(QStringLiteral("id")).toString();
    }
    return QString();
}

QString ScriptEngineGIS::findLayerIdByName(const QString& name) const
{
    if (name.isEmpty())
        return QString();

    LayerApi* la = LayerApi::instance();
    const QStringList ids = la->getLayers();
    for (const QString& id : ids) {
        if (la->getLayerName(id) == name)
            return id;
    }
    return QString();
}

double ScriptEngineGIS::convertMeasurement(double meters) const
{
    if (measurementUnit == QLatin1String("meters"))     return meters;
    if (measurementUnit == QLatin1String("kilometers")) return meters / 1000.0;
    if (measurementUnit == QLatin1String("miles"))      return meters / 1609.344;
    if (measurementUnit == QLatin1String("feet"))       return meters * 3.280839895;
    return meters / 1000.0; // default: kilometers
}

// Set selected drawing shape on  (interactive draw mode)
void ScriptEngineGIS::setSelectedShape(const std::string &shapeName)
{
    if (!gisMap) {
        qWarning() << "[ScriptEngineGIS] MapWidget not available. Cannot set selected shape.";
        return;
    }

    QString qShape = QString::fromStdString(shapeName);
    gisMap->startDrawing(qShape);
    qDebug() << "[ScriptEngineGIS] Shape drawing mode enabled for:" << qShape;
}

void ScriptEngineGIS::AddLayer(const std::string &layerName)
{
    QString qname = QString::fromStdString(layerName);
    qDebug() << "SCRIPT creating layer:" << qname;

    QString layerId = LayerApi::instance()->addLayer(qname);
    if (layerId.isEmpty()) {
        if (logFn) {
            logFn(ReportCategory::GIS, "Layer Creation", qname, "", "", "Not Created",
                  "FAILED", "Could not create layer", "");
        }
        return;
    }

    LayerApi::instance()->setActiveLayer(layerId);

    if (logFn) {
        logFn(ReportCategory::GIS, "Layer Created", qname, "", "",
              "Active Layer Set", "SUCCESS", "", "");
    }
}

void ScriptEngineGIS::SelectLayer(const std::string &layerName)
{
    QString qname = QString::fromStdString(layerName);
    QString layerId = findLayerIdByName(qname);
    if (layerId.isEmpty()) {
        if (logFn) {
            logFn(ReportCategory::GIS, "Layer Select", qname, "", "", "Not Found",
                  "FAILED", "Layer does not exist", "");
        }
        return;
    }

    LayerApi::instance()->setActiveLayer(layerId);

    if (logFn) {
        logFn(ReportCategory::GIS, "Layer Selected", qname, "", "",
              "Active Layer Set", "SUCCESS", "", "");
    }
}

void ScriptEngineGIS::RenameShape(const std::string &id,const std::string &newName)
{
    QString qid = QString::fromStdString(id);
    QString qname = QString::fromStdString(newName);

    QJsonObject patch;
    patch[QStringLiteral("text")] = qname;
    ShapeApi::instance()->updateShape(qid, patch);
}

//===================LINE TOOL====================
// Start line drawing
void ScriptEngineGIS::StartLine()
{
    linePoints.clear();
    lastShapeBounds = QRectF();

    if (gisMap)
        gisMap->startDrawing(QStringLiteral("line"));
}

// Add line point (parameter order: latitude, then longitude)
void ScriptEngineGIS::AddLinePoint(float lat, float lon)
{
    QPointF geo(lon, lat);
    lastGeoCursor = geo;

    // collect point for final report
    linePoints.push_back(geo);

    updateBounds(lastShapeBounds, lat, lon);
}

// Finish line drawing
// ============================================================
// Finish line drawing
// ============================================================
void ScriptEngineGIS::FinishLine()
{
    if (linePoints.size() < 2) {
        if (logFn) {
            logFn(
                ReportCategory::GIS,
                "Line Created",
                "Line Tool",
                "",
                "",
                "Not Rendered",
                "FAILED",
                "At least 2 points required",
                ""
                );
        }

        // IMPORTANT:
        // Even when script has insufficient points,
        // exit interactive drawing mode.
        if (gisMap)
            gisMap->cancelDrawing();

        linePoints.clear();
        lastShapeBounds = QRectF();

        return;
    }

    bool geoOk = true;
    QVector<QPointF> pts;
    pts.reserve(static_cast<int>(linePoints.size()));

    for (const auto& p : linePoints) {
        // p = (lon, lat)
        pts.append(p);

        updateBounds(
            lastShapeBounds,
            p.y(),   // lat
            p.x()    // lon
            );

        if (!isLatLonInIndia(p.y(), p.x()))
            geoOk = false;
    }

    // Create actual line in ShapeApi
    QString lineId = ShapeApi::instance()->addLine(pts);

    // IMPORTANT:
    // Script-created line is finished, so disable
    // MapWidget's interactive mouse drawing mode.
    if (gisMap)
        gisMap->cancelDrawing();

    if (gisMap && !lastShapeBounds.isNull()) {
        gisMap->setCenter(
            lastShapeBounds.center().y(), // lat
            lastShapeBounds.center().x()  // lon
            );

        gisMap->update();
    }

    // Capture screenshot
    QString screenshot =
        screenshotFn
            ? screenshotFn("line")
            : QString();

    if (logFn) {
        logFn(
            ReportCategory::GIS,
            "Line Created",
            "Line Tool",
            "Auto From Points",
            QString("Total Points : %1").arg(linePoints.size()),
            lineId.isEmpty()
                ? "Line Creation Failed"
                : QString("Line Created"),
            geoOk
                ? "Geo Verification : VERIFIED (India Region)"
                : "Geo Verification : FAILED (Outside India Region)",
            lineId.isEmpty()
                ? "FAILED"
                : (geoOk ? "SUCCESS" : "FAILED"),
            screenshot
            );
    }

    // Clear script-side points also
    linePoints.clear();
    lastShapeBounds = QRectF();
}

// Add a polygon
// NOTE: each point string in the array is parsed as "lat,lon" (latitude first,
// then longitude) to match the rest of the geo APIs.
void ScriptEngineGIS::AddPolygon(const std::string &name, CScriptArray* points)
{
    QString qname = QString::fromStdString(name);

    if (!points || points->GetSize() < 3) {
        if(logFn) {
            logFn(
                ReportCategory::GIS,
                "Polygon Creation",
                qname,
                "",
                "Vertices < 3",
                "Not Rendered",
                "FAILED",
                "Polygon requires minimum 3 points",
                ""
                );
        }
        return;
    }

    bool geoOk = true;
    lastShapeBounds = QRectF();
    QVector<QPointF> pts;

    for (asUINT i = 0; i < points->GetSize(); i++) {
        QString pt = QString::fromStdString(*(std::string*)points->At(i));
        auto xy = pt.split(",");

        if (xy.size() != 2)
            continue;

        double lat = xy[0].toDouble();
        double lon = xy[1].toDouble();

        QPointF geo(lon, lat);
        lastGeoCursor = geo;
        pts.append(geo);

        updateBounds(lastShapeBounds, lat, lon);

        if (!isLatLonInIndia(lat, lon))
            geoOk = false;
    }

    if (pts.size() < 3) {
        if (logFn) {
            logFn(ReportCategory::GIS, "Polygon Creation", qname, "", "Vertices < 3",
                  "Not Rendered", "FAILED", "Not enough valid points", "");
        }
        return;
    }

    QString id = ShapeApi::instance()->addPolygon(pts);
    if (!id.isEmpty()) {
        QJsonObject patch;
        patch[QStringLiteral("text")] = qname;
        ShapeApi::instance()->updateShape(id, patch);
    }

    if (gisMap && !lastShapeBounds.isNull()) {
        gisMap->setCenter(lastShapeBounds.center().y(), lastShapeBounds.center().x());
        gisMap->update();
    }

    // SCREENSHOT
    QString screenshot = screenshotFn ? screenshotFn("polygon") : QString();

    if(logFn) {
        logFn(
            ReportCategory::GIS,
            "Polygon Created",
            qname,
            "Auto From Vertices",
            QString("Vertices = %1").arg(points->GetSize()),
            geoOk
                ? "Geo Verification : VERIFIED (India Region)"
                : "Geo Verification : FAILED (Outside India Region)",
            geoOk ? "SUCCESS" : "FAILED",
            geoOk ? "" : "Polygon has vertices outside India",
            screenshot
            );
    }
}

// Add a rectangle
void ScriptEngineGIS::AddRectangle(
    const std::string &name,
    float width,
    float height
    )
{
    QString qname = QString::fromStdString(name);

    if (!cityActive){
        if(logFn) {
            logFn(
                ReportCategory::GIS,
                "Rectangle Creation",
                qname,
                "",
                "",
                "Not Rendered",
                "FAILED",
                "City not selected",
                ""
                );
        }
        return;
    }

    lastGeoCursor = QPointF(cityLon, cityLat);
    bool geoOk = isLatLonInIndia(cityLat, cityLon);

    // Convert meters -> degrees (approx, screenshot-safe). width/height are
    // the FULL width/height of the rectangle (see RectangleShapeType::finishDrawing).
    double widthDeg  = width  / METERS_PER_DEGREE;
    double heightDeg = height / METERS_PER_DEGREE;

    QString id = ShapeApi::instance()->addRectangle(
        cityLon,
        cityLat,
        widthDeg,
        heightDeg
        );

    if (!id.isEmpty())
    {
        QJsonObject patch;
        patch[QStringLiteral("text")] = qname;

        ShapeApi::instance()->updateShape(id, patch);

        // IMPORTANT:
        // Store actual ShapeApi ID
        shapeNameToId[qname] = id;

        qDebug()
            << "[GIS] Rectangle created"
            << "Name =" << qname
            << "ID =" << id;
    }

    // Reset previous bounds
    lastShapeBounds = QRectF();
    updateBounds(lastShapeBounds, cityLat + heightDeg / 2.0, cityLon + widthDeg / 2.0);
    updateBounds(lastShapeBounds, cityLat - heightDeg / 2.0, cityLon - widthDeg / 2.0);

    if (gisMap) {
        gisMap->setCenter(cityLat, cityLon);
        gisMap->update();
    }

    // Capture screenshot AFTER drawing & bounds update
    QString screenshot = screenshotFn ? screenshotFn("rectangle") : QString();

    // Log event
    if(logFn) {
        logFn(
            ReportCategory::GIS,
            "Rectangle Created",
            qname,
            cityLocationString(),
            QString("Width : %1 m, Height : %2 m").arg(width).arg(height),
            geoVerificationLine(cityLat, cityLon),
            geoOk ? "SUCCESS" : "FAILED",
            "",
            screenshot
            );
    }
}


void ScriptEngineGIS::AddCircle(
    const std::string &name,
    float radius)
{
    QString qname = QString::fromStdString(name);

    if (!cityActive) {
        if (logFn) {
            logFn(
                ReportCategory::GIS,
                "Circle Creation",
                qname,
                "",
                "",
                "Not Rendered",
                "FAILED",
                "City not selected",
                ""
                );
        }
        return;
    }

    // Active city = circle center
    lastGeoCursor = QPointF(cityLon, cityLat);

    bool geoOk =
        isLatLonInIndia(cityLat, cityLon);

    // Convert meters -> degrees
    double radiusDeg =
        radius / METERS_PER_DEGREE;

    // Create circle
    QString id =
        ShapeApi::instance()->addCircle(
            cityLon,
            cityLat,
            radiusDeg
            );

    if (id.isEmpty()) {
        qWarning()
        << "[GIS] Failed to create circle:"
        << qname;

        return;
    }

    qDebug()
        << "[GIS] Circle created"
        << "Name =" << qname
        << "ID =" << id;

    // Set display/name
    QJsonObject patch;

    patch[QStringLiteral("text")] = qname;

    ShapeApi::instance()->updateShape(
        id,
        patch
        );

    // =====================================================
    // IMPORTANT:
    // Store mapping Name -> actual ShapeApi ID
    // =====================================================

    shapeNameToId[qname] = id;

    // Bounds
    lastShapeBounds = QRectF();

    updateBounds(
        lastShapeBounds,
        cityLat + radiusDeg,
        cityLon + radiusDeg
        );

    updateBounds(
        lastShapeBounds,
        cityLat - radiusDeg,
        cityLon - radiusDeg
        );

    if (gisMap) {
        gisMap->setCenter(
            cityLat,
            cityLon
            );

        gisMap->update();
    }

    // Screenshot
    QString screenshot =
        screenshotFn
            ? screenshotFn("circle")
            : QString();

    if (logFn) {

        logFn(
            ReportCategory::GIS,
            "Circle Created",
            qname,
            cityLocationString(),
            QString("Radius : %1 m").arg(radius),
            geoVerificationLine(
                cityLat,
                cityLon
                ),
            geoOk ? "SUCCESS" : "FAILED",
            "",
            screenshot
            );
    }
}

// Add a point (parameter order: latitude, then longitude)
void ScriptEngineGIS::AddPoint(const std::string &name, float lat, float lon)
{
    QString qname = QString::fromStdString(name);
    QPointF geo(lon, lat);
    lastGeoCursor = geo;

    bool geoOk = isLatLonInIndia(lat, lon);

    QString id = ShapeApi::instance()->addPoint(lon, lat);
    if (!id.isEmpty()) {
        QJsonObject patch;
        patch[QStringLiteral("text")] = qname;
        ShapeApi::instance()->updateShape(id, patch);
    }

    // RESET & EXPAND BOUNDS
    lastShapeBounds = QRectF();
    expandPointBounds(lastShapeBounds, lat, lon);

    if (gisMap) {
        gisMap->setCenter(lat, lon);
        gisMap->update();
    }

    QString screenshot = screenshotFn ? screenshotFn("point") : QString();

    // LOG
    if(logFn) {
        logFn(
            ReportCategory::GIS,
            "Point Created",
            qname,
            geoToString(lat, lon),
            "",
            geoVerificationLine(lat, lon),
            geoOk ? "SUCCESS" : "FAILED",
            geoOk ? "" : "Outside India",
            screenshot
            );
    }
}

// BOUNDS HELPERS
void ScriptEngineGIS::expandPointBounds(QRectF &bounds, double lat, double lon)
{
    // approx 15-20 km box (state / district level)
    double pad = 0.15; // degrees (~16 km)

    bounds = QRectF(
        QPointF(lon - pad, lat - pad),
        QPointF(lon + pad, lat + pad)
        );
}

// BITMAP HANDLING
void ScriptEngineGIS::expandBitmapBoundsFrom(QRectF &bounds)
{
    // Find the most recently added bitmap-type shape and pad a box around it.
    const QJsonArray shapes = ShapeApi::instance()->getAllShapes();
    for (int i = shapes.size() - 1; i >= 0; --i) {
        const QJsonObject obj = shapes.at(i).toObject();
        if (obj.value(QStringLiteral("type")).toString() != QLatin1String("bitmap"))
            continue;

        const double lon = obj.value(QStringLiteral("longitude")).toDouble();
        const double lat = obj.value(QStringLiteral("latitude")).toDouble();

        // small visual padding (~10-15 km)
        double pad = 0.12;
        bounds = QRectF(
            QPointF(lon - pad, lat - pad),
            QPointF(lon + pad, lat + pad)
            );
        return;
    }
}

// select bitmap (parameter order: latitude, then longitude)
void ScriptEngineGIS::onBitmapSelected(const std::string &bitmapType, float lat, float lon)
{
    QString qtype = QString::fromStdString(bitmapType);
    QString iconPath = builtinBitmapIconPath(qtype);
    if (iconPath.isEmpty()) {
        if (logFn) {
            logFn(ReportCategory::UI, "Bitmap Placed", qtype, geoToString(lat, lon), "",
                  "Unknown bitmap type", "FAILED", "", "");
        }
        return;
    }

    QPointF geo(lon, lat);
    lastGeoCursor = geo;

    QJsonObject shapeJson;
    shapeJson[QStringLiteral("type")]      = QStringLiteral("bitmap");
    shapeJson[QStringLiteral("longitude")] = static_cast<double>(lon);
    shapeJson[QStringLiteral("latitude")]  = static_cast<double>(lat);
    shapeJson[QStringLiteral("width")]     = 0.5;
    shapeJson[QStringLiteral("height")]    = 0.5;
    shapeJson[QStringLiteral("imagePath")] = iconPath;
    shapeJson[QStringLiteral("rotation")]  = 0.0;
    shapeJson[QStringLiteral("text")]      = qtype;
    ShapeApi::instance()->addShape(shapeJson);

    // Bounds (treat bitmap as point)
    lastShapeBounds = QRectF();
    expandPointBounds(lastShapeBounds, lat, lon);

    if (gisMap) {
        gisMap->setCenter(lat, lon);
        gisMap->update();
    }

    // Screenshot
    QString screenshot = screenshotFn ? screenshotFn("bitmap") : QString();

    if(logFn) {
        logFn(
            ReportCategory::UI,
            "Bitmap Placed",
            qtype,
            geoToString(lat, lon),
            "Script Placed Bitmap",
            "Bitmap Visible",
            "SUCCESS",
            "",
            screenshot
            );
    }
}

// Get path of built-in bitmap
std::string ScriptEngineGIS::getBitmapImagePath(const std::string &bitmapType)
{
    QString path = builtinBitmapIconPath(QString::fromStdString(bitmapType));
    return path.toStdString();
}

// select bitmap from device (parameter order: latitude, then longitude)
void ScriptEngineGIS::onBitmapImageSelected(
    const std::string &filePath,
    float lat,
    float lon
    )
{
    QString qpath = QString::fromStdString(filePath);
    QPointF geo(lon, lat);
    lastGeoCursor = geo;

    QJsonObject shapeJson;
    shapeJson[QStringLiteral("type")]      = QStringLiteral("bitmap");
    shapeJson[QStringLiteral("longitude")] = static_cast<double>(lon);
    shapeJson[QStringLiteral("latitude")]  = static_cast<double>(lat);
    shapeJson[QStringLiteral("width")]     = 28;
    shapeJson[QStringLiteral("height")]    = 28;
    shapeJson[QStringLiteral("imagePath")] = qpath;
    shapeJson[QStringLiteral("rotation")]  = 0.0;
    ShapeApi::instance()->addShape(shapeJson);

    // TAKE BOUNDS FROM ACTUAL BITMAP POSITION
    lastShapeBounds = QRectF();
    expandBitmapBoundsFrom(lastShapeBounds);

    if (gisMap) {
        gisMap->setCenter(lat, lon);
        gisMap->update();
    }

    // Screenshot (zoom happens inside screenshotFn)
    QString screenshot = screenshotFn ? screenshotFn("user_bitmap") : QString();

    if(logFn) {
        logFn(
            ReportCategory::UI,
            "Bitmap Placed",
            QFileInfo(qpath).fileName(),
            geoToString(lat, lon),
            "User Image (Script)",
            "Bitmap Visible",
            "SUCCESS",
            "",
            screenshot
            );
    }
}

// GEOJSON
void ScriptEngineGIS::ImportGeoJsonLayer(const std::string &filePath)
{
    QString qpath = QString::fromStdString(filePath);
    registerBuiltInLayerFormats();

    bool ok = LayerApi::instance()->importLayer(qpath, QStringLiteral("geojson"));

    if(logFn) {
        logFn(
            ReportCategory::GIS,
            "GeoJSON Imported",
            QFileInfo(qpath).fileName(),
            "",
            "File Import",
            ok ? "Layer Loaded" : "Import Failed",
            ok ? "SUCCESS" : "FAILED",
            "",
            ""
            );
    }
}

void ScriptEngineGIS::ToggleGeoJsonLayer(const std::string &layerName, bool visible)
{
    QString qname = QString::fromStdString(layerName);
    QString layerId = findLayerIdByName(qname);
    if (layerId.isEmpty()) {
        if (logFn) {
            logFn(ReportCategory::GIS, "GeoJSON Layer Toggled", qname, "", "", "Not Found",
                  "FAILED", "Layer does not exist", "");
        }
        return;
    }

    LayerApi::instance()->setVisibility(layerId, visible);

    if(logFn) {
        logFn(
            ReportCategory::GIS,
            "GeoJSON Layer Toggled",
            qname,
            "",
            "Not Rendered",
            visible ? "ON" : "OFF",
            visible ? "Visible" : "Hidden",
            "SUCCESS",
            ""
            );
    }
}

// DISTANCE MEASUREMENT
// void ScriptEngineGIS::StartDistanceMeasurement()
// {
//     measurementPoints.clear();
//     lastShapeBounds = QRectF();

//     MeasureApi* api = MeasureApi::instance();

//     if (!api)
//         return;

//     // Start measurement at API level
//     api->clearPoints();
//     api->startMeasuring();

//     // Enable GIS interaction and start map measurement
//     if (gisMap) {
//         gisMap->setGisInteractionEnabled(true);
//         gisMap->startMeasuring();
//         gisMap->update();
//     }
// }

void ScriptEngineGIS::StartDistanceMeasurement()
{
    measurementPoints.clear();
    lastShapeBounds = QRectF();

    MeasureApi* api = MeasureApi::instance();

    qDebug() << "[MEASURE] StartDistanceMeasurement";
    qDebug() << "[MEASURE] gisMap =" << gisMap;
    qDebug() << "[MEASURE] api =" << api;

    if (!api)
        return;

    api->clearPoints();
    api->startMeasuring();

    qDebug() << "[MEASURE] API measuring =" << api->isMeasuring();

    if (gisMap) {
        gisMap->setGisInteractionEnabled(true);
        gisMap->startMeasuring();
        gisMap->update();

        qDebug() << "[MEASURE] Map measurement started";
    } else {
        qDebug() << "[MEASURE] ERROR: gisMap is NULL";
    }
}

// void ScriptEngineGIS::AddMeasurePoint(double lat, double lon)
// {
//     if (!gisMap)
//         return;

//     lastGeoCursor = QPointF(lon, lat);

//     // Script API = lat, lon
//     // MeasureApi  = lon, lat
//     MeasureApi* api = MeasureApi::instance();

//     if (!api)
//         return;

//     if (!api->isMeasuring()) {
//         gisMap->setGisInteractionEnabled(true);
//         gisMap->startMeasuring();
//     }

//     api->addPoint(lon, lat);

//     // Keep ScriptEngine-side points for report/logging
//     measurementPoints.push_back(QPointF(lon, lat));

//     updateBounds(lastShapeBounds, lat, lon);

//     gisMap->update();
// }

void ScriptEngineGIS::AddMeasurePoint(double lat, double lon)
{
    qDebug() << "[MEASURE] AddPoint lat =" << lat
             << "lon =" << lon
             << "gisMap =" << gisMap;

    if (!gisMap) {
        qDebug() << "[MEASURE] ERROR: gisMap is NULL";
        return;
    }

    lastGeoCursor = QPointF(lon, lat);

    MeasureApi* api = MeasureApi::instance();

    if (!api)
        return;

    if (!api->isMeasuring()) {
        gisMap->setGisInteractionEnabled(true);
        gisMap->startMeasuring();
    }

    api->addPoint(lon, lat);

    measurementPoints.push_back(QPointF(lon, lat));

    updateBounds(lastShapeBounds, lat, lon);

    gisMap->update();

    qDebug() << "[MEASURE] Point count =" << api->pointCount();
}

double ScriptEngineGIS::GetLastSegmentDistance()
{
    MeasureApi* api = MeasureApi::instance();

    if (!api)
        return 0.0;

    const QVector<double> segments = api->getSegmentMeters();

    if (segments.isEmpty())
        return 0.0;

    return convertMeasurement(segments.last());
}

double ScriptEngineGIS::GetTotalDistance()
{
    MeasureApi* api = MeasureApi::instance();

    if (!api)
        return 0.0;

    if (api->pointCount() < 2)
        return 0.0;

    const double totalMeters = api->getTotalMeters();

    const double dist = convertMeasurement(totalMeters);

    if (logFn) {
        logFn(
            ReportCategory::MEASUREMENT,
            "Distance Measurement",
            "Distance Tool",
            "Auto From Points",
            QString("Total Points : %1").arg(api->pointCount()),
            QString("Total Distance : %1 %2")
                .arg(dist, 0, 'f', 2)
                .arg(measurementUnit),
            "SUCCESS",
            "",
            screenshotFn ? screenshotFn("measurement") : QString()
            );
    }

    return dist;
}

// void ScriptEngineGIS::SetMeasurementUnit(const std::string &unit)
// {
//     measurementUnit = QString::fromStdString(unit).toLower();
// }
void ScriptEngineGIS::SetMeasurementUnit(const std::string &unit)
{
    measurementUnit =
        QString::fromStdString(unit).trimmed().toLower();

    MeasureApi* api = MeasureApi::instance();

    if (api)
        api->setDisplayUnit(measurementUnit);
}

void ScriptEngineGIS::ToggleAirbases(bool visible)
{
    AirbaseApi* api = AirbaseApi::instance();
    if (!api)
        return;

    api->initialize();

    if (api->isVisible() != visible)
        api->toggleVisibility();

    airbaseLayerVisible = api->isVisible();

    if (gisMap)
        gisMap->update();

    if(logFn) {
        logFn(
            ReportCategory::UI,
            "Airbase Layer Toggled",
            "Airbase",
            "",
            "",
            airbaseLayerVisible ? "Airbases Visible" : "Airbases Hidden",
            "SUCCESS",
            "",
            ""
            );
    }
}

void ScriptEngineGIS::SwitchMap(const std::string& mapName)
{
    QString layer = QString::fromStdString(mapName).toLower();

    bool ok = MapApi::instance()->setActiveTileSource(layer);
    if (gisMap)
        gisMap->reloadTiles();

    if (logFn) {
        logFn(
            ReportCategory::GIS,
            "Map Switch",
            layer,
            "",
            "",
            layer,
            ok ? "SUCCESS" : "FAILED",
            "",
            ""
            );
    }
}

void ScriptEngineGIS::switchCoordinateSystem(const std::string& system)
{
    QString mode = QString::fromStdString(system).toLower();
    QString crsId;

    if (mode == "latlon" || mode == "wgs84") {
        crsId = "EPSG:4326";
    }
    else if (mode == "utm") {
        crsId = "UTM_AUTO";
    }
    else if (mode == "mgrs") {
        crsId = "MGRS";
    }
    else {
        Console::error("Invalid coordinate system: " + system);
        return;
    }

    // Coordinate system is a MapWidget display concern (not part of MapApi).
    if (gisMap)
        gisMap->setCoordSystem(crsId);

    if (logFn) {
        logFn(
            ReportCategory::GIS,
            "Coordinate System Switch",
            crsId,
            "",
            system.c_str(),
            crsId,
            "SUCCESS",
            "",
            ""
            );
    }
}

// Move a shape to a new geo location (parameter order: latitude, then longitude)
// void ScriptEngineGIS::moveShape(const std::string& shapeName,
//                                 double lat, double lon)
// {
//     QString name = QString::fromStdString(shapeName);
//     QString id = findShapeIdByName(name);
//     if (id.isEmpty())
//         id = name; // allow a raw shape id to be passed directly

//     if (!ShapeApi::instance()->moveShape(id, lon, lat)) {
//         Console::error("Shape not found: " + shapeName);
//         return;
//     }

//     if (gisMap)
//         gisMap->update();
// }
void ScriptEngineGIS::moveShape(const std::string& shapeName,
                                double lat,
                                double lon)
{
    QString name = QString::fromStdString(shapeName);

    QString id;

    // --------------------------------------------------------
    // 1. First use the ID stored when the shape was created
    // --------------------------------------------------------
    auto it = shapeNameToId.find(name);

    if (it != shapeNameToId.end())
    {
        id = it.value();
    }

    // --------------------------------------------------------
    // 2. Fallback: search ShapeApi by display name
    // --------------------------------------------------------
    if (id.isEmpty())
    {
        id = findShapeIdByName(name);
    }

    // --------------------------------------------------------
    // 3. Allow raw ShapeApi ID also
    // --------------------------------------------------------
    if (id.isEmpty())
    {
        id = name;
    }

    // --------------------------------------------------------
    // 4. Move
    // ShapeApi expects: longitude, latitude
    // --------------------------------------------------------
    if (!ShapeApi::instance()->moveShape(id, lon, lat))
    {
        Console::error(
            QString("Shape not found or move failed: %1")
                .arg(name)
                .toStdString()
            );
        return;
    }

    // --------------------------------------------------------
    // 5. Update script-side state
    // --------------------------------------------------------
    lastGeoCursor = QPointF(lon, lat);

    // --------------------------------------------------------
    // 6. Refresh map
    // --------------------------------------------------------
    if (gisMap)
        gisMap->update();

}
// rotate shape by angle
// void ScriptEngineGIS::rotateShape(const std::string& shapeName, double angleDeg)
// {
//     QString name = QString::fromStdString(shapeName);
//     QString id = findShapeIdByName(name);
//     if (id.isEmpty())
//         id = name;

//     QJsonObject patch;
//     patch[QStringLiteral("rotation")] = angleDeg;

//     if (!ShapeApi::instance()->updateShape(id, patch)) {
//         Console::error("Shape not found: " + shapeName);
//         return;
//     }

//     if (gisMap)
//         gisMap->update();
// }

void ScriptEngineGIS::rotateShape(
    const std::string& shapeName,
    double angleDeg)
{
    QString name = QString::fromStdString(shapeName);

    QString id;

    // First use stored ID
    auto it = shapeNameToId.find(name);

    if (it != shapeNameToId.end())
    {
        id = it.value();
    }

    // Fallback: search by display name
    if (id.isEmpty())
    {
        id = findShapeIdByName(name);
    }

    // Allow raw ID
    if (id.isEmpty())
    {
        id = name;
    }

    QJsonObject patch;
    patch[QStringLiteral("rotation")] = angleDeg;

    if (!ShapeApi::instance()->updateShape(id, patch))
    {
        Console::error(
            QString("Shape rotation failed: %1")
                .arg(name)
                .toStdString()
            );
        return;
    }

    if (gisMap)
        gisMap->update();
}

void ScriptEngineGIS::showShapeHistory(const std::string& shapeName)
{
    QString name = QString::fromStdString(shapeName);
    QString id = findShapeIdByName(name);
    if (id.isEmpty())
        id = name;

    ShapeHistoryApi::instance()->startPreview(id);

    if (gisMap)
        gisMap->update();
}

void ScriptEngineGIS::hideShapeHistory()
{
    ShapeHistoryApi::instance()->hidePreview();

    if (gisMap)
        gisMap->update();
}

void ScriptEngineGIS::restoreShapeHistory(const std::string& shapeName)
{
    QString name = QString::fromStdString(shapeName);
    QString id = findShapeIdByName(name);
    if (id.isEmpty())
        id = name;

    ShapeHistoryApi::instance()->restoreHistory(id);

    if (gisMap)
        gisMap->update();
}

// Add text at a geo location (parameter order: latitude, then longitude)
void ScriptEngineGIS::addText(const std::string& text, double lat, double lon)
{
    QString qtext = QString::fromStdString(text);
    QPointF geo(lon, lat);
    lastGeoCursor = geo;

    ShapeApi::instance()->addText(lon, lat, qtext);

    if (gisMap)
        gisMap->update();
}

void ScriptEngineGIS::addShapeProperties(const std::string& shapeName,int r, int g, int b,int borderThickness)
{
    QString name = QString::fromStdString(shapeName);
    QString id = findShapeIdByName(name);
    if (id.isEmpty())
        id = name;

    ShapeApi::instance()->updateShapeColor(id, QColor(r, g, b));
    ShapeApi::instance()->updateShapeWidth(id, borderThickness);

    if (gisMap)
        gisMap->update();
}

void ScriptEngineGIS::deleteshape(const std::string& id)
{
    QString qid = QString::fromStdString(id);
    QString realId = findShapeIdByName(qid);
    if (realId.isEmpty())
        realId = qid; // allow a raw shape id to be passed directly

    ShapeApi::instance()->deleteShape(realId);

    if (gisMap)
        gisMap->update();
}



ScriptEngineGIS::ScriptEngineGIS() {
    gisMap = nullptr;
    registerBuiltInLayerFormats();

    loadCityDatabase();
}


ScriptEngineGIS::~ScriptEngineGIS()
{
    // gisMap is injected (owned by RuntimeEditor / MapWith), not owned here.
}
