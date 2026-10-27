#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "GUI/Hierarchytree/hierarchytree.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Render/scenerenderer.h"
#include <QObject>
#include <QtGlobal>
#include <angelscript.h>
#include <angelscript/add_on/scriptarray/scriptarray.h>

#include <QElapsedTimer>
#include <QStringList>

// AngelScript wrapper for SensorProfile::getSensor
Sensor* AS_SensorProfile_getSensor(
    SensorProfile* self,
    const std::string& id
    );

// ================= AUTOSCRIPT REPORT SYSTEM =================
// Categories used for execution report logging
enum class ReportCategory {
    GIS,
    UI,
    MEASUREMENT,
    MATH,
    SIMULATION,
    SYSTEM
};

// Structure representing a single script execution event
struct ReportEvent {
    ReportCategory category;   // Category of the event
    QString action;            // Action performed (e.g., Create, Update)
    QString name;              // Name of object/entity
    QString location;          // Location info (lat/long or context)
    QString input;             // Input parameters
    QString output;            // Output/result
    QString status;            // SUCCESS / WARNING / FAILED
    QString reason;            // Failure or warning reason

    QString screenshotPath;    // Optional screenshot path
};


// ================= FORWARD DECLARATIONS =================
class ScriptEngineGIS;
class MapWidget;      // GIS_API/frontend/widgets/map_widget.h -- replaces Widget/GISlib
class Platform;
class ProfileCategaory;
class Runtime;
class Sensor;
class SensorProfile;
class SidebarWidget;
class RuntimeEditor;
class Radio;

class MyObj {
public:
    float x, y;
    MyObj() : x(0), y(0) {};
    void moveBy(float dx, float dy)
    {
        this->x += dx;
        this->y += dy;
    }

};

class ScriptEngine : public QObject
{
    Q_OBJECT

public:
    ScriptEngine();
    ~ScriptEngine();

    void setHierarchy(Hierarchy *hier, HierarchyTree *tr, SceneRenderer *rend);
    bool loadAndCompileScript(QString scriptContent);
    void run();
    void setRuntime(Runtime* rt) { runtime = rt; }
    Runtime* getRuntime() const { return runtime; }

    void generatePDFReport(const std::string &filePath);

    // autoscript  /////////////////////////////////////
    bool isScriptRunning() const { return scriptRunning; }

    // Restrict ReadFile/WriteFile/AppendFile to one folder (used for auto scripts)
    void setFileSandbox(bool enabled, const QString& dir = QString());

    // Timeouts (ms). 0 = disabled.
    void setMaxSliceTime(int ms) { m_maxSliceMs = ms; }   // per continuous run (between sleeps)
    void setMaxTotalTime(int ms) { m_maxTotalMs = ms; }   // whole script incl. sleeps

    void appendEngineMessage(const QString& line);        // used by MessageCallback

public slots:
    void ScriptSleep(int milliseconds);
    ProfileCategaory* addProfiles(const std::string &name);
    Platform *addEntity(const std::string &Id, const std::string &name, bool &profile);
    Folder* addFolder(const std::string &Id, const std::string &name, bool &profile);
    void addComponent(const std::string &Id,const std::string &name);

    void renameEntity(const std::string &id, const std::string &newName);
    // Rename a profile by ID
    void renameProfile(const std::string& profileID, const std::string& newName);
    // Wrapper for AngelScript
    void renameFolder(const std::string& folderID, const std::string& newName);
    // ------------------ Remove Profile Wrapper ------------------
    // Usage in AS: removeProfile("profileId123");
    void removeProfile(const std::string& profileID);
    // Remove Folder wrapper for AngelScript
    void removeFolder(const std::string& parentId, const std::string& folderID);
    // Remove Entity wrapper for AngelScript
    void removeEntity(const std::string& parentId, const std::string& entityID);
    // === Finder functions ===
    Entity* getEntityById(const std::string &id);
    CScriptArray* findEntitiesByType(int type);
    CScriptArray* getAllEntities();
    CScriptArray* getAllEntityIds();
    void renderscene();


    //================ GIS wrappers (AngelScript exposed) ====================

    Q_INVOKABLE void AddLayer(const std::string &layerName);
    Q_INVOKABLE void SelectLayer(const std::string &layerName);
    //Q_INVOKABLE void RenameShape(const std::string &id,const std::string &newName);

    //Q_INVOKABLE void AddCircle(const std::string &name, float x, float y, float radius);
    Q_INVOKABLE void AddCircle(const std::string &name, float radius);
    // Q_INVOKABLE void AddRectangle(const std::string &name, float x, float y, float width, float height);
    Q_INVOKABLE void AddRectangle(const std::string &name, float w, float h);
    Q_INVOKABLE void AddPolygon(const std::string &name, CScriptArray* pts);

    // ===== Script Line Tool =====
    Q_INVOKABLE void StartLine();
    Q_INVOKABLE void AddLinePoint(float lat, float lon);
    Q_INVOKABLE void FinishLine();
    Q_INVOKABLE void AddPoint(const std::string &name, float lat, float lon);

    Q_INVOKABLE void onBitmapSelected(const std::string &bitmapType, float lat, float lon);

    // Get path of built-in bitmap
    Q_INVOKABLE void  getBitmapImagePath(const std::string &bitmapType);
    Q_INVOKABLE void onBitmapImageSelected(const std::string &filePath, float lat, float lon);

    // ===== Preset Layers =====
    void ToggleAirbases(bool visible);

    // // === GeoJSON Layer Wrappers ===
    // Q_INVOKABLE void ImportGeoJsonLayer(const std::string &filePath);
    Q_INVOKABLE void ToggleGeoJsonLayer(const std::string &layerName, bool visible);

    // ===== Measurement =====
    void StartDistanceMeasurement();
    void AddMeasurePoint(double lat, double lon);
    double GetLastSegmentDistance();
    double GetTotalDistance();
    void SetMeasurementUnit(const std::string &unit);

    // Switch base map (osm, satellite, tarrine, opentopo)
    Q_INVOKABLE void SwitchMap(const std::string& mapName);

    // Switch coordinate system (latlon, utm, mgrs)
    Q_INVOKABLE void switchCoordinateSystem(const std::string& system);

    // Move any shape to new geo location (latitude first, then longitude)
    Q_INVOKABLE void moveShape(const std::string& shapeName, double lat, double lon);

    // rotate shape by angle
    Q_INVOKABLE void rotateShape(const std::string& shapeName, double angleDeg);

    // shape history
    Q_INVOKABLE void showShapeHistory(const std::string& shapeName);
    Q_INVOKABLE void hideShapeHistory();
    Q_INVOKABLE void restoreShapeHistory(const std::string& shapeName);

    Q_INVOKABLE void addText(const std::string& text, double lat, double lon);

    Q_INVOKABLE void addShapeProperties(const std::string& shapeName,int r, int g, int b,int borderThickness);

    Q_INVOKABLE void deleteshape(const std::string& id);

public:
    Hierarchy *hierarchy;
    HierarchyTree *tree;
    SceneRenderer *render;
    MyObj* e = nullptr;
public:
    // ===  Shape Wrappers (for AngelScript) ===
    Q_INVOKABLE void setSelectedShape(const std::string &shapeName);

    // Fetch existing ProfileCategaory by name
    ProfileCategaory* getProfileByName(const std::string& name);

    // Returns the newly created Platform entity under an existing profile
    Platform* addEntityToPlatform(ProfileCategaory* platformProfile, const std::string& entityName);

    // Attach a Sensor to an Entity by ID
    void attachSensorToEntity(const std::string& entityId, const std::string& sensorName,const std::string& sensorType);
    void attachIFFToEntity(const std::string& entityID, const std::string& iffName);
    void attachRadioToEntity(const std::string& entityID, const std::string& radioName);

    // // ================= CITY CONTEXT =================
    Q_INVOKABLE void useCity(const std::string& cityName);
    QString cityLocationString() const;

    // ================= MAP WIDGET SETTER =================
    // Replaces the old set(Widget*) + setGIS(GISlib*) pair.
    // MapWidget is the only widget-level dependency ScriptEngine/ScriptEngineGIS
    // still need (for screenshots / forced repaint / interactive draw mode);
    // everything else goes through the ShapeApi/LayerApi/MapApi singletons.
    void setMapWidget(MapWidget* map);

    void logEvent(
        ReportCategory category,
        const QString& action,
        const QString& name,
        const QString& location,
        const QString& input,
        const QString& output,
        const QString& status,
        const QString& reason = "",
        const QString& screenshotPath = ""
        );

    void addSubComponent(
        const std::string& entityId,
        ComponentType type,
        const std::string& name,
        const std::string& data1,
        const std::string& data2,
        const std::string& data3
        );
    void addSensorSubComponent(
        Platform* platform,
        const std::string& subName,
        const std::string& sensorType
        );

    void showSidebarView(const std::string &viewName);
    void selectEntityDisplay(Sensor* sensor);
    void captureSensorScreenshot(const std::string &filePath);
    void addRadioSubComponent(Platform* platform,const std::string& subName);
    void selectEntityDisplay(Radio* radio);
    void addIFFSubComponent(Platform* platform, const std::string& subName);
    void selectEntityDisplay(IFF* iff);

    void stopScript();

private:
    asIScriptEngine* engine = nullptr;
    asIScriptModule* mod = nullptr;
    asIScriptFunction* func = nullptr;
    asIScriptContext* ctx = nullptr;
    asITypeInfo* arrayEntityType = nullptr; // store type info once
    MapWidget* gisMap = nullptr;            // <--- replaces  + gis
    Runtime* runtime = nullptr;

    quint64 scriptGeneration = 0;
    bool scriptRunning = false;

    ScriptEngineGIS* gisEngine = nullptr;

    std::vector<ReportEvent> executionReport;

    //for screenshot capture
    QString captureScreenshot(const QString& tag);


    // ================= SCREENSHOT AUTO-ZOOM =================
    struct GeoBounds {
        double minLat = 0;
        double minLon = 0;
        double maxLat = 0;
        double maxLon = 0;
        bool valid = false;
    };

    // GeoBounds lastShapeBounds;
    QRectF lastShapeBounds;

    ////////////////////////////////////
    void executeContext();
    void handleExecuteResult(int r);
    void lineCallback(asIScriptContext* c);

    QStringList   m_engineMessages;
    bool          m_timedOut = false;
    int           m_maxSliceMs = 10000;
    int           m_maxTotalMs = 0;
    QElapsedTimer m_sliceTimer;
    QElapsedTimer m_totalTimer;

signals:
    void requestSidebarView(const QString &viewName);
    void requestDisplayTab(const QString &tabName);
    void requestSelectEntity(const QString &entityId);
    void requestSensorScreenshot(const QString &filePath);
    void scriptEntityAboutToBeRemoved(const QString &entityId);


    // status: DONE | ERROR | ABORTED
    void scriptFinished(bool ok, const QString& status, const QString& message);  /////////////////////////////

};

#endif // SCRIPTENGINE_H
