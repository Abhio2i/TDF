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

// warpper AngelScript entry point
void ScriptEngine::setCanvasSelectedShape(const std::string &shapeName)
{
    if (!gisEngine) {
        qWarning() << "GIS Engine not available";
        return;
    }

    gisEngine->setCanvasSelectedShape(shapeName);
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
    if (!hierarchy ) {
        qDebug() << "[ERROR] Hierarchy not initialized!";
        return nullptr;
    }

    // ✅ Master key access: use unordered_map directly
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
    if (!self->meshRenderer2d)
        self->addComponent("bitmap");

    if (!self->meshRenderer2d) return;

    // Direct sprite update (correct for your architecture)
    if (self->meshRenderer2d->Sprite)
        *(self->meshRenderer2d->Sprite) = path;

    // Optional: refresh mesh if needed
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
            captureCanvasScreenshot("Entity")
            );
    }

    if (executionReport.empty()) {
        drawLine("No AutoScript operations were recorded.");
    } else {
        // ✅ SEPARATE CANVAS AND RADAR SCREENSHOTS
        std::vector<ReportEvent> canvasEntries;
        std::vector<ReportEvent> radarEntries;
        std::vector<ReportEvent> otherEntries;

        for (const auto &e : executionReport) {
            if (e.category == ReportCategory::SYSTEM)
                continue;

            if (e.action.contains("Tactical Canvas", Qt::CaseInsensitive)) {
                canvasEntries.push_back(e);
            } else if (e.action.contains("Sensor Display", Qt::CaseInsensitive)) {
                radarEntries.push_back(e);
            } else {
                otherEntries.push_back(e);
            }
        }

        // ✅ 1. CANVAS FIRST (ONLY ONE)
        if (!canvasEntries.empty()) {
            const auto &e = canvasEntries[0];

            // Draw header
            painter.setFont(QFont("Arial", 12, QFont::Bold));
            drawLine("📍 TACTICAL CANVAS (CONDITION REACHED)");
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


// canvas capture screenshot
QString ScriptEngine::captureCanvasScreenshot(const QString& tag)
{
    if (!canvas || !canvas->gislib)
        return "";

    // Ensure map tiles & canvas render ho chuke ho
    QCoreApplication::processEvents();
    QThread::msleep(150);
    QCoreApplication::processEvents();

    QWidget* container = canvas->parentWidget();
    if (!container) return "";

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

void ScriptEngine::addSubComponent(
    const std::string& entityId,
    ComponentType type,
    const std::string& name,
    const std::string& data1,
    const std::string& data2,
    const std::string& data3
    ) {
    if (!hierarchy) return;

    hierarchy->addSubComponent(
        QString::fromStdString(entityId),
        type,
        QString::fromStdString(name),
        QString::fromStdString(data1),
        QString::fromStdString(data2),
        QString::fromStdString(data3)
        );
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
    if (!platform->sensors) {
        platform->addComponent("sensors");
    }

    if (!platform->sensors) {
        Console::log("❌ Sensor component could not be created");
        return;
    }

    // IMPORTANT: parent ID is SENSOR COMPONENT ID
    QString sensorComponentID =
        QString::fromStdString(platform->sensors->ID);

    // ✅ data1 = sensorType (CRITICAL)
    h->addSubComponent(
        sensorComponentID,
        ComponentType::SensorProfile,
        QString::fromStdString(subName),
        QString::fromStdString(sensorType)
        );

    Console::log(
        "✅ Sensor subcomponent '" + subName +
        "' [" + sensorType +
        "] added under Sensor (" + platform->sensors->ID + ")"
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
    if (!platform) {
        qWarning() << "Platform is null in addRadioSubComponent";
        return;
    }

    if (!hierarchy) {
        qWarning() << "Hierarchy is null in addRadioSubComponent";
        return;
    }

    // RadioProfile is directly accessible from Platform
    if (!platform->radios) {
        qWarning() << "No RadioProfile component on platform:"
                   << QString::fromStdString(platform->Name);
        return;
    }

    // Add radio subcomponent using the RadioProfile's ID
    QString qSubName = QString::fromStdString(subName);

    hierarchy->addSubComponent(
        QString::fromStdString(platform->radios->ID),
        ComponentType::RadioProfile,
        qSubName,
        "",  // data1 - empty for radio
        "",  // data2 - empty (or template ID if copying from existing)
        ""   // data3 - empty
        );

    qDebug() << "Added radio subcomponent:" << qSubName
             << "to platform:" << QString::fromStdString(platform->Name);
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

    // IFFProfile is directly accessible from Platform
    if (!platform->iffs) {
        qWarning() << "No IFFProfile component on platform:"
                   << QString::fromStdString(platform->Name);
        return;
    }

    // Add IFF subcomponent using the IFFProfile's ID
    QString qSubName = QString::fromStdString(subName);

    hierarchy->addSubComponent(
        QString::fromStdString(platform->iffs->ID),
        ComponentType::IFFProfile,
        qSubName,
        "",  // data1 - empty for IFF
        "",  // data2 - empty (or template ID if copying from existing)
        ""   // data3 - empty
        );

    qDebug() << "Added IFF subcomponent:" << qSubName
             << "to platform:" << QString::fromStdString(platform->Name);
}
void ScriptEngine::selectEntityDisplay(IFF* iff) {
    if (!iff) {
        qWarning() << "selectEntityDisplay: null iff";
        return;
    }

    qDebug() << "Selecting display for IFF:" << QString::fromStdString(iff->Name);

    // ✅ STEP 1: Show Sensors sidebar tab FIRST
    qDebug() << "Step 1: Opening Sensors sidebar";
    emit requestSidebarView("Sensors");

    // ✅ STEP 2: Select the parent entity in hierarchy tree
    if (iff->parentEntity) {
        QString entityId = QString::fromStdString(iff->parentEntity->ID);
        qDebug() << "Step 2: Selecting entity:" << entityId;
        emit requestSelectEntity(entityId);
    } else {
        qWarning() << "IFF has no parentEntity set!";
        return;
    }

    // ✅ STEP 3: Switch to IFF tab (check if uppercase or mixed case)
    qDebug() << "Step 3: Switching to IFF tab";
    emit requestDisplayTab("IFF");  // or "Iff" depending on your tab name
}

void ScriptEngine::canvasCreateVectorLayer(const std::string &layerName)
{
    if (!gisEngine) return;
    gisEngine->canvasCreateVectorLayer(layerName);
}

void ScriptEngine::canvasSelectLayer(const std::string &layerName)
{
    if (!gisEngine) return;
    gisEngine->canvasSelectLayer(layerName);
}

// void ScriptEngine::canvasRenameShape(const std::string &newName)
// {
//     if (!gisEngine) return;
//     gisEngine->canvasRenameShape(newName);
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

// Set canvas widget used for drawing GIS elements
void ScriptEngine::setCanvas(CanvasWidget* c)
{
    canvas = c; // Store canvas reference

    // Pass canvas to GIS engine if available
    if (gisEngine)
        gisEngine->setCanvas(c);
}

// Set GIS library reference
void ScriptEngine::setGIS(GISlib* g)
{
    gis = g; // Store GIS library instance

    if (gisEngine)
        gisEngine->setGIS(g);
}

// Set container widget for map canvas
void ScriptEngine::setMapCanvasContainer(QWidget* w)
{
    // Store map canvas container widget
    mapCanvasContainer = w;

    if (gisEngine)
        gisEngine->setMapCanvasContainer(w);
}

// Add a circular shape on canvas
void ScriptEngine::canvasAddCircle(const std::string &name, float radius)
{
    if (!gisEngine) return;  // Exit if GIS engine is not initialized
    gisEngine->canvasAddCircle(name, radius);  // Delegate circle creation to GIS engine
}


// Add a rectangular shape on canvas
void ScriptEngine::canvasAddRectangle(const std::string &name, float w, float h)
{
    if (!gisEngine) return; // Exit if GIS engine is not initialized
    gisEngine->canvasAddRectangle(name, w, h); // Exit if GIS engine is not initialized
}

// Add a polygon using a list of points
void ScriptEngine::canvasAddPolygon(const std::string &name, CScriptArray* pts)
{
    if (!gisEngine) return;
    gisEngine->canvasAddPolygon(name, pts);
}

// ---------------- Add line APIs ----------------
// Start drawing a polyline
void ScriptEngine::canvasStartLine()
{
    if (!gisEngine) return;
    gisEngine->canvasStartLine();  // Initialize line drawing
}

// Add a point to the currently drawn line
void ScriptEngine::canvasAddLinePoint(float lon, float lat)
{
    if (!gisEngine) return;
    gisEngine->canvasAddLinePoint(lon, lat);
}

// Finish drawing the current line
void ScriptEngine::canvasFinishLine()
{
    if (!gisEngine) return;
    gisEngine->canvasFinishLine();
}

// Add a single point marker on canvas
void ScriptEngine::canvasAddPoint(const std::string &name, float lon, float lat)
{
    if (!gisEngine) return;
    gisEngine->canvasAddPoint(name, lon, lat);
}


// Handle bitmap selection using predefined bitmap type
void ScriptEngine::onBitmapSelected(const std::string &bitmapType, float lon, float lat)
{
    if (!gisEngine) return;
    gisEngine->onBitmapSelected(bitmapType, lon, lat);
}

// Set bitmap image file path
void ScriptEngine::getBitmapImagePath(const std::string &filePath)
{
    if (!gisEngine) return;
    gisEngine->getBitmapImagePath(filePath);
}

// Handle bitmap image selection and placement
void ScriptEngine::onBitmapImageSelected(const std::string &filePath, float lon, float lat)
{
    if (!gisEngine) return;
    gisEngine->onBitmapImageSelected(filePath, lon, lat);
}

// Toggle visibility of airbase markers on canvas
void ScriptEngine::canvasToggleAirbases()
{
    if (!gisEngine) return;
    gisEngine->canvasToggleAirbases();    // Toggle airbase layer visibility
}

// Import a GeoJSON layer into the canvas
void ScriptEngine::canvasImportGeoJsonLayer(const std::string &filePath)
{
    if (!gisEngine) return;
    gisEngine->canvasImportGeoJsonLayer(filePath);
}

// Toggle visibility of a specific GeoJSON layer
void ScriptEngine::canvasToggleGeoJsonLayer(const std::string &layerName, bool visible)
{
    if (!gisEngine) return;

    // Show or hide specified GeoJSON layer
    gisEngine->canvasToggleGeoJsonLayer(layerName, visible);
}

// ---------------- Measurement Distance APIs ----------------
// Start distance measurement mode
void ScriptEngine::canvasStartDistanceMeasurement()
{
    if (!gisEngine) return;
    gisEngine->canvasStartDistanceMeasurement();   // Begin distance measurement
}

// Add a measurement point (lon/lat)
void ScriptEngine::canvasAddMeasurePoint(double lon, double lat)
{
    if (!gisEngine) return;
    gisEngine->canvasAddMeasurePoint(lon, lat);
}

// Get distance of the last measured segment
double ScriptEngine::canvasGetLastSegmentDistance()
{
    // Return zero if GIS engine is not available
    if (!gisEngine) return 0.0;
    return gisEngine->canvasGetLastSegmentDistance();
}

// Get total measured distance
double ScriptEngine::canvasGetTotalDistance()
{
    if (!gisEngine) return 0.0;
    return gisEngine->canvasGetTotalDistance();
}

// Set unit for distance measurement (e.g., km, m, nm)
void ScriptEngine::canvasSetMeasurementUnit(const std::string &unit)
{
    if (!gisEngine) return;
    gisEngine->canvasSetMeasurementUnit(unit);    // Set measurement unit in GIS engine
}

void ScriptEngine :: canvasSwitchMap(const std::string& mapName){
    if(!gisEngine) return;
    gisEngine->canvasSwitchMap(mapName);
}

void ScriptEngine :: switchCoordinateSystem(const std::string& system){
    if(!gisEngine) return;
    gisEngine->switchCoordinateSystem(system);
}

void ScriptEngine::moveShape(const std::string& shapeName,
                             double lon, double lat)
{
    if (!gisEngine) return;
    gisEngine->moveShape(shapeName, lon, lat);
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

void ScriptEngine::addText(const std::string& text, double lon, double lat)
{
    if (!gisEngine) return;
    gisEngine->addText(text, lon, lat);
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
        return captureCanvasScreenshot(tag);
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
        "void addWaypoint(float lat, float alt, float lon)",
        asMETHODPR(Trajectory, addWaypoint, (float, float, float), void),
        asCALL_THISCALL);
    assert(r >= 0);

    // New 4‑parameter version with sensor flag
    r = engine->RegisterObjectMethod(
        "Trajectory",
        "void addWaypoint(float lat, float alt, float lon, bool activateSensor)",
        asMETHODPR(Trajectory, addWaypoint, (float, float, float, bool), void),
        asCALL_THISCALL);
    assert(r >= 0);
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

    s = engine->RegisterObjectProperty("Platform", "Transform@ transform", asOFFSET(Platform, transform)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Platform", "DynamicModel@ dynamicModel", asOFFSET(Platform, dynamicModel)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Platform", "Trajectory@ trajectory", asOFFSET(Platform, trajectory)); Q_ASSERT(s >= 0);
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
        "void canvasCreateVectorLayer(const string &in)",
        asMETHOD(ScriptEngine, canvasCreateVectorLayer),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasSelectLayer(const string &in)",
                                     asMETHOD(ScriptEngineGIS,canvasSelectLayer),
                                     asCALL_THISCALL);
    Q_ASSERT(r >= 0);

    // selected shap
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void setCanvasSelectedShape(const string &in)",
                                     asMETHOD(ScriptEngine, setCanvasSelectedShape), asCALL_THISCALL); Q_ASSERT(s >= 0);
    Q_ASSERT(r >= 0);

    // Circle
    // r = engine->RegisterObjectMethod("ScriptEngine",
    //                                  "void canvasAddCircle(const string &in, float, float, float)",
    //                                  asMETHOD(ScriptEngine, canvasAddCircle), asCALL_THISCALL); Q_ASSERT(s >= 0);
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void canvasAddCircle(const string &in, float)",
        asMETHOD(ScriptEngine, canvasAddCircle),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    // Rectangle
    // r = engine->RegisterObjectMethod("ScriptEngine",
    //                                  "void canvasAddRectangle(const string &in, float, float, float, float)",
    //                                  asMETHOD(ScriptEngine, canvasAddRectangle), asCALL_THISCALL); Q_ASSERT(s >= 0);
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void canvasAddRectangle(const string &in, float, float)",
        asMETHOD(ScriptEngine, canvasAddRectangle),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    // Polygon
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddPolygon(const string &in, array<string>@)",
                                     asMETHOD(ScriptEngine, canvasAddPolygon), asCALL_THISCALL); Q_ASSERT(s >= 0);

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

    // add point
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddPoint(const string &in, float, float)",
                                     asMETHOD(ScriptEngine, canvasAddPoint), asCALL_THISCALL); Q_ASSERT(s >= 0);

    //canvasStartLine
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasStartLine()", asMETHOD(ScriptEngine, canvasStartLine), asCALL_THISCALL);Q_ASSERT(s >= 0);
    //canvasAddLinePoint
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddLinePoint(float, float)", asMETHOD(ScriptEngine, canvasAddLinePoint), asCALL_THISCALL); Q_ASSERT(s >= 0);

    // // canvasFinishLine
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasFinishLine()", asMETHOD(ScriptEngine, canvasFinishLine), asCALL_THISCALL); Q_ASSERT(s >= 0);

    qDebug() << "[OK] Canvas wrapper methods registered successfully";

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

    // User-selected bitmap from disk
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void onBitmapImageSelected(const string &in, float, float)",
        asMETHOD(ScriptEngine, onBitmapImageSelected),
        asCALL_THISCALL
        ); Q_ASSERT(r >= 0);
    qDebug() << "[OK] Bitmap wrapper methods registered successfully";

    // canvasToggleAirbases
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void canvasToggleAirbases()",
        asMETHOD(ScriptEngine, canvasToggleAirbases),
        asCALL_THISCALL
        );
    Q_ASSERT(r >= 0);

    // canvasImportGeoJsonLayer
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasImportGeoJsonLayer(const string &in)",
                                     asMETHOD(ScriptEngine, canvasImportGeoJsonLayer),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    // canvasToggleGeoJsonLayer
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasToggleGeoJsonLayer(const string &in, bool)",
                                     asMETHOD(ScriptEngine, canvasToggleGeoJsonLayer),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    //canvasStartDistanceMeasurement
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasStartDistanceMeasurement()",
                                     asMETHOD(ScriptEngine, canvasStartDistanceMeasurement),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    // canvasAddMeasurePoint
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddMeasurePoint(double, double)",
                                     asMETHOD(ScriptEngine, canvasAddMeasurePoint),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    // canvasGetLastSegmentDistance
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "double canvasGetLastSegmentDistance()",
                                     asMETHOD(ScriptEngine, canvasGetLastSegmentDistance),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    // canvasGetTotalDistance
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "double canvasGetTotalDistance()",
                                     asMETHOD(ScriptEngine, canvasGetTotalDistance),
                                     asCALL_THISCALL); Q_ASSERT(r >= 0);

    // canvasSetMeasurementUnit
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasSetMeasurementUnit(const string &in)",
                                     asMETHOD(ScriptEngine, canvasSetMeasurementUnit),
                                     asCALL_THISCALL);
    Q_ASSERT(r >= 0);

    // switch map
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasSwitchMap(const string &in)",
                                     asMETHOD(ScriptEngine, canvasSwitchMap),
                                     asCALL_THISCALL);
    Q_ASSERT(r >= 0);

    // switch coordinate system
    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void switchCoordinateSystem(const string &in)",
                                     asMETHOD(ScriptEngine, switchCoordinateSystem),
                                     asCALL_THISCALL);
    Q_ASSERT(r >= 0);

    // move shape
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
}

ScriptEngine::~ScriptEngine()
{
    delete gisEngine;
    gisEngine = nullptr;
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

    return true;
}

void ScriptEngine::run()
{
    if (!ctx || !func)
        return;

    //ctx->Prepare(func);
    ctx->Execute();
}
