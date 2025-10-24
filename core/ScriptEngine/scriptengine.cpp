// #include "scriptengine.h"
// #include "qtimer.h"
// #include <angelscript.h>
// #include <angelscript/add_on/scriptstdstring/scriptstdstring.h>
// #include <QMetaObject>
// #include <QDebug>
// #include <string>
// #include <QObject>
// #include <QVector3D>
// #include "core/Hierarchy/entity.h"
// #include <QPointF>
// #include <cmath>
// #include <cstdlib>
// #include <angelscript/add_on/scriptarray/scriptarray.h>  // make sure included
// #include "core/Hierarchy/hierarchy.h"
// #include "GUI/Tacticaldisplay/Gis/gislib.h"
// // Random number generator
// static float Math_Random()
// {
//     return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
// }

// // Trigonometric functions
// static float Math_Sin(float angle) { return sin(angle); }
// static float Math_Cos(float angle) { return cos(angle); }
// static float Math_Tan(float angle) { return tan(angle); }
// static float Math_ASin(float val) { return asin(val); }
// static float Math_ACos(float val) { return acos(val); }
// static float Math_ATan(float val) { return atan(val); }
// static float Math_ATan2(float y, float x) { return atan2(y, x); }

// // Exponential and power functions
// static float Math_Pow(float base, float exp) { return pow(base, exp); }
// static float Math_Sqrt(float val) { return sqrt(val); }
// static float Math_Exp(float val) { return exp(val); }
// static float Math_Log(float val) { return log(val); }
// static float Math_Log10(float val) { return log10(val); }

// // Rounding and absolute value functions
// static float Math_Abs(float val) { return abs(val); }
// static float Math_Floor(float val) { return floor(val); }
// static float Math_Ceil(float val) { return ceil(val); }
// static float Math_Round(float val) { return round(val); }

// // Constant values
// static const float Math_PI = 3.14159265359f;

// void MessageCallback(const asSMessageInfo *msg, void *param)
// {
//     const char *type = "ERR";
//     if (msg->type == asMSGTYPE_WARNING){
//         type = "WARN";
//         qWarning() << msg->section << "(" << msg->row << "," << msg->col << "):" << type << ":" << msg->message;
//     }else if (msg->type == asMSGTYPE_INFORMATION){
//         type = "INFO";
//         qDebug() << msg->section << "(" << msg->row << "," << msg->col << "):" << type << ":" << msg->message;
//     }else if (msg->type == asMSGTYPE_ERROR){
//         type = "ERROR";
//         qDebug() << msg->section << "(" << msg->row << "," << msg->col << "):" << type << ":" << msg->message;
//     }

// }

// void Print(const std::string &msg)
// {
//     qDebug() << "[AngelScript]:" << QString::fromStdString(msg);
// }



// ProfileCategaory* ScriptEngine::addProfiles(const std::string &name)
// {
//     if (!hierarchy) {
//         qDebug() << "[ERROR] Hierarchy not set in ScriptEngine::addProfiles!";
//         return nullptr;
//     }

//     ProfileCategaory* profile = hierarchy->addProfileCategaory(QString::fromStdString(name));
//     if (!profile) {
//         qDebug() << "[ERROR] Failed to create ProfileCategaory!";
//         return nullptr;
//     }

//     profile->setProfileType(Constants::EntityType::Platform);
//     qDebug() << "[OK] Created Profile:" << QString::fromStdString(name);
//     return profile;
// }

// Folder* ScriptEngine::addFolder(const std::string &Id, const std::string &name, bool &profile)
// {
//     if (!hierarchy) {
//         qDebug() << "[ERROR] Hierarchy not set in ScriptEngine::addFolder!";
//         return nullptr;
//     }

//     Folder* folder = hierarchy->addFolder(QString::fromStdString(Id),
//                                           QString::fromStdString(name),
//                                           profile);
//     if (!folder) {
//         qDebug() << "[ERROR] Failed to create Folder under ID:" << QString::fromStdString(Id);
//         return nullptr;
//     }

//     qDebug() << "[OK] Created Folder:" << QString::fromStdString(name);
//     return folder;
// }


// Platform* ScriptEngine::addEntity(const std::string &Id, const std::string &name, bool &profile)
// {
//     if (!hierarchy) {
//         qDebug() << "[ERROR] Hierarchy not set in ScriptEngine::addEntity!";
//         return nullptr;
//     }

//     Entity* rawEntity = hierarchy->addEntity(QString::fromStdString(Id),
//                                              QString::fromStdString(name),
//                                              profile);
//     if (!rawEntity) {
//         qDebug() << "[ERROR] Failed to create Entity under ID:" << QString::fromStdString(Id);
//         return nullptr;
//     }

//     Platform* entity = static_cast<Platform*>(rawEntity);
//     hierarchy->addComponent(QString::fromStdString(entity->ID), "dynamicModel");
//     hierarchy->addComponent(QString::fromStdString(entity->ID), "meshRenderer2d");
//     hierarchy->addComponent(QString::fromStdString(entity->ID), "trajectory");

//     qDebug() << "[OK] Created Entity:" << QString::fromStdString(name);
//     return entity;
// }

// void ScriptEngine::addComponent(const std::string &Id, const std::string &name)
// {
//     if (!hierarchy) {
//         qDebug() << "[ERROR] Hierarchy not set in ScriptEngine::addComponent!";
//         return;
//     }

//     if (Id.empty() || name.empty()) {
//         qDebug() << "[ERROR] Invalid arguments in addComponent (Id or name empty)";
//         return;
//     }

//     hierarchy->addComponent(QString::fromStdString(Id), QString::fromStdString(name));
//     qDebug() << "[OK] Added Component:" << QString::fromStdString(name)
//              << "to Entity ID:" << QString::fromStdString(Id);
// }



// // scriptengine.cpp
// void ScriptEngine::ScriptSleep(int milliseconds)
// {
//     ctx->Suspend();
//     QTimer::singleShot(milliseconds, [this]() {
//         this->ctx->Execute();
//     });
// }

// void ScriptEngine::renderscene(){
//     if (render){
//         emit render->Render(0.01);
//     }
// }

// // Get a single entity by ID from AngelScript
// Entity* ScriptEngine::getEntityById(const std::string &id)
// {
//     if (!hierarchy || !hierarchy->Entities) {
//         qDebug() << "[ERROR] Hierarchy not initialized!";
//         return nullptr;
//     }

//     // ✅ Master key access: use unordered_map directly
//     auto it = hierarchy->Entities->find(id);
//     if (it != hierarchy->Entities->end()) {
//         Entity* ent = it->second;
//         qDebug() << "[OK] getEntityById found Entity:" << QString::fromStdString(ent->Name)
//                  << "for ID:" << QString::fromStdString(id);
//         return ent;
//     }

//     qDebug() << "[WARN] getEntityById: No entity found for ID:" << QString::fromStdString(id);
//     return nullptr;
// }
// #include <sstream>   // for std::ostringstream

// // Return all entities filtered by type
// CScriptArray* ScriptEngine::findEntitiesByType(int typeId) {
//     if (!hierarchy || !hierarchy->Entities) return nullptr;

//     // Create array<Entity@> type for AngelScript
//     asITypeInfo* arrayEntityType = engine->GetTypeInfoByDecl("array<Entity@>");
//     if (!arrayEntityType) return nullptr;

//     CScriptArray* (*CreateArray)(asITypeInfo*, asUINT) = &CScriptArray::Create;
//     CScriptArray* arr = CreateArray(arrayEntityType, 0);

//     std::ostringstream names;
//     size_t count = 0;

//     for (auto& pair : *(hierarchy->Entities)) {
//         Entity* ent = pair.second;
//         if (ent && ent->type == typeId) {
//             arr->InsertLast(&ent);
//             if (count > 0) names << ", ";
//             names << "\"" << ent->Name << "\"";   // ✅ Use correct field
//             count++;
//         }
//     }

//     // Map int → type string
//     std::string typeName = "Unknown";
//     switch (typeId) {
//     case 0: typeName = "Platform"; break;
//     case 1: typeName = "Radio"; break;
//     case 2: typeName = "Sensor"; break;
//     case 3: typeName = "SpecialZone"; break;
//     case 4: typeName = "Weapon"; break;
//     case 5: typeName = "IFF"; break;
//     case 6: typeName = "Supply"; break;
//     case 7: typeName = "FixedPoint"; break;
//     case 8: typeName = "Formation"; break;
//     }

//     // Pretty print log
//     if (count == 0) {
//         qDebug() << "[EntityFinder] No entities of this type";
//     } else if (count == 1) {
//         qDebug().noquote() << QString("[EntityFinder] Found 1 entity named %1 of \"%2\" Type")
//                                   .arg(QString::fromStdString(names.str()))
//                                   .arg(QString::fromStdString(typeName));
//     } else {
//         qDebug().noquote() << QString("[EntityFinder] Found %1 entities named %2 of \"%3\" Type")
//                                   .arg((int)count)
//                                   .arg(QString::fromStdString(names.str()))
//                                   .arg(QString::fromStdString(typeName));
//     }

//     return arr;
// }


// // Return all entity pointers
// CScriptArray* ScriptEngine::getAllEntities() {
//     if (!hierarchy || !hierarchy->Entities) return nullptr;

//     asITypeInfo* arrayEntityType = engine->GetTypeInfoByDecl("array<Entity@>");
//     if (!arrayEntityType) return nullptr;

//     CScriptArray* (*CreateArray)(asITypeInfo*, asUINT) = &CScriptArray::Create;
//     CScriptArray* arr = CreateArray(arrayEntityType, 0);

//     for (auto& pair : *(hierarchy->Entities)) {
//         Entity* ent = pair.second;
//         arr->InsertLast(&ent);
//     }

//     qDebug() << "[OK] getAllEntities returned" << arr->GetSize() << "entities.";
//     return arr;
// }

// // Return all IDs as strings
// CScriptArray* ScriptEngine::getAllEntityIds() {
//     if (!hierarchy || !hierarchy->Entities) return nullptr;

//     asITypeInfo* arrayStringType = engine->GetTypeInfoByDecl("array<string>");
//     if (!arrayStringType) return nullptr;

//     CScriptArray* (*CreateArray)(asITypeInfo*, asUINT) = &CScriptArray::Create;
//     CScriptArray* arr = CreateArray(arrayStringType, 0);

//     for (auto& pair : *(hierarchy->Entities)) {
//         std::string id = pair.first;
//         arr->InsertLast(&id);
//     }

//     qDebug() << "[OK] getAllEntityIds returned" << arr->GetSize() << "IDs.";
//     return arr;
// }
// // ------------------ Rename Entity Wrapper for AngelScript ------------------
// // Usage in AS: renameEntity("12345", "F16_Falcon");
// // This will rename the entity and update the UI automatically.
// void ScriptEngine::renameEntity(const std::string& id, const std::string& newName)
// {
//     if (!hierarchy) {
//         qDebug() << "[ERROR] Hierarchy not initialized!";
//         return;
//     }

//     // ✅ Use existing master key function to get entity
//     Entity* ent = getEntityById(id);
//     if (!ent) {
//         qDebug() << "[WARN] renameEntity: Entity not found for ID:" << QString::fromStdString(id);
//         return;
//     }

//     // Rename entity
//     QString oldName = QString::fromStdString(ent->Name);
//     ent->Name = newName; // Update name

//     // Reflect change on UI using Hierarchy signals
//     if (hierarchy->Entities->find(id) != hierarchy->Entities->end()) {
//         emit hierarchy->entityRenamed(QString::fromStdString(id), QString::fromStdString(newName));
//     }

//     qDebug() << "[OK] renameEntity: Entity renamed from"
//              << oldName << "to" << QString::fromStdString(newName);
// }
// void ScriptEngine::renameProfile(const std::string& profileID, const std::string& newName)
// {
//     if (!hierarchy) {
//         qDebug() << "[ERROR] Hierarchy not initialized!";
//         return;
//     }

//     // Check if profile exists in the map
//     auto it = hierarchy->ProfileCategories.find(profileID);
//     if (it == hierarchy->ProfileCategories.end()) {
//         qDebug() << "[WARN] renameProfile: No profile found for ID:"
//                  << QString::fromStdString(profileID);
//         return;
//     }

//     // Call Hierarchy rename function
//     hierarchy->renameProfileCategaory(QString::fromStdString(profileID),
//                                       QString::fromStdString(newName));

//     qDebug() << "[OK] renameProfile: Profile"
//              << QString::fromStdString(profileID)
//              << "renamed to"
//              << QString::fromStdString(newName);
// }

// void ScriptEngine::removeEntity(const std::string& parentId, const std::string& entityID)
// {
//     if (!hierarchy) {
//         qDebug() << "[ERROR] Hierarchy not initialized!";
//         return;
//     }

//     // Call Hierarchy's removeEntity using parentId (folder ID)
//     hierarchy->removeEntity(QString::fromStdString(parentId),
//                             QString::fromStdString(entityID),
//                             false); // Profile flag false for entity removal

//     qDebug() << "[OK] removeEntity: Entity"
//              << QString::fromStdString(entityID)
//              << "under parent"
//              << QString::fromStdString(parentId)
//              << "removed successfully.";
// }

// // ScriptEngine.cpp
// void ScriptEngine::removeProfile(const std::string& profileID)
// {
//     if (!hierarchy) {
//         qDebug() << "[ERROR] Hierarchy not initialized!";
//         return;
//     }

//     // Direct call to Hierarchy
//     hierarchy->removeProfileCategaory(QString::fromStdString(profileID));

//     qDebug() << "[OK] removeProfile: Profile"
//              << QString::fromStdString(profileID)
//              << "removed successfully.";
// }
// void ScriptEngine::removeFolder(const std::string& parentId, const std::string& folderID)
// {
//     if (!hierarchy) {
//         qDebug() << "[ERROR] Hierarchy not initialized!";
//         return;
//     }

//     // Call Hierarchy's removeFolder directly
//     hierarchy->removeFolder(QString::fromStdString(parentId),
//                             QString::fromStdString(folderID),
//                             false); // Profile flag false for removal

//     qDebug() << "[OK] removeFolder: Folder"
//              << QString::fromStdString(folderID)
//              << "under parent"
//              << QString::fromStdString(parentId)
//              << "removed successfully.";
// }
// void ScriptEngine::renameFolder(const std::string& folderID, const std::string& newName)
// {
//     Q_ASSERT(hierarchy && "Hierarchy must be initialized before renaming a folder");

//     hierarchy->renameFolder(QString::fromStdString(folderID), QString::fromStdString(newName));

//     qDebug() << "[OK] renameFolder: Folder"
//              << QString::fromStdString(folderID)
//              << "renamed to:"
//              << QString::fromStdString(newName);
// }
// // 1️⃣ Set selected shape type
// void ScriptEngine::setCanvasSelectedShape(const std::string &shapeName)
// {
//     if (!canvas) {
//         qWarning() << "Canvas not set! Cannot set selected shape.";
//         return;
//     }

//     QString qShape = QString::fromStdString(shapeName);
//     canvas->setShapeDrawingMode(true, qShape);
//     qDebug() << "[ScriptEngine] Shape drawing mode enabled for:" << qShape;
// }

// // 2️⃣ Add a line
// void ScriptEngine::canvasAddLine(const std::string &name, CScriptArray* points)
// {
//     if (!canvas || !points) {
//         qWarning() << "Canvas or points array is null.";
//         return;
//     }

//     for (asUINT i = 0; i < points->GetSize(); i++) {
//         QString ptStr = QString::fromStdString(*(std::string*)points->At(i));
//         QStringList xy = ptStr.split(",");
//         if (xy.size() == 2) {
//             QPointF geoPos(xy[0].toFloat(), xy[1].toFloat());
//             bool finalize = (i == points->GetSize() - 1);
//             canvas->drawLine(geoPos, finalize);
//         }
//     }
//     qDebug() << "[ScriptEngine] Line added:" << QString::fromStdString(name);
// }

// // 3️⃣ Add a polygon
// void ScriptEngine::canvasAddPolygon(const std::string &name, CScriptArray* points)
// {
//     if (!canvas || !points) {
//         qWarning() << "Canvas or points array is null.";
//         return;
//     }

//     for (asUINT i = 0; i < points->GetSize(); i++) {
//         QString ptStr = QString::fromStdString(*(std::string*)points->At(i));
//         QStringList xy = ptStr.split(",");
//         if (xy.size() == 2) {
//             QPointF geoPos(xy[0].toFloat(), xy[1].toFloat());
//             bool finalize = (i == points->GetSize() - 1);
//             canvas->drawPolygon(geoPos, finalize);
//         }
//     }
//     qDebug() << "[ScriptEngine] Polygon added:" << QString::fromStdString(name);
// }

// // 4️⃣ Add a rectangle
// void ScriptEngine::canvasAddRectangle(const std::string &name, float x, float y, float width, float height)
// {
//     if (!canvas) {
//         qWarning() << "Canvas not set!";
//         return;
//     }

//     QPointF geoPos(x, y);
//     canvas->drawRectangle(geoPos);
//     qDebug() << "[ScriptEngine] Rectangle added:" << QString::fromStdString(name)
//              << "at (" << x << "," << y << ") size:" << width << "x" << height;
// }

// // 5️⃣ Add a circle
// void ScriptEngine::canvasAddCircle(const std::string &name, float x, float y, float radius)
// {
//     if (!canvas) {
//         qWarning() << "Canvas not set!";
//         return;
//     }

//     QPointF geoPos(x, y);
//     canvas->drawCircle(geoPos);
//     qDebug() << "[ScriptEngine] Circle added:" << QString::fromStdString(name)
//              << "at (" << x << "," << y << ") radius:" << radius;
// }

// // 6️⃣ Add a point
// void ScriptEngine::canvasAddPoint(const std::string &name, float x, float y)
// {
//     if (!canvas) {
//         qWarning() << "Canvas not set!";
//         return;
//     }

//     QPointF geoPos(x, y);
//     canvas->drawPoints(geoPos);
//     qDebug() << "[ScriptEngine] Point added:" << QString::fromStdString(name)
//              << "at (" << x << "," << y << ")";
// }
// // Built-in bitmap placement (dropdown selection)
// void ScriptEngine::onBitmapSelected(const std::string &bitmapType)
// {
//     if (!canvas) return;
//     canvas->onBitmapSelected(QString::fromStdString(bitmapType));
//     qDebug() << "[ScriptEngine] Bitmap placement mode enabled for:" << QString::fromStdString(bitmapType);
// }

// // Get path of built-in bitmap
// std::string ScriptEngine::getBitmapImagePath(const std::string &bitmapType)
// {
//     if (!canvas) return "";
//     QString path = canvas->getBitmapImagePath(QString::fromStdString(bitmapType));
//     return path.toStdString();
// }

// // Place bitmap image from arbitrary file
// void ScriptEngine::onBitmapImageSelected(const std::string &filePath)
// {
//     if (!canvas) return;
//     canvas->onBitmapImageSelected(QString::fromStdString(filePath));
//     qDebug() << "[ScriptEngine] Bitmap image placed from path:" << QString::fromStdString(filePath);
// }
// void ScriptEngine::canvasImportGeoJsonLayer(const std::string &filePath)
// {
//     if (!canvas) return;
//     canvas->importGeoJsonLayer(QString::fromStdString(filePath));
//     qDebug() << "[ScriptEngine] Imported GeoJSON layer from path:" << QString::fromStdString(filePath);
// }

// void ScriptEngine::canvasToggleGeoJsonLayer(const std::string &layerName, bool visible)
// {
//     if (!canvas) return;
//     canvas->onGeoJsonLayerToggled(QString::fromStdString(layerName), visible);
//     qDebug() << "[ScriptEngine] Toggled GeoJSON layer" << QString::fromStdString(layerName)
//              << "to" << (visible ? "visible" : "hidden");
// }
// // ScriptEngine.cpp
// void ScriptEngine::canvasStartDistanceMeasurement() {
//     if (canvas) canvas->startDistanceMeasurement();
// }

// void ScriptEngine::canvasAddMeasurePoint(double lon, double lat) {
//     if (canvas) canvas->addMeasurePoint(lon, lat);
// }

// double ScriptEngine::canvasGetLastSegmentDistance() {
//     return canvas ? canvas->getLastSegmentDistance() : 0.0;
// }

// double ScriptEngine::canvasGetTotalDistance() {
//     return canvas ? canvas->getTotalDistance() : 0.0;
// }

// void ScriptEngine::canvasSetMeasurementUnit(const std::string &unit) {
//     if (canvas) canvas->setMeasurementUnit(QString::fromStdString(unit));
// }
// void ScriptEngine::canvasToggleAirbases() {
//     if (!canvas) return;
//     canvas->onPresetLayerSelected("Airbase");
// }

// ScriptEngine::ScriptEngine()
// {
//     // AngelScript setup
//     engine = asCreateScriptEngine();
//     engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);

//     RegisterStdString(engine);
//     engine->RegisterGlobalFunction("float Random()", asFUNCTION(Math_Random), asCALL_CDECL);

//     // Trigonometric functions
//     engine->RegisterGlobalFunction("float Sin(float)", asFUNCTION(Math_Sin), asCALL_CDECL);
//     engine->RegisterGlobalFunction("float Cos(float)", asFUNCTION(Math_Cos), asCALL_CDECL);
//     engine->RegisterGlobalFunction("float Tan(float)", asFUNCTION(Math_Tan), asCALL_CDECL);
//     engine->RegisterGlobalFunction("float ASin(float)", asFUNCTION(Math_ASin), asCALL_CDECL);
//     engine->RegisterGlobalFunction("float ACos(float)", asFUNCTION(Math_ACos), asCALL_CDECL);
//     engine->RegisterGlobalFunction("float ATan(float)", asFUNCTION(Math_ATan), asCALL_CDECL);
//     engine->RegisterGlobalFunction("float ATan2(float, float)", asFUNCTION(Math_ATan2), asCALL_CDECL);

//     // Exponential and power functions
//     engine->RegisterGlobalFunction("float Pow(float, float)", asFUNCTION(Math_Pow), asCALL_CDECL);
//     engine->RegisterGlobalFunction("float Sqrt(float)", asFUNCTION(Math_Sqrt), asCALL_CDECL);
//     engine->RegisterGlobalFunction("float Exp(float)", asFUNCTION(Math_Exp), asCALL_CDECL);
//     engine->RegisterGlobalFunction("float Log(float)", asFUNCTION(Math_Log), asCALL_CDECL);
//     engine->RegisterGlobalFunction("float Log10(float)", asFUNCTION(Math_Log10), asCALL_CDECL);

//     // Rounding and absolute value functions
//     engine->RegisterGlobalFunction("float Abs(float)", asFUNCTION(Math_Abs), asCALL_CDECL);
//     engine->RegisterGlobalFunction("float Floor(float)", asFUNCTION(Math_Floor), asCALL_CDECL);
//     engine->RegisterGlobalFunction("float Ceil(float)", asFUNCTION(Math_Ceil), asCALL_CDECL);
//     engine->RegisterGlobalFunction("float Round(float)", asFUNCTION(Math_Round), asCALL_CDECL);

//     // Constant
//     engine->RegisterGlobalProperty("const float PI", (void*)&Math_PI);

//     // Register print + profile functions
//     engine->RegisterGlobalFunction("void Print(const string &in)", asFUNCTION(Print), asCALL_CDECL);
//     // engine->RegisterGlobalFunction("void sleep(int milliseconds)", asFUNCTION(ScriptSleep), asCALL_CDECL);

//     int s;
//     s = engine->RegisterObjectType("MyObj", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("MyObj", "float x", asOFFSET(MyObj, x)); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("MyObj", "float y", asOFFSET(MyObj, y)); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("MyObj", "void moveBy(float dx, float dy)", asMETHOD(MyObj, moveBy), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectType("QVector3D", sizeof(QVector3D), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CDA); Q_ASSERT(s >= 0);
//     // 3. Register the property accessors (getters and setters)
//     // Note: RegisterObjectMethod is used for both. The const keyword is important!
//     s = engine->RegisterObjectMethod("QVector3D", "float x() const", asMETHOD(QVector3D, x), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("QVector3D", "void setX(float)", asMETHOD(QVector3D, setX), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectMethod("QVector3D", "float y() const", asMETHOD(QVector3D, y), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("QVector3D", "void setY(float)", asMETHOD(QVector3D, setY), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectMethod("QVector3D", "float z() const", asMETHOD(QVector3D, z), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("QVector3D", "void setZ(float)", asMETHOD(QVector3D, setZ), asCALL_THISCALL); Q_ASSERT(s >= 0);


//     s = engine->RegisterObjectType("QQuaternion", sizeof(QQuaternion), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CDA); Q_ASSERT(s >= 0);
//     // Register methods for QQuaternion (you can add more as needed)
//     s = engine->RegisterObjectMethod("QQuaternion", "QVector3D toEulerAngles()", asMETHOD(QQuaternion, toEulerAngles), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("QQuaternion", "void setX(float)", asMETHOD(QQuaternion, setX), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("QQuaternion", "void setY(float)", asMETHOD(QQuaternion, setY), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("QQuaternion", "void setZ(float)", asMETHOD(QQuaternion, setZ), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     //s = engine->RegisterObjectMethod("QQuaternion", "QQuaternion@ fromEulerAngles(const QVector3D& in)", asFUNCTIONPR(QQuaternion::fromEulerAngles, (const QVector3D&), QQuaternion), asCALL_CDECL); Q_ASSERT(s >= 0);


//     s = engine->RegisterObjectType("QTransform", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("QTransform", "QVector3D translation()", asMETHOD(Qt3DCore::QTransform, translation), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("QTransform", "void setTranslation(const QVector3D &in)", asMETHOD(Qt3DCore::QTransform, setTranslation), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("QTransform", "QQuaternion rotation()", asMETHOD(Qt3DCore::QTransform, rotation), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("QTransform", "void setRotation(const QQuaternion &in)", asMETHOD(Qt3DCore::QTransform, setRotation), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("QTransform", "QVector3D scale3D()", asMETHOD(Qt3DCore::QTransform, scale3D), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("QTransform", "void setScale3D(const QVector3D &in)", asMETHOD(Qt3DCore::QTransform, setScale3D), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     // 2. Register the Transform class as a reference-counted type
//     s = engine->RegisterObjectType("Transform", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("Transform", "QTransform@ matrix", asOFFSET(Transform, matrix)); Q_ASSERT(s >= 0);
//     // // Register the getter and setter methods
//     s = engine->RegisterObjectMethod("Transform", "QVector3D translation()", asMETHOD(Transform, translation), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Transform", "void setTranslation(const QVector3D& in)", asMETHOD(Transform, setTranslation), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Transform", "void addTranslation(const QVector3D& in)", asMETHOD(Transform, addTranslation), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectMethod("Transform", "QQuaternion rotation() const", asMETHOD(Transform, rotation), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Transform", "void setRotation(const QQuaternion& in)", asMETHOD(Transform, setRotation), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectMethod("Transform", "QVector3D scale3D() const", asMETHOD(Transform, scale3D), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Transform", "void setScale3D(const QVector3D& in)", asMETHOD(Transform, setScale3D), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectMethod("Transform", "QVector3D toEulerAngles() const", asMETHOD(Transform, toEulerAngles), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Transform", "void setFromEulerAngles(const QVector3D& in)", asMETHOD(Transform, setFromEulerAngles), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectMethod("Transform", "QVector3D forward()", asMETHOD(Transform, forward), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Transform", "QVector3D up()", asMETHOD(Transform, up), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Transform", "QVector3D right()", asMETHOD(Transform, right), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Transform", "QVector3D back()", asMETHOD(Transform, back), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Transform", "QVector3D left()", asMETHOD(Transform, left), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Transform", "QVector3D down()", asMETHOD(Transform, down), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectMethod("Transform", "QVector3D TransformDirection(const QVector3D& in)", asMETHOD(Transform, TransformDirection), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Transform", "QVector3D inverseTransformDirection(const QVector3D& in)", asMETHOD(Transform, inverseTransformDirection), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectType("Trajectory", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Trajectory", "void addWaypoint(float,float,float)", asMETHOD(Trajectory, addWaypoint), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectType("ProfileCategaory", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("ProfileCategaory", "string id", asOFFSET(ProfileCategaory, ID)); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("ProfileCategaory", "string name", asOFFSET(ProfileCategaory, Name)); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectType("Folder", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("Folder", "string id", asOFFSET(Folder, ID)); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("Folder", "string name", asOFFSET(Folder, Name)); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectType("Platform", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("Platform", "string id", asOFFSET(Entity, ID)); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("Platform", "string name", asOFFSET(Entity, Name)); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("Platform", "Transform@ transform", asOFFSET(Platform, transform)); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("Platform", "Trajectory@ trajectory", asOFFSET(Platform, trajectory)); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Platform", "void addParam(const string &in,const string &in)", asMETHOD(Platform, addParam), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Platform", "void editParam(const string &in,const string &in)", asMETHOD(Platform, editParam), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Platform", "string getParam(const string &in)", asMETHOD(Platform, getParam), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Platform", "void removeParam(const string &in)", asMETHOD(Platform, removeParam), asCALL_THISCALL); Q_ASSERT(s >= 0);


//     s = engine->RegisterObjectType("ScriptEngine", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine", "void render()", asMETHOD(ScriptEngine, renderscene), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine", "void sleep(int milliseconds)", asMETHOD(ScriptEngine, ScriptSleep), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine", "ProfileCategaory@ NewProfile(const string &in)", asMETHOD(ScriptEngine, addProfiles), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine", "Platform@ addEntity(const string &in,const string &in, const bool &in)", asMETHOD(ScriptEngine, addEntity), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine", "Folder@ addFolder(const string &in,const string &in, const bool &in)", asMETHOD(ScriptEngine, addFolder), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine", "void addComponent(const string &in,const string &in)", asMETHOD(ScriptEngine, addComponent), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectType("Entity", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("Entity", "string id", asOFFSET(Entity, ID)); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("Entity", "string name", asOFFSET(Entity, Name)); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("Entity", "string parentId", asOFFSET(Entity, parentID)); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectProperty("Entity", "bool Active", asOFFSET(Entity, Active)); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Entity", "void addComponent(const string &in)", asMETHOD(Entity, addComponent), asCALL_THISCALL); Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("Entity", "void removeComponent(const string &in)", asMETHOD(Entity, removeComponent), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     // === 4️⃣ Register ScriptArray add-on AFTER all ref types ===
//     RegisterScriptArray(engine, true); // true = array of references
//     arrayEntityType = engine->GetTypeInfoByDecl("array<Entity@>");
//     Q_ASSERT(arrayEntityType != nullptr);

//     qDebug() << "[OK] Registered array<Entity@> type successfully";

//     // ------------------ AngelScript Binding for renameEntity ------------------
//     s = engine->RegisterObjectMethod("ScriptEngine","void renameEntity(const string &in, const string &in)",asMETHOD(ScriptEngine, renameEntity),asCALL_THISCALL);Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine", "Entity@ getEntityById(const string &in)",asMETHOD(ScriptEngine, getEntityById), asCALL_THISCALL);Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine","array<Entity@>@ findEntitiesByType(int)",asMETHOD(ScriptEngine, findEntitiesByType),asCALL_THISCALL);assert(s >= 0);Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine","array<string>@ getAllEntityIds()",asMETHOD(ScriptEngine, getAllEntityIds),asCALL_THISCALL);Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine","array<Entity@>@ getAllEntities()",asMETHOD(ScriptEngine, getAllEntities),asCALL_THISCALL);Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine","void removeEntity(const string &in)",asMETHOD(ScriptEngine, removeEntity),asCALL_THISCALL);Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine","void removeProfile(const string &in)",asMETHOD(ScriptEngine, removeProfile),asCALL_THISCALL);Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine","void removeFolder(const string &in, const string &in)",asMETHOD(ScriptEngine, removeFolder),asCALL_THISCALL);Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine","void removeEntity(const string &in, const string &in)",asMETHOD(ScriptEngine, removeEntity),asCALL_THISCALL);Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine","void renameProfile(const string &in, const string &in)",asMETHOD(ScriptEngine, renameProfile),asCALL_THISCALL);Q_ASSERT(s >= 0);
//     s = engine->RegisterObjectMethod("ScriptEngine","void renameFolder(const string &in, const string &in)",asMETHOD(ScriptEngine, renameFolder),asCALL_THISCALL);Q_ASSERT(s >= 0);
//     // === 5️⃣ CanvasWidget / Shape Drawing Bindings ===
//     s = engine->RegisterObjectMethod("ScriptEngine",
//                                      "void setCanvasSelectedShape(const string &in)",
//                                      asMETHOD(ScriptEngine, setCanvasSelectedShape), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectMethod("ScriptEngine",
//                                      "void canvasAddLine(const string &in, array<string>@)",
//                                      asMETHOD(ScriptEngine, canvasAddLine), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectMethod("ScriptEngine",
//                                      "void canvasAddPolygon(const string &in, array<string>@)",
//                                      asMETHOD(ScriptEngine, canvasAddPolygon), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectMethod("ScriptEngine",
//                                      "void canvasAddRectangle(const string &in, float, float, float, float)",
//                                      asMETHOD(ScriptEngine, canvasAddRectangle), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectMethod("ScriptEngine",
//                                      "void canvasAddCircle(const string &in, float, float, float)",
//                                      asMETHOD(ScriptEngine, canvasAddCircle), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     s = engine->RegisterObjectMethod("ScriptEngine",
//                                      "void canvasAddPoint(const string &in, float, float)",
//                                      asMETHOD(ScriptEngine, canvasAddPoint), asCALL_THISCALL); Q_ASSERT(s >= 0);

//     qDebug() << "[OK] Canvas wrapper methods registered successfully";
//     int r;

//     // Built-in bitmap selection
//     r = engine->RegisterObjectMethod(
//         "ScriptEngine",
//         "void onBitmapSelected(const string &in)",
//         asMETHOD(ScriptEngine, onBitmapSelected),
//         asCALL_THISCALL
//         ); Q_ASSERT(r >= 0);

//     // Get built-in bitmap path
//     r = engine->RegisterObjectMethod(
//         "ScriptEngine",
//         "string getBitmapImagePath(const string &in)",
//         asMETHOD(ScriptEngine, getBitmapImagePath),
//         asCALL_THISCALL
//         ); Q_ASSERT(r >= 0);

//     // User-selected bitmap from disk
//     r = engine->RegisterObjectMethod(
//         "ScriptEngine",
//         "void onBitmapImageSelected(const string &in)",
//         asMETHOD(ScriptEngine, onBitmapImageSelected),
//         asCALL_THISCALL
//         ); Q_ASSERT(r >= 0);
//     qDebug() << "[OK] Bitmap wrapper methods registered successfully";

//     r = engine->RegisterObjectMethod("ScriptEngine",
//                                      "void canvasImportGeoJsonLayer(const string &in)",
//                                      asMETHOD(ScriptEngine, canvasImportGeoJsonLayer),
//                                      asCALL_THISCALL); Q_ASSERT(r >= 0);

//     r = engine->RegisterObjectMethod("ScriptEngine",
//                                      "void canvasToggleGeoJsonLayer(const string &in, bool)",
//                                      asMETHOD(ScriptEngine, canvasToggleGeoJsonLayer),
//                                      asCALL_THISCALL); Q_ASSERT(r >= 0);

//     r = engine->RegisterObjectMethod("ScriptEngine",
//                                      "void canvasStartDistanceMeasurement()",
//                                      asMETHOD(ScriptEngine, canvasStartDistanceMeasurement),
//                                      asCALL_THISCALL);
//     Q_ASSERT(r >= 0);

//     r = engine->RegisterObjectMethod("ScriptEngine",
//                                      "void canvasAddMeasurePoint(double, double)",
//                                      asMETHOD(ScriptEngine, canvasAddMeasurePoint),
//                                      asCALL_THISCALL);
//     Q_ASSERT(r >= 0);

//     r = engine->RegisterObjectMethod("ScriptEngine",
//                                      "double canvasGetLastSegmentDistance()",
//                                      asMETHOD(ScriptEngine, canvasGetLastSegmentDistance),
//                                      asCALL_THISCALL);
//     Q_ASSERT(r >= 0);

//     r = engine->RegisterObjectMethod("ScriptEngine",
//                                      "double canvasGetTotalDistance()",
//                                      asMETHOD(ScriptEngine, canvasGetTotalDistance),
//                                      asCALL_THISCALL);
//     Q_ASSERT(r >= 0);

//     r = engine->RegisterObjectMethod("ScriptEngine",
//                                      "void canvasSetMeasurementUnit(const string &in)",
//                                      asMETHOD(ScriptEngine, canvasSetMeasurementUnit),
//                                      asCALL_THISCALL);
//     Q_ASSERT(r >= 0);

//     r = engine->RegisterObjectMethod(
//         "ScriptEngine",
//         "void canvasToggleAirbases()",
//         asMETHOD(ScriptEngine, canvasToggleAirbases),
//         asCALL_THISCALL
//         );
//     Q_ASSERT(r >= 0);

//     // In ScriptEngine::bindToAngelScript()

//     // --- Add Parameter to Existing Entity ---
//     // s = engine->RegisterObjectMethod(
//     //     "ScriptEngine",
//     //     "void addParameterToEntity(const string &in, const string &in, int, const string &in)",
//     //     asMETHOD(ScriptEngine, addParameterToEntity),
//     //     asCALL_THISCALL
//     //     );
//     // Q_ASSERT(s >= 0);



//     e = new MyObj();
//     e->x = 10;
//     e->y = 20;

//     // Sample script
//     // const char *script =
//     //     "void main() { \n"
//     //     "   Print('Hello from AngelScript + Qt!'); \n"
//     //     "   //NewProfile('TestProfile'); \n"
//     //     "}";



//     mod = engine->GetModule("MyModule", asGM_ALWAYS_CREATE);
//     // mod->AddScriptSection("script", script);

//     // int s = mod->Build();
//     // if (r < 0) {
//     //     qDebug() << "Failed to compile script";
//     //     return;
//     // }

//     // func = mod->GetFunctionByDecl("void main()");
//     // if (!func) {
//     //     qDebug() << "Function not found";
//     //     return;
//     // }

//     // ctx = engine->CreateContext();
//     // const char* script =
//     //     "void on_update(MyObj@ e) { \n"
//     //     "  Print('Before: x=' + e.x + ', y=' + e.y); \n"
//     //     "  e.x += 5; \n"
//     //     "  e.y += 10; \n"
//     //     "  e.moveBy(100,100); \n"
//     //     "  Print('After: x=' + e.x + ', y=' + e.y); \n"
//     //     "}";
//     // const char* script =
//     //     "void on_update(ScriptEngine@ e) { \n"
//     //     "  Print('Before'); \n"
//     //     "  string d = e.NewProfile('hello'); \n"
//     //     "  e.addEntity(d, 'entity',true); \n"
//     //     "  Print('After'); \n"
//     //     "}";
//     // mod = engine->GetModule("MyModule", asGM_ALWAYS_CREATE);
//     // mod->AddScriptSection("script", script);
//     // if (mod->Build() < 0) {
//     //     qDebug() << "Script Build Failed!";
//     //     return;
//     // } else {
//     //     qDebug() << "Script Compiled.";
//     // }

//     ctx = engine->CreateContext();
// }

// ScriptEngine::~ScriptEngine()
// {
// }

// void ScriptEngine::setHierarchy(Hierarchy *hiers, HierarchyTree *tr, SceneRenderer *rend)
// {
//     hierarchy = hiers;
//     tree = tr;
//     render = rend;
// }

// bool ScriptEngine::loadAndCompileScript(QString scriptContent)
// {
//     engine->DiscardModule("MyModule");
//     mod = engine->GetModule("MyModule", asGM_ALWAYS_CREATE);
//     mod->AddScriptSection("script", scriptContent.toUtf8().data());

//     int s = mod->Build();
//     if (s < 0) {
//         qDebug() << "Failed to compile script!";
//         return false;
//     }

//     func = mod->GetFunctionByName("main");

//     if (!func) {
//         qDebug() << "Function main() not found!";
//         return false;
//     }
//     ctx->Prepare(func);
//     ctx->SetArgObject(0, this);
//     run();

//     // //addProfiles("hrgff");

//     // asIScriptFunction* func = mod->GetFunctionByName("on_update");
//     // if (!func) {
//     //     qDebug() << "on_update() function not found!";
//     //     return false;
//     // }

//     // ctx->Prepare(func);
//     // ctx->SetArgObject(0, this);
//     // int s = ctx->Execute();
//     // if (r != asEXECUTION_FINISHED) {
//     //     qDebug() << "Script execution failed!";
//     // } else {
//     //     qDebug() << "Executed. Entity now: x =" << e->x << ", y =" << e->y;
//     // }
//     // //ctx->Release();
//     return true;
// }

// void ScriptEngine::run()
// {
//     if (!ctx || !func)
//         return;

//     //ctx->Prepare(func);
//     ctx->Execute();
// }




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
// Random number generator
static float Math_Random()
{
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
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
    if (auto* sim = GetSimulation(engine))
        sim->start();
}

void AS_SimPause(ScriptEngine* engine)
{
    if (auto* sim = GetSimulation(engine))
        sim->pause();
}
void Print(const std::string &msg)
{
    qDebug() << "[AngelScript]:" << QString::fromStdString(msg);
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
    hierarchy->addComponent(QString::fromStdString(entity->ID), "meshRenderer2d");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "trajectory");

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
void ScriptEngine::canvasAddLine(const std::string &name, CScriptArray* points)
{
    if (!canvas || !points) {
        qWarning() << "Canvas or points array is null.";
        return;
    }

    for (asUINT i = 0; i < points->GetSize(); i++) {
        QString ptStr = QString::fromStdString(*(std::string*)points->At(i));
        QStringList xy = ptStr.split(",");
        if (xy.size() == 2) {
            QPointF geoPos(xy[0].toFloat(), xy[1].toFloat());
            bool finalize = (i == points->GetSize() - 1);
            canvas->drawLine(geoPos, finalize);
        }
    }
    qDebug() << "[ScriptEngine] Line added:" << QString::fromStdString(name);
}

// 3️⃣ Add a polygon
void ScriptEngine::canvasAddPolygon(const std::string &name, CScriptArray* points)
{
    if (!canvas || !points) {
        qWarning() << "Canvas or points array is null.";
        return;
    }

    for (asUINT i = 0; i < points->GetSize(); i++) {
        QString ptStr = QString::fromStdString(*(std::string*)points->At(i));
        QStringList xy = ptStr.split(",");
        if (xy.size() == 2) {
            QPointF geoPos(xy[0].toFloat(), xy[1].toFloat());
            bool finalize = (i == points->GetSize() - 1);
            canvas->drawPolygon(geoPos, finalize);
        }
    }
    qDebug() << "[ScriptEngine] Polygon added:" << QString::fromStdString(name);
}

// 4️⃣ Add a rectangle
void ScriptEngine::canvasAddRectangle(const std::string &name, float x, float y, float width, float height)
{
    if (!canvas) {
        qWarning() << "Canvas not set!";
        return;
    }

    QPointF geoPos(x, y);
    canvas->drawRectangle(geoPos);
    qDebug() << "[ScriptEngine] Rectangle added:" << QString::fromStdString(name)
             << "at (" << x << "," << y << ") size:" << width << "x" << height;
}

// 5️⃣ Add a circle
void ScriptEngine::canvasAddCircle(const std::string &name, float x, float y, float radius)
{
    if (!canvas) {
        qWarning() << "Canvas not set!";
        return;
    }

    QPointF geoPos(x, y);
    canvas->drawCircle(geoPos);
    qDebug() << "[ScriptEngine] Circle added:" << QString::fromStdString(name)
             << "at (" << x << "," << y << ") radius:" << radius;
}

// 6️⃣ Add a point
void ScriptEngine::canvasAddPoint(const std::string &name, float x, float y)
{
    if (!canvas) {
        qWarning() << "Canvas not set!";
        return;
    }

    QPointF geoPos(x, y);
    canvas->drawPoints(geoPos);
    qDebug() << "[ScriptEngine] Point added:" << QString::fromStdString(name)
             << "at (" << x << "," << y << ")";
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
    qDebug() << "[ScriptEngine] Bitmap image placed from path:" << QString::fromStdString(filePath);
}
void ScriptEngine::canvasImportGeoJsonLayer(const std::string &filePath)
{
    if (!canvas) return;
    canvas->importGeoJsonLayer(QString::fromStdString(filePath));
    qDebug() << "[ScriptEngine] Imported GeoJSON layer from path:" << QString::fromStdString(filePath);
}

void ScriptEngine::canvasToggleGeoJsonLayer(const std::string &layerName, bool visible)
{
    if (!canvas) return;
    canvas->onGeoJsonLayerToggled(QString::fromStdString(layerName), visible);
    qDebug() << "[ScriptEngine] Toggled GeoJSON layer" << QString::fromStdString(layerName)
             << "to" << (visible ? "visible" : "hidden");
}
// ScriptEngine.cpp
void ScriptEngine::canvasStartDistanceMeasurement() {
    if (canvas) canvas->startDistanceMeasurement();
}

void ScriptEngine::canvasAddMeasurePoint(double lon, double lat) {
    if (canvas) canvas->addMeasurePoint(lon, lat);
}

double ScriptEngine::canvasGetLastSegmentDistance() {
    return canvas ? canvas->getLastSegmentDistance() : 0.0;
}

double ScriptEngine::canvasGetTotalDistance() {
    return canvas ? canvas->getTotalDistance() : 0.0;
}

void ScriptEngine::canvasSetMeasurementUnit(const std::string &unit) {
    if (canvas) canvas->setMeasurementUnit(QString::fromStdString(unit));
}
void ScriptEngine::canvasToggleAirbases() {
    if (!canvas) return;
    canvas->onPresetLayerSelected("Airbase");
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
    hierarchy->addComponent(QString::fromStdString(entity->ID), "meshRenderer2d");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "sensors");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "iff");
    hierarchy->addComponent(QString::fromStdString(entity->ID), "radios");

    qDebug() << "[OK] Created Entity:" << QString::fromStdString(entityName);

    return entity;
}

// 3️⃣ Attach sensor
void ScriptEngine::attachSensorToEntity(const std::string& entityId, const std::string& sensorName) {
    if (!hierarchy) return;  // use the actual hierarchy pointer
    hierarchy->attachSensors(QString::fromStdString(entityId), QString::fromStdString(sensorName));
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

    s = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddRectangle(const string &in, float, float, float, float)",
                                     asMETHOD(ScriptEngine, canvasAddRectangle), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddCircle(const string &in, float, float, float)",
                                     asMETHOD(ScriptEngine, canvasAddCircle), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddPoint(const string &in, float, float)",
                                     asMETHOD(ScriptEngine, canvasAddPoint), asCALL_THISCALL); Q_ASSERT(s >= 0);

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
                                     asCALL_THISCALL);
    Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "void canvasAddMeasurePoint(double, double)",
                                     asMETHOD(ScriptEngine, canvasAddMeasurePoint),
                                     asCALL_THISCALL);
    Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "double canvasGetLastSegmentDistance()",
                                     asMETHOD(ScriptEngine, canvasGetLastSegmentDistance),
                                     asCALL_THISCALL);
    Q_ASSERT(r >= 0);

    r = engine->RegisterObjectMethod("ScriptEngine",
                                     "double canvasGetTotalDistance()",
                                     asMETHOD(ScriptEngine, canvasGetTotalDistance),
                                     asCALL_THISCALL);
    Q_ASSERT(r >= 0);

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
        ); assert(r >= 0);

    // Attach sensor to entity
    r = engine->RegisterObjectMethod(
        "ScriptEngine",
        "void attachSensorToEntity(const string &in, const string &in)",
        asMETHOD(ScriptEngine, attachSensorToEntity),
        asCALL_THISCALL); Q_ASSERT(r >= 0);

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
    //     qDebug() << "Executed. Entity now: x =" << e->x << ", y =" << e->y;
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


