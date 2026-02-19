/******************************************************************************************
 * Description:
 * This file implements ScriptEngineGIS, which provides script-driven GIS functionality.
 * It handles drawing of points, lines, polygons, circles, rectangles, bitmaps, distance
 * measurement, offline geo-verification (India region), auto-zoom, screenshots and logging.
 * Author - Amjad
 ******************************************************************************************/


#include "scriptenginegis.h"
#include "qtimer.h"
#include <angelscript.h>
#include <angelscript/add_on/scriptstdstring/scriptstdstring.h>
#include <QMetaObject>
#include <QDebug>
#include <string>
#include <QObject>
#include <QVector3D>
#include "core/Hierarchy/entity.h"
#include <QPointF>
#include <cstdlib>
#include <angelscript/add_on/scriptarray/scriptarray.h>  // make sure included
#include "core/GlobalRegistry.h"
#include "core/Simulation/simulation.h"
#include "core/structure/runtime.h"
#include <fstream>
#include <QPrinter>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QDateTime>
#include <QPageSize>
#include <QDir>
#include <QPixmap>
#include <QDateTime>
#include <QPainter>
#include <QPrinter>
#include <QCoreApplication>
#include <QThread>
#include <QRectF>
#include <QtMath>
#include "scriptengine.h"
#include "GUI/Tacticaldisplay/Gis/shapes_feature.h"


// Helper: Convert latitude & longitude to formatted string
static QString geoToString(double lat, double lon)
{
    return QString("Lat:%1 Lon:%2")
    .arg(lat, 0, 'f', 6)
        .arg(lon, 0, 'f', 6);
}

// Inject GIS library dependency
void ScriptEngineGIS::setGIS(GISlib* g)
{
    gis = g;
}

// Static city database (offline, defence-safe)
struct CityInfo {
    QString name;
    double lat;
    double lon;
};

// Supported city list
static const CityInfo CITY_DB[] = {
    {"Bhopal",    23.278665, 77.369882},
    {"Delhi",     28.613939, 77.209023},
    {"Hyderabad", 17.385044, 78.486671},
    {"Mumbai",    19.076090, 72.877426},
    {"Bangalore", 12.9629, 77.5775},
    {"Chennai", 13.0843, 80.2705},

    };

// Return active city location string
QString ScriptEngineGIS::cityLocationString() const
{
    if (!cityActive) return "City : NOT SET";

    return QString("City : %1 | Lat : %2 | Lon : %3")
        .arg(activeCity)
        .arg(cityLat, 0, 'f', 6)
        .arg(cityLon, 0, 'f', 6);
}

// ================= GEO VERIFICATION (OFFLINE – INDIA REGION) =================
void ScriptEngineGIS::useCity(const std::string& cityName)
{
    QString name = QString::fromStdString(cityName);

    // Search city in static DB
    for (const auto& c : CITY_DB) {
        if (c.name.compare(name, Qt::CaseInsensitive) == 0) {

            // Activate city context
            activeCity = c.name;
            cityLat = c.lat;
            cityLon = c.lon;
            cityActive = true;

            // Log success
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
    }

    // City not found
    cityActive = false;

    if (logFn) {
        logFn(
            ReportCategory::SYSTEM,
            "City Selected",
            name,
            "",
            "",
            "Unknown city",
            "FAILED",
            "",
            ""
            );
    }
}

// Helper: Expand bounding box using lat/lon
static void updateBounds(QRectF& r, double lat, double lon)
{
    if (r.isNull())
        r = QRectF(QPointF(lon, lat), QSizeF(0, 0));
    else
        r = r.united(QRectF(QPointF(lon, lat), QSizeF(0, 0)));
}

// ================= GEO VERIFICATION (OFFLINE – INDIA REGION) =================
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


// Set selected drawing shape on canvas
void ScriptEngineGIS::setCanvasSelectedShape(const std::string &shapeName)
{
    if (!canvas) {
        qWarning() << "Canvas not set! Cannot set selected shape.";
        return;
    }

    QString qShape = QString::fromStdString(shapeName);
    canvas->setShapeDrawingMode(true, qShape);
    qDebug() << "[ScriptEngineGIS] Shape drawing mode enabled for:" << qShape;
}

void ScriptEngineGIS::setCanvas(CanvasWidget* c)
{
    canvas = c;

    // 🔥 CREATE ShapesFeature HERE (ONLY ONCE)
    // if (canvas && !canvas->getShapesFeature()) {
    //     canvas->getShapesFeature() = new ShapesFeature(canvas);
    // }
}


//===================LINE TOOL====================
// Start line drawing
void ScriptEngineGIS::canvasStartLine()
{
    if (!canvas) return;

    canvas->getShapesFeature()->scriptStartLine();

    // reset state
    linePoints.clear();
    lastShapeBounds = QRectF();
}
// Add line point
void ScriptEngineGIS::canvasAddLinePoint(float lon, float lat)
{
    if (!canvas) return;

    QPointF geo(lon, lat);
    lastGeoCursor = geo;

    canvas->getShapesFeature()->scriptAddLinePoint(geo);

    // collect point for final report
    linePoints.push_back(geo);

    updateBounds(lastShapeBounds, lat, lon);
}
// Finish line drawing
void ScriptEngineGIS::canvasFinishLine()
{
    if (!canvas || linePoints.size() < 2)
        return;

    canvas->getShapesFeature()->scriptFinishLine();

    // lastShapeBounds = QRectF();
    bool geoOk = true;

    for (const auto& p : linePoints) {
        updateBounds(lastShapeBounds, p.y(), p.x());
        if (!isLatLonInIndia(p.y(), p.x()))
            geoOk = false;
    }

    // FORCE AUTO ZOOM (state level)
    canvas->gislib->fitToBounds(
        lastShapeBounds.top(),
        lastShapeBounds.left(),
        lastShapeBounds.bottom(),
        lastShapeBounds.right(),
        -2   // slightly zoomed out
        );


    // Capture screenshot
    QString screenshot = screenshotFn("line");

    if(logFn) {
        logFn(
            ReportCategory::GIS,
            "Line Created",
            "Line Tool",
            "Auto From Points",
            QString("Total Points : %1").arg(linePoints.size()),
            geoOk ? "Geo Verification : VERIFIED (India Region)"
                  : "Geo Verification : FAILED (Outside India Region)",
            geoOk ? "SUCCESS" : "FAILED",
            "",
            screenshot
            );
    }
    linePoints.clear();
}

// Add a polygon
void ScriptEngineGIS::canvasAddPolygon(const std::string &name, CScriptArray* points)
{
    if (!canvas || !points || points->GetSize() < 3) {
        if(logFn) {
            logFn(
                ReportCategory::GIS,
                "Polygon Creation",
                QString::fromStdString(name),
                "",
                "Vertices < 3",
                "Not Rendered",
                "FAILED",
                "Polygon requires minimum 3 points",
                ""   // ✅ screenshotPath (EMPTY for failure)
                );
        }
        return;
    }

    bool geoOk = true;

    // RESET bounds
    lastShapeBounds = QRectF();

    //  DRAW POLYGON
    for (asUINT i = 0; i < points->GetSize(); i++) {
        QString pt = QString::fromStdString(*(std::string*)points->At(i));
        auto xy = pt.split(",");

        if (xy.size() != 2)
            continue;

        double lon = xy[0].toDouble();
        double lat = xy[1].toDouble();

        QPointF geo(lon, lat);
        lastGeoCursor = geo;

        // Update bounds (same helper as line/circle/rectangle)
        updateBounds(lastShapeBounds, lat, lon);

        // India geo check
        if (!isLatLonInIndia(lat, lon))
            geoOk = false;

        // FORCE AUTO ZOOM (state level)
        canvas->gislib->fitToBounds(
            lastShapeBounds.top(),
            lastShapeBounds.left(),
            lastShapeBounds.bottom(),
            lastShapeBounds.right(),
            -2   // slightly zoomed out
            );


        // Draw polygon vertex
        canvas->getShapesFeature()->drawPolygon(geo, i == points->GetSize() - 1);
    }

    // FORCE redraw before screenshot
    canvas->update();
    if (canvas->gislib)
        canvas->gislib->update();

    QCoreApplication::processEvents();
    QThread::msleep(150);
    QCoreApplication::processEvents();

    // SCREENSHOT
    QString screenshot = screenshotFn("polygon");

    if(logFn) {
        logFn(
            ReportCategory::GIS,
            "Polygon Created",
            QString::fromStdString(name),
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

// 4Add a rectangle
void ScriptEngineGIS::canvasAddRectangle(
    const std::string &name,
    float width,
    float height
    )
{
    if (!cityActive || !canvas){
        if(logFn) {
            logFn(
                ReportCategory::GIS,
                "Rectangle Creation",
                QString::fromStdString(name),
                "",
                "",
                "Not Rendered",
                "FAILED",
                "City not selected",
                ""   // ✅ screenshotPath (EMPTY for failure)
                );
        }
        return;
    }

    // Center of rectangle
    QPointF center(cityLon, cityLat);
    lastGeoCursor = center;

    // Geo validation
    bool geoOk = isLatLonInIndia(cityLat, cityLon);

    // Draw rectangle (center-based)
    canvas->getShapesFeature()->drawRectangle(center);

    // Reset previous bounds
    lastShapeBounds = {};

    // Convert meters → degrees (approx, screenshot-safe)
    double halfWidthDeg  = (width  / 2.0) / 111000.0;
    double halfHeightDeg = (height / 2.0) / 111000.0;

    // Update bounding box
    updateBounds(lastShapeBounds, cityLat + halfHeightDeg, cityLon + halfWidthDeg);
    updateBounds(lastShapeBounds, cityLat - halfHeightDeg, cityLon - halfWidthDeg);

    // FORCE AUTO ZOOM (state level)
    canvas->gislib->fitToBounds(
        lastShapeBounds.top(),
        lastShapeBounds.left(),
        lastShapeBounds.bottom(),
        lastShapeBounds.right(),
        -2   // slightly zoomed out
        );


    // Capture screenshot AFTER drawing & bounds update
    QString screenshot = screenshotFn("rectangle");

    // Log event
    if(logFn) {
        logFn(
            ReportCategory::GIS,
            "Rectangle Created",
            QString::fromStdString(name),
            cityLocationString(),
            QString("Width : %1 m, Height : %2 m").arg(width).arg(height),
            geoVerificationLine(cityLat, cityLon),
            geoOk ? "SUCCESS" : "FAILED",
            "",
            screenshot
            );
    }
}

// Add a circle
void ScriptEngineGIS::canvasAddCircle(const std::string &name, float radius)
{
    if (!cityActive || !canvas || !canvas->getShapesFeature()) {
        return;
    }

    //center
    QPointF center(cityLon, cityLat);
    lastGeoCursor = center;

    bool geoOk = isLatLonInIndia(cityLat, cityLon);

    canvas->getShapesFeature()->drawCircle(center);

    // Reset previous bounds
    // lastShapeBounds = {};
    lastShapeBounds = QRectF();

    // Convert meters → degrees (approx, safe for screenshots)
    double radiusDeg = radius / 111000.0;

    updateBounds(lastShapeBounds, cityLat + radiusDeg, cityLon + radiusDeg);
    updateBounds(lastShapeBounds, cityLat - radiusDeg, cityLon - radiusDeg);

    // FORCE AUTO ZOOM (state level)
    canvas->gislib->fitToBounds(
        lastShapeBounds.top(),
        lastShapeBounds.left(),
        lastShapeBounds.bottom(),
        lastShapeBounds.right(),
        -2   // slightly zoomed out
        );

    // Screenshot AFTER draw
    QString screenshot = screenshotFn("circle");

    if(logFn) {
        logFn(
            ReportCategory::GIS,
            "Circle Created",
            QString::fromStdString(name),
            cityLocationString(),
            QString("Radius : %1").arg(radius),
            geoVerificationLine(cityLat, cityLon),
            geoOk ? "SUCCESS" : "FAILED",
            "",
            screenshot        // store path
            );
    }
}


// Add a point
void ScriptEngineGIS::canvasAddPoint(const std::string &name, float lon, float lat)
{
    if (!canvas || !canvas->gislib || !canvas->getShapesFeature()) {
        if(logFn) {
            logFn(
                ReportCategory::GIS,
                "Point Creation",
                QString::fromStdString(name),
                geoToString(lat, lon),
                "",
                "Not Rendered",
                "FAILED",
                "Canvas or GIS not available",
                ""   // ✅ screenshotPath (EMPTY for failure)
                );
        }
        return;
    }

    QPointF geo(lon, lat);
    lastGeoCursor = geo;

    bool geoOk = isLatLonInIndia(lat, lon);

    //Draw point
    canvas->getShapesFeature()->drawPoints(geo);

    // RESET & EXPAND BOUNDS (🔥 KEY FIX)
    lastShapeBounds = QRectF();
    expandPointBounds(lastShapeBounds, lat, lon);

    // FORCE AUTO ZOOM (state level)
    canvas->gislib->fitToBounds(
        lastShapeBounds.top(),
        lastShapeBounds.left(),
        lastShapeBounds.bottom(),
        lastShapeBounds.right(),
        -1   // slightly zoomed out
        );

    // Force render
    canvas->update();
    canvas->gislib->update();
    QCoreApplication::processEvents();
    QThread::msleep(180);
    QCoreApplication::processEvents();

    // SCREENSHOT (MAP + CANVAS)
    QWidget* container = canvas->parentWidget();
    if (!container) return;

    QPixmap pixmap = container->grab();

    QString dir = QDir::tempPath() + "/gis_reports";
    QDir().mkpath(dir);

    QString screenshot =
        dir + "/" +
        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz") +
        "_point.png";

    pixmap.save(screenshot, "PNG");

    // LOG
    if(logFn) {
        logFn(
            ReportCategory::GIS,
            "Point Created",
            QString::fromStdString(name),
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
    // approx 15–20 km box (state / district level)
    double pad = 0.15; // degrees (~16 km)

    bounds = QRectF(
        QPointF(lon - pad, lat - pad),
        QPointF(lon + pad, lat + pad)
        );
}


// BITMAP HANDLING
void ScriptEngineGIS::expandBitmapBoundsFromCanvas(QRectF &bounds)
{
    if (!canvas || canvas->tempMeshes.empty())
        return;

    const MeshEntry &entry = canvas->tempMeshes.back();
    if (!entry.position)
        return;

    double lon = entry.position->x();
    double lat = entry.position->y();

    // small visual padding (~10–15 km)
    double pad = 0.12;

    bounds = QRectF(
        QPointF(lon - pad, lat - pad),
        QPointF(lon + pad, lat + pad)
        );
}

// select bitmap
void ScriptEngineGIS::onBitmapSelected(const std::string &bitmapType, float lon, float lat)
{
    if (!canvas || !canvas->gislib)
        return;

    // Set exact geo position
    QPointF geo(lon, lat);
    lastGeoCursor = geo;

    // Place bitmap
    canvas->onBitmapSelectedAtGeo(
        QString::fromStdString(bitmapType),
        geo
        );

    // Bounds (treat bitmap as point)
    lastShapeBounds = QRectF();
    expandPointBounds(lastShapeBounds, lat, lon);

    // FORCE AUTO ZOOM (state level)
    canvas->gislib->fitToBounds(
        lastShapeBounds.top(),
        lastShapeBounds.left(),
        lastShapeBounds.bottom(),
        lastShapeBounds.right(),
        -2   // slightly zoomed out
        );


    // Screenshot
    QString screenshot = screenshotFn("bitmap");

    if(logFn) {
        logFn(
            ReportCategory::UI,
            "Bitmap Placed",
            QString::fromStdString(bitmapType),
            geoToString(lat, lon),
            "Script Placed Bitmap",
            "Bitmap Visible",
            "SUCCESS",
            "",
            screenshot
            );
    }
}

// Place bitmap image from arbitrary file
// Get path of built-in bitmap
std::string ScriptEngineGIS::getBitmapImagePath(const std::string &bitmapType)
{
    if (!canvas) return "";
    QString path = canvas->getBitmapImagePath(QString::fromStdString(bitmapType));
    return path.toStdString();
}
// select bitmap from device
void ScriptEngineGIS::onBitmapImageSelected(
    const std::string &filePath,
    float lon,
    float lat
    )
{
    if (!canvas || !canvas->gislib)
        return;

    QPointF geo(lon, lat);
    lastGeoCursor = geo;

    // Place bitmap (canvas decides final geo)
    canvas->onBitmapImageSelectedAtGeo(
        QString::fromStdString(filePath),
        geo
        );

    // TAKE BOUNDS FROM ACTUAL BITMAP POSITION
    lastShapeBounds = QRectF();
    expandBitmapBoundsFromCanvas(lastShapeBounds);

    // FORCE AUTO ZOOM (state level)
    canvas->gislib->fitToBounds(
        lastShapeBounds.top(),
        lastShapeBounds.left(),
        lastShapeBounds.bottom(),
        lastShapeBounds.right(),
        -2   // slightly zoomed out
        );


    // Screenshot (zoom happens inside screenshotFn)
    QString screenshot = screenshotFn("user_bitmap");

    if(logFn) {
        logFn(
            ReportCategory::UI,
            "Bitmap Placed",
            QFileInfo(QString::fromStdString(filePath)).fileName(),
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
void ScriptEngineGIS::canvasImportGeoJsonLayer(const std::string &filePath)
{
    if (!canvas) return;

    canvas->importGeoJsonLayer(QString::fromStdString(filePath));

    if(logFn) {
        logFn(
            ReportCategory::GIS,
            "GeoJSON Imported",
            QFileInfo(QString::fromStdString(filePath)).fileName(),
            "",
            "Not Rendered",
            "File Import",
            "Layer Loaded",
            "SUCCESS",
            ""   // ✅ screenshotPath (EMPTY for failure)
            );
    }
}

void ScriptEngineGIS::canvasToggleGeoJsonLayer(const std::string &layerName, bool visible)
{
    if (!canvas) return;

    canvas->onGeoJsonLayerToggled(QString::fromStdString(layerName), visible);

    if(logFn) {
        logFn(
            ReportCategory::GIS,
            "GeoJSON Layer Toggled",
            QString::fromStdString(layerName),
            "",
            "Not Rendered",
            visible ? "ON" : "OFF",
            visible ? "Visible" : "Hidden",
            "SUCCESS",
            ""   // ✅ screenshotPath (EMPTY for failure)
            );
    }
}

// DISTANCE MEASUREMENT
void ScriptEngineGIS::canvasStartDistanceMeasurement()
{
    if (!canvas) return;

    canvas->startDistanceMeasurement();

    // ✅ reset measurement session
    measurementPoints.clear();

    lastShapeBounds = QRectF();
}

void ScriptEngineGIS::canvasAddMeasurePoint(double lon, double lat)
{
    lastGeoCursor = QPointF(lon, lat);
    if (!canvas) return;

    canvas->addMeasurePoint(lon, lat);

    // ✅ collect points for final report
    measurementPoints.push_back(QPointF(lon, lat));

    updateBounds(lastShapeBounds, lat, lon);
}

double ScriptEngineGIS::canvasGetLastSegmentDistance() {
    return canvas ? canvas->getLastSegmentDistance() : 0.0;
}

double ScriptEngineGIS::canvasGetTotalDistance()
{
    if (!canvas || measurementPoints.size() < 2) {
        if(logFn) {
            logFn(
                ReportCategory::MEASUREMENT,
                "Distance Measurement",
                "Distance Tool",
                "",
                "",
                "Not Rendered",
                "FAILED",
                "At least 2 points required",
                ""   // ✅ screenshotPath (EMPTY for failure)
                );
            return 0.0;
        }
    }

    bool geoOk = true;
    for (const auto& p : measurementPoints) {
        if (!isLatLonInIndia(p.y(), p.x())) {
            geoOk = false;
        }
    }

    double dist = canvas->getTotalDistance();

    // 🔹 Take screenshot (MAP + CANVAS + MEASUREMENT)
    QString screenshot = screenshotFn("measurement");

    if(logFn) {
        logFn(
            ReportCategory::MEASUREMENT,
            "Distance Measurement",
            "Distance Tool",
            "Auto From Points",
            QString("Total Points : %1").arg(measurementPoints.size()),
            QString("Total Distance : %1 %2").arg(dist, 0, 'f', 2).arg(measurementUnit),
            geoOk ? "SUCCESS" : "FAILED",
            "",
            screenshot
            );
    }

    return dist;
}

void ScriptEngineGIS::canvasSetMeasurementUnit(const std::string &unit)
{
    if (!canvas) return;

    measurementUnit = QString::fromStdString(unit).toLower();
    canvas->setMeasurementUnit(measurementUnit);
}


void ScriptEngineGIS::canvasToggleAirbases()
{
    if (!canvas)
        return;

    // Toggle Airbase layer ON / OFF
    canvas->onPresetLayerSelected("Airbase");
    airbaseLayerVisible = !airbaseLayerVisible;

    // Let map + icons render at CURRENT zoom
    canvas->update();
    if (canvas->gislib)
        canvas->gislib->update();

    QCoreApplication::processEvents();
    QThread::msleep(200);
    QCoreApplication::processEvents();

    // DIRECT SCREENSHOT (NO ZOOM CHANGE)
    QWidget* container = canvas->parentWidget();
    if (!container)
        return;

    QPixmap pixmap = container->grab();

    QString dir = QDir::tempPath() + "/gis_reports";
    QDir().mkpath(dir);

    QString screenshot =
        dir + "/" +
        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz") +
        "_airbases.png";

    pixmap.save(screenshot, "PNG");

    // CLEAN LOG (NO LOCATION / NO COUNT)
    if(logFn) {
        logFn(
            ReportCategory::UI,
            "Airbase Layer Toggled",
            "Airbase",
            "",
            "",
            airbaseLayerVisible ? "Airbases Visible"
                                : "Airbases Hidden",
            "SUCCESS",
            "",
            screenshot
            );
    }
}


void ScriptEngineGIS::canvasSwitchMap(const std::string& mapName)
{
    if (!gis) return;

    QString layer = QString::fromStdString(mapName).toLower();

    // Switch base layer
    gis->setLayers(QStringList() << layer);

    if (logFn) {
        logFn(
            ReportCategory::GIS,
            "Map Switch",
            layer,
            "",
            "",
            layer,
            "SUCCESS",
            "",
            screenshotFn ? screenshotFn("map_switch_" + layer) : ""
            );
    }
}

void ScriptEngineGIS::switchCoordinateSystem(const std::string& system)
{
    if (!gis) return;

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

    gis->setCoordinateSystem(crsId);

    // Optional logging + screenshot (same pattern as map switch)
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
            screenshotFn ? screenshotFn("coord_" + crsId) : ""
            );
    }
}

void ScriptEngineGIS::moveShape(const std::string& shapeName,
                                double lon, double lat)
{
    if (!canvas) return;

    canvas->moveShapeByName(shapeName, QPointF(lon, lat));
}

// rotate shape by angle
void ScriptEngineGIS::rotateShape(const std::string& shapeName, double angleDeg)
{
    if (!canvas) {
        Console::error("Canvas not available");
        return;
    }

    for (auto &entry : canvas->tempMeshes) {
        if (entry.name.toStdString() == shapeName) {

            // ✅ SAVE STATE FIRST
            canvas->getShapesFeature()->saveShapeState(entry.name, &entry);

            // if (!entry.rotation)
            //     return;

            // degrees → radians
            double rad = qDegreesToRadians(angleDeg);

            entry.rotation->setZ(rad);

            canvas->Refresh();
            return;
        }
    }

    Console::error("Shape not found: " + shapeName);
}


void ScriptEngineGIS::showShapeHistory(const std::string& shapeName)
{
    if (!canvas || !canvas->getShapesFeature()) return;

    canvas->getShapesFeature()->showHistoryPreview(
        QString::fromStdString(shapeName)
        );
}

void ScriptEngineGIS::hideShapeHistory()
{
    if (!canvas || !canvas->getShapesFeature()) return;

    canvas->getShapesFeature()->hideHistoryPreview();
}

void ScriptEngineGIS::restoreShapeHistory(const std::string& shapeName)
{
    if (!canvas || !canvas->getShapesFeature()) return;

    for (auto& entry : canvas->tempMeshes) {
        if (entry.name.toStdString() == shapeName) {
            canvas->getShapesFeature()->restorePreviousState(
                entry.name,
                &entry
                );
            canvas->Refresh();
            return;
        }
    }

    Console::error("Shape not found for history restore: " + shapeName);
}

void ScriptEngineGIS::addText(const std::string& text, double lon, double lat)
{
    if (!canvas) return;

    QPointF geo(lon, lat);
    lastGeoCursor = geo;

    canvas->addTextAtGeo(QString::fromStdString(text), geo);
}

void ScriptEngineGIS::addShapeProperties(const std::string& shapeName,int r, int g, int b,int borderThickness)
{
    if (!canvas) return;

    canvas->updateShapeProperties(
        QString::fromStdString(shapeName),
        QColor(r, g, b),
        borderThickness
        );
}

void ScriptEngineGIS::deleteshape(const std::string& id)
{
    if (!canvas) return;
    canvas->deleteObjectById(QString::fromStdString(id));
}



ScriptEngineGIS::ScriptEngineGIS() {

    canvas = nullptr;
    gis = nullptr;
    // canvas->getShapesFeature() = nullptr;
}


ScriptEngineGIS::~ScriptEngineGIS()
{
    delete canvas->getShapesFeature();
    //canvas->getShapesFeature() = nullptr;
}
