//Author - Amjad
// Updated for new GIS backend (ShapeApi / LayerApi / MapApi / GISEngine)
// Widget + GISlib have been replaced by MapWidget + the API singletons.

#ifndef SCRIPTENGINEGIS_H
#define SCRIPTENGINEGIS_H

#include <QObject>
#include <QString>
#include <QPointF>
#include <QRectF>
#include <QColor>
#include <angelscript.h>
#include <angelscript/add_on/scriptarray/scriptarray.h>
#include <vector>
#include <functional>
#include <QMap>

struct CityInfo {
    QString name;
    double lat;
    double lon;
};

// Forward declarations
class MapWidget;   // GIS_API/frontend/widgets/map_widget.h
enum class ReportCategory;

class ScriptEngineGIS
{
public:
    ScriptEngineGIS();
    ~ScriptEngineGIS();

    // ===== Dependency Injection =====
    // Replaces the old set(Widget*) / setGIS(GISlib*) pair.
    // MapWidget is the only widget-level dependency left: it is only needed
    // for things that touch actual pixels (screenshots, forcing a repaint,
    // interactive draw-mode, coordinate-system display). All data operations
    // (shapes/layers/tiles) go through the ShapeApi / LayerApi / MapApi
    // singletons and don't need this pointer at all.
    void setMapWidget(MapWidget* map);

    // ===  Shape Wrappers (for AngelScript) ===

    // Set currently selected shape name (enables interactive draw mode)
    void setSelectedShape(const std::string &shapeName);

    // Vector layer management
    void AddLayer(const std::string &layerName);
    void SelectLayer(const std::string &layerName);
    void RenameShape(const std::string &id,const std::string &newName);

    // ===== Script Line Tool =====
    void StartLine();   // Begin line drawing mode
    void AddLinePoint(float lat, float lon);   // Add a point to the current line (latitude, longitude)
    void FinishLine();  // Finish and commit the current line

    // ================= SHAPE CREATION =================
    // Add polygon using list of points
    void AddPolygon(const std::string &name, CScriptArray* points);

    // Add rectangle shape (center-based dimensions, meters)
    void AddRectangle(const std::string &name, float width, float height);

    // Add circular shape (radius in meters, centered on active city)
    void AddCircle(const std::string &name, float radius);

    // Add a single point marker
    void AddPoint(const std::string &name, float lat, float lon);

    // ================= BOUND CALCULATIONS =================
    // Expand geographic bounds using a lat/lon point
    void expandPointBounds(QRectF &bounds, double lat, double lon);
    // Expand bounds based on the last bitmap shape that was placed
    void expandBitmapBoundsFrom(QRectF &bounds);

    // ================= BITMAP HANDLING =================
    // Place a predefined bitmap at coordinates
    void onBitmapSelected(const std::string &bitmapType, float lat, float lon);

    // Get filesystem path of a built-in bitmap
    std::string getBitmapImagePath(const std::string &bitmapType);

    // Place bitmap image from external file
    void onBitmapImageSelected(const std::string &filePath, float lat, float lon);

    // ===== Preset Layers =====
    void ToggleAirbases(bool visible);

    // ================= GEOJSON LAYERS =================
    // Import a GeoJSON file as a map layer
    void ImportGeoJsonLayer(const std::string &filePath);
    // Toggle visibility of a GeoJSON layer (by layer name)
    void ToggleGeoJsonLayer(const std::string &layerName, bool visible);

    // ================= DISTANCE MEASUREMENT =================
    // Start distance measurement mode
    void StartDistanceMeasurement();

    // Add measurement coordinate


    // Get distance of last measured segment
    void AddMeasurePoint(double lat, double lon);
    double GetLastSegmentDistance();

    // Get total measured distance
    double GetTotalDistance();

    // Set unit for distance calculation (meters/kilometers/miles/feet)
    void SetMeasurementUnit(const std::string &unit);


    // ================= MAP SWITCH =================
    // Switch base map / tile source (osm, satellite, terrain, opentopo, ...)
    void SwitchMap(const std::string& mapName);


    // ================= CITY CONTEXT =================
    // Activate a city context by name
    void useCity(const std::string& cityName);
    QString cityLocationString() const;

    // ================= COORDINATE SYSTEM =================
    // Switch coordinate system (latlon, utm, mgrs)
    void switchCoordinateSystem(const std::string& system);

    // Move any shape to new geo location (by shape name/label, or raw id)
    void moveShape(const std::string& shapeName, double lat, double lon);

    // rotate shape
    void rotateShape(const std::string& shapeName, double angleDeg);

    void showShapeHistory(const std::string& shapeName);     // show history
    void hideShapeHistory();  // hide history
    void restoreShapeHistory(const std::string& shapeName);  // restore history

    // add text
    void addText(const std::string& text, double lat, double lon);

    void addShapeProperties(const std::string& shapeName, int r, int g, int b, int borderThickness);

    void deleteshape(const std::string& id);

    // ===== Report Hook (BOUND BY ScriptEngine) =====
    // Bound by ScriptEngine to log execution events
    std::function<void(
        ReportCategory,
        const QString& action,
        const QString& name,
        const QString& location,
        const QString& input,
        const QString& output,
        const QString& status,
        const QString& reason,
        const QString& screenshot
        )> logFn;

    // ===== Screenshot Hook (BOUND BY ScriptEngine) =====
    std::function<QString(const QString& tag)> screenshotFn;

    static void loadCityDatabase(const QString& jsonPath = ":/jsonData/DB/jsonData/city.json");


private:
    // Only widget-level dependency left. Not owned by this class.
    MapWidget* gisMap = nullptr;

    bool airbaseLayerVisible = false;      // Airbase layer visibility state

    // ===== State =====
    QPointF lastGeoCursor;                    // Last cursor position in geographic coordinates
    QRectF lastShapeBounds;                   // Bounding box of last drawn shape
    std::vector<QPointF> linePoints;          // Points collected for line drawing
    std::vector<QPointF> measurementPoints;   // Points collected for distance measurement

    QString measurementUnit = "kilometers";  // Measurement unit (default: kilometers)

    // ================= GEO VERIFICATION =================
    bool isLatLonInIndia(double lat, double lon) const;
    QString geoVerificationLine(double lat, double lon) const;

    // ================= NAME -> ID RESOLUTION =================
    // Shapes/layers are created with a friendly "name" (stored in the
    // shape's "text" property / layer name) but the new backend addresses
    // everything by generated id. These helpers resolve name -> id so the
    // existing AngelScript API (which takes names) keeps working unchanged.
    QString findShapeIdByName(const QString& name) const;
    QString findLayerIdByName(const QString& name) const;

    // Convert a raw distance in meters into the currently selected
    // measurementUnit.
    double convertMeasurement(double meters) const;

    QMap<QString, QString> shapeNameToId;

    // ================= LOCATION CONTEXT =================
    QString activeCity;      // Current selected city
    double cityLat = 0.0;
    double cityLon = 0.0;
    bool cityActive = false;

    static QMap<QString, CityInfo> s_cityDatabase;
    static bool s_cityDatabaseLoaded;

};

#endif // SCRIPTENGINEGIS_H
