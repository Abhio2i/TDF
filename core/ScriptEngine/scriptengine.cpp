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
#include "core/Hierarchy/Components/sensorprofile.h"
#include"core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/profilecategaory.h"
#include "core/Simulation/simulation.h"
#include "core/structure/runtime.h"
#include "scriptenginegis.h"
#include <GIS_API/frontend/widgets/map_widget.h>
#include "backend/api/measure_api.h"
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
#include <QScreen>
#include <QApplication>

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

static Transform* Platform_getTransform(Platform* self)
{
    if (!self)
        return nullptr;

    return &self->transform;
}

static DynamicModel* Platform_getDynamicModel(Platform* self)
{
    return self ? &self->dynamicModel : nullptr;
}

static Trajectory* Platform_getTrajectory(Platform* self)
{
    return self ? &self->trajectory : nullptr;
}

static void Trajectory_addWaypoint(
    Trajectory* self,
    float lat,
    float lon,
    float alt)
{
    if (!self)
        return;

    // AutoScript: lat, lon, alt
    // Internal C++: lat, alt, lon
    self->addWaypoint(lat, alt, lon);
}

static void Trajectory_addWaypointWithSensor(
    Trajectory* self,
    float lat,
    float lon,
    float alt,
    bool activateSensor)
{
    if (!self)
        return;

    // AutoScript: lat, lon, alt
    // Internal C++: lat, alt, lon
    self->addWaypoint(lat, alt, lon, activateSensor);
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
    const char *type = "ERROR";
    if (msg->type == asMSGTYPE_WARNING)          type = "WARN";
    else if (msg->type == asMSGTYPE_INFORMATION) type = "INFO";

    const QString line = QString("%1 (%2,%3) : %4 : %5")
                             .arg(QString::fromUtf8(msg->section))
                             .arg(msg->row).arg(msg->col)
                             .arg(QString::fromUtf8(type))
                             .arg(QString::fromUtf8(msg->message));

    if (msg->type == asMSGTYPE_WARNING) qWarning().noquote() << line;
    else                                qDebug().noquote()   << line;

    if (msg->type != asMSGTYPE_INFORMATION) {
        if (auto* self = static_cast<ScriptEngine*>(param))
            self->appendEngineMessage(line);
    }
}

class FileIO
{
public:
    static bool    s_sandboxActive;
    static QString s_sandboxDir;

    // Sandbox active: path must resolve INSIDE s_sandboxDir (relative paths are resolved there).
    static bool resolve(const std::string &in, std::string &out)
    {
        if (!s_sandboxActive) { out = in; return true; }

        QDir base(s_sandboxDir);
        QDir().mkpath(base.absolutePath());
        const QString baseClean = QDir::cleanPath(base.absolutePath());
        const QString cand = QDir::cleanPath(base.absoluteFilePath(QString::fromStdString(in)));

        if (cand == baseClean || !cand.startsWith(baseClean + "/")) {
            qWarning() << "[FileIO] Blocked path outside sandbox:" << QString::fromStdString(in);
            return false;
        }
        out = cand.toStdString();
        return true;
    }

    static std::string readText(const std::string &path)
    {
        std::string real;
        if (!resolve(path, real)) return "";

        std::ifstream file(real);
        if (!file.is_open()) {
            qDebug() << "[FileIO] Could not open file for reading:" << QString::fromStdString(real);
            return "";
        }
        return std::string((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    }

    static void writeText(const std::string &path, const std::string &content)
    {
        std::string real;
        if (!resolve(path, real)) return;

        std::ofstream file(real);
        if (!file.is_open()) {
            qDebug() << "[FileIO] Could not open file for writing:" << QString::fromStdString(real);
            return;
        }
        file << content;
    }

    static void appendText(const std::string &path, const std::string &content)
    {
        std::string real;
        if (!resolve(path, real)) return;

        std::ofstream file(real, std::ios::app);
        if (!file.is_open()) {
            qDebug() << "[FileIO] Could not open file for appending:" << QString::fromStdString(real);
            return;
        }
        file << content;
    }
};
bool    FileIO::s_sandboxActive = false;
QString FileIO::s_sandboxDir;

void ScriptEngine::setFileSandbox(bool enabled, const QString& dir)
{
    if (!dir.isEmpty())
        FileIO::s_sandboxDir = dir;
    FileIO::s_sandboxActive = enabled;
    if (enabled)
        QDir().mkpath(FileIO::s_sandboxDir);
}

void ScriptEngine::appendEngineMessage(const QString& line)
{
    if (m_engineMessages.size() < 50)
        m_engineMessages.append(line);
}
// warpper AngelScript entry point
void ScriptEngine::setSelectedShape(const std::string &shapeName)
{
    if (!gisEngine) {
        qWarning() << "GIS Engine not available";
        return;
    }

    gisEngine->setSelectedShape(shapeName);
}


// --- Simulation Access Helper ---
static Simulation* GetSimulation(ScriptEngine* engine)
{
    if (!engine) return nullptr;

    Runtime* runtime = engine->getRuntime();  // suse getter
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
        return nullptr;
    }

    Entity* rawEntity = hierarchy->addEntity(QString::fromStdString(Id),
                                             QString::fromStdString(name),
                                             profile);
    if (!rawEntity) {
        return nullptr;
    }

    Platform* entity = static_cast<Platform*>(rawEntity);

    // match addEntityToPlatform() exactly
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

    //hierarchy->addComponent(QString::fromStdString(entity->ID), "mission");
    // hierarchy->addComponent(QString::fromStdString(entity->ID), "networkObject");

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
// void ScriptEngine::ScriptSleep(int milliseconds)
// {
//     ctx->Suspend();
//     QTimer::singleShot(milliseconds, [this]() {
//         this->ctx->Execute();
//     });
// }

void ScriptEngine::ScriptSleep(int milliseconds)
{
    if (!ctx || !scriptRunning)
        return;

    const quint64 generation = scriptGeneration;

    ctx->Suspend();

    QTimer::singleShot(milliseconds, this,
                       [this, generation]()
                       {
                           // Old script callback
                           if (generation != scriptGeneration)
                           {
                               qDebug() << "[ScriptEngine] Ignoring stale sleep callback";
                               return;
                           }

                           // Script was stopped/replaced
                           if (!scriptRunning)
                           {
                               qDebug() << "[ScriptEngine] Script no longer running";
                               return;
                           }

                           if (!ctx)
                               return;

                           if (ctx->GetState() != asEXECUTION_SUSPENDED)
                           {
                               qDebug() << "[ScriptEngine] Context is not suspended";
                               return;
                           }

                          // ctx->Execute();
                           executeContext();
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
    if (!hierarchy ) {
        qDebug() << "[ERROR] Hierarchy not initialized!";
        return nullptr;
    }

    //  Master key access: use unordered_map directly
    auto it = hierarchy->Entities.find(id);
    if (it != hierarchy->Entities.end()) {
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
    if (!hierarchy ) return nullptr;

    // Create array<Entity@> type for AngelScript
    asITypeInfo* arrayEntityType = engine->GetTypeInfoByDecl("array<Entity@>");
    if (!arrayEntityType) return nullptr;

    CScriptArray* (*CreateArray)(asITypeInfo*, asUINT) = &CScriptArray::Create;
    CScriptArray* arr = CreateArray(arrayEntityType, 0);

    std::ostringstream names;
    size_t count = 0;

    for (auto& pair : (hierarchy->Entities)) {
        Entity* ent = pair.second;
        if (ent && ent->type == typeId) {
            arr->InsertLast(&ent);
            if (count > 0) names << ", ";
            names << "\"" << ent->Name << "\"";   //  Use correct field
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
    if (!hierarchy ) return nullptr;

    asITypeInfo* arrayEntityType = engine->GetTypeInfoByDecl("array<Entity@>");
    if (!arrayEntityType) return nullptr;

    CScriptArray* (*CreateArray)(asITypeInfo*, asUINT) = &CScriptArray::Create;
    CScriptArray* arr = CreateArray(arrayEntityType, 0);

    for (auto& pair : (hierarchy->Entities)) {
        Entity* ent = pair.second;
        arr->InsertLast(&ent);
    }

    qDebug() << "[OK] getAllEntities returned" << arr->GetSize() << "entities.";
    return arr;
}

// Return all IDs as strings
CScriptArray* ScriptEngine::getAllEntityIds() {
    if (!hierarchy ) return nullptr;

    asITypeInfo* arrayStringType = engine->GetTypeInfoByDecl("array<string>");
    if (!arrayStringType) return nullptr;

    CScriptArray* (*CreateArray)(asITypeInfo*, asUINT) = &CScriptArray::Create;
    CScriptArray* arr = CreateArray(arrayStringType, 0);

    for (auto& pair : (hierarchy->Entities)) {
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
    if (hierarchy->Entities.find(id) != hierarchy->Entities.end()) {
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

static void Platform_setImage(Platform* self, std::string path)
{
    if (!self) return;

    // Ensure bitmap exists
    // if (!self->meshRenderer2d)
    // self->addComponent("bitmap");

    // if (!self->meshRenderer2d) return;

    // Direct sprite update (correct for your architecture)
    if (self->meshRenderer2d.Sprite)
        *(self->meshRenderer2d.Sprite) = path;

    // Optional: refresh mesh if needed
}

void ScriptEngine::removeEntity(const std::string& parentId, const std::string& entityID)
{
    if (!hierarchy) {
        qDebug() << "[ERROR] Hierarchy not initialized!";
        return;
    }

    emit scriptEntityAboutToBeRemoved(QString::fromStdString(entityID));

    // Call Hierarchy's removeEntity using parentId (folder ID)
    hierarchy->removeEntity(QString::fromStdString(parentId),
                            QString::fromStdString(entityID)); // Profile flag false for entity removal

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
                            QString::fromStdString(folderID)); // Profile flag false for removal

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
    ////////////////////////////////////////////////////////////////////////////////
    auto drawImage = [&](const QString& imagePath) {
        if (imagePath.isEmpty()) return;

        QImage img(imagePath);
        if (img.isNull()) return;

        int maxWidth = pageWidth;
        int maxHeight = printer.pageRect().height() - 200; // ✅ ADD HEIGHT LIMIT

        // Scale image to fit page
        int imgHeight = (img.height() * maxWidth) / img.width();

        // ✅ If image too tall, scale down
        if (imgHeight > maxHeight) {
            imgHeight = maxHeight;
            int imgWidth = (img.width() * maxHeight) / img.height();
            painter.drawImage(QRect(x, y, imgWidth, imgHeight), img);
        } else {
            painter.drawImage(QRect(x, y, maxWidth, imgHeight), img);
        }

        y += imgHeight + 14;

        newPageIfNeeded();
    };
    //////////////////////////////////////////////////////////////////////////////

    // ================= TITLE =================
    painter.setFont(QFont("Arial", 18, QFont::Bold));
    drawLine("Auto Generated Report");
    divider();
    smallGap();

    painter.setFont(QFont("Arial", 10));
    drawLine("Generated By : ScriptEngine");
    drawLine("Report Mode  : AutoScript Driven");
    smallGap();

    painter.setFont(QFont("Arial", 14, QFont::Bold));
    drawLine("AutoScript Execution Summary");
    divider();
    smallGap();

    painter.setFont(QFont("Arial", 11));
    if(executionReport.empty()){
        logEvent(
            ReportCategory::SIMULATION,
            "",
            "",
            "",
            "",
            "",
            "SUCCESS",
            "screenshot",
            captureScreenshot("Entity")
            );
    }

    if (executionReport.empty()) {
        drawLine("No AutoScript operations were recorded.");
    } else {
        // ✅ SEPARATE  AND RADAR SCREENSHOTS
        std::vector<ReportEvent> Entries;
        std::vector<ReportEvent> radarEntries;
        std::vector<ReportEvent> otherEntries;

        for (const auto &e : executionReport) {
            if (e.category == ReportCategory::SYSTEM)
                continue;

            if (e.action.contains("Tactical ", Qt::CaseInsensitive)) {
                Entries.push_back(e);
            } else if (e.action.contains("Sensor Display", Qt::CaseInsensitive)) {
                radarEntries.push_back(e);
            } else {
                otherEntries.push_back(e);
            }
        }

        // ✅ 1.  FIRST (ONLY ONE)
        if (!Entries.empty()) {
            const auto &e = Entries[0];

            // Draw header
            painter.setFont(QFont("Arial", 12, QFont::Bold));
            drawLine("📍 TACTICAL  (CONDITION REACHED)");
            divider();
            smallGap();

            // ✅ NOW CHECK IF REMARKS + IMAGE FIT ON REMAINING SPACE
            int imgHeight = 0;
            QImage img(e.screenshotPath);
            if (!img.isNull()) {
                imgHeight = (img.height() * pageWidth) / img.width();
                if (imgHeight > printer.pageRect().height() - 200) {
                    imgHeight = printer.pageRect().height() - 200;
                }
            }

            // Check current position - if not enough space, new page
            int requiredSpace = 60 + imgHeight;
            if (y + requiredSpace > printer.pageRect().height() - 100) {
                printer.newPage();
                y = 60;
            }

            // Draw remarks and image
            painter.setFont(QFont("Arial", 11));
            drawLine("Status      : " + e.status);
            if (!e.reason.isEmpty())
                drawLine("Remarks     : " + e.reason);

            if (!e.screenshotPath.isEmpty()) {
                smallGap();
                drawImage(e.screenshotPath);
            }

            divider();
            sectionGap();
        }

        // ✅ 2. RADAR SCREENSHOTS (MULTIPLE)
        if (!radarEntries.empty()) {
            painter.setFont(QFont("Arial", 12, QFont::Bold));
            drawLine("📡 RADAR DISPLAY SCREENSHOTS");
            divider();
            smallGap();

            int count = 1;
            for (const auto &e : radarEntries) {
                // ✅ PRE-CALCULATE IMAGE HEIGHT
                int imgHeight = 0;
                QImage img(e.screenshotPath);
                if (!img.isNull()) {
                    int radarSize = pageWidth * 0.5;
                    imgHeight = (img.height() * radarSize) / img.width();
                }

                // ✅ CHECK IF REMARKS + IMAGE FIT TOGETHER
                int requiredSpace = 80 + imgHeight;
                if (y + requiredSpace > printer.pageRect().height() - 100) {
                    printer.newPage();
                    y = 60;
                }

                // Draw remarks and image together
                painter.setFont(QFont("Arial", 11, QFont::Bold));
                drawLine(QString("Screenshot #%1 - %2").arg(count++).arg(e.name));

                painter.setFont(QFont("Arial", 10));
                drawLine("Status      : " + e.status);

                if (!img.isNull()) {
                    smallGap();
                    int radarSize = pageWidth * 0.5;
                    int centerX = x + (pageWidth - radarSize) / 2;
                    painter.drawImage(QRect(centerX, y, radarSize, imgHeight), img);
                    y += imgHeight + 14;
                }

                divider();
                smallGap();
            }
            sectionGap();
        }

        // ✅ 3. OTHER ENTRIES (IF ANY)
        for (const auto &e : otherEntries) {
            painter.setFont(QFont("Arial", 11, QFont::Bold));
            drawLine(QString("%1 CREATED").arg(e.action.toUpper()));

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

            if (!e.screenshotPath.isEmpty()) {
                smallGap();
                drawImage(e.screenshotPath);
            }

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

    int entityCount = (hierarchy )
                          ? hierarchy->Entities.size()
                          : 0;

    drawLine("Total Simulation Entities: " + QString::number(entityCount));
    y += 8;

    if (entityCount == 0) {
        drawLine("No simulation entities were created by this AutoScript.");
    } else {
        painter.setFont(QFont("Arial", 14, QFont::Bold));
        drawLine("Simulation Entity Details");

        painter.setFont(QFont("Arial", 11));
        for (auto &pair : (hierarchy->Entities)) {
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

    executionReport.clear();
    qDebug() << "[PDF] AutoScript-based report generated at:"
             << QString::fromStdString(filePath);
}


//  capture screenshot
// UPDATED: ->gislib->grab() -> gisMap->grab(). MapWidget renders both the
// map tiles and the GIS shapes (see MapWidget::drawShapes()), so grabbing it
// directly is the correct replacement for the old "container->grab()" call.
QString ScriptEngine::captureScreenshot(const QString& tag)
{
    if (!gisMap)
        return "";

    // Ensure map tiles & shapes render ho chuke ho
    QCoreApplication::processEvents();
    QThread::msleep(150);
    QCoreApplication::processEvents();

    QPixmap pixmap = gisMap->grab(); // MAP + SHAPES

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
// void ScriptEngine::attachSensorToEntity(const std::string& entityId,
//                                         const std::string& sensorName,
//                                         const std::string& sensorType)
// {
//     if (!hierarchy) {
//         qWarning() << "❌ [ScriptEngine] Hierarchy not set. Cannot attach sensor.";
//         return;
//     }

//     qDebug() << "⚙️ [ScriptEngine] Attaching sensor:"
//              << QString::fromStdString(sensorName)
//              << "of type" << QString::fromStdString(sensorType)
//              << "to entity:" << QString::fromStdString(entityId);

//     hierarchy->attachSensors(QString::fromStdString(entityId),
//                              QString::fromStdString(sensorName),
//                              QString::fromStdString(sensorType));
// }

void ScriptEngine::attachSensorToEntity(const std::string& entityId,
                                        const std::string& sensorName,
                                        const std::string& sensorType)
{
    if (!hierarchy) {
        qWarning() << "❌ [ScriptEngine] Hierarchy not set. Cannot attach sensor.";
        return;
    }

    // ✅ FIX: hierarchy->attachSensors() ki jagah SAME safe path use karo jo
    // manual/UI sensor creation use karta hai (SensorProfile::addSubComponent).
    // hierarchy->attachSensors() sensor ko EK ALAG tree Entity ki tarah bhi
    // register karta tha (addEntity) + Platform ke SensorProfile mein bhi
    // (addSensor) — is dual-registration ki wajah se jab UI se entity delete
    // hoti thi (removeEntity), SensorProfile::sensors map mein dangling
    // pointer reh jata tha → crash. Ab sirf ek hi safe registration hogi.
    auto it = hierarchy->Entities.find(entityId);
    if (it == hierarchy->Entities.end() || !it->second) {
        qWarning() << "❌ [ScriptEngine] attachSensorToEntity: entity not found:"
                   << QString::fromStdString(entityId);
        return;
    }

    Platform* platform = dynamic_cast<Platform*>(it->second);
    if (!platform) {
        qWarning() << "❌ [ScriptEngine] attachSensorToEntity: entity is not a Platform:"
                   << QString::fromStdString(entityId);
        return;
    }

    platform->addComponent("sensors");   // ensure SensorProfile component exists

    QString sensorComponentID = QString::fromStdString(platform->sensors.ID);

    hierarchy->addSubComponent(
        sensorComponentID,
        QString::fromStdString(sensorName),
        QString::fromStdString(sensorType)   // data1 = sensorType
        );

    qDebug() << "⚙️ [ScriptEngine] Attached sensor:"
             << QString::fromStdString(sensorName)
             << "of type" << QString::fromStdString(sensorType)
             << "to entity:" << QString::fromStdString(entityId)
             << "via safe subcomponent path";
}

// void ScriptEngine::attachIFFToEntity(const std::string& entityID, const std::string& iffName) {
//     if (!hierarchy) return;
//     hierarchy->attchedIff(QString::fromStdString(entityID), QString::fromStdString(iffName));
// }

void ScriptEngine::attachIFFToEntity(const std::string& entityID, const std::string& iffName) {
    if (!hierarchy) return;

    // ✅ FIX: safe path — same jo addIFFSubComponent use karta hai
    auto it = hierarchy->Entities.find(entityID);
    if (it == hierarchy->Entities.end() || !it->second) {
        qWarning() << "❌ [ScriptEngine] attachIFFToEntity: entity not found:"
                   << QString::fromStdString(entityID);
        return;
    }

    Platform* platform = dynamic_cast<Platform*>(it->second);
    if (!platform) {
        qWarning() << "❌ [ScriptEngine] attachIFFToEntity: entity is not a Platform:"
                   << QString::fromStdString(entityID);
        return;
    }

    QString iffProfileId = QString::fromStdString(platform->iffs.ID);

    hierarchy->addSubComponent(
        iffProfileId,
        QString::fromStdString(iffName),
        ""
        );

    qDebug() << "[IFF] Attached IFF:" << QString::fromStdString(iffName)
             << "to platform:" << QString::fromStdString(entityID)
             << "via safe subcomponent path";
}

// void ScriptEngine::attachRadioToEntity(const std::string& entityID, const std::string& radioName)
// {
//     if (!hierarchy) {
//         qDebug() << "[ERROR] Hierarchy not set in ScriptEngine::attachRadioToEntity!";
//         return;
//     }

//     hierarchy->attachRadios(QString::fromStdString(entityID), QString::fromStdString(radioName));
// }

void ScriptEngine::attachRadioToEntity(const std::string& entityID, const std::string& radioName)
{
    if (!hierarchy) {
        qDebug() << "[ERROR] Hierarchy not set in ScriptEngine::attachRadioToEntity!";
        return;
    }

    // ✅ FIX: safe path — same jo addRadioSubComponent use karta hai
    auto it = hierarchy->Entities.find(entityID);
    if (it == hierarchy->Entities.end() || !it->second) {
        qWarning() << "❌ [ScriptEngine] attachRadioToEntity: entity not found:"
                   << QString::fromStdString(entityID);
        return;
    }

    Platform* platform = dynamic_cast<Platform*>(it->second);
    if (!platform) {
        qWarning() << "❌ [ScriptEngine] attachRadioToEntity: entity is not a Platform:"
                   << QString::fromStdString(entityID);
        return;
    }

    QString radioProfileId = QString::fromStdString(platform->radios.ID);

    hierarchy->addSubComponent(
        radioProfileId,
        QString::fromStdString(radioName),
        "Radio"
        );

    qDebug() << "[RADIO] Attached radio:" << QString::fromStdString(radioName)
             << "to platform:" << QString::fromStdString(entityID)
             << "via safe subcomponent path";
}

void ScriptEngine::addSubComponent(
    const std::string& entityId,
    ComponentType type,
    const std::string& name,
    const std::string& data1,
    const std::string& data2,
    const std::string& data3
    ) {
    if (!hierarchy) return;

}

void ScriptEngine::addSensorSubComponent(
    Platform* platform,
    const std::string& subName,
    const std::string& sensorType
    )
{
    if (!platform) {
        Console::log("❌ Platform is null");
        return;
    }

    Hierarchy* h = this->hierarchy;
    if (!h) {
        Console::log("❌ Hierarchy is null");
        return;
    }

    // Ensure Sensor component exists
    /*if (!platform->sensors)*/ {
        platform->addComponent("sensors");
    }

    // if (!platform->sensors) {
    //     Console::log("❌ Sensor component could not be created");
    //     return;
    // }

    // IMPORTANT: parent ID is SENSOR COMPONENT ID
    QString sensorComponentID =
        QString::fromStdString(platform->sensors.ID);

    // ✅ data1 = sensorType (CRITICAL)
    h->addSubComponent(
        sensorComponentID,
        QString::fromStdString(subName),
        QString::fromStdString(sensorType)
        );

    Console::log(
        "✅ Sensor subcomponent '" + subName +
        "' [" + sensorType +
        "] added under Sensor (" + platform->sensors.ID + ")"
        );
}
Sensor* AS_SensorProfile_getSensor(
    SensorProfile* self,
    const std::string& id
    )
{
    if (!self) return nullptr;
    return self->getSensor(id);
}

void ScriptEngine::showSidebarView(const std::string &viewName) {
    qDebug() << "ScriptEngine::showSidebarView called with:" << QString::fromStdString(viewName);
    emit requestSidebarView(QString::fromStdString(viewName));
}
void ScriptEngine::selectEntityDisplay(Sensor* sensor) {
    if (!sensor) {
        qWarning() << "selectEntityDisplay: null sensor";
        return;
    }

    // Get the sensor subType and convert to string
    QString sensorType = sensor->subTypeToString(sensor->subType);

    qDebug() << "Selecting display for sensor subType:" << sensorType;

    // ✅ STEP 1: Show Sensors sidebar tab FIRST
    qDebug() << "Step 1: Opening Sensors sidebar";
    emit requestSidebarView("Sensors");

    // ✅ STEP 2: Select the parent entity in hierarchy tree
    if (sensor->parentEntity) {
        QString entityId = QString::fromStdString(sensor->parentEntity->ID);
        qDebug() << "Step 2: Selecting entity:" << entityId;
        emit requestSelectEntity(entityId);
    } else {
        qWarning() << "Sensor has no parentEntity set!";
        return;
    }

    // ✅ STEP 3: Map sensor subType to tab name and switch to it
    qDebug() << "Step 3: Switching to" << sensorType << "tab";
    if (sensorType == "Generic") {
        emit requestDisplayTab("Radar");
    } else if (sensorType == "CSM") {
        emit requestDisplayTab("CSM");
    } else if (sensorType == "ESM") {
        emit requestDisplayTab("ESM");
    } else {
        qWarning() << "Unknown sensor subType:" << sensorType;
        emit requestDisplayTab("Radar");
    }
}
void ScriptEngine::selectEntityDisplay(Radio* radio) {
    if (!radio) {
        qWarning() << "selectEntityDisplay: null radio";
        return;
    }

    qDebug() << "Selecting display for radio:" << QString::fromStdString(radio->Name);

    // ✅ STEP 1: Show Sensors sidebar tab FIRST
    qDebug() << "Step 1: Opening Sensors sidebar";
    emit requestSidebarView("Sensors");

    // ✅ STEP 2: Select the parent entity in hierarchy tree
    if (radio->parentEntity) {
        QString entityId = QString::fromStdString(radio->parentEntity->ID);
        qDebug() << "Step 2: Selecting entity:" << entityId;
        emit requestSelectEntity(entityId);
    } else {
        qWarning() << "Radio has no parentEntity set!";
        return;
    }

    // ✅ STEP 3: Switch to Radio tab (UPPERCASE!)
    qDebug() << "Step 3: Switching to RADIO tab";
    emit requestDisplayTab("RADIO");  // ✅ Changed from "Radio" to "RADIO"
}
void ScriptEngine::captureSensorScreenshot(const std::string &filePath) {
    QString qFilePath = QString::fromStdString(filePath);
    qDebug() << "Requesting sensor display screenshot:" << qFilePath;
    emit requestSensorScreenshot(qFilePath);
}

void ScriptEngine::addRadioSubComponent(
    Platform* platform,
    const std::string& subName)
{
    if (!platform)
    {
        return;
    }

    if (!hierarchy)
    {
        return;
    }

    const QString qSubName =
        QString::fromStdString(subName);

    const QString radioProfileId =
        QString::fromStdString(platform->radios.ID);


    hierarchy->addSubComponent(
        radioProfileId,
        qSubName,
        "Radio",
        "",
        QJsonObject()
        );

    // IMPORTANT: verify actual registration
    Radio* radio =
        platform->getRadioByName(subName);

}

void ScriptEngine::addIFFSubComponent(
    Platform* platform,
    const std::string& subName)
{
    if (!platform) {
        qWarning() << "Platform is null in addIFFSubComponent";
        return;
    }

    if (!hierarchy) {
        qWarning() << "Hierarchy is null in addIFFSubComponent";
        return;
    }


    // Add IFF subcomponent using the IFFProfile's ID
    QString qSubName = QString::fromStdString(subName);

    hierarchy->addSubComponent(
        QString::fromStdString(platform->iffs.ID),
        qSubName,
        "",  // data1 - empty for IFF
        "",  // data2 - empty (or template ID if copying from existing)
        QJsonObject()   // data3 - empty
        );

    qDebug() << "Added IFF subcomponent:" << qSubName
             << "to platform:" << QString::fromStdString(platform->Name);
}
void ScriptEngine::selectEntityDisplay(IFF* iff) {

    if (!iff) {
        qWarning() << "selectEntityDisplay: null iff";
        return;
    }


    // ✅ STEP 1: Show Sensors sidebar tab FIRST
    emit requestSidebarView("Sensors");

    // ✅ STEP 2: Select the parent entity in hierarchy tree
    if (iff->parentEntity) {
        QString entityId = QString::fromStdString(iff->parentEntity->ID);
        emit requestSelectEntity(entityId);
    } else {
        qWarning() << "IFF has no parentEntity set!";
        return;
    }

    emit requestDisplayTab("IFF");
}

void ScriptEngine::stopScript()
{
    const bool wasRunning = scriptRunning;

    ++scriptGeneration;
    scriptRunning = false;

    if (ctx)
    {
        const int state = ctx->GetState();
        if (state == asEXECUTION_ACTIVE || state == asEXECUTION_SUSPENDED)
            ctx->Abort();
        ctx->Unprepare();
    }
    func = nullptr;

    if (wasRunning)
        emit scriptFinished(false, "ABORTED", "Script stopped");
}

void ScriptEngine::AddLayer(const std::string &layerName)
{
    if (!gisEngine) return;
    gisEngine->AddLayer(layerName);
}

void ScriptEngine::SelectLayer(const std::string &layerName)
{
    if (!gisEngine) return;
    gisEngine->SelectLayer(layerName);
}

// void ScriptEngine::RenameShape(const std::string &newName)
// {
//     if (!gisEngine) return;
//     gisEngine->RenameShape(newName);
// }

// Set active city in GIS engine using city name
void ScriptEngine::useCity(const std::string& cityName)
{
    // Check if GIS engine is available
    if (!gisEngine) {
        qWarning() << "[useCity] GIS Engine not available";
        return;
    }
    // Delegate city selection to GIS engine
    gisEngine->useCity(cityName);
}

// Returns formatted city location string (e.g., lat/long info)
QString ScriptEngine::cityLocationString() const
{
    // Return empty string if GIS engine is not available
    if (!gisEngine) return "";

    // Fetch city location string from GIS engine
    return gisEngine->cityLocationString();
}

// Inject the MapWidget dependency (replaces the old set(Widget*) +
// setGIS(GISlib*) + setMapContainer(QWidget*) trio). Call this once the
// new MapWith/MapWidget has been created, e.g. from RuntimeEditor:
//   runtime->scriptengine->setMapWidget(m_apiMapWidget->map());
void ScriptEngine::setMapWidget(MapWidget* map)
{
    gisMap = map; // Store map widget reference

    // Pass map widget to GIS engine if available
    if (gisEngine)
        gisEngine->setMapWidget(map);
}

// Add a circular shape on
void ScriptEngine::AddCircle(const std::string &name, float radius)
{
    if (!gisEngine) return;  // Exit if GIS engine is not initialized
    gisEngine->AddCircle(name, radius);  // Delegate circle creation to GIS engine
}


// Add a rectangular shape on
void ScriptEngine::AddRectangle(const std::string &name, float w, float h)
{
    if (!gisEngine) return; // Exit if GIS engine is not initialized
    gisEngine->AddRectangle(name, w, h); // Exit if GIS engine is not initialized
}

// Add a polygon using a list of points
void ScriptEngine::AddPolygon(const std::string &name, CScriptArray* pts)
{
    if (!gisEngine) return;
    gisEngine->AddPolygon(name, pts);
}

// ---------------- Add line APIs ----------------
// Start drawing a polyline
void ScriptEngine::StartLine()
{
    if (!gisEngine) return;
    gisEngine->StartLine();  // Initialize line drawing
}

// Add a point to the currently drawn line (latitude, then longitude)
void ScriptEngine::AddLinePoint(float lat, float lon)
{
    if (!gisEngine) return;
    gisEngine->AddLinePoint(lat, lon);
}

// Finish drawing the current line
void ScriptEngine::FinishLine()
{
    if (!gisEngine) return;
    gisEngine->FinishLine();
}

// Add a single point marker on  (latitude, then longitude)
void ScriptEngine::AddPoint(const std::string &name, float lat, float lon)
{
    if (!gisEngine) return;
    gisEngine->AddPoint(name, lat, lon);
}


// Handle bitmap selection using predefined bitmap type (latitude, then longitude)
void ScriptEngine::onBitmapSelected(const std::string &bitmapType, float lat, float lon)
{
    if (!gisEngine) return;
    gisEngine->onBitmapSelected(bitmapType, lat, lon);
}

// Set bitmap image file path
void ScriptEngine::getBitmapImagePath(const std::string &filePath)
{
    if (!gisEngine) return;
    gisEngine->getBitmapImagePath(filePath);
}

// Handle bitmap image selection and placement (latitude, then longitude)
void ScriptEngine::onBitmapImageSelected(const std::string &filePath, float lat, float lon)
{
    if (!gisEngine) return;
    gisEngine->onBitmapImageSelected(filePath, lat, lon);
}

// Toggle visibility of airbase markers on
void ScriptEngine::ToggleAirbases(bool visible)
{
    if (!gisEngine) return;
    gisEngine->ToggleAirbases(visible);    // Toggle airbase layer visibility
}


// Import a GeoJSON layer into the
// void ScriptEngine::ImportGeoJsonLayer(const std::string &filePath)
// {
//     if (!gisEngine) return;
//     gisEngine->ImportGeoJsonLayer(filePath);
// }

// Toggle visibility of a specific GeoJSON layer
void ScriptEngine::ToggleGeoJsonLayer(const std::string &layerName, bool visible)
{
    if (!gisEngine) return;

    // Show or hide specified GeoJSON layer
    gisEngine->ToggleGeoJsonLayer(layerName, visible);
}

// ---------------- Measurement Distance APIs ----------------
// Start distance measurement mode
void ScriptEngine::StartDistanceMeasurement()
{
    if (!gisEngine) return;
    gisEngine->StartDistanceMeasurement();   // Begin distance measurement
}

// Add a measurement point (latitude, then longitude)
void ScriptEngine::AddMeasurePoint(double lat, double lon)
{
    if (!gisEngine) return;
    gisEngine->AddMeasurePoint(lat, lon);
}

// Get distance of the last measured segment
double ScriptEngine::GetLastSegmentDistance()
{
    // Return zero if GIS engine is not available
    if (!gisEngine) return 0.0;
    return gisEngine->GetLastSegmentDistance();
}

// Get total measured distance
double ScriptEngine::GetTotalDistance()
{
    if (!gisEngine) return 0.0;
    return gisEngine->GetTotalDistance();
}

// Set unit for distance measurement (e.g., km, m, nm)
void ScriptEngine::SetMeasurementUnit(const std::string &unit)
{
    if (!gisEngine) return;
    gisEngine->SetMeasurementUnit(unit);    // Set measurement unit in GIS engine
}

void ScriptEngine :: SwitchMap(const std::string& mapName){
    if(!gisEngine) return;
    gisEngine->SwitchMap(mapName);
}

void ScriptEngine :: switchCoordinateSystem(const std::string& system){
    if(!gisEngine) return;
    gisEngine->switchCoordinateSystem(system);
}

void ScriptEngine::moveShape(const std::string& shapeName,
                             double lat, double lon)
{
    if (!gisEngine) return;
    gisEngine->moveShape(shapeName, lat, lon);
}

void ScriptEngine::rotateShape(const std::string& shapeName, double angleDeg)
{
    if (!gisEngine) return;
    gisEngine->rotateShape(shapeName, angleDeg);
}

void ScriptEngine::showShapeHistory(const std::string& shapeName)
{
    if (!gisEngine) return;
    gisEngine->showShapeHistory(shapeName);
}

void ScriptEngine::hideShapeHistory()
{
    if (!gisEngine) return;
    gisEngine->hideShapeHistory();
}

void ScriptEngine::restoreShapeHistory(const std::string& shapeName)
{
    if (!gisEngine) return;
    gisEngine->restoreShapeHistory(shapeName);
}

void ScriptEngine::addText(const std::string& text, double lat, double lon)
{
    if (!gisEngine) return;
    gisEngine->addText(text, lat, lon);
}

void ScriptEngine::addShapeProperties(const std::string& shapeName,int r, int g, int b,int borderThickness)
{
    if (!gisEngine) return;

    gisEngine->addShapeProperties(
        shapeName,
        r, g, b,
        borderThickness
        );
}

void ScriptEngine::deleteshape(const std::string& id)
{
    if (!gisEngine) return;

    gisEngine->deleteshape(id);
}


ScriptEngine::ScriptEngine()
{
    // AngelScript setup
    engine = asCreateScriptEngine();
    engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);  

    // ✅ Enable virtual property syntax so scripts can write
    // "falcon.transform.setGeoCord(...)" / "falcon.trajectory.addWaypoint(...)"
    // instead of "falcon.getTransform().setGeoCord(...)". This must be set
    // before any get_xxx()/set_xxx() methods are registered below.
    engine->SetEngineProperty(asEP_PROPERTY_ACCESSOR_MODE, 2);

    RegisterStdString(engine);
    engine->RegisterGlobalFunction("float Random()", asFUNCTION(Math_Random), asCALL_CDECL);

    gisEngine = new ScriptEngineGIS();

    // Bind GIS → ScriptEngine report bridge by amjad
    gisEngine->logFn = [this](
                           ReportCategory category,
                           const QString& action,
                           const QString& name,
                           const QString& location,
                           const QString& input,
                           const QString& output,
                           const QString& status,
                           const QString& reason,
                           const QString& screenshot
                           ) {
        logEvent(
            category,
            action,
            name,
            location,
            input,
            output,
            status,
            reason,
            screenshot
            );
    };

    gisEngine->screenshotFn = [this](const QString& tag) {
        return captureScreenshot(tag);
    };

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
    s = engine->RegisterObjectMethod("Transform", "void setGeoCord(float lat, float lon, float alt)", asMETHODPR(Transform, setGeoCord, (float, float, float), void), asCALL_THISCALL); Q_ASSERT(s >= 0);
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
    // Old 3‑parameter version (now with asMETHODPR)
    r = engine->RegisterObjectMethod(
        "Trajectory",
        "void addWaypoint(float lat, float lon, float alt)",
        asFUNCTION(Trajectory_addWaypoint),
        asCALL_CDECL_OBJFIRST
        );
    Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(
        "Trajectory",
        "void addWaypoint(float lat, float lon, float alt, bool activateSensor)",
        asFUNCTION(Trajectory_addWaypointWithSensor),
        asCALL_CDECL_OBJFIRST
        );
    Q_ASSERT(r >= 0);
    //////////////////////////////////////////////////////////////////////////////
    s = engine->RegisterObjectType("DynamicModel", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);

    // dynamic maximum
    s = engine->RegisterObjectProperty("DynamicModel", "float moveSpeed", asOFFSET(DynamicModel, moveSpeed)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("DynamicModel", "float maxSpeed", asOFFSET(DynamicModel, maxSpeed)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("DynamicModel", "float minSpeed", asOFFSET(DynamicModel, minSpeed)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("DynamicModel", "float Acceleration", asOFFSET(DynamicModel, Acceleration)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("DynamicModel", "float Decceleration", asOFFSET(DynamicModel, Decceleration)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("DynamicModel", "float turnRate", asOFFSET(DynamicModel, turnRate)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("DynamicModel", "float Roll", asOFFSET(DynamicModel, Roll)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("DynamicModel", "float maxAltitude", asOFFSET(DynamicModel, maxAltitude)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("DynamicModel", "float Altitude", asOFFSET(DynamicModel, Altitude)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("DynamicModel", "float climbRate", asOFFSET(DynamicModel, climbRate)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("DynamicModel", "float diveRate", asOFFSET(DynamicModel, diveRate)); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("ProfileCategaory", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("ProfileCategaory", "string id", asOFFSET(ProfileCategaory, ID)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("ProfileCategaory", "string name", asOFFSET(ProfileCategaory, Name)); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("Folder", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Folder", "string id", asOFFSET(Folder, ID)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Folder", "string name", asOFFSET(Folder, Name)); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("Platform", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Platform", "string id", asOFFSET(Entity, ID)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Platform", "string name", asOFFSET(Entity, Name)); Q_ASSERT(s >= 0);

    // s = engine->RegisterObjectProperty("Platform", "Transform@ transform", asOFFSET(Platform, transform)); Q_ASSERT(s >= 0);
    // ✅ Registered as virtual properties (get_transform / get_dynamicModel / get_trajectory)
    // so scripts can write "falcon.transform.setGeoCord(...)" and
    // "falcon.trajectory.addWaypoint(...)" directly, without calling a getter method.
    s = engine->RegisterObjectMethod(
        "Platform",
        "Transform@ get_transform()",
        asFUNCTION(Platform_getTransform),
        asCALL_CDECL_OBJFIRST
        );
    Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod(
        "Platform",
        "DynamicModel@ get_dynamicModel()",
        asFUNCTION(Platform_getDynamicModel),
        asCALL_CDECL_OBJFIRST
        );
    Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod(
        "Platform",
        "Trajectory@ get_trajectory()",
        asFUNCTION(Platform_getTrajectory),
        asCALL_CDECL_OBJFIRST
        );
    Q_ASSERT(s >= 0);

    // s = engine->RegisterObjectProperty("Platform", "DynamicModel@ dynamicModel", asOFFSET(Platform, dynamicModel)); Q_ASSERT(s >= 0);
    //s = engine->RegisterObjectProperty("Platform", "Trajectory@ trajectory", asOFFSET(Platform, trajectory)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Platform", "void addParam(const string &in,const string &in)", asMETHOD(Platform, addParam), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Platform", "void editParam(const string &in,const string &in)", asMETHOD(Platform, editParam), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Platform", "string getParam(const string &in)", asMETHOD(Platform, getParam), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("Platform", "void removeParam(const string &in)", asMETHOD(Platform, removeParam), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod(
        "Platform",
        "void setImage(string)",
        asFUNCTION(Platform_setImage),
        asCALL_CDECL_OBJFIRST
        );
    assert(s >= 0);

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
    r = engine->RegisterObjectProperty(
        "Sensor",
        "float azimuth",
        asOFFSET(Sensor, azimuth)
        );
    Q_ASSERT(r >= 0);

    r = engine->RegisterObjectProperty(
        "Sensor",
        "float frequency",
        asOFFSET(Sensor, frequency)
        );
    Q_ASSERT(r >= 0);

    r = engine->RegisterObjectProperty(
        "Sensor",
        "float range",
        asOFFSET(Sensor, range)
        );
    Q_ASSERT(r >= 0);
    s = engine->RegisterObjectProperty("Sensor", "string name", asOFFSET(Sensor, Name)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Sensor", "string parentID", asOFFSET(Sensor, parentID)); Q_ASSERT(s >= 0);
    // ================= SENSOR PROFILE =================
    s = engine->RegisterObjectType(
        "SensorProfile",
        0,
        asOBJ_REF | asOBJ_NOCOUNT
        );
    Q_ASSERT(s >= 0);
    s = engine->RegisterEnum("ComponentType"); Q_ASSERT(s >= 0);

    s = engine->RegisterEnumValue("ComponentType", "Unknown",           (int)ComponentType::Unknown); Q_ASSERT(s >= 0);
    s = engine->RegisterEnumValue("ComponentType", "Transform",         (int)ComponentType::Transform); Q_ASSERT(s >= 0);
    s = engine->RegisterEnumValue("ComponentType", "Rigidbody",         (int)ComponentType::Rigidbody); Q_ASSERT(s >= 0);
    s = engine->RegisterEnumValue("ComponentType", "NetworkObject",     (int)ComponentType::NetworkObject); Q_ASSERT(s >= 0);
    s = engine->RegisterEnumValue("ComponentType", "Mission",           (int)ComponentType::Mission); Q_ASSERT(s >= 0);
    s = engine->RegisterEnumValue("ComponentType", "MeshRenderer2D",    (int)ComponentType::MeshRenderer2D); Q_ASSERT(s >= 0);
    s = engine->RegisterEnumValue("ComponentType", "DynamicModel",      (int)ComponentType::DynamicModel); Q_ASSERT(s >= 0);
    s = engine->RegisterEnumValue("ComponentType", "Collider",          (int)ComponentType::Collider); Q_ASSERT(s >= 0);
    s = engine->RegisterEnumValue("ComponentType", "Trajectory",        (int)ComponentType::Trajectory); Q_ASSERT(s >= 0);
    s = engine->RegisterEnumValue("ComponentType", "AttachedEnitities", (int)ComponentType::AttachedEnitities); Q_ASSERT(s >= 0);
    s = engine->RegisterEnumValue("ComponentType", "CrossSection",      (int)ComponentType::CrossSection); Q_ASSERT(s >= 0);
    s = engine->RegisterEnumValue("ComponentType", "SensorProfile",     (int)ComponentType::SensorProfile); Q_ASSERT(s >= 0);
    s = engine->RegisterEnumValue("ComponentType", "IFFProfile",        (int)ComponentType::IFFProfile); Q_ASSERT(s >= 0);


    // ================= TARGET =================
    engine->RegisterObjectType(
        "Target",
        sizeof(Target),
        asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Target>()
        );

    // Properties
    engine->RegisterObjectProperty("Target", "Platform@ entity", asOFFSET(Target, entity));
    engine->RegisterObjectProperty("Target", "float radius",    asOFFSET(Target, radius));
    engine->RegisterObjectProperty("Target", "float angle",     asOFFSET(Target, angle));
    engine->RegisterObjectProperty("Target", "float speed",     asOFFSET(Target, speed));
    engine->RegisterObjectProperty("Target", "float direction", asOFFSET(Target, direction));
    engine->RegisterObjectProperty("Target", "float altitude",  asOFFSET(Target, altitude));
    engine->RegisterObjectProperty("Target", "float lat",       asOFFSET(Target, lat));
    engine->RegisterObjectProperty("Target", "float lon",       asOFFSET(Target, lon));
    // Generic
    engine->RegisterObjectMethod("Sensor", "int getTargetCount()",
                                 asMETHOD(Sensor, getTargetCount), asCALL_THISCALL);
    engine->RegisterObjectMethod("Sensor", "Target getTarget(int)",
                                 asMETHOD(Sensor, getTarget), asCALL_THISCALL);

    // CSM
    engine->RegisterObjectMethod("Sensor", "int getCSMTargetCount()",
                                 asMETHOD(Sensor, getCSMTargetCount), asCALL_THISCALL);
    engine->RegisterObjectMethod("Sensor", "Target getCSMTarget(int)",
                                 asMETHOD(Sensor, getCSMTarget), asCALL_THISCALL);

    // ESM
    engine->RegisterObjectMethod("Sensor", "int getESMTargetCount()",
                                 asMETHOD(Sensor, getESMTargetCount), asCALL_THISCALL);

    engine->RegisterObjectMethod("Sensor", "Target getESMTarget(int)",
                                 asMETHOD(Sensor, getESMTarget), asCALL_THISCALL);

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
    //s = engine->RegisterObjectMethod("ScriptEngine","void removeEntity(const string &in)",asMETHOD(ScriptEngine, removeEntity),asCALL_THISCALL);Q_ASSERT(s >= 0); // check same
    s = engine->RegisterObjectMethod("ScriptEngine","void removeProfile(const string &in)",asMETHOD(ScriptEngine, removeProfile),asCALL_THISCALL);Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine","void removeFolder(const string &in, const string &in)",asMETHOD(ScriptEngine, removeFolder),asCALL_THISCALL);Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine","void removeEntity(const string &in, const string &in)",asMETHOD(ScriptEngine, removeEntity),asCALL_THISCALL);Q_ASSERT(s >= 0);  // check same
    s = engine->RegisterObjectMethod("ScriptEngine","void renameProfile(const string &in, const string &in)",asMETHOD(ScriptEngine, renameProfile),asCALL_THISCALL);Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine","void renameFolder(const string &in, const string &in)",asMETHOD(ScriptEngine, renameFolder),asCALL_THISCALL);Q_ASSERT(s >= 0);


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

    //new//
    s = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void addSubComponent(const string &in, ComponentType, const string &in, const string &in, const string &in, const string &in)",
        asMETHOD(ScriptEngine, addSubComponent),
        asCALL_THISCALL
        );
    Q_ASSERT(s >= 0);
    // ✅ ADD THIS — exact signature match
    s = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void addSensorSubComponent(Platform@, const string &in, const string &in)",
        asMETHOD(ScriptEngine, addSensorSubComponent),
        asCALL_THISCALL
        );
    Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod(
        "SensorProfile",
        "Sensor@ getSensor(const string &in)",
        asFUNCTION(AS_SensorProfile_getSensor),
        asCALL_CDECL_OBJFIRST
        );
    Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod(
        "Platform",
        "Sensor@ getSensorByName(const string &in)",
        asMETHOD(Platform, getSensorByName),
        asCALL_THISCALL
        );Q_ASSERT(s >= 0);

    // Register selectEntityDisplay for Sensor
    engine->RegisterObjectMethod("ScriptEngine",
                                 "void selectEntityDisplay(Sensor@)",
                                 asMETHODPR(ScriptEngine, selectEntityDisplay, (Sensor*), void),
                                 asCALL_THISCALL);

    engine->RegisterObjectMethod(
        "ScriptEngine",
        "void captureSensorScreenshot(const string &in)",
        asMETHOD(ScriptEngine, captureSensorScreenshot),
        asCALL_THISCALL
        );

    // ============= RADIO REGISTRATION =============
    // Register Radio type ONCE
    engine->RegisterObjectType("Radio", 0, asOBJ_REF | asOBJ_NOCOUNT);

    // Register addRadioSubComponent
    engine->RegisterObjectMethod(
        "ScriptEngine",
        "void addRadioSubComponent(Platform@, const string &in)",
        asMETHOD(ScriptEngine, addRadioSubComponent),
        asCALL_THISCALL
        );

    // Register Radio methods
    engine->RegisterObjectMethod("Radio",
                                 "int getRadioTargetCount() const",
                                 asMETHOD(Radio, getRadioTargetCount),
                                 asCALL_THISCALL);

    engine->RegisterObjectMethod("Radio",
                                 "bool getRadioTarget(int, string &out, float &out, float &out, float &out, float &out) const",
                                 asMETHOD(Radio, getRadioTarget),
                                 asCALL_THISCALL);

    // Register Platform::getRadioByName
    engine->RegisterObjectMethod("Platform",
                                 "Radio@ getRadioByName(const string &in) const",
                                 asMETHOD(Platform, getRadioByName),
                                 asCALL_THISCALL);

    // Register selectEntityDisplay for Radio
    engine->RegisterObjectMethod("ScriptEngine",
                                 "void selectEntityDisplay(Radio@)",
                                 asMETHODPR(ScriptEngine, selectEntityDisplay, (Radio*), void),
                                 asCALL_THISCALL);
    // Register IFF type
    engine->RegisterObjectType("IFF", 0, asOBJ_REF | asOBJ_NOCOUNT);

    // Register addIFFSubComponent
    engine->RegisterObjectMethod(
        "ScriptEngine",
        "void addIFFSubComponent(Platform@, const string &in)",
        asMETHOD(ScriptEngine, addIFFSubComponent),
        asCALL_THISCALL
        );

    // Register IFF methods
    engine->RegisterObjectMethod("IFF",
                                 "int getIFFTargetCount() const",
                                 asMETHOD(IFF, getIFFTargetCount),
                                 asCALL_THISCALL);

    engine->RegisterObjectMethod("IFF",
                                 "bool getIFFTarget(int, string &out, string &out, string &out, string &out, float &out, float &out, int &out) const",
                                 asMETHOD(IFF, getIFFTarget),
                                 asCALL_THISCALL);

    // Register Platform::getIFFByName
    engine->RegisterObjectMethod("Platform",
                                 "IFF@ getIFFByName(const string &in) const",
                                 asMETHOD(Platform, getIFFByName),
                                 asCALL_THISCALL);

    // Register selectEntityDisplay for IFF
    engine->RegisterObjectMethod("ScriptEngine",
                                 "void selectEntityDisplay(IFF@)",
                                 asMETHODPR(ScriptEngine, selectEntityDisplay, (IFF*), void),
                                 asCALL_THISCALL);

    // ================= GIS AUTOSCRIPT BINDINGS by amjad =================

    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void AddLayer(const string &in)",
        asMETHOD(ScriptEngine, AddLayer),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void SelectLayer(const string &in)",
                                     asMETHOD(ScriptEngine,SelectLayer),
                                     asCALL_THISCALL);
    Q_ASSERT(r >= 0);

    // selected shap
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void setSelectedShape(const string &in)",
                                     asMETHOD(ScriptEngine, setSelectedShape), asCALL_THISCALL); Q_ASSERT(s >= 0);
    Q_ASSERT(r >= 0);

    // Circle
    // r = engine->RegisterObjectMethod("ScriptEngine",
    //                                  "void AddCircle(const string &in, float, float, float)",
    //                                  asMETHOD(ScriptEngine, AddCircle), asCALL_THISCALL); Q_ASSERT(s >= 0);
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void AddCircle(const string &in, float)",
        asMETHOD(ScriptEngine, AddCircle),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    // Rectangle
    // r = engine->RegisterObjectMethod("ScriptEngine",
    //                                  "void AddRectangle(const string &in, float, float, float, float)",
    //                                  asMETHOD(ScriptEngine, AddRectangle), asCALL_THISCALL); Q_ASSERT(s >= 0);
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void AddRectangle(const string &in, float, float)",
        asMETHOD(ScriptEngine, AddRectangle),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    // Polygon
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void AddPolygon(const string &in, array<string>@)",
                                     asMETHOD(ScriptEngine, AddPolygon), asCALL_THISCALL); Q_ASSERT(s >= 0);

    // useCity
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void useCity(const string &in)",
        asMETHOD(ScriptEngine, useCity),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    // cityLocationString
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "string cityLocationString() const",
        asMETHOD(ScriptEngine, cityLocationString),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    // add point (latitude, then longitude)
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void AddPoint(const string &in, float, float)",
                                     asMETHOD(ScriptEngine, AddPoint), asCALL_THISCALL); Q_ASSERT(s >= 0);

    //StartLine
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void StartLine()", asMETHOD(ScriptEngine, StartLine), asCALL_THISCALL);Q_ASSERT(s >= 0);

    //AddLinePoint (latitude, then longitude)
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void AddLinePoint(float, float)", asMETHOD(ScriptEngine, AddLinePoint), asCALL_THISCALL); Q_ASSERT(s >= 0);

    // // FinishLine
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void FinishLine()", asMETHOD(ScriptEngine, FinishLine), asCALL_THISCALL); Q_ASSERT(s >= 0);

    qDebug() << "[OK]  wrapper methods registered successfully";

    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void onBitmapSelected(const string &in, float, float)",
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

    // User-selected bitmap from disk (latitude, then longitude)
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void onBitmapImageSelected(const string &in, float, float)",
        asMETHOD(ScriptEngine, onBitmapImageSelected),
        asCALL_THISCALL
        ); Q_ASSERT(r >= 0);
    qDebug() << "[OK] Bitmap wrapper methods registered successfully";

    // ToggleAirbases
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void ToggleAirbases(bool)",
        asMETHOD(ScriptEngine, ToggleAirbases),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    // ImportGeoJsonLayer
    // r = engine->RegisterObjectMethod("ScriptEngine",
    //                                  "void ImportGeoJsonLayer(const string &in)",
    //                                  asMETHOD(ScriptEngine, ImportGeoJsonLayer),
    //                                  asCALL_THISCALL); Q_ASSERT(r >= 0);

    // ToggleGeoJsonLayer
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void ToggleGeoJsonLayer(const string &in, bool)",
                                     asMETHOD(ScriptEngine, ToggleGeoJsonLayer),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    //StartDistanceMeasurement
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void StartDistanceMeasurement()",
                                     asMETHOD(ScriptEngine, StartDistanceMeasurement),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    // AddMeasurePoint (latitude, then longitude)
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void AddMeasurePoint(double, double)",
                                     asMETHOD(ScriptEngine, AddMeasurePoint),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    // GetLastSegmentDistance
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "double GetLastSegmentDistance()",
                                     asMETHOD(ScriptEngine, GetLastSegmentDistance),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    // GetTotalDistance
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "double GetTotalDistance()",
                                     asMETHOD(ScriptEngine, GetTotalDistance),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    // SetMeasurementUnit
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void SetMeasurementUnit(const string &in)",
                                     asMETHOD(ScriptEngine, SetMeasurementUnit),
                                     asCALL_THISCALL);
    Q_ASSERT(r >= 0);

    // switch map
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void SwitchMap(const string &in)",
                                     asMETHOD(ScriptEngine, SwitchMap),
                                     asCALL_THISCALL);
    Q_ASSERT(r >= 0);

    // switch coordinate system
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void switchCoordinateSystem(const string &in)",
                                     asMETHOD(ScriptEngine, switchCoordinateSystem),
                                     asCALL_THISCALL);
    Q_ASSERT(r >= 0);

    // move shape (latitude, then longitude)
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void moveShape(const string &in, double, double)",
        asMETHODPR(
            ScriptEngine,
            moveShape,
            (const std::string&, double, double),
            void
            ),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void rotateShape(const string &in, double)",
        asMETHODPR(
            ScriptEngine,
            rotateShape,
            (const std::string&, double),
            void
            ),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);


    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void showShapeHistory(const string &in)",
        asMETHOD(ScriptEngine, showShapeHistory),
        asCALL_THISCALL
        );

    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void hideShapeHistory()",
        asMETHOD(ScriptEngine, hideShapeHistory),
        asCALL_THISCALL
        );

    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void restoreShapeHistory(const string &in)",
        asMETHOD(ScriptEngine, restoreShapeHistory),
        asCALL_THISCALL
        );

    r = engine->RegisterObjectMethod( "ScriptEngine","void addText(const string &in, double, double)",asMETHOD(ScriptEngine, addText), asCALL_THISCALL); Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("ScriptEngine","void addShapeProperties(const string &in, int, int, int, int)", asMETHOD(ScriptEngine, addShapeProperties),asCALL_THISCALL);

    r = engine->RegisterObjectMethod("ScriptEngine","void deleteshape(const string &in)", asMETHOD(ScriptEngine, deleteshape),asCALL_THISCALL);

    // ================= GIS AUTOSCRIPT BINDINGS END=================

    //=================== GENERAL AUTOSCRIPT BINDINGS ===============


    e = new MyObj();
    e->x = 10;
    e->y = 20;


    mod = engine->GetModule("MyModule", asGM_ALWAYS_CREATE);
    // mod->AddScriptSection("script", script);

    ctx = engine->CreateContext();

    ctx->SetLineCallback(asMETHOD(ScriptEngine, lineCallback), this, asCALL_THISCALL);   // <-- add ////////////////////////////////
}

// ScriptEngine::~ScriptEngine()
// {
//     delete gisEngine;
//     gisEngine = nullptr;
// }

ScriptEngine::~ScriptEngine()
{
    // Invalidate all pending sleep callbacks
    ++scriptGeneration;

    scriptRunning = false;

    // Stop script execution
    if (ctx)
    {
        int state = ctx->GetState();

        if (state == asEXECUTION_ACTIVE ||
            state == asEXECUTION_SUSPENDED)
        {
            ctx->Abort();
        }

        ctx->Unprepare();
        ctx->Release();
        ctx = nullptr;
    }

    func = nullptr;
    mod = nullptr;

    if (engine)
    {
        engine->ShutDownAndRelease();
        engine = nullptr;
    }

    delete gisEngine;
    gisEngine = nullptr;

    delete e;
    e = nullptr;
}

void ScriptEngine::setHierarchy(Hierarchy *hiers, HierarchyTree *tr, SceneRenderer *rend)
{
    hierarchy = hiers;
    tree = tr;
    render = rend;
}

// bool ScriptEngine::loadAndCompileScript(QString scriptContent)
// {
//     // ---------------------------------------------------------
//     // INVALIDATE ALL PREVIOUS SCRIPT CALLBACKS
//     // ---------------------------------------------------------
//     ++scriptGeneration;
//     scriptRunning = false;

//     // ---------------------------------------------------------
//     // STOP OLD ANGELSCRIPT CONTEXT
//     // ---------------------------------------------------------
//     if (ctx)
//     {
//         int state = ctx->GetState();

//         if (state == asEXECUTION_ACTIVE ||
//             state == asEXECUTION_SUSPENDED)
//         {
//             qDebug() << "[ScriptEngine] Aborting previous script context";
//             ctx->Abort();
//         }

//         ctx->Unprepare();
//     }

//     // Old function pointer is no longer valid
//     func = nullptr;

//     // ---------------------------------------------------------
//     // DISCARD OLD MODULE
//     // ---------------------------------------------------------
//     if (engine)
//     {
//         engine->DiscardModule("MyModule");
//     }

//     // ---------------------------------------------------------
//     // CREATE NEW MODULE
//     // ---------------------------------------------------------
//     mod = engine->GetModule("MyModule", asGM_ALWAYS_CREATE);

//     if (!mod)
//     {
//         qWarning() << "[ScriptEngine] Failed to create script module";
//         return false;
//     }

//     QByteArray source = scriptContent.toUtf8();

//     int r = mod->AddScriptSection(
//         "script",
//         source.constData()
//         );

//     if (r < 0)
//     {
//         qWarning() << "[ScriptEngine] Failed to add script section";
//         return false;
//     }

//     // ---------------------------------------------------------
//     // BUILD
//     // ---------------------------------------------------------
//     int s = mod->Build();

//     if (s < 0)
//     {
//         qDebug() << "[ScriptEngine] Failed to compile script!";
//         return false;
//     }

//     // ---------------------------------------------------------
//     // GET MAIN
//     // ---------------------------------------------------------
//     func = mod->GetFunctionByName("main");

//     if (!func)
//     {
//         qDebug() << "[ScriptEngine] Function main() not found!";
//         return false;
//     }

//     // ---------------------------------------------------------
//     // PREPARE CONTEXT
//     // ---------------------------------------------------------
//     if (!ctx)
//     {
//         qWarning() << "[ScriptEngine] Context is null";
//         return false;
//     }

//     int prepareResult = ctx->Prepare(func);

//     if (prepareResult < 0)
//     {
//         qWarning() << "[ScriptEngine] Failed to prepare context:"
//                    << prepareResult;

//         func = nullptr;
//         return false;
//     }

//     ctx->SetArgObject(0, this);

//     // ---------------------------------------------------------
//     // START NEW SCRIPT GENERATION
//     // ---------------------------------------------------------
//     scriptRunning = true;

//     run();

//     return true;
// }

bool ScriptEngine::loadAndCompileScript(QString scriptContent)
{
    const bool wasRunning = scriptRunning;

    ++scriptGeneration;
    scriptRunning = false;
    m_timedOut = false;
    m_engineMessages.clear();

    if (ctx)
    {
        const int state = ctx->GetState();
        if (state == asEXECUTION_ACTIVE || state == asEXECUTION_SUSPENDED)
        {
            qDebug() << "[ScriptEngine] Aborting previous script context";
            ctx->Abort();
        }
        ctx->Unprepare();
    }
    func = nullptr;

    if (wasRunning)
        emit scriptFinished(false, "ABORTED", "Replaced by a new script");

    auto fail = [this](const QString& msg) {
        QString full = msg;
        if (!m_engineMessages.isEmpty())
            full += "\n" + m_engineMessages.join("\n");
        qWarning().noquote() << "[ScriptEngine]" << full;
        emit scriptFinished(false, "ERROR", full);
        return false;
    };

    if (engine) engine->DiscardModule("MyModule");

    mod = engine->GetModule("MyModule", asGM_ALWAYS_CREATE);
    if (!mod) return fail("Failed to create script module");

    const QByteArray source = scriptContent.toUtf8();
    if (mod->AddScriptSection("script", source.constData()) < 0)
        return fail("Failed to add script section");

    if (mod->Build() < 0)
        return fail("Compile failed");

    func = mod->GetFunctionByName("main");
    if (!func)
        return fail("Function main() not found");

    if (!ctx)
        return fail("Context is null");

    const int prepareResult = ctx->Prepare(func);
    if (prepareResult < 0) {
        func = nullptr;
        return fail(QString("Failed to prepare context (%1)").arg(prepareResult));
    }

    ctx->SetArgObject(0, this);

    scriptRunning = true;
    m_totalTimer.restart();
    run();

    return true;      // "started" - the real result arrives via scriptFinished
}


// void ScriptEngine::run()
// {
//     if (!ctx || !func)
//     {
//         qDebug() << "[ScriptEngine] Invalid context/function";
//         return;
//     }

//     if (!scriptRunning)
//     {
//         qDebug() << "[ScriptEngine] Script execution is not active";
//         return;
//     }

//     qDebug() << "[ScriptEngine] Executing:"
//              << func->GetDeclaration();

//     int r = ctx->Execute();

//     qDebug() << "[ScriptEngine] Execute returned:" << r;

//     if (r == asEXECUTION_FINISHED)
//     {
//         qDebug() << "[ScriptEngine] Script finished successfully";

//         scriptRunning = false;

//         return;
//     }

//     if (r == asEXECUTION_SUSPENDED)
//     {
//         qDebug() << "[ScriptEngine] Script suspended at sleep/yield";

//         return;
//     }

//     if (r == asEXECUTION_EXCEPTION)
//     {
//         qDebug() << "[ScriptEngine] Script exception";

//         scriptRunning = false;

//         asIScriptFunction* exceptionFunc =
//             ctx->GetExceptionFunction();

//         if (exceptionFunc)
//         {
//             qDebug() << "[ScriptEngine] Exception function:"
//                      << exceptionFunc->GetDeclaration();
//         }

//         return;
//     }

//     qDebug() << "[ScriptEngine] Script stopped with code:"
//              << r;

//     scriptRunning = false;
// }


void ScriptEngine::run()
{
    if (!ctx || !func)   { qDebug() << "[ScriptEngine] Invalid context/function"; return; }
    if (!scriptRunning)  { qDebug() << "[ScriptEngine] Script execution is not active"; return; }

    qDebug() << "[ScriptEngine] Executing:" << func->GetDeclaration();
    executeContext();
}

void ScriptEngine::executeContext()
{
    if (!ctx || !func || !scriptRunning)
        return;

    const quint64 generation = scriptGeneration;
    m_sliceTimer.restart();

    const int r = ctx->Execute();

    // stopScript()/new script happened while we were executing: it already emitted its signal
    if (generation != scriptGeneration)
        return;

    handleExecuteResult(r);
}

void ScriptEngine::handleExecuteResult(int r)
{
    auto withMessages = [this](QString s) {
        if (!m_engineMessages.isEmpty())
            s += "\n" + m_engineMessages.join("\n");
        return s;
    };

    switch (r)
    {
    case asEXECUTION_FINISHED:
        scriptRunning = false;
        qDebug() << "[ScriptEngine] Script finished successfully";
        emit scriptFinished(true, "DONE", QString());
        break;

    case asEXECUTION_SUSPENDED:
        qDebug() << "[ScriptEngine] Script suspended (sleep/yield)";
        break;

    case asEXECUTION_EXCEPTION:
    {
        scriptRunning = false;
        asIScriptFunction* f = ctx->GetExceptionFunction();
        const QString msg = QString("Exception: %1 (line %2) in %3")
                                .arg(QString::fromUtf8(ctx->GetExceptionString()))
                                .arg(ctx->GetExceptionLineNumber())
                                .arg(f ? QString::fromUtf8(f->GetDeclaration()) : QString("?"));
        qWarning().noquote() << "[ScriptEngine]" << msg;
        emit scriptFinished(false, "ERROR", withMessages(msg));
        break;
    }

    case asEXECUTION_ABORTED:
        scriptRunning = false;
        if (m_timedOut)
            emit scriptFinished(false, "ERROR", "Script timed out and was aborted");
        else
            emit scriptFinished(false, "ABORTED", "Script aborted");
        break;

    default:
        scriptRunning = false;
        emit scriptFinished(false, "ERROR", QString("Script stopped with code %1").arg(r));
        break;
    }
}

void ScriptEngine::lineCallback(asIScriptContext* c)
{
    if ((m_maxSliceMs > 0 && m_sliceTimer.isValid() && m_sliceTimer.elapsed() > m_maxSliceMs) ||
        (m_maxTotalMs > 0 && m_totalTimer.isValid() && m_totalTimer.elapsed() > m_maxTotalMs))
    {
        m_timedOut = true;
        c->Abort();
    }
}
