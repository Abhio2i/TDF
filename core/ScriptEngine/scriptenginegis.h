//Author - Amjad

#ifndef SCRIPTENGINEGIS_H
#define SCRIPTENGINEGIS_H

#include <QObject>
#include <angelscript.h>
#include <angelscript/add_on/scriptarray/scriptarray.h>
#include "GUI/Tacticaldisplay/canvaswidget.h"

// Forward declaration
class GISlib;
class CanvasWidget;
enum class ReportCategory;

class ScriptEngineGIS
{
public:
    ScriptEngineGIS();
    ~ScriptEngineGIS();

    // ===== Dependency Injection =====
    void setCanvas(CanvasWidget* c);
    void setGIS(GISlib* g); // for map

    // Inject container widget holding the map canvas
    void setMapCanvasContainer(QWidget* w) {
        mapCanvasContainer = w;
    }

    // === Canvas Shape Wrappers (for AngelScript) ===

    // Set currently selected shape name
    void setCanvasSelectedShape(const std::string &shapeName);

    // Vector layer management
    void canvasCreateVectorLayer(const std::string &layerName);
    void canvasSelectLayer(const std::string &layerName);
    void canvasRenameShape(const std::string &id,const std::string &newName);

    // ===== Script Line Tool =====
    void canvasStartLine();   // Begin line drawing mode
    void canvasAddLinePoint(float lon, float lat);   // Add a point to the current line (longitude, latitude)
    void canvasFinishLine();  // Finish and commit the current line

    // ================= SHAPE CREATION =================
    // Add polygon using list of points
    void canvasAddPolygon(const std::string &name, CScriptArray* points);

    // Add rectangle shape (center-based dimensions)
    //  void canvasAddRectangle(const std::string &name, float x, float y, float width, float height);
    void canvasAddRectangle(const std::string &name, float width, float height);


    // Add circular shape
    // void canvasAddCircle(const std::string &name, float x, float y, float radius);
    void canvasAddCircle(const std::string &name, float radius);

    // Add a single point marker
    void canvasAddPoint(const std::string &name, float x, float y);

    // ================= BOUND CALCULATIONS =================
    // Expand geographic bounds using a lat/lon point
    void expandPointBounds(QRectF &bounds, double lat, double lon);
    // Expand bounds based on bitmap placed on canvas
    void expandBitmapBoundsFromCanvas(QRectF &bounds);

    // ================= BITMAP HANDLING =================
    // Place a predefined bitmap at coordinates
    // void onBitmapSelected(const std::string &bitmapType);
    void onBitmapSelected(const std::string &bitmapType, float x, float y);

    // Get filesystem path of a built-in bitmap
    std::string getBitmapImagePath(const std::string &bitmapType);

    // Place bitmap image from external file
    // void onBitmapImageSelected(const std::string &filePath);
    void onBitmapImageSelected(const std::string &filePath, float lon, float lat);

    // ===== Preset Layers =====
    void canvasToggleAirbases();

    // ================= GEOJSON LAYERS =================
    //Import a GeoJSON file as a map layer
    void canvasImportGeoJsonLayer(const std::string &filePath);
    // Toggle visibility of a GeoJSON layer
    void canvasToggleGeoJsonLayer(const std::string &layerName, bool visible);

    // ================= DISTANCE MEASUREMENT =================
    // Start distance measurement mode
    void canvasStartDistanceMeasurement();

    // Add measurement coordinate
    void canvasAddMeasurePoint(double lon, double lat);

    // Get distance of last measured segment
    double canvasGetLastSegmentDistance();

    // Get total measured distance
    double canvasGetTotalDistance();

    // Set unit for distance calculation
    void canvasSetMeasurementUnit(const std::string &unit);


    // ================= MAP SWITCH =================
    // Switch base map (osm, satellite, tarrine, opentopo)
    void canvasSwitchMap(const std::string& mapName);


    // ================= CITY CONTEXT =================
    // Activate a city context by name
    void useCity(const std::string& cityName);
    QString cityLocationString() const;

    // ================= COORDINATE SYSTEM =================
    // Switch coordinate system (latlon, utm, mgrs)
    void switchCoordinateSystem(const std::string& system);

    // Move any shape to new geo location
    void moveShape(const std::string& shapeName, double lon, double lat);

    // rotate shape
    void rotateShape(const std::string& shapeName, double angleDeg);

    void showShapeHistory(const std::string& shapeName);     // show history
    void hideShapeHistory();  /// hide history
    void restoreShapeHistory(const std::string& shapeName);  // restore history

    // add text
    void addText(const std::string& text, double lon, double lat);

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

private:
    // Canvas widget used for rendering
    CanvasWidget* canvas = nullptr;

    // ShapesFeature* m_shapes;

    // GIS library instance
    GISlib* gis = nullptr;       // for map

    QWidget* mapCanvasContainer = nullptr;

    bool airbaseLayerVisible = false;      // Airbase layer visibility state

    // ===== State =====
    // Last cursor position in geographic coordinates
    QPointF lastGeoCursor;   // Bounding box of last drawn shape
    QRectF lastShapeBounds; // GeoBounds lastShapeBounds;
    std::vector<QPointF> linePoints;   // Points collected for line drawing
    std::vector<QPointF> measurementPoints;   // Points collected for distance measurement

    QString measurementUnit = "kilometers";  // Measurement unit (default: kilometers)
    bool measurementActive = false;  // Measurement active flag

    // ================= GEO VERIFICATION =================
    // Check whether given lat/lon lies within India
    bool isLatLonInIndia(double lat, double lon) const;

    // Return formatted verification string for lat/lon
    QString geoVerificationLine(double lat, double lon) const;

    // ================= LOCATION CONTEXT =================
    QString activeCity;      // Current selected city
    double cityLat = 0.0;
    double cityLon = 0.0;
    bool cityActive = false;

};

#endif // SCRIPTENGINEGIS_H
