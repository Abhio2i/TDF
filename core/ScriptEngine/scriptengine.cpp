#include "scriptengine.h"
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
#include <cmath>
#include <cstdlib>
#include <angelscript/add_on/scriptarray/scriptarray.h>  // make sure included
#include "core/GlobalRegistry.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include"core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/profilecategaory.h"
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

// Random number generator
static float Math_Random()
{
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

static QString geoToString(double lat, double lon)
{
    return QString("Lat:%1 Lon:%2")
    .arg(lat, 0, 'f', 6)
        .arg(lon, 0, 'f', 6);
}

struct CityInfo {
    QString name;
    double lat;
    double lon;
};

static const CityInfo CITY_DB[] = {
    {"Bhopal",    23.278665, 77.369882},
    {"Delhi",     28.613939, 77.209023},
    {"Hyderabad", 17.385044, 78.486671},
    {"Mumbai",    19.076090, 72.877426},
    {"Bangalore", 12.9629, 77.5775},
    {"Chennai", 13.0843, 80.2705},

    };

static void updateBounds(QRectF& r, double lat, double lon)
{
    if (r.isNull())
        r = QRectF(QPointF(lon, lat), QSizeF(0, 0));
    else
        r = r.united(QRectF(QPointF(lon, lat), QSizeF(0, 0)));
}


// Trigonometric functions
static float Math_Sin(float angle) { return sin(angle); }
static float Math_Cos(float angle) { return cos(angle); }
static float Math_Tan(float angle) { return tan(angle); }
static float Math_ASin(float val) { return asin(val); }
static float Math_ACos(float val) { return acos(val); }
static float Math_ATan(float val) { return atan(val); }
static float Math_ATan2(float y, float x) { return atan2(y, x); }

// Exponential and power functions
static float Math_Pow(float base, float exp) { return pow(base, exp); }
static float Math_Sqrt(float val) { return sqrt(val); }
static float Math_Exp(float val) { return exp(val); }
static float Math_Log(float val) { return log(val); }
static float Math_Log10(float val) { return log10(val); }

// Rounding and absolute value functions
static float Math_Abs(float val) { return abs(val); }
static float Math_Floor(float val) { return floor(val); }
static float Math_Ceil(float val) { return ceil(val); }
static float Math_Round(float val) { return round(val); }

// Constant values
static const float Math_PI = 3.14159265359f;

void MessageCallback(const asSMessageInfo *msg, void *param)
{
    const char *type = "ERR";
    if (msg->type == asMSGTYPE_WARNING){
        type = "WARN";
        qWarning() << msg->section << "(" << msg->row << "," << msg->col << "):" << type << ":" << msg->message;
    }else if (msg->type == asMSGTYPE_INFORMATION){
        type = "INFO";
        qDebug() << msg->section << "(" << msg->row << "," << msg->col << "):" << type << ":" << msg->message;
    }else if (msg->type == asMSGTYPE_ERROR){
        type = "ERROR";
        qDebug() << msg->section << "(" << msg->row << "," << msg->col << "):" << type << ":" << msg->message;
    }

}
class FileIO
{
public:
    static std::string readText(const std::string &path)
    {
        qDebug() << "[FileIO] readText() called with path:" << QString::fromStdString(path);

        std::ifstream file(path);
        if (!file.is_open()) {
            qDebug() << "[FileIO] Could not open file for reading:" << QString::fromStdString(path);
            return "";
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

        qDebug() << "[FileIO] Successfully read content:" << QString::fromStdString(content);

        return content;
    }


    static void writeText(const std::string &path, const std::string &content)
    {
        std::ofstream file(path);
        if (!file.is_open()) {
            qDebug() << "[FileIO] Could not open file for writing:" << QString::fromStdString(path);
            return;
        }
        file << content;
    }

    static void appendText(const std::string &path, const std::string &content)
    {
        std::ofstream file(path, std::ios::app);
        if (!file.is_open()) {
            qDebug() << "[FileIO] Could not open file for appending:" << QString::fromStdString(path);
            return;
        }
        file << content;
    }
};


// --- Simulation Access Helper ---
static Simulation* GetSimulation(ScriptEngine* engine)
{
    if (!engine) return nullptr;

    Runtime* runtime = engine->getRuntime();  // ✅ use getter
    if (!runtime) return nullptr;

    return runtime->simulation;  // direct access is fine
}
// --- Wrapper Functions for AngelScript ---
void AS_SimStart(ScriptEngine* engine)
{
    if (!engine) return;

    Runtime* runtime = engine->getRuntime();
    if (!runtime) {
        qWarning() << "[SimStart] Runtime is NULL";
        return;
    }

    QMetaObject::invokeMethod(
        runtime,
        "handleStart",
        Qt::QueuedConnection
        );
}


void AS_SimPause(ScriptEngine* engine)
{
    if (!engine) return;

    Runtime* runtime = engine->getRuntime();
    if (!runtime) return;

    QMetaObject::invokeMethod(
        runtime,
        "handleStop",
        Qt::QueuedConnection
        );
}


void Print(const std::string &msg)
{
    qDebug() << "[AngelScript]:" << QString::fromStdString(msg);
}

void ScriptEngine::logEvent(
    ReportCategory category,
    const QString& action,
    const QString& name,
    const QString& location,
    const QString& input,
    const QString& output,
    const QString& status,
    const QString& reason,
    const QString& screenshotPath
    ) {
    executionReport.push_back({
        category,
        action,
        name,
        location,
        input,
        output,
        status,
        reason,
        screenshotPath
    });
}

// ================= GEO VERIFICATION (OFFLINE – INDIA REGION) =================

// Simple India bounding box (defence-safe offline check)
bool ScriptEngine::isLatLonInIndia(double lat, double lon) const
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
QString ScriptEngine::geoVerificationLine(double lat, double lon) const
{
    if (isLatLonInIndia(lat, lon)) {
        return "Geo Verification : VERIFIED (India Region)";
    }
    return "Geo Verification : FAILED (Outside India Region)";
}

void ScriptEngine::useCity(const std::string& cityName)
{
    QString name = QString::fromStdString(cityName);

    for (const auto& c : CITY_DB) {
        if (c.name.compare(name, Qt::CaseInsensitive) == 0) {

            activeCity = c.name;
            cityLat = c.lat;
            cityLon = c.lon;
            cityActive = true;

            logEvent(
                ReportCategory::SYSTEM,
                "City Selected",
                activeCity,
                geoToString(cityLat, cityLon),
                "",
                "City context set",
                "SUCCESS"
                );
            return;
        }
    }

    cityActive = false;

    logEvent(
        ReportCategory::SYSTEM,
        "City Selected",
        name,
        "",
        "",
        "FAILED",
        "Unknown city"
        );
}

QString ScriptEngine::cityLocationString() const
{
    if (!cityActive) return "City : NOT SET";

    return QString("City : %1 | Lat : %2 | Lon : %3")
        .arg(activeCity)
        .arg(cityLat, 0, 'f', 6)
        .arg(cityLon, 0, 'f', 6);
}


ProfileCategaory* ScriptEngine::addProfiles(const std::string &name)
{
    if (!hierarchy) {
        qDebug() << "[ERROR] Hierarchy not set in ScriptEngine::addProfiles!";
        return nullptr;
    }

    ProfileCategaory* profile = hierarchy->addProfileCategaory(QString::fromStdString(name));
    if (!profile) {
        qDebug() << "[ERROR] Failed to create ProfileCategaory!";
        return nullptr;
    }

    profile->setProfileType(Constants::EntityType::Platform);
    qDebug() << "[OK] Created Profile:" << QString::fromStdString(name);
    return profile;
}

Folder* ScriptEngine::addFolder(const std::string &Id, const std::string &name, bool &profile)
{
    if (!hierarchy) {
        qDebug() << "[ERROR] Hierarchy not set in ScriptEngine::addFolder!";
        return nullptr;
    }

    Folder* folder = hierarchy->addFolder(QString::fromStdString(Id),
                                          QString::fromStdString(name),
                                          profile);
    if (!folder) {
        qDebug() << "[ERROR] Failed to create Folder under ID:" << QString::fromStdString(Id);
        return nullptr;
    }

    qDebug() << "[OK] Created Folder:" << QString::fromStdString(name);
    return folder;
}


Platform* ScriptEngine::addEntity(const std::string &Id, const std::string &name, bool &profile)
{
    if (!hierarchy) {
        qDebug() << "[ERROR] Hierarchy not set in ScriptEngine::addEntity!";
        return nullptr;
    }

    Entity* rawEntity = hierarchy->addEntity(QString::fromStdString(Id),
                                             QString::fromStdString(name),
                                             profile);
    if (!rawEntity) {
        qDebug() << "[ERROR] Failed to create Entity under ID:" << QString::fromStdString(Id);
        return nullptr;
    }

    Platform* entity = static_cast<Platform*>(rawEntity);
    hierarchy->addComponent(QString::fromStdString(entity->ID), "dynamicModel");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "bitmap");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "trajectory");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "crossSection");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "iffs");
    //hierarchy->addComponent(QString::fromStdString(entity->ID), "mission");
    // hierarchy->addComponent(QString::fromStdString(entity->ID), "networkObject");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "radios");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "sensors");

    qDebug() << "[OK] Created Entity:" << QString::fromStdString(name);
    return entity;
}

void ScriptEngine::addComponent(const std::string &Id, const std::string &name)
{
    if (!hierarchy) {
        qDebug() << "[ERROR] Hierarchy not set in ScriptEngine::addComponent!";
        return;
    }

    if (Id.empty() || name.empty()) {
        qDebug() << "[ERROR] Invalid arguments in addComponent (Id or name empty)";
        return;
    }

    hierarchy->addComponent(QString::fromStdString(Id), QString::fromStdString(name));
    qDebug() << "[OK] Added Component:" << QString::fromStdString(name)
             << "to Entity ID:" << QString::fromStdString(Id);
}

// scriptengine.cpp
void ScriptEngine::ScriptSleep(int milliseconds)
{
    ctx->Suspend();
    QTimer::singleShot(milliseconds, [this]() {
        this->ctx->Execute();
    });
}

void ScriptEngine::renderscene(){
    if (render){
        emit render->Render(0.01);
    }
}

// Get a single entity by ID from AngelScript
Entity* ScriptEngine::getEntityById(const std::string &id)
{
    if (!hierarchy || !hierarchy->Entities) {
        qDebug() << "[ERROR] Hierarchy not initialized!";
        return nullptr;
    }

    // ✅ Master key access: use unordered_map directly
    auto it = hierarchy->Entities->find(id);
    if (it != hierarchy->Entities->end()) {
        Entity* ent = it->second;
        qDebug() << "[OK] getEntityById found Entity:" << QString::fromStdString(ent->Name)
                 << "for ID:" << QString::fromStdString(id);
        return ent;
    }

    qDebug() << "[WARN] getEntityById: No entity found for ID:" << QString::fromStdString(id);
    return nullptr;
}
#include <sstream>   // for std::ostringstream

// Return all entities filtered by type
CScriptArray* ScriptEngine::findEntitiesByType(int typeId) {
    if (!hierarchy || !hierarchy->Entities) return nullptr;

    // Create array<Entity@> type for AngelScript
    asITypeInfo* arrayEntityType = engine->GetTypeInfoByDecl("array<Entity@>");
    if (!arrayEntityType) return nullptr;

    CScriptArray* (*CreateArray)(asITypeInfo*, asUINT) = &CScriptArray::Create;
    CScriptArray* arr = CreateArray(arrayEntityType, 0);

    std::ostringstream names;
    size_t count = 0;

    for (auto& pair : *(hierarchy->Entities)) {
        Entity* ent = pair.second;
        if (ent && ent->type == typeId) {
            arr->InsertLast(&ent);
            if (count > 0) names << ", ";
            names << "\"" << ent->Name << "\"";   // ✅ Use correct field
            count++;
        }
    }

    // Map int → type string
    std::string typeName = "Unknown";
    switch (typeId) {
    case 0: typeName = "Platform"; break;
    case 1: typeName = "Radio"; break;
    case 2: typeName = "Sensor"; break;
    case 3: typeName = "SpecialZone"; break;
    case 4: typeName = "Weapon"; break;
    case 5: typeName = "IFF"; break;
    case 6: typeName = "Supply"; break;
    case 7: typeName = "FixedPoint"; break;
    case 8: typeName = "Formation"; break;
    }

    // Pretty print log
    if (count == 0) {
        qDebug() << "[EntityFinder] No entities of this type";
    } else if (count == 1) {
        qDebug().noquote() << QString("[EntityFinder] Found 1 entity named %1 of \"%2\" Type")
        .arg(QString::fromStdString(names.str()))
            .arg(QString::fromStdString(typeName));
    } else {
        qDebug().noquote() << QString("[EntityFinder] Found %1 entities named %2 of \"%3\" Type")
        .arg((int)count)
            .arg(QString::fromStdString(names.str()))
            .arg(QString::fromStdString(typeName));
    }

    return arr;
}


// Return all entity pointers
CScriptArray* ScriptEngine::getAllEntities() {
    if (!hierarchy || !hierarchy->Entities) return nullptr;

    asITypeInfo* arrayEntityType = engine->GetTypeInfoByDecl("array<Entity@>");
    if (!arrayEntityType) return nullptr;

    CScriptArray* (*CreateArray)(asITypeInfo*, asUINT) = &CScriptArray::Create;
    CScriptArray* arr = CreateArray(arrayEntityType, 0);

    for (auto& pair : *(hierarchy->Entities)) {
        Entity* ent = pair.second;
        arr->InsertLast(&ent);
    }

    qDebug() << "[OK] getAllEntities returned" << arr->GetSize() << "entities.";
    return arr;
}

// Return all IDs as strings
CScriptArray* ScriptEngine::getAllEntityIds() {
    if (!hierarchy || !hierarchy->Entities) return nullptr;

    asITypeInfo* arrayStringType = engine->GetTypeInfoByDecl("array<string>");
    if (!arrayStringType) return nullptr;

    CScriptArray* (*CreateArray)(asITypeInfo*, asUINT) = &CScriptArray::Create;
    CScriptArray* arr = CreateArray(arrayStringType, 0);

    for (auto& pair : *(hierarchy->Entities)) {
        std::string id = pair.first;
        arr->InsertLast(&id);
    }

    qDebug() << "[OK] getAllEntityIds returned" << arr->GetSize() << "IDs.";
    return arr;
}
// ------------------ Rename Entity Wrapper for AngelScript ------------------
// Usage in AS: renameEntity("12345", "F16_Falcon");
// This will rename the entity and update the UI automatically.
void ScriptEngine::renameEntity(const std::string& id, const std::string& newName)
{
    if (!hierarchy) {
        qDebug() << "[ERROR] Hierarchy not initialized!";
        return;
    }

    // ✅ Use existing master key function to get entity
    Entity* ent = getEntityById(id);
    if (!ent) {
        qDebug() << "[WARN] renameEntity: Entity not found for ID:" << QString::fromStdString(id);
        return;
    }

    // Rename entity
    QString oldName = QString::fromStdString(ent->Name);
    ent->Name = newName; // Update name

    // Reflect change on UI using Hierarchy signals
    if (hierarchy->Entities->find(id) != hierarchy->Entities->end()) {
        emit hierarchy->entityRenamed(QString::fromStdString(id), QString::fromStdString(newName));
    }

    qDebug() << "[OK] renameEntity: Entity renamed from"
             << oldName << "to" << QString::fromStdString(newName);
}
void ScriptEngine::renameProfile(const std::string& profileID, const std::string& newName)
{
    if (!hierarchy) {
        qDebug() << "[ERROR] Hierarchy not initialized!";
        return;
    }

    // Check if profile exists in the map
    auto it = hierarchy->ProfileCategories.find(profileID);
    if (it == hierarchy->ProfileCategories.end()) {
        qDebug() << "[WARN] renameProfile: No profile found for ID:"
                 << QString::fromStdString(profileID);
        return;
    }

    // Call Hierarchy rename function
    hierarchy->renameProfileCategaory(QString::fromStdString(profileID),
                                      QString::fromStdString(newName));

    qDebug() << "[OK] renameProfile: Profile"
             << QString::fromStdString(profileID)
             << "renamed to"
             << QString::fromStdString(newName);
}

void ScriptEngine::removeEntity(const std::string& parentId, const std::string& entityID)
{
    if (!hierarchy) {
        qDebug() << "[ERROR] Hierarchy not initialized!";
        return;
    }

    // Call Hierarchy's removeEntity using parentId (folder ID)
    hierarchy->removeEntity(QString::fromStdString(parentId),
                            QString::fromStdString(entityID),
                            false); // Profile flag false for entity removal

    qDebug() << "[OK] removeEntity: Entity"
             << QString::fromStdString(entityID)
             << "under parent"
             << QString::fromStdString(parentId)
             << "removed successfully.";
}

// ScriptEngine.cpp
void ScriptEngine::removeProfile(const std::string& profileID)
{
    if (!hierarchy) {
        qDebug() << "[ERROR] Hierarchy not initialized!";
        return;
    }

    // Direct call to Hierarchy
    hierarchy->removeProfileCategaory(QString::fromStdString(profileID));

    qDebug() << "[OK] removeProfile: Profile"
             << QString::fromStdString(profileID)
             << "removed successfully.";
}
void ScriptEngine::removeFolder(const std::string& parentId, const std::string& folderID)
{
    if (!hierarchy) {
        qDebug() << "[ERROR] Hierarchy not initialized!";
        return;
    }

    // Call Hierarchy's removeFolder directly
    hierarchy->removeFolder(QString::fromStdString(parentId),
                            QString::fromStdString(folderID),
                            false); // Profile flag false for removal

    qDebug() << "[OK] removeFolder: Folder"
             << QString::fromStdString(folderID)
             << "under parent"
             << QString::fromStdString(parentId)
             << "removed successfully.";
}
void ScriptEngine::renameFolder(const std::string& folderID, const std::string& newName)
{
    Q_ASSERT(hierarchy && "Hierarchy must be initialized before renaming a folder");

    hierarchy->renameFolder(QString::fromStdString(folderID), QString::fromStdString(newName));

    qDebug() << "[OK] renameFolder: Folder"
             << QString::fromStdString(folderID)
             << "renamed to:"
             << QString::fromStdString(newName);
}
// 1️⃣ Set selected shape type
void ScriptEngine::setCanvasSelectedShape(const std::string &shapeName)
{
    if (!canvas) {
        qWarning() << "Canvas not set! Cannot set selected shape.";
        return;
    }

    QString qShape = QString::fromStdString(shapeName);
    canvas->setShapeDrawingMode(true, qShape);
    qDebug() << "[ScriptEngine] Shape drawing mode enabled for:" << qShape;
}

// 2️⃣ Add a line
// void ScriptEngine::canvasAddLine(const std::string &name, CScriptArray* points)
// {
//     if (!canvas || !points || points->GetSize() < 2) {
//         logEvent(
//             ReportCategory::GIS,
//             "Line Creation",
//             QString::fromStdString(name),
//             "",
//             "Vertices<2",
//             "Not Rendered",
//             "FAILED",
//             "Line needs minimum 2 points"
//             );
//         return;
//     }

//     bool geoOk = true;

//     for (asUINT i = 0; i < points->GetSize(); i++) {
//         QString pt = QString::fromStdString(*(std::string*)points->At(i));
//         auto xy = pt.split(",");
//         QPointF geo(xy[0].toDouble(), xy[1].toDouble());
//         lastGeoCursor = geo;

//         if (!isLatLonInIndia(geo.y(), geo.x())) {
//             geoOk = false;
//         }

//         canvas->drawLine(geo, i == points->GetSize() - 1);
//     }

//     logEvent(
//         ReportCategory::GIS,
//         "Line Created",
//         QString::fromStdString(name),
//         "Auto From Points",
//         QString("Points=%1").arg(points->GetSize()),
//         geoOk ? "Geo Verification : VERIFIED (India Region)"
//               : "Geo Verification : FAILED (Outside India Region)",
//         geoOk ? "SUCCESS" : "FAILED",
//         geoOk ? "" : "One or more vertices outside India"
//         );
// }

void ScriptEngine::canvasAddLine(const std::string &name, CScriptArray* points)
{
    if (!canvas || !points || points->GetSize() < 2) {
        logEvent(
            ReportCategory::GIS,
            "Line Creation",
            QString::fromStdString(name),
            "",
            "Vertices < 2",
            "Not Rendered",
            "FAILED",
            "Line needs minimum 2 points"
            );
        return;
    }

    bool geoOk = true;

    // 🔹 RESET bounds for this shape
    lastShapeBounds = QRectF();

    // 🔹 DRAW LINE (incremental)
    for (asUINT i = 0; i < points->GetSize(); i++) {
        QString pt = QString::fromStdString(*(std::string*)points->At(i));
        auto xy = pt.split(",");

        if (xy.size() != 2)
            continue;

        double lon = xy[0].toDouble();
        double lat = xy[1].toDouble();

        QPointF geo(lon, lat);
        lastGeoCursor = geo;

        // 🔹 Update bounds (FOR AUTO ZOOM)
        updateBounds(lastShapeBounds, lat, lon);

        if (!isLatLonInIndia(lat, lon))
            geoOk = false;

        // last point = finish line
        canvas->drawLine(geo, i == points->GetSize() - 1);
    }

    // 🔹 FORCE CANVAS + MAP UPDATE
    canvas->update();
    if (canvas->gislib)
        canvas->gislib->update();

    // 🔹 Let Qt finish paint + GIS tiles
    QCoreApplication::processEvents();
    QThread::msleep(180);
    QCoreApplication::processEvents();

    // 🔹 SCREENSHOT AFTER COMPLETE DRAW
    QString screenshot = captureCanvasScreenshot("line");

    // 🔹 FINAL LOG
    logEvent(
        ReportCategory::GIS,
        "Line Created",
        QString::fromStdString(name),
        "Auto From Points",
        QString("Points = %1").arg(points->GetSize()),
        geoOk
            ? "Geo Verification : VERIFIED (India Region)"
            : "Geo Verification : FAILED (Outside India Region)",
        geoOk ? "SUCCESS" : "FAILED",
        geoOk ? "" : "One or more vertices outside India",
        screenshot
        );
}



// 3️⃣ Add a polygon
// void ScriptEngine::canvasAddPolygon(const std::string &name, CScriptArray* points)
// {
//     if (!canvas || !points || points->GetSize() < 3) {
//         logEvent(
//             ReportCategory::GIS,
//             "Polygon Creation",
//             QString::fromStdString(name),
//             "",
//             "Vertices<3",
//             "Not Rendered",
//             "FAILED",
//             "Polygon requires minimum 3 points"
//             );
//         return;
//     }

//     bool geoOk = true;

//     for (asUINT i = 0; i < points->GetSize(); i++) {
//         QString pt = QString::fromStdString(*(std::string*)points->At(i));
//         auto xy = pt.split(",");
//         QPointF geo(xy[0].toDouble(), xy[1].toDouble());
//         lastGeoCursor = geo;

//         if (!isLatLonInIndia(geo.y(), geo.x())) {
//             geoOk = false;
//         }

//         canvas->drawPolygon(geo, i == points->GetSize() - 1);
//     }

//     logEvent(
//         ReportCategory::GIS,
//         "Polygon Created",
//         QString::fromStdString(name),
//         "Auto From Vertices",
//         QString("Vertices=%1").arg(points->GetSize()),
//         geoOk ? "Geo Verification : VERIFIED (India Region)"
//               : "Geo Verification : FAILED (Outside India Region)",
//         geoOk ? "SUCCESS" : "FAILED",
//         geoOk ? "" : "Polygon has vertices outside India"
//         );
// }

void ScriptEngine::canvasAddPolygon(const std::string &name, CScriptArray* points)
{
    if (!canvas || !points || points->GetSize() < 3) {
        logEvent(
            ReportCategory::GIS,
            "Polygon Creation",
            QString::fromStdString(name),
            "",
            "Vertices < 3",
            "Not Rendered",
            "FAILED",
            "Polygon requires minimum 3 points"
            );
        return;
    }

    bool geoOk = true;

    // 🔹 RESET bounds (VERY IMPORTANT)
    lastShapeBounds = QRectF();

    // 🔹 DRAW POLYGON
    for (asUINT i = 0; i < points->GetSize(); i++) {
        QString pt = QString::fromStdString(*(std::string*)points->At(i));
        auto xy = pt.split(",");

        if (xy.size() != 2)
            continue;

        double lon = xy[0].toDouble();
        double lat = xy[1].toDouble();

        QPointF geo(lon, lat);
        lastGeoCursor = geo;

        // 🔹 Update bounds (same helper as line/circle/rectangle)
        updateBounds(lastShapeBounds, lat, lon);

        // 🔹 India geo check
        if (!isLatLonInIndia(lat, lon))
            geoOk = false;

        // 🔹 Draw polygon vertex
        canvas->drawPolygon(geo, i == points->GetSize() - 1);
    }

    // 🔹 FORCE redraw before screenshot
    canvas->update();
    if (canvas->gislib)
        canvas->gislib->update();

    QCoreApplication::processEvents();
    QThread::msleep(150);
    QCoreApplication::processEvents();

    // 🔹 SCREENSHOT (AUTO-ZOOM happens inside this)
    QString screenshot = captureCanvasScreenshot("polygon");

    // 🔹 LOG
    logEvent(
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



// 4️⃣ Add a rectangle
// void ScriptEngine::canvasAddRectangle(
//     const std::string &name,
//     float lon, float lat,
//     float width, float height
//     ) {
//     lastGeoCursor = QPointF(lon, lat);

//     bool geoOk = isLatLonInIndia(lat, lon);

//     if (!canvas || width <= 0 || height <= 0) {
//         logEvent(
//             ReportCategory::GIS,
//             "Rectangle Creation",
//             QString::fromStdString(name),
//             geoToString(lat, lon),
//             QString("W=%1 H=%2").arg(width).arg(height),
//             "Not Rendered",
//             "FAILED",
//             "Invalid dimensions"
//             );
//         return;
//     }

//     canvas->drawRectangle(lastGeoCursor);

//     logEvent(
//         ReportCategory::GIS,
//         "Rectangle Created",
//         QString::fromStdString(name),
//         geoToString(lat, lon),
//         QString("W=%1 H=%2").arg(width).arg(height),
//         geoVerificationLine(lat, lon),
//         geoOk ? "SUCCESS" : "FAILED",
//         geoOk ? "" : "Rectangle origin outside India"
//         );
// }
void ScriptEngine::canvasAddRectangle(
    const std::string &name,
    float width,
    float height
    )
{
    if (!cityActive || !canvas){
        logEvent(
            ReportCategory::GIS,
            "Rectangle Creation",
            QString::fromStdString(name),
            "",
            "",
            "FAILED",
            "City not selected"
            );
        return;
    }

    // Center of rectangle
    QPointF center(cityLon, cityLat);
    lastGeoCursor = center;

    // Geo validation
    bool geoOk = isLatLonInIndia(cityLat, cityLon);

    // Draw rectangle (center-based)
    canvas->drawRectangle(center);

    // Reset previous bounds
    lastShapeBounds = {};

    // Convert meters → degrees (approx, screenshot-safe)
    double halfWidthDeg  = (width  / 2.0) / 111000.0;
    double halfHeightDeg = (height / 2.0) / 111000.0;

    // Update bounding box
    updateBounds(lastShapeBounds, cityLat + halfHeightDeg, cityLon + halfWidthDeg);
    updateBounds(lastShapeBounds, cityLat - halfHeightDeg, cityLon - halfWidthDeg);

    // Capture screenshot AFTER drawing & bounds update
    QString screenshot = captureCanvasScreenshot("rectangle");

    // Log event
    logEvent(
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


// 5️⃣ Add a circle
// void ScriptEngine::canvasAddCircle(const std::string &name, float lon, float lat, float radius)
// {
//     lastGeoCursor = QPointF(lon, lat);

//     bool geoOk = isLatLonInIndia(lat, lon);

//     if (!canvas || radius <= 0) {
//         logEvent(
//             ReportCategory::GIS,
//             "Circle Creation",
//             QString::fromStdString(name),
//             geoToString(lat, lon),
//             "Radius=" + QString::number(radius),
//             "Not Rendered",
//             "FAILED",
//             "Invalid radius or canvas not available"
//             );
//         return;
//     }

//     canvas->drawCircle(lastGeoCursor);

//     logEvent(
//         ReportCategory::GIS,
//         "Circle Created",
//         QString::fromStdString(name),
//         geoToString(lat, lon),
//         "Radius=" + QString::number(radius),
//         geoVerificationLine(lat, lon),
//         geoOk ? "SUCCESS" : "FAILED",
//         geoOk ? "" : "Circle center outside India"
//         );
// }

void ScriptEngine::canvasAddCircle(const std::string &name, float radius)
{
    if (!cityActive || !canvas) {
        return;
    }

    //center
    QPointF center(cityLon, cityLat);
    lastGeoCursor = center;

    bool geoOk = isLatLonInIndia(cityLat, cityLon);

    canvas->drawCircle(center);

    // Reset previous bounds
    // lastShapeBounds = {};
    lastShapeBounds = QRectF();

    // Convert meters → degrees (approx, safe for screenshots)
    double radiusDeg = radius / 111000.0;

    updateBounds(lastShapeBounds, cityLat + radiusDeg, cityLon + radiusDeg);
    updateBounds(lastShapeBounds, cityLat - radiusDeg, cityLon - radiusDeg);

    // Screenshot AFTER draw
    QString screenshot = captureCanvasScreenshot("circle");

    logEvent(
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



// 6️⃣ Add a point
// void ScriptEngine::canvasAddPoint(const std::string &name, float lon, float lat)
// {
//     lastGeoCursor = QPointF(lon, lat);

//     if (!canvas) {
//         logEvent(
//             ReportCategory::GIS,
//             "Point Creation",
//             QString::fromStdString(name),
//             geoToString(lat, lon),
//             "",
//             "Not Rendered",
//             "FAILED",
//             "Canvas not available"
//             );
//         return;
//     }

//     bool geoOk = isLatLonInIndia(lat, lon);

//     canvas->drawPoints(lastGeoCursor);

//     logEvent(
//         ReportCategory::GIS,
//         "Point Created",
//         QString::fromStdString(name),
//         geoToString(lat, lon),
//         "",
//         geoVerificationLine(lat, lon),
//         geoOk ? "SUCCESS" : "FAILED",
//         geoOk ? "" : "Invalid coordinates (Outside India)"
//         );
// }
void ScriptEngine::expandPointBounds(QRectF &bounds, double lat, double lon)
{
    // 🔥 approx 15–20 km box (state / district level)
    double pad = 0.15; // degrees (~16 km)

    bounds = QRectF(
        QPointF(lon - pad, lat - pad),
        QPointF(lon + pad, lat + pad)
        );
}


void ScriptEngine::canvasAddPoint(const std::string &name, float lon, float lat)
{
    if (!canvas || !canvas->gislib) {
        logEvent(
            ReportCategory::GIS,
            "Point Creation",
            QString::fromStdString(name),
            geoToString(lat, lon),
            "",
            "Not Rendered",
            "FAILED",
            "Canvas or GIS not available"
            );
        return;
    }

    QPointF geo(lon, lat);
    lastGeoCursor = geo;

    bool geoOk = isLatLonInIndia(lat, lon);

    // 🔹 Draw point
    canvas->drawPoints(geo);

    // 🔹 RESET & EXPAND BOUNDS (🔥 KEY FIX)
    lastShapeBounds = QRectF();
    expandPointBounds(lastShapeBounds, lat, lon);

    // 🔹 FORCE AUTO ZOOM (state level)
    canvas->gislib->fitToBounds(
        lastShapeBounds.top(),
        lastShapeBounds.left(),
        lastShapeBounds.bottom(),
        lastShapeBounds.right(),
        -1   // slightly zoomed out
        );

    // 🔹 Force render
    canvas->update();
    canvas->gislib->update();
    QCoreApplication::processEvents();
    QThread::msleep(180);
    QCoreApplication::processEvents();

    // 🔹 SCREENSHOT (MAP + CANVAS)
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

    // 🔹 LOG
    logEvent(
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


// Built-in bitmap placement (dropdown selection)
void ScriptEngine::onBitmapSelected(const std::string &bitmapType)
{
    if (!canvas) return;
    canvas->onBitmapSelected(QString::fromStdString(bitmapType));
    qDebug() << "[ScriptEngine] Bitmap placement mode enabled for:" << QString::fromStdString(bitmapType);
}

// Get path of built-in bitmap
std::string ScriptEngine::getBitmapImagePath(const std::string &bitmapType)
{
    if (!canvas) return "";
    QString path = canvas->getBitmapImagePath(QString::fromStdString(bitmapType));
    return path.toStdString();
}

// Place bitmap image from arbitrary file
void ScriptEngine::onBitmapImageSelected(const std::string &filePath)
{
    if (!canvas) return;

    canvas->onBitmapImageSelected(QString::fromStdString(filePath));

    logEvent(
        ReportCategory::UI,
        "Bitmap Placed",
        QFileInfo(QString::fromStdString(filePath)).fileName(),
        geoToString(lastGeoCursor.y(), lastGeoCursor.x()),
        "User Selected Bitmap",
        "Bitmap Visible",
        "SUCCESS"
        );
}


void ScriptEngine::canvasStartLine()
{
    if (!canvas) return;

    canvas->scriptStartLine();

    // reset state
    linePoints.clear();
    lastShapeBounds = QRectF();
}



void ScriptEngine::canvasAddLinePoint(float lon, float lat)
{
    if (!canvas) return;

    QPointF geo(lon, lat);
    lastGeoCursor = geo;

    canvas->scriptAddLinePoint(geo);

    // collect point for final report
    linePoints.push_back(geo);

    updateBounds(lastShapeBounds, lat, lon);
}



// void ScriptEngine::canvasFinishLine()
// {
//     if (!canvas) return;

//     canvas->scriptFinishLine();

//     // ================= VALIDATION =================
//     if (linePoints.size() < 2) {
//         logEvent(
//             ReportCategory::GIS,
//             "Line Creation",
//             "Line Tool",
//             "",
//             "Points < 2",
//             "Line Not Created",
//             "FAILED",
//             "Minimum 2 points required"
//             );
//         return;
//     }

//     // ================= GEO VERIFICATION =================
//     bool geoOk = true;

//     for (const auto& p : linePoints) {
//         if (!isLatLonInIndia(p.y(), p.x())) {
//             geoOk = false;
//             break;
//         }
//     }

//     // ================= POINT SUMMARY =================
//     QString pointsStr;
//     for (int i = 0; i < linePoints.size(); ++i) {
//         const auto& p = linePoints[i];
//         pointsStr += QString("%1) Lat:%2 Lon:%3\n")
//                          .arg(i + 1)
//                          .arg(p.y(), 0, 'f', 6)
//                          .arg(p.x(), 0, 'f', 6);
//     }

//     // ================= FINAL LOG =================
//     logEvent(
//         ReportCategory::GIS,
//         "Line Created",
//         "Line Tool",
//         "Auto From Points",
//         pointsStr.trimmed(),
//         geoOk
//             ? QString("Geo Verification : VERIFIED (India Region)\nTotal Points : %1")
//                   .arg(linePoints.size())
//             : QString("Geo Verification : FAILED (Outside India Region)\nTotal Points : %1")
//                   .arg(linePoints.size()),
//         geoOk ? "SUCCESS" : "FAILED",
//         geoOk ? "" : "One or more line points outside India"
//         );
// }


void ScriptEngine::canvasFinishLine()
{
    if (!canvas || linePoints.size() < 2)
        return;

    canvas->scriptFinishLine();

    // lastShapeBounds = QRectF();
    bool geoOk = true;

    for (const auto& p : linePoints) {
        updateBounds(lastShapeBounds, p.y(), p.x());
        if (!isLatLonInIndia(p.y(), p.x()))
            geoOk = false;
    }


    QString screenshot = captureCanvasScreenshot("line");

    logEvent(
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

    linePoints.clear();
}




void ScriptEngine::canvasImportGeoJsonLayer(const std::string &filePath)
{
    if (!canvas) return;

    canvas->importGeoJsonLayer(QString::fromStdString(filePath));

    logEvent(
        ReportCategory::GIS,
        "GeoJSON Imported",
        QFileInfo(QString::fromStdString(filePath)).fileName(),
        "",
        "File Import",
        "Layer Loaded",
        "SUCCESS"
        );
}


void ScriptEngine::canvasToggleGeoJsonLayer(const std::string &layerName, bool visible)
{
    if (!canvas) return;

    canvas->onGeoJsonLayerToggled(QString::fromStdString(layerName), visible);

    logEvent(
        ReportCategory::GIS,
        "GeoJSON Layer Toggled",
        QString::fromStdString(layerName),
        "",
        visible ? "ON" : "OFF",
        visible ? "Visible" : "Hidden",
        "SUCCESS"
        );
}

// ScriptEngine.cpp
void ScriptEngine::canvasStartDistanceMeasurement()
{
    if (!canvas) return;

    canvas->startDistanceMeasurement();

    // ✅ reset measurement session
    measurementPoints.clear();

    lastShapeBounds = QRectF();
}



void ScriptEngine::canvasAddMeasurePoint(double lon, double lat)
{
    lastGeoCursor = QPointF(lon, lat);
    if (!canvas) return;

    canvas->addMeasurePoint(lon, lat);

    // ✅ collect points for final report
    measurementPoints.push_back(QPointF(lon, lat));

    updateBounds(lastShapeBounds, lat, lon);
}


double ScriptEngine::canvasGetLastSegmentDistance() {
    return canvas ? canvas->getLastSegmentDistance() : 0.0;
}

// double ScriptEngine::canvasGetTotalDistance()
// {
//     if (!canvas || measurementPoints.size() < 2) {
//         logEvent(
//             ReportCategory::MEASUREMENT,
//             "Distance Measurement",
//             "Distance Tool",
//             "",
//             "",
//             "FAILED",
//             "At least 2 points required"
//             );
//         return 0.0;
//     }

//     bool geoOk = true;

//     double dist = canvas->getTotalDistance(); // already in selected unit

//     // ---- Build points list ----
//     QString pointsStr;
//     for (int i = 0; i < measurementPoints.size(); ++i) {
//         auto p = measurementPoints[i];
//         pointsStr += QString("%1) Lat:%2 Lon:%3\n")
//                          .arg(i + 1)
//                          .arg(p.y(), 0, 'f', 6)
//                          .arg(p.x(), 0, 'f', 6);
//     }

//     // ---- Human readable unit ----
//     QString unitLabel;
//     if (measurementUnit == "meters")
//         unitLabel = "meters";
//     else if (measurementUnit == "kilometers")
//         unitLabel = "kilometers";
//     else if (measurementUnit == "feet")
//         unitLabel = "feet";
//     else if (measurementUnit == "miles")
//         unitLabel = "miles";
//     else if (measurementUnit == "degrees")
//         unitLabel = "degrees";
//     else
//         unitLabel = measurementUnit; // fallback

//     QString output =
//         QString("Total Distance : %1 %2")
//             .arg(dist, 0, 'f', 2)
//             .arg(unitLabel);

//     logEvent(
//         ReportCategory::MEASUREMENT,
//         "Distance Measurement",
//         "Distance Tool",
//         "Auto From Points",
//         pointsStr.trimmed(),
//         geoOk
//             ? output + "\nGeo Verification : VERIFIED (India Region)"
//             : output + "\nGeo Verification : FAILED (Outside India Region)",
//         geoOk ? "SUCCESS" : "FAILED",
//         geoOk ? "" : "One or more measurement points outside India"
//         );

//     return dist;
// }


double ScriptEngine::canvasGetTotalDistance()
{
    if (!canvas || measurementPoints.size() < 2) {
        logEvent(
            ReportCategory::MEASUREMENT,
            "Distance Measurement",
            "Distance Tool",
            "",
            "",
            "FAILED",
            "At least 2 points required"
            );
        return 0.0;
    }

    bool geoOk = true;

    for (const auto& p : measurementPoints) {
        if (!isLatLonInIndia(p.y(), p.x())) {
            geoOk = false;
            break;
        }
    }

    double dist = canvas->getTotalDistance();

    QString pointsStr;
    for (int i = 0; i < measurementPoints.size(); ++i) {
        auto p = measurementPoints[i];
        pointsStr += QString("%1) Lat:%2 Lon:%3\n")
                         .arg(i + 1)
                         .arg(p.y(), 0, 'f', 6)
                         .arg(p.x(), 0, 'f', 6);
    }

    QString unitLabel = measurementUnit;

    QString output =
        QString("Total Distance : %1 %2")
            .arg(dist, 0, 'f', 2)
            .arg(unitLabel);

    logEvent(
        ReportCategory::MEASUREMENT,
        "Distance Measurement",
        "Distance Tool",
        "Auto From Points",
        pointsStr.trimmed(),
        geoOk
            ? output + "\nGeo Verification : VERIFIED (India Region)"
            : output + "\nGeo Verification : FAILED (Outside India Region)",
        geoOk ? "SUCCESS" : "FAILED",
        geoOk ? "" : "One or more measurement points outside India"
        );

    return dist;
}

void ScriptEngine::canvasSetMeasurementUnit(const std::string &unit)
{
    if (!canvas) return;

    measurementUnit = QString::fromStdString(unit).toLower();
    canvas->setMeasurementUnit(measurementUnit);
}


void ScriptEngine::canvasToggleAirbases()
{
    if (!canvas) return;

    canvas->onPresetLayerSelected("Airbase");

    // Toggle state locally
    airbaseLayerVisible = !airbaseLayerVisible;

    logEvent(
        ReportCategory::UI,
        "Airbase Layer Toggled",
        "Airbase",
        "",
        "",
        airbaseLayerVisible ? "ON" : "OFF",
        "SUCCESS"
        );
}

void ScriptEngine::generatePDFReport(const std::string &filePath)
{
    if (filePath.empty()) {
        qWarning() << "[PDF] File path is empty!";
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(QString::fromStdString(filePath));
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(20, 20, 20, 20));

    QPainter painter;
    if (!painter.begin(&printer)) {
        qWarning() << "[PDF] Failed to start painter";
        return;
    }

    int x = 50;
    int y = 60;
    int pageWidth = printer.pageRect().width() - 100;

    auto newPageIfNeeded = [&]() {
        if (y > printer.pageRect().height() - 80) {
            printer.newPage();
            y = 60;
        }
    };

    auto drawLine = [&](const QString &text) {
        QFontMetrics fm(painter.font());

        QRect textRect = fm.boundingRect(
            QRect(x, y, pageWidth, 1000),   // large height
            Qt::TextWordWrap | Qt::AlignLeft,
            text
            );

        painter.drawText(
            QRect(x, y, pageWidth, textRect.height()),
            Qt::TextWordWrap | Qt::AlignLeft,
            text
            );

        y += textRect.height() + 8;

        newPageIfNeeded();
    };


    // ===== SPACING & DIVIDER HELPERS =====
    auto sectionGap = [&]() { y += 32; };

    auto smallGap = [&]() { y += 18; };

    auto divider = [&]() {
        drawLine("────────────────────────────────────────────");
        y += 6;
    };

    auto entryGap = [&]() { y += 28; };

    auto categoryToString = [](ReportCategory c) {
        switch (c) {
        case ReportCategory::GIS: return "GIS";
        case ReportCategory::UI: return "UI";
        case ReportCategory::MEASUREMENT: return "MEASUREMENT";
        case ReportCategory::MATH: return "MATH";
        case ReportCategory::SIMULATION: return "SIMULATION";
        case ReportCategory::SYSTEM: return "SYSTEM";
        }
        return "UNKNOWN";
    };

    auto drawImage = [&](const QString& imagePath) {
        if (imagePath.isEmpty()) return;

        QImage img(imagePath);
        if (img.isNull()) return;

        int maxWidth = pageWidth;
        int imgHeight = (img.height() * maxWidth) / img.width();

        painter.drawImage(QRect(x, y, maxWidth, imgHeight), img);
        y += imgHeight + 14;

        newPageIfNeeded();
    };


    // ================= TITLE =================
    painter.setFont(QFont("Arial", 18, QFont::Bold));
    drawLine("Auto Generated Report");
    divider();
    smallGap();

    painter.setFont(QFont("Arial", 10));
    drawLine("Generated By : ScriptEngine");
    drawLine("Report Mode  : AutoScript Driven");
    smallGap();


    // =================================================
    // 🔵 AUTOSCRIPT EXECUTION SUMMARY (PRIMARY)
    // =================================================
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    drawLine("AutoScript Execution Summary");
    divider();
    smallGap();

    // painter.setFont(QFont("Arial", 11));
    // drawLine("Total Script Actions : " + QString::number(executionReport.size()));
    // smallGap();

    // painter.setFont(QFont("Arial", 14, QFont::Bold));
    // drawLine("AutoScript Execution Log");
    // sectionGap();

    painter.setFont(QFont("Arial", 11));

    if (executionReport.empty()) {
        drawLine("No AutoScript operations were recorded.");
    } else {
        // for (const auto &e : executionReport) {

        //     entryGap();

        //     drawLine(QString("[%1] %2 | %3 | %4")
        //                  .arg(categoryToString(e.category))
        //                  .arg(e.action)
        //                  .arg(e.name)
        //                  .arg(e.status));

        //     if (!e.location.isEmpty())
        //         drawLine("   Location : " + e.location);

        //     if (!e.input.isEmpty())
        //         drawLine("   Input    : " + e.input);

        //     if (!e.output.isEmpty())
        //         drawLine("   Output   : " + e.output);

        //     if (!e.reason.isEmpty())
        //         drawLine("   Reason   : " + e.reason);

        //     entryGap();

        //     newPageIfNeeded();
        // }

        for (const auto &e : executionReport) {

            // Skip system-only logs
            if (e.category == ReportCategory::SYSTEM)
                continue;

            painter.setFont(QFont("Arial", 11, QFont::Bold));
            drawLine(QString("%1 CREATED")
                         .arg(e.action.toUpper()));

            painter.setFont(QFont("Arial", 11));

            drawLine("Name        : " + e.name);
            drawLine("Status      : " + e.status);

            if (!e.location.isEmpty())
                drawLine("Location    : " + e.location);

            if (!e.input.isEmpty())
                drawLine("Properties  : " + e.input);

            if (!e.output.isEmpty())
                drawLine(e.output);

            if (!e.reason.isEmpty())
                drawLine("Remarks     : " + e.reason);

            // ---- SCREENSHOT (NO PATH TEXT) ----
            if (!e.screenshotPath.isEmpty()) {
                smallGap();
                drawLine("Visual Evidence:");
                drawImage(e.screenshotPath);
            }

            // ONLY ONE divider AFTER complete shape
            divider();
            smallGap();

            newPageIfNeeded();
        }

    }

    // =================================================
    // 🟢 SIMULATION SUMMARY (SECONDARY, IF EXISTS)
    // =================================================
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    drawLine("Simulation Summary");

    painter.setFont(QFont("Arial", 11));

    int entityCount = (hierarchy && hierarchy->Entities)
                          ? hierarchy->Entities->size()
                          : 0;

    drawLine("Total Simulation Entities: " + QString::number(entityCount));
    y += 8;

    if (entityCount == 0) {
        drawLine("No simulation entities were created by this AutoScript.");
    } else {
        painter.setFont(QFont("Arial", 14, QFont::Bold));
        drawLine("Simulation Entity Details");

        painter.setFont(QFont("Arial", 11));
        for (auto &pair : *(hierarchy->Entities)) {
            Entity *ent = pair.second;
            if (!ent) continue;

            drawLine("Name: " + QString::fromStdString(ent->Name) +
                     " | Active: " + (ent->Active ? "Yes" : "No"));

            newPageIfNeeded();
        }
    }

    // ================= FOOTER =================
    printer.newPage();
    painter.setFont(QFont("Arial", 10, QFont::Normal, true));
    drawLine("End of Auto Generated Report");

    painter.end();

    qDebug() << "[PDF] AutoScript-based report generated at:"
             << QString::fromStdString(filePath);
}

// canvas capture screenshot
QString ScriptEngine::captureCanvasScreenshot(const QString& tag)
{
    if (!canvas || !canvas->gislib)
        return "";

    // 🔹 AUTO-ZOOM BEFORE SCREENSHOT
    canvas->gislib->fitToBounds(
        lastShapeBounds.top(),
        lastShapeBounds.left(),
        lastShapeBounds.bottom(),
        lastShapeBounds.right(),
        -2    // reduce acording to maximum zoom
        );

    // Ensure map tiles & canvas render ho chuke ho
    QCoreApplication::processEvents();
    QThread::msleep(150);
    QCoreApplication::processEvents();

    QWidget* container = canvas->parentWidget();
    if (!container) return "";

    // ✅ GISlib grab = map + all child widgets (canvas included)
    // QPixmap pixmap = canvas->grab();   // Qt canvas snapshot
    QPixmap pixmap = container->grab(); // ✅ MAP + CANVAS

    QString dir = QDir::tempPath() + "/gis_reports";
    QDir().mkpath(dir);

    QString filePath =
        dir + "/" +
        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz") +
        "_" + tag + ".png";

    pixmap.save(filePath, "PNG");

    return filePath;
}




// 1️⃣ Fetch existing profile by name (GUI-safe)
ProfileCategaory* ScriptEngine::getProfileByName(const std::string& name) {
    if (!hierarchy) return nullptr;

    for (auto& [id, profile] : hierarchy->ProfileCategories) {
        if (profile && profile->Name == name)
            return profile;
    }
    return nullptr;
}

// 2️⃣ Add entity to existing Platform profile (GUI-safe)
// Add a new entity under an existing Platform profile
Platform* ScriptEngine::addEntityToPlatform(ProfileCategaory* platformProfile, const std::string& entityName) {
    if (!platformProfile) return nullptr;
    if (!hierarchy) {
        qDebug() << "[ERROR] Hierarchy not set in ScriptEngine::addEntityToPlatform!";
        return nullptr;
    }

    // Create the entity using the hierarchy, linking it to the Platform profile
    bool isPlatformProfile = true; // flag for hierarchy
    Entity* rawEntity = hierarchy->addEntity(
        QString::fromStdString(platformProfile->ID),   // parent ID
        QString::fromStdString(entityName),            // entity name
        isPlatformProfile
        );

    if (!rawEntity) {
        qDebug() << "[ERROR] Failed to create Entity under Profile ID:" << QString::fromStdString(platformProfile->ID);
        return nullptr;
    }

    // Cast to Platform if needed
    Platform* entity = static_cast<Platform*>(rawEntity);

    // Add default components
    hierarchy->addComponent(QString::fromStdString(entity->ID), "transform");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "rigidbody");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "collider");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "trajectory");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "dynamicModel");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "bitmap");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "sensors");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "crossSection");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "iffs");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "radios");

    qDebug() << "[OK] Created Entity:" << QString::fromStdString(entityName);

    return entity;
}

// 3️⃣ Attach sensor
void ScriptEngine::attachSensorToEntity(const std::string& entityId,
                                        const std::string& sensorName,
                                        const std::string& sensorType)
{
    if (!hierarchy) {
        qWarning() << "❌ [ScriptEngine] Hierarchy not set. Cannot attach sensor.";
        return;
    }

    qDebug() << "⚙️ [ScriptEngine] Attaching sensor:"
             << QString::fromStdString(sensorName)
             << "of type" << QString::fromStdString(sensorType)
             << "to entity:" << QString::fromStdString(entityId);

    hierarchy->attachSensors(QString::fromStdString(entityId),
                             QString::fromStdString(sensorName),
                             QString::fromStdString(sensorType));
}

void ScriptEngine::attachIFFToEntity(const std::string& entityID, const std::string& iffName) {
    if (!hierarchy) return;
    hierarchy->attchedIff(QString::fromStdString(entityID), QString::fromStdString(iffName));
}
void ScriptEngine::attachRadioToEntity(const std::string& entityID, const std::string& radioName)
{
    if (!hierarchy) {
        qDebug() << "[ERROR] Hierarchy not set in ScriptEngine::attachRadioToEntity!";
        return;
    }

    hierarchy->attachRadios(QString::fromStdString(entityID), QString::fromStdString(radioName));
}

ScriptEngine::ScriptEngine()
{
    // AngelScript setup
    engine = asCreateScriptEngine();
    engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);

    RegisterStdString(engine);
    engine->RegisterGlobalFunction("float Random()", asFUNCTION(Math_Random), asCALL_CDECL);

    // Trigonometric functions
    engine->RegisterGlobalFunction("float Sin(float)", asFUNCTION(Math_Sin), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Cos(float)", asFUNCTION(Math_Cos), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Tan(float)", asFUNCTION(Math_Tan), asCALL_CDECL);
    engine->RegisterGlobalFunction("float ASin(float)", asFUNCTION(Math_ASin), asCALL_CDECL);
    engine->RegisterGlobalFunction("float ACos(float)", asFUNCTION(Math_ACos), asCALL_CDECL);
    engine->RegisterGlobalFunction("float ATan(float)", asFUNCTION(Math_ATan), asCALL_CDECL);
    engine->RegisterGlobalFunction("float ATan2(float, float)", asFUNCTION(Math_ATan2), asCALL_CDECL);

    // Exponential and power functions
    engine->RegisterGlobalFunction("float Pow(float, float)", asFUNCTION(Math_Pow), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Sqrt(float)", asFUNCTION(Math_Sqrt), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Exp(float)", asFUNCTION(Math_Exp), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Log(float)", asFUNCTION(Math_Log), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Log10(float)", asFUNCTION(Math_Log10), asCALL_CDECL);

    // Rounding and absolute value functions
    engine->RegisterGlobalFunction("float Abs(float)", asFUNCTION(Math_Abs), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Floor(float)", asFUNCTION(Math_Floor), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Ceil(float)", asFUNCTION(Math_Ceil), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Round(float)", asFUNCTION(Math_Round), asCALL_CDECL);
    // Constant
    engine->RegisterGlobalProperty("const float PI", (void*)&Math_PI);

    // Register print + profile functions
    engine->RegisterGlobalFunction("void Print(const string &in)", asFUNCTION(Print), asCALL_CDECL);
    // engine->RegisterGlobalFunction("void sleep(int milliseconds)", asFUNCTION(ScriptSleep), asCALL_CDECL);
    int r;
    r = engine->RegisterGlobalFunction(
        "string ReadFile(const string &in)",
        asFUNCTION(FileIO::readText),
        asCALL_CDECL
        ); Q_ASSERT(r >= 0);

    r = engine->RegisterGlobalFunction(
        "void WriteFile(const string &in, const string &in)",
        asFUNCTION(FileIO::writeText),
        asCALL_CDECL
        ); Q_ASSERT(r >= 0);

    r = engine->RegisterGlobalFunction(
        "void AppendFile(const string &in, const string &in)",
        asFUNCTION(FileIO::appendText),
        asCALL_CDECL
        ); Q_ASSERT(r >= 0);

    int s;
    s = engine->RegisterObjectType("MyObj", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("MyObj", "float x", asOFFSET(MyObj, x)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("MyObj", "float y", asOFFSET(MyObj, y)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("MyObj", "void moveBy(float dx, float dy)", asMETHOD(MyObj, moveBy), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("QVector3D", sizeof(QVector3D), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CDA); Q_ASSERT(s >= 0);
    // 3. Register the property accessors (getters and setters)
    // Note: RegisterObjectMethod is used for both. The const keyword is important!
    s = engine->RegisterObjectMethod("QVector3D", "float x() const", asMETHOD(QVector3D, x), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("QVector3D", "void setX(float)", asMETHOD(QVector3D, setX), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("QVector3D", "float y() const", asMETHOD(QVector3D, y), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("QVector3D", "void setY(float)", asMETHOD(QVector3D, setY), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("QVector3D", "float z() const", asMETHOD(QVector3D, z), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("QVector3D", "void setZ(float)", asMETHOD(QVector3D, setZ), asCALL_THISCALL); Q_ASSERT(s >= 0);


    s = engine->RegisterObjectType("QQuaternion", sizeof(QQuaternion), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CDA); Q_ASSERT(s >= 0);
    // Register methods for QQuaternion (you can add more as needed)
    s = engine->RegisterObjectMethod("QQuaternion", "QVector3D toEulerAngles()", asMETHOD(QQuaternion, toEulerAngles), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("QQuaternion", "void setX(float)", asMETHOD(QQuaternion, setX), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("QQuaternion", "void setY(float)", asMETHOD(QQuaternion, setY), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("QQuaternion", "void setZ(float)", asMETHOD(QQuaternion, setZ), asCALL_THISCALL); Q_ASSERT(s >= 0);
    //s = engine->RegisterObjectMethod("QQuaternion", "QQuaternion@ fromEulerAngles(const QVector3D& in)", asFUNCTIONPR(QQuaternion::fromEulerAngles, (const QVector3D&), QQuaternion), asCALL_CDECL); Q_ASSERT(s >= 0);


    s = engine->RegisterObjectType("QTransform", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("QTransform", "QVector3D translation()", asMETHOD(Qt3DCore::QTransform, translation), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("QTransform", "void setTranslation(const QVector3D &in)", asMETHOD(Qt3DCore::QTransform, setTranslation), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("QTransform", "QQuaternion rotation()", asMETHOD(Qt3DCore::QTransform, rotation), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("QTransform", "void setRotation(const QQuaternion &in)", asMETHOD(Qt3DCore::QTransform, setRotation), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("QTransform", "QVector3D scale3D()", asMETHOD(Qt3DCore::QTransform, scale3D), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("QTransform", "void setScale3D(const QVector3D &in)", asMETHOD(Qt3DCore::QTransform, setScale3D), asCALL_THISCALL); Q_ASSERT(s >= 0);

    // 2. Register the Transform class as a reference-counted type
    s = engine->RegisterObjectType("Transform", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Transform", "QTransform@ matrix", asOFFSET(Transform, matrix)); Q_ASSERT(s >= 0);
    // // Register the getter and setter methods
    s = engine->RegisterObjectMethod("Transform", "QVector3D translation()", asMETHOD(Transform, translation), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Transform", "void setTranslation(const QVector3D& in)", asMETHOD(Transform, setTranslation), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Transform", "void addTranslation(const QVector3D& in)", asMETHOD(Transform, addTranslation), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("Transform", "QQuaternion rotation() const", asMETHOD(Transform, rotation), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Transform", "void setRotation(const QQuaternion& in)", asMETHOD(Transform, setRotation), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("Transform", "QVector3D scale3D() const", asMETHOD(Transform, scale3D), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Transform", "void setScale3D(const QVector3D& in)", asMETHOD(Transform, setScale3D), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("Transform", "QVector3D toEulerAngles() const", asMETHOD(Transform, toEulerAngles), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Transform", "void setFromEulerAngles(const QVector3D& in)", asMETHOD(Transform, setFromEulerAngles), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("Transform", "QVector3D forward()", asMETHOD(Transform, forward), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Transform", "QVector3D up()", asMETHOD(Transform, up), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Transform", "QVector3D right()", asMETHOD(Transform, right), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Transform", "QVector3D back()", asMETHOD(Transform, back), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Transform", "QVector3D left()", asMETHOD(Transform, left), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Transform", "QVector3D down()", asMETHOD(Transform, down), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("Transform", "QVector3D TransformDirection(const QVector3D& in)", asMETHOD(Transform, TransformDirection), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Transform", "QVector3D inverseTransformDirection(const QVector3D& in)", asMETHOD(Transform, inverseTransformDirection), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("Trajectory", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Trajectory", "void addWaypoint(float,float,float)", asMETHOD(Trajectory, addWaypoint), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("DynamicModel", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("DynamicModel", "float moveSpeed", asOFFSET(DynamicModel, moveSpeed)); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("ProfileCategaory", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("ProfileCategaory", "string id", asOFFSET(ProfileCategaory, ID)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("ProfileCategaory", "string name", asOFFSET(ProfileCategaory, Name)); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("Folder", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Folder", "string id", asOFFSET(Folder, ID)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Folder", "string name", asOFFSET(Folder, Name)); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("Platform", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Platform", "string id", asOFFSET(Entity, ID)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Platform", "string name", asOFFSET(Entity, Name)); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectProperty("Platform", "Transform@ transform", asOFFSET(Platform, transform)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Platform", "DynamicModel@ dynamicModel", asOFFSET(Platform, dynamicModel)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Platform", "Trajectory@ trajectory", asOFFSET(Platform, trajectory)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Platform", "void addParam(const string &in,const string &in)", asMETHOD(Platform, addParam), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Platform", "void editParam(const string &in,const string &in)", asMETHOD(Platform, editParam), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Platform", "string getParam(const string &in)", asMETHOD(Platform, getParam), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Platform", "void removeParam(const string &in)", asMETHOD(Platform, removeParam), asCALL_THISCALL); Q_ASSERT(s >= 0);


    s = engine->RegisterObjectType("ScriptEngine", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine", "void render()", asMETHOD(ScriptEngine, renderscene), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine", "void sleep(int milliseconds)", asMETHOD(ScriptEngine, ScriptSleep), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine", "ProfileCategaory@ NewProfile(const string &in)", asMETHOD(ScriptEngine, addProfiles), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine", "Platform@ addEntity(const string &in,const string &in, const bool &in)", asMETHOD(ScriptEngine, addEntity), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine", "Folder@ addFolder(const string &in,const string &in, const bool &in)", asMETHOD(ScriptEngine, addFolder), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine", "void addComponent(const string &in,const string &in)", asMETHOD(ScriptEngine, addComponent), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("Entity", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Entity", "string id", asOFFSET(Entity, ID)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Entity", "string name", asOFFSET(Entity, Name)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Entity", "string parentId", asOFFSET(Entity, parentID)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Entity", "bool Active", asOFFSET(Entity, Active)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Entity", "void addComponent(const string &in)", asMETHOD(Entity, addComponent), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Entity", "void removeComponent(const string &in)", asMETHOD(Entity, removeComponent), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("Sensor", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Sensor", "string name", asOFFSET(Sensor, Name)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Sensor", "string parentID", asOFFSET(Sensor, parentID)); Q_ASSERT(s >= 0);
    // === 4️⃣ Register ScriptArray add-on AFTER all ref types ===
    RegisterScriptArray(engine, true); // true = array of references
    arrayEntityType = engine->GetTypeInfoByDecl("array<Entity@>");
    Q_ASSERT(arrayEntityType != nullptr);

    qDebug() << "[OK] Registered array<Entity@> type successfully";

    // ------------------ AngelScript Binding for renameEntity ------------------
    s = engine->RegisterObjectMethod("ScriptEngine","void renameEntity(const string &in, const string &in)",asMETHOD(ScriptEngine, renameEntity),asCALL_THISCALL);Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine", "Entity@ getEntityById(const string &in)",asMETHOD(ScriptEngine, getEntityById), asCALL_THISCALL);Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine","array<Entity@>@ findEntitiesByType(int)",asMETHOD(ScriptEngine, findEntitiesByType),asCALL_THISCALL);assert(s >= 0);Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine","array<string>@ getAllEntityIds()",asMETHOD(ScriptEngine, getAllEntityIds),asCALL_THISCALL);Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine","array<Entity@>@ getAllEntities()",asMETHOD(ScriptEngine, getAllEntities),asCALL_THISCALL);Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine","void removeEntity(const string &in)",asMETHOD(ScriptEngine, removeEntity),asCALL_THISCALL);Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine","void removeProfile(const string &in)",asMETHOD(ScriptEngine, removeProfile),asCALL_THISCALL);Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine","void removeFolder(const string &in, const string &in)",asMETHOD(ScriptEngine, removeFolder),asCALL_THISCALL);Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine","void removeEntity(const string &in, const string &in)",asMETHOD(ScriptEngine, removeEntity),asCALL_THISCALL);Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine","void renameProfile(const string &in, const string &in)",asMETHOD(ScriptEngine, renameProfile),asCALL_THISCALL);Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine","void renameFolder(const string &in, const string &in)",asMETHOD(ScriptEngine, renameFolder),asCALL_THISCALL);Q_ASSERT(s >= 0);
    // === 5️⃣ CanvasWidget / Shape Drawing Bindings ===
    s = engine->RegisterObjectMethod("ScriptEngine",
                                     "void setCanvasSelectedShape(const string &in)",
                                     asMETHOD(ScriptEngine, setCanvasSelectedShape), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddLine(const string &in, array<string>@)",
                                     asMETHOD(ScriptEngine, canvasAddLine), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddPolygon(const string &in, array<string>@)",
                                     asMETHOD(ScriptEngine, canvasAddPolygon), asCALL_THISCALL); Q_ASSERT(s >= 0);

    // s = engine->RegisterObjectMethod("ScriptEngine",
    //                                  "void canvasAddRectangle(const string &in, float, float, float, float)",
    //                                  asMETHOD(ScriptEngine, canvasAddRectangle), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddRectangle(const string &in, float, float)",
                                     asMETHOD(ScriptEngine, canvasAddRectangle), asCALL_THISCALL); Q_ASSERT(s >= 0);

    // s = engine->RegisterObjectMethod("ScriptEngine",
    //                                  "void canvasAddCircle(const string &in, float, float, float)",
    //                                  asMETHOD(ScriptEngine, canvasAddCircle), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddCircle(const string &in, float)",
                                     asMETHOD(ScriptEngine, canvasAddCircle), asCALL_THISCALL); Q_ASSERT(s >= 0);


    s = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void useCity(const string &in)",
        asMETHOD(ScriptEngine, useCity),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    s = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddPoint(const string &in, float, float)",
                                     asMETHOD(ScriptEngine, canvasAddPoint), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasStartLine()", asMETHOD(ScriptEngine, canvasStartLine), asCALL_THISCALL);Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddLinePoint(float, float)", asMETHOD(ScriptEngine, canvasAddLinePoint), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasFinishLine()", asMETHOD(ScriptEngine, canvasFinishLine), asCALL_THISCALL); Q_ASSERT(s >= 0);


    qDebug() << "[OK] Canvas wrapper methods registered successfully";


    // ✅ Bind SimStart/SimpPause after ScriptEngine is registered
    s = engine->RegisterGlobalFunction(
        "void SimStart(ScriptEngine@)",
        asFUNCTION(AS_SimStart),
        asCALL_CDECL);
    Q_ASSERT(s >= 0);

    s = engine->RegisterGlobalFunction(
        "void SimPause(ScriptEngine@)",
        asFUNCTION(AS_SimPause),
        asCALL_CDECL);
    Q_ASSERT(s >= 0);

    // Built-in bitmap selection
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void onBitmapSelected(const string &in)",
        asMETHOD(ScriptEngine, onBitmapSelected),
        asCALL_THISCALL
        ); Q_ASSERT(r >= 0);

    // Get built-in bitmap path
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "string getBitmapImagePath(const string &in)",
        asMETHOD(ScriptEngine, getBitmapImagePath),
        asCALL_THISCALL
        ); Q_ASSERT(r >= 0);

    // User-selected bitmap from disk
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void onBitmapImageSelected(const string &in)",
        asMETHOD(ScriptEngine, onBitmapImageSelected),
        asCALL_THISCALL
        ); Q_ASSERT(r >= 0);
    qDebug() << "[OK] Bitmap wrapper methods registered successfully";

    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasImportGeoJsonLayer(const string &in)",
                                     asMETHOD(ScriptEngine, canvasImportGeoJsonLayer),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasToggleGeoJsonLayer(const string &in, bool)",
                                     asMETHOD(ScriptEngine, canvasToggleGeoJsonLayer),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasStartDistanceMeasurement()",
                                     asMETHOD(ScriptEngine, canvasStartDistanceMeasurement),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddMeasurePoint(double, double)",
                                     asMETHOD(ScriptEngine, canvasAddMeasurePoint),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "double canvasGetLastSegmentDistance()",
                                     asMETHOD(ScriptEngine, canvasGetLastSegmentDistance),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "double canvasGetTotalDistance()",
                                     asMETHOD(ScriptEngine, canvasGetTotalDistance),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasSetMeasurementUnit(const string &in)",
                                     asMETHOD(ScriptEngine, canvasSetMeasurementUnit),
                                     asCALL_THISCALL);
    Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void canvasToggleAirbases()",
        asMETHOD(ScriptEngine, canvasToggleAirbases),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    // Get profile by name
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "ProfileCategaory@ getProfileByName(const string &in)",
        asMETHOD(ScriptEngine, getProfileByName),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "Platform@ addEntityToPlatform(ProfileCategaory@, const string &in)",
        asMETHOD(ScriptEngine, addEntityToPlatform),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void generatePDFReport(const string &in)",
        asMETHOD(ScriptEngine, generatePDFReport),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    // Attach sensor to entity
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void attachSensorToEntity(const string &in, const string &in, const string &in)",
        asMETHOD(ScriptEngine, attachSensorToEntity),
        asCALL_THISCALL); Q_ASSERT(r >= 0);

    // Attach IFF to entity
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void attachIFFToEntity(const string &in, const string &in)",
        asMETHOD(ScriptEngine, attachIFFToEntity),
        asCALL_THISCALL);
    Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void attachRadioToEntity(const string &in, const string &in)",
        asMETHOD(ScriptEngine, attachRadioToEntity),
        asCALL_THISCALL);
    Q_ASSERT(r >= 0);

    // In ScriptEngine::bindToAngelScript()

    // --- Add Parameter to Existing Entity ---
    // s = engine->RegisterObjectMethod(
    //     "ScriptEngine",
    //     "void addParameterToEntity(const string &in, const string &in, int, const string &in)",
    //     asMETHOD(ScriptEngine, addParameterToEntity),
    //     asCALL_THISCALL
    //     );
    // Q_ASSERT(s >= 0);



    e = new MyObj();
    e->x = 10;
    e->y = 20;

    // Sample script
    // const char *script =
    //     "void main() { \n"
    //     "   Print('Hello from AngelScript + Qt!'); \n"
    //     "   //NewProfile('TestProfile'); \n"
    //     "}";



    mod = engine->GetModule("MyModule", asGM_ALWAYS_CREATE);
    // mod->AddScriptSection("script", script);

    // int s = mod->Build();
    // if (r < 0) {
    //     qDebug() << "Failed to compile script";
    //     return;
    // }

    // func = mod->GetFunctionByDecl("void main()");
    // if (!func) {
    //     qDebug() << "Function not found";
    //     return;
    // }

    // ctx = engine->CreateContext();
    // const char* script =
    //     "void on_update(MyObj@ e) { \n"
    //     "  Print('Before: x=' + e.x + ', y=' + e.y); \n"
    //     "  e.x += 5; \n"
    //     "  e.y += 10; \n"
    //     "  e.moveBy(100,100); \n"
    //     "  Print('After: x=' + e.x + ', y=' + e.y); \n"
    //     "}";
    // const char* script =
    //     "void on_update(ScriptEngine@ e) { \n"
    //     "  Print('Before'); \n"
    //     "  string d = e.NewProfile('hello'); \n"
    //     "  e.addEntity(d, 'entity',true); \n"
    //     "  Print('After'); \n"
    //     "}";
    // mod = engine->GetModule("MyModule", asGM_ALWAYS_CREATE);
    // mod->AddScriptSection("script", script);
    // if (mod->Build() < 0) {
    //     qDebug() << "Script Build Failed!";
    //     return;
    // } else {
    //     qDebug() << "Script Compiled.";
    // }

    ctx = engine->CreateContext();
}

ScriptEngine::~ScriptEngine()
{
}

void ScriptEngine::setHierarchy(Hierarchy *hiers, HierarchyTree *tr, SceneRenderer *rend)
{
    hierarchy = hiers;
    tree = tr;
    render = rend;
}

bool ScriptEngine::loadAndCompileScript(QString scriptContent)
{
    engine->DiscardModule("MyModule");
    mod = engine->GetModule("MyModule", asGM_ALWAYS_CREATE);
    mod->AddScriptSection("script", scriptContent.toUtf8().data());

    int s = mod->Build();
    if (s < 0) {
        qDebug() << "Failed to compile script!";
        return false;
    }

    func = mod->GetFunctionByName("main");

    if (!func) {
        qDebug() << "Function main() not found!";
        return false;
    }
    ctx->Prepare(func);
    ctx->SetArgObject(0, this);
    run();

    // //addProfiles("hrgff");

    // asIScriptFunction* func = mod->GetFunctionByName("on_update");
    // if (!func) {
    //     qDebug() << "on_update() function not found!";
    //     return false;
    // }

    // ctx->Prepare(func);
    // ctx->SetArgObject(0, this);
    // int s = ctx->Execute();
    // if (r != asEXECUTION_FINISHED) {
    //     qDebug() << "Script execution failed!";
    // } else {
    //     qDebugz() << "Executed. Entity now: x =" << e->x << ", y =" << e->y;
    // }
    // //ctx->Release();
    return true;
}

void ScriptEngine::run()
{
    if (!ctx || !func)
        return;

    //ctx->Prepare(func);
    ctx->Execute();
}
