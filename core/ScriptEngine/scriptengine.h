
// #ifndef SCRIPTENGINE_H
// #define SCRIPTENGINE_H

// #include "GUI/Hierarchytree/hierarchytree.h"
// #include "core/Hierarchy/hierarchy.h"
// #include "core/Render/scenerenderer.h"
// #include <QObject>
// #include <angelscript.h>
// #include <angelscript/add_on/scriptarray/scriptarray.h>
// #include "GUI/Tacticaldisplay/canvaswidget.h"
// #include "GUI/Tacticaldisplay/Gis/gislib.h"

// class MyObj {
// public:
//     float x, y;
//     MyObj() : x(0), y(0) {};
//     void moveBy(float dx, float dy)
//     {
//         this->x += dx;
//         this->y += dy;
//     }

// };

// class ScriptEngine : public QObject
// {
//     Q_OBJECT

// public:
//     ScriptEngine();
//     ~ScriptEngine();

//     void setHierarchy(Hierarchy *hier, HierarchyTree *tr, SceneRenderer *rend);
//     bool loadAndCompileScript(QString scriptContent);
//     void run();

// public slots:
//     void ScriptSleep(int milliseconds);
//     ProfileCategaory* addProfiles(const std::string &name);
//     Platform *addEntity(const std::string &Id, const std::string &name, bool &profile);
//     Folder* addFolder(const std::string &Id, const std::string &name, bool &profile);
//     void addComponent(const std::string &Id,const std::string &name);
//     // Add Parameter to an existing entity by ID
//     // void addParameterToEntity(const std::string& entityId,
//     //                           const std::string& paramName,
//     //                           int paramType,
//     //                           const std::string& paramValue);
//     // === Renaming functions ===
//     void renameEntity(const std::string &id, const std::string &newName);
//     // Rename a profile by ID
//     void renameProfile(const std::string& profileID, const std::string& newName);
//     // Wrapper for AngelScript
//     void renameFolder(const std::string& folderID, const std::string& newName);
//     // ------------------ Remove Profile Wrapper ------------------
//     // Usage in AS: removeProfile("profileId123");
//     void removeProfile(const std::string& profileID);
//     // Remove Folder wrapper for AngelScript
//     void removeFolder(const std::string& parentId, const std::string& folderID);
//     // Remove Entity wrapper for AngelScript
//     void removeEntity(const std::string& parentId, const std::string& entityID);
//     // === Finder functions ===
//     Entity* getEntityById(const std::string &id);
//     CScriptArray* findEntitiesByType(int type);
//     CScriptArray* getAllEntities();
//     CScriptArray* getAllEntityIds();

//     void renderscene();

// public:
//     Hierarchy *hierarchy;
//     HierarchyTree *tree;
//     SceneRenderer *render;
//     MyObj* e = nullptr;
// public:
//     void setCanvas(CanvasWidget* c) { canvas = c; }
//     // === Canvas Shape Wrappers (for AngelScript) ===
//     Q_INVOKABLE void setCanvasSelectedShape(const std::string &shapeName);

//     Q_INVOKABLE void canvasAddLine(const std::string &name, CScriptArray* points);
//     Q_INVOKABLE void canvasAddPolygon(const std::string &name, CScriptArray* points);
//     Q_INVOKABLE void canvasAddRectangle(const std::string &name, float x, float y, float width, float height);
//     Q_INVOKABLE void canvasAddCircle(const std::string &name, float x, float y, float radius);
//     Q_INVOKABLE void canvasAddPoint(const std::string &name, float x, float y);
//     // Built-in bitmap selection
//     Q_INVOKABLE void onBitmapSelected(const std::string &bitmapType);

//     // Get path of built-in bitmap
//     Q_INVOKABLE std::string getBitmapImagePath(const std::string &bitmapType);

//     // Place bitmap image from arbitrary file
//     Q_INVOKABLE void onBitmapImageSelected(const std::string &filePath);
//     // === GeoJSON Layer Wrappers ===
//     Q_INVOKABLE void canvasImportGeoJsonLayer(const std::string &filePath);
//     Q_INVOKABLE void canvasToggleGeoJsonLayer(const std::string &layerName, bool visible);
//     // ScriptEngine.h
//     void canvasStartDistanceMeasurement();
//     void canvasAddMeasurePoint(double lon, double lat);
//     double canvasGetLastSegmentDistance();
//     double canvasGetTotalDistance();
//     void canvasSetMeasurementUnit(const std::string &unit);
//     void canvasToggleAirbases();

// private:
//     asIScriptEngine* engine = nullptr;
//     asIScriptModule* mod = nullptr;
//     asIScriptFunction* func = nullptr;
//     asIScriptContext* ctx = nullptr;
//     asITypeInfo* arrayEntityType = nullptr; // ⚡ store type info once
//     CanvasWidget* canvas = nullptr;              // <--- NEW
// };

// #endif // SCRIPTENGINE_H






#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "GUI/Hierarchytree/hierarchytree.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Render/scenerenderer.h"
#include <QObject>
#include <angelscript.h>
#include <angelscript/add_on/scriptarray/scriptarray.h>
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
class Platform;
class ProfileCategaory;
class Runtime;
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

public slots:
    void ScriptSleep(int milliseconds);
    ProfileCategaory* addProfiles(const std::string &name);
    Platform *addEntity(const std::string &Id, const std::string &name, bool &profile);
    Folder* addFolder(const std::string &Id, const std::string &name, bool &profile);
    void addComponent(const std::string &Id,const std::string &name);
    // Add Parameter to an existing entity by ID
    // void addParameterToEntity(const std::string& entityId,
    //                           const std::string& paramName,
    //                           int paramType,
    //                           const std::string& paramValue);
    // === Renaming functions ===
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

public:
    Hierarchy *hierarchy;
    HierarchyTree *tree;
    SceneRenderer *render;
    MyObj* e = nullptr;
public:
    void setCanvas(CanvasWidget* c) { canvas = c; }
    // === Canvas Shape Wrappers (for AngelScript) ===
    Q_INVOKABLE void setCanvasSelectedShape(const std::string &shapeName);

    Q_INVOKABLE void canvasAddLine(const std::string &name, CScriptArray* points);
    Q_INVOKABLE void canvasAddPolygon(const std::string &name, CScriptArray* points);
    Q_INVOKABLE void canvasAddRectangle(const std::string &name, float x, float y, float width, float height);
    Q_INVOKABLE void canvasAddCircle(const std::string &name, float x, float y, float radius);
    Q_INVOKABLE void canvasAddPoint(const std::string &name, float x, float y);
    // Built-in bitmap selection
    Q_INVOKABLE void onBitmapSelected(const std::string &bitmapType);

    // Get path of built-in bitmap
    Q_INVOKABLE std::string getBitmapImagePath(const std::string &bitmapType);

    // Place bitmap image from arbitrary file
    Q_INVOKABLE void onBitmapImageSelected(const std::string &filePath);
    // === GeoJSON Layer Wrappers ===
    Q_INVOKABLE void canvasImportGeoJsonLayer(const std::string &filePath);
    Q_INVOKABLE void canvasToggleGeoJsonLayer(const std::string &layerName, bool visible);
    // ScriptEngine.h
    void canvasStartDistanceMeasurement();
    void canvasAddMeasurePoint(double lon, double lat);
    double canvasGetLastSegmentDistance();
    double canvasGetTotalDistance();
    void canvasSetMeasurementUnit(const std::string &unit);
    void canvasToggleAirbases();
    // Fetch existing ProfileCategaory by name
    ProfileCategaory* getProfileByName(const std::string& name);

    // Returns the newly created Platform entity under an existing profile
    Platform* addEntityToPlatform(ProfileCategaory* platformProfile, const std::string& entityName);

    // Attach a Sensor to an Entity by ID
    void attachSensorToEntity(const std::string& entityId, const std::string& sensorName);

private:
    asIScriptEngine* engine = nullptr;
    asIScriptModule* mod = nullptr;
    asIScriptFunction* func = nullptr;
    asIScriptContext* ctx = nullptr;
    asITypeInfo* arrayEntityType = nullptr; // ⚡ store type info once
    CanvasWidget* canvas = nullptr;              // <--- NEW
    Runtime* runtime = nullptr;
};

#endif // SCRIPTENGINE_H
