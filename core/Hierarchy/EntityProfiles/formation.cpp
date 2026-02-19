// backup for Manual creation formation

// //============================================================================
// // Written by: Waris
// // Formation
// // Purpose:
// // - Manages group formations (Line, V, Diamond, Square, etc.)
// // - Handles creation, serialization, deserialization, and updates of
// //   formation positions and their entity bindings
// // - Integrates formation logic with hierarchy and dynamic follow behavior
// //============================================================================

// #include "formation.h"
// #include "qjsonarray.h"
// #include "core/Hierarchy/Utils/entityutils.h"
// #include "core/GlobalRegistry.h"
// #include <core/Hierarchy/hierarchy.h>
// #include <core/Debug/console.h>

// // Constructor
// // Initializes formation defaults and allocates storage for formation positions
// Formation::Formation(Hierarchy* h)
//     : Entity(h), formationType(Constants::FormationType::Line), count(0),
//     mothership(nullptr), formationPositions(new std::unordered_map<std::string,FormationPosition*>())
// {
// }

// // Spawns the formation entity into the hierarchy
// // Registers the entity and creates the mothership component
// void Formation::spawn() {
//     Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
//     emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
//     addComponent("mothership");

// }
// // Returns supported components for the formation (currently none exposed)
// std::vector<std::string> Formation::getSupportedComponents() {
//     std::vector<std::string> supported;
//     return supported;
// }
// // Serializes the formation state into JSON
// // Includes formation metadata, mothership, positions, and formation type
// QJsonObject Formation::toJson() const {
//     QJsonObject obj;
//     obj["name"] = QString::fromStdString(Name);
//     obj["branch"] = QString::fromStdString("Entity");
//     obj["id"] = QString::fromStdString(ID);
//     obj["parent_id"] = QString::fromStdString(parentID);
//     obj["active"] = Active;
//     obj["count"] = QString::number(count);

//     // Save mothership
//     if (mothership) obj["mothership"] = mothership->toJson();

//     // Save all formation positions (ally_0, ally_1, etc.)
//     QJsonArray formationArray;
//     for (const auto& pair : *formationPositions) {
//         const std::string& positionName = pair.first;
//         FormationPosition* position = pair.second;

//         if (position) {
//             QJsonObject posObj = position->toJson();
//             // Add the position name so we know which position this is
//             posObj["positionName"] = QString::fromStdString(positionName);
//             formationArray.append(posObj);
//         }
//     }
//     obj["formationPositions"] = formationArray;  // Changed key to "formationPositions"

//     // Save formation type
//     QJsonObject entityObj;
//     entityObj["type"] = "option";
//     QJsonArray optionsArray;
//     for (const QString& opt : formationTypeOptions())
//         optionsArray.append(opt);

//     entityObj["options"] = optionsArray;
//     entityObj["value"] = formationTypeToString(formationType);
//     obj["Formation_Type"] = entityObj;

//     return obj;
// }
// // Deserializes formation data from JSON
// // Restores entity links, formation positions, offsets, and formation type
// void Formation::fromJson(const QJsonObject& obj) {
//     if(obj.contains("name")){
//         Name = obj["name"].toString().toStdString();
//     }
//     if(obj.contains("id")){
//         ID = obj["id"].toString().toStdString();
//     }
//     if(obj.contains("parent_id")){
//         parentID = obj["parent_id"].toString().toStdString();
//     }
//     if(obj.contains("active")){
//         Active = obj["active"].toBool();
//     }
//     if(obj.contains("count")){
//         count = obj["count"].toString().toInt();
//     }

//     // Deserialize mothership
//     if (obj.contains("mothership") && obj["mothership"].isObject()) {
//         if (!mothership) mothership = new FormationPosition();
//         mothership->fromJson(obj["mothership"].toObject());

//         // Resolve mothership entity
//         resolveEntityReference(mothership, obj["mothership"].toObject());
//     }

//     // Deserialize formation positions
//     if (obj.contains("formationPositions") && obj["formationPositions"].isArray()) {
//         QJsonArray positionsArray = obj["formationPositions"].toArray();

//         // Clear existing positions first
//         std::vector<std::string> keysToRemove;
//         for (const auto& pair : *formationPositions) {
//             keysToRemove.push_back(pair.first);
//         }
//         for (const auto& name : keysToRemove) {
//             removeComponent(name);
//         }

//         // Load positions from JSON
//         for (const QJsonValue& value : positionsArray) {
//             QJsonObject posObj = value.toObject();
//             if (posObj.contains("positionName") && posObj["positionName"].isString()) {
//                 std::string positionName = posObj["positionName"].toString().toStdString();

//                 // Create the formation position
//                 addComponent(positionName);

//                 // Get the position and load its data
//                 FormationPosition* position = (*formationPositions)[positionName];
//                 if (position) {
//                     position->fromJson(posObj);

//                     // Resolve entity reference
//                     resolveEntityReference(position, posObj);

//                     // If we have offset data in JSON, use it (otherwise use default from formationCreate)
//                     if (posObj.contains("Offset") && posObj["Offset"].isObject()) {
//                         QJsonObject offsetObj = posObj["Offset"].toObject();
//                         if (position->Offset) {
//                             position->Offset->x = offsetObj["x"].toDouble();
//                             position->Offset->y = offsetObj["y"].toDouble();
//                             position->Offset->z = offsetObj["z"].toDouble();
//                         }
//                     }
//                 }
//             }
//         }
//     }

//     // Deserialize formation type
//     if (obj.contains("Formation_Type") && obj["Formation_Type"].isObject()) {
//         QJsonObject typeObj = obj["Formation_Type"].toObject();
//         QString typeVal = typeObj["value"].toString();
//         formationType = stringToFormationType(typeVal);

//         // Only create default formation if we don't have positions loaded from JSON
//         if (!obj.contains("formationPositions") || !obj["formationPositions"].isArray() ||
//             obj["formationPositions"].toArray().isEmpty()) {
//             formationCreate();
//         }
//     }
// }

// void Formation::formationCreate() {
//     // 1. Cleanup: Safely remove existing positions
//     std::vector<std::string> keysToRemove;
//     for (const auto& pair : *formationPositions) {
//         keysToRemove.push_back(pair.first);
//     }
//     for (const auto& name : keysToRemove) {
//         removeComponent(name);
//     }
//     if (formationPositions) {
//         formationPositions->clear();
//     }

    // // 2. Dynamic Generation
    // float spacing = 0.5f;

    // for (int i = 0; i < count; i++) {
    //     std::string name = "ally_" + std::to_string(i);
    //     addComponent(name);

    //     FormationPosition* pos = (*formationPositions)[name];
    //     if (!pos || !pos->Offset) continue;

    //     pos->Offset->y = 0;

    //     // 1. LINE (Horizontal spread)
    //     if (formationType == Constants::FormationType::Line) {
    //         float side = (i % 2 == 0) ? -1.0f : 1.0f;
    //         float multiplier = (i / 2) + 1;
    //         pos->Offset->x = side * spacing * multiplier;
    //         pos->Offset->z = 0;
    //     }

    //     // 2. V (Trailing edges)
    //     else if (formationType == Constants::FormationType::V) {
    //         float side = (i % 2 == 0) ? -1.0f : 1.0f;
    //         float multiplier = (i / 2) + 1;
    //         pos->Offset->x = side * spacing * multiplier;
    //         pos->Offset->z = -spacing * multiplier;
    //     }

    //     // --- DIAMOND FORMATION (HOLLOW / OUTLINE) ---
    //     else if (formationType == Constants::FormationType::Diamond) {


    //         // Minimum points to form a diamond
    //         if (count == 1) {
    //             pos->Offset->x = 0;
    //             pos->Offset->z = 0;
    //             return;
    //         }

    //         float radius = spacing * (count / 4.0f + 1);

    //         // Normalize index to [0, 1)
    //         float t = (float)i / (float)count;

    //         float x = 0.0f;
    //         float z = 0.0f;

    //         if (t < 0.25f) {
    //             // Top → Right
    //             float u = t / 0.25f;
    //             x =  u * radius;
    //             z = -radius + u * radius;
    //         }
    //         else if (t < 0.50f) {
    //             // Right → Bottom
    //             float u = (t - 0.25f) / 0.25f;
    //             x =  radius - u * radius;
    //             z =  u * radius;
    //         }
    //         else if (t < 0.75f) {
    //             // Bottom → Left
    //             float u = (t - 0.50f) / 0.25f;
    //             x = -u * radius;
    //             z =  radius - u * radius;
    //         }
    //         else {
    //             // Left → Top
    //             float u = (t - 0.75f) / 0.25f;
    //             x = -radius + u * radius;
    //             z = -u * radius;
    //         }

    //         pos->Offset->x = x;
    //         pos->Offset->z = z;
    //     }

    //     // 5. SQUARE (FIXED: Added float x, z declaration)
    //     else if (formationType == Constants::FormationType::Square) {
    //         int pointsPerSide = std::ceil((float)count / 4.0f);
    //         float sideLength = pointsPerSide * spacing;
    //         float halfSide = sideLength / 2.0f;

    //         int side = i / pointsPerSide;
    //         int indexOnSide = i % pointsPerSide;
    //         float step = (float)indexOnSide / (float)pointsPerSide * sideLength;

    //         // Variables declare karna zaroori hai
    //         float x = 0;
    //         float z = 0;

    //         switch (side) {
    //         case 0: // TOP
    //             x = -halfSide + step;
    //             z = halfSide;
    //             break;
    //         case 1: // RIGHT
    //             x = halfSide;
    //             z = halfSide - step;
    //             break;
    //         case 2: // BOTTOM
    //             x = halfSide - step;
    //             z = -halfSide;
    //             break;
    //         case 3: // LEFT
    //             x = -halfSide;
    //             z = -halfSide + step;
    //             break;
    //         }
    //         pos->Offset->x = x;
    //         pos->Offset->z = z;
    //     }

    //     // 4. COLUMN (Single file)
    //     else if (formationType == Constants::FormationType::Column) {
    //         pos->Offset->x = 0;
    //         pos->Offset->z = -spacing * (i + 1);
    //     }

    //     // 5. ECHELON LEFT (Staircase Left)
    //     else if (formationType == Constants::FormationType::EchelonLeft) {
    //         pos->Offset->x = spacing * (i + 1);
    //         pos->Offset->z = -spacing * (i + 1);
    //     }

    //     // 6. ECHELON RIGHT (Staircase Right)
    //     else if (formationType == Constants::FormationType::EchelonRight) {
    //         pos->Offset->x = -spacing * (i + 1);
    //         pos->Offset->z = -spacing * (i + 1);
    //     }

    //     // 7. STAGGERED COLUMN (Zig-Zag)
    //     else if (formationType == Constants::FormationType::StaggeredColumn) {
    //         pos->Offset->x = (i % 2 == 0) ? spacing : -spacing;
    //         pos->Offset->z = -spacing * (i + 1);
    //     }

    //     else if (formationType == Constants::FormationType::Wedge) {
    //         // Determine side: Even indices (0, 2, 4...) go Left (-1), Odd (1, 3, 5...) go Right (1)
    //         float side = (i % 2 == 0) ? -1.0f : 1.0f;

    //         // Calculate how far back they are:
    //         // Ally 0 (Left) -> Row 1
    //         // Ally 1 (Right) -> Row 1
    //         // Ally 2 (Left) -> Row 2
    //         // Ally 3 (Right) -> Row 2
    //         int row = (i / 2) + 1;

    //         pos->Offset->x = side * spacing * row;
    //         pos->Offset->z = -spacing * row;
    //         pos->Offset->y = 0;
    //     }
    // }
// }

// // Adds a component (mothership or ally position) to the formation
// void Formation::addComponent(std::string name) {
//     Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
//     FormationPosition* fp = new FormationPosition();
//     if(name == "mothership"){
//         mothership = fp;
//     }else{
//         (*formationPositions)[name] = fp;
//     }
//     emit parent->componentAdded(QString::fromStdString(ID),"ID", QString::fromStdString(name));

// }

// // Removes a formation position component
// void Formation::removeComponent(std::string name) {
//     Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
//     (*formationPositions).erase(name);
//     emit parent->componentRemoved(QString::fromStdString(ID), QString::fromStdString(name));
// }
// // Returns a component's serialized data by name
// // QJsonObject Formation::getComponent(std::string name) {
// //     if(name == "mothership"){
// //         return mothership->toJson();
// //     }else{
// //         if (formationPositions->find(name) != formationPositions->end()) {
// //             // exists
// //             return (*formationPositions)[name]->toJson();
// //         }
// //     }

// //     return QJsonObject();
// // }

// QJsonObject Formation::getComponent(std::string name) {
//     if(name == "mothership"){
//         qDebug() << "Getting mothership component";
//         return mothership->toJson();
//     }else{
//         if (formationPositions->find(name) != formationPositions->end()) {
//             // exists
//             FormationPosition* pos = (*formationPositions)[name];
//             qDebug() << "Getting component:" << name.c_str();
//             qDebug() << "Position has Offset:" << (pos->Offset ? "Yes" : "No");
//             if (pos->Offset) {
//                 qDebug() << "Offset values: x=" << pos->Offset->x
//                          << " y=" << pos->Offset->y
//                          << " z=" << pos->Offset->z;
//             }
//             qDebug() << "JSON output:" << pos->toJson();
//             return pos->toJson();
//         }
//     }

//     return QJsonObject();
// }

// void Formation::updateComponent(QString name, const QJsonObject& obj) {
//     // 1. Handle Formation Type change
//     if (name == "type") {
//         if (obj.contains("value")) {
//             formationType = stringToFormationType(obj["value"].toString());
//             formationCreate(); // Re-apply geometric logic to existing count
//         }
//         return;
//     }

//     // 2. Handle Count change (Integration for increasing allies)
//     if (name == "count") {
//         if (obj.contains("value")) {
//             int newCount = obj["value"].toString().toInt();
//             if (newCount >= 0) {
//                 this->count = newCount;
//                 formationCreate(); // Dynamically add/remove ally slots
//             }
//         }
//         return;
//     }

//     // 3. Handle Mothership updates
//     if (name == "mothership") {
//         if (obj.contains("entity") && obj["entity"].isObject()) {
//             QJsonObject entityObj = obj["entity"].toObject();
//             if (entityObj.contains("id")) {
//                 std::string id = entityObj["id"].toString().toStdString();
//                 Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
//                 if (parent && parent->Entities->find(id) != parent->Entities->end()) {
//                     mothership->entity = (*parent->Entities)[id];
//                 }
//             }
//         }
//         mothership->fromJson(obj);
//         return;
//     }

//     // 4. Handle Individual Ally updates (Assignment of entities to slots)
//     if (!formationPositions) return;
//     std::string key = name.toStdString();
//     auto it = formationPositions->find(key);
//     if (it == formationPositions->end()) return;

//     FormationPosition* pos = it->second;
//     if (obj.contains("entity") && obj["entity"].isObject()) {
//         QJsonObject entityObj = obj["entity"].toObject();
//         if (entityObj.contains("id")) {
//             std::string id = entityObj["id"].toString().toStdString();
//             Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
//             if (parent && parent->Entities->find(id) != parent->Entities->end()) {
//                 pos->entity = (*parent->Entities)[id];

//                 // Link the platform's dynamic model to the formation logic
//                 Platform* platform = dynamic_cast<Platform*>(pos->entity);
//                 Platform* mother = dynamic_cast<Platform*>(mothership->entity);
//                 if (platform && platform->dynamicModel && mother) {
//                     platform->dynamicModel->followEntity = mother;
//                     platform->dynamicModel->formationPosition = pos;
//                     platform->dynamicModel->follow = true;
//                 }
//             }
//         }
//     }
//     pos->fromJson(obj);
// }
// // Converts formation enum to display string
// QString Formation::formationTypeToString(Constants::FormationType type) const {
//     switch (type) {
//     case Constants::FormationType::Line:    return "Line";
//     case Constants::FormationType::V:       return "V";
//     case Constants::FormationType::Diamond: return "Diamond";
//     case Constants::FormationType::Square:  return "Square"; // Added
//     case Constants::FormationType::Column:  return "Column";
//     case Constants::FormationType::EchelonLeft: return "Echelon Left";
//     case Constants::FormationType::EchelonRight: return "Echelon Right"; // Add this
//     case Constants::FormationType::StaggeredColumn: return "Staggered Column";
//     case Constants::FormationType::Wedge: return "Wedge";
//     default: return "Line";
//     }
// }
// // Converts display string back to formation enum
// Constants::FormationType Formation::stringToFormationType(QString str) const {
//     if (str == "Line")    return Constants::FormationType::Line;
//     if (str == "V")       return Constants::FormationType::V;
//     if (str == "Diamond") return Constants::FormationType::Diamond;
//     if (str == "Square")  return Constants::FormationType::Square; // Added
//     if (str == "Column")  return Constants::FormationType::Column;
//     if (str == "Echelon Left") return Constants::FormationType::EchelonLeft;
//     if (str == "Echelon Right") return Constants::FormationType::EchelonRight; // Add this
//     if (str == "Staggered Column") return Constants::FormationType::StaggeredColumn;
//     if (str == "Wedge") return Constants::FormationType::Wedge;
//     return Constants::FormationType::Line;
// }
// // Returns available formation type options
// QStringList Formation::formationTypeOptions() const {
//     return {"Line", "V", "Diamond", "Square","Column", "Echelon Left", "Echelon Right", "Staggered Column", "Wedge"};
// }
// // Resolves entity references after loading from JSON
// // Links formation positions to actual entities and enables follow behavior
// void Formation::resolveEntityReference(FormationPosition* position, const QJsonObject& obj) {
//     if (obj.contains("entity") && obj["entity"].isObject()) {
//         QJsonObject entityObj = obj["entity"].toObject();
//         std::string entityId = entityObj["id"].toString().toStdString();
//         std::string entityName = entityObj["name"].toString().toStdString();

//         // Only resolve if it's not a dummy ID
//         if (entityId != "dummy" && !entityId.empty()) {
//             Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
//             if (parent && parent->Entities->find(entityId) != parent->Entities->end()) {
//                 position->entity = (*parent->Entities)[entityId];

//                 // If this is a formation position (not mothership), setup follow behavior
//                 if (position != mothership && mothership && mothership->entity) {
//                     Platform* platform = dynamic_cast<Platform*>(position->entity);
//                     Platform* mother = dynamic_cast<Platform*>(mothership->entity);
//                     if (platform && platform->dynamicModel && mother) {
//                         platform->dynamicModel->followEntity = mother;
//                         platform->dynamicModel->formationPosition = position;
//                         platform->dynamicModel->follow = true;
//                     }
//                 }
//             } else {
//                 // Entity not found in hierarchy, create placeholder
//                 if (!position->entity || position->entity->ID == "dummy") {
//                     if (position->entity) delete position->entity;
//                     position->entity = new Platform(nullptr);
//                     position->entity->ID = entityId;
//                     position->entity->Name = entityName.empty() ? "Unknown Entity" : entityName;
//                 }
//             }
//         }
//     }
// }


//============================================================================
// Written by: Waris
// Formation
// Purpose:
// - Manages group formations (Line, V, Diamond, Square, etc.)
// - Handles creation, serialization, deserialization, and updates of
//   formation positions and their entity bindings
// - Integrates formation logic with hierarchy and dynamic follow behavior
//============================================================================

#include "formation.h"
#include "qjsonarray.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/GlobalRegistry.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Debug/console.h>

// Constructor
// Initializes formation defaults and allocates storage for formation positions
Formation::Formation(Hierarchy* h)
    : Entity(h), formationType(Constants::FormationType::Line), count(0),
    mothership(nullptr), formationPositions(new std::unordered_map<std::string,FormationPosition*>())
{
}

// Spawns the formation entity into the hierarchy
// Registers the entity and creates the mothership component
void Formation::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
    addComponent("mothership");

}
// Returns supported components for the formation (currently none exposed)
std::vector<std::string> Formation::getSupportedComponents() {
    std::vector<std::string> supported;
    return supported;
}
// Serializes the formation state into JSON
// Includes formation metadata, mothership, positions, and formation type
QJsonObject Formation::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;
    obj["count"] = QString::number(count);

    // Save mothership
    if (mothership) obj["mothership"] = mothership->toJson();

    // Save all formation positions (ally_0, ally_1, etc.)
    QJsonArray formationArray;
    for (const auto& pair : *formationPositions) {
        const std::string& positionName = pair.first;
        FormationPosition* position = pair.second;

        if (position) {
            QJsonObject posObj = position->toJson();
            // Add the position name so we know which position this is
            posObj["positionName"] = QString::fromStdString(positionName);
            formationArray.append(posObj);
        }
    }
    obj["formationPositions"] = formationArray;  // Changed key to "formationPositions"

    // Save formation type
    QJsonObject entityObj;
    entityObj["type"] = "option";
    QJsonArray optionsArray;
    for (const QString& opt : formationTypeOptions())
        optionsArray.append(opt);

    entityObj["options"] = optionsArray;
    entityObj["value"] = formationTypeToString(formationType);
    obj["Formation_Type"] = entityObj;

    return obj;
}
// Deserializes formation data from JSON
// Restores entity links, formation positions, offsets, and formation type
void Formation::fromJson(const QJsonObject& obj) {
    if(obj.contains("name")){
        Name = obj["name"].toString().toStdString();
    }
    if(obj.contains("id")){
        ID = obj["id"].toString().toStdString();
    }
    if(obj.contains("parent_id")){
        parentID = obj["parent_id"].toString().toStdString();
    }
    if(obj.contains("active")){
        Active = obj["active"].toBool();
    }
    if(obj.contains("count")){
        count = obj["count"].toString().toInt();
    }

    // Deserialize mothership
    if (obj.contains("mothership") && obj["mothership"].isObject()) {
        if (!mothership) mothership = new FormationPosition();
        mothership->fromJson(obj["mothership"].toObject());

        // Resolve mothership entity
        resolveEntityReference(mothership, obj["mothership"].toObject());
    }

    // Deserialize formation positions
    if (obj.contains("formationPositions") && obj["formationPositions"].isArray()) {
        QJsonArray positionsArray = obj["formationPositions"].toArray();

        // Clear existing positions first
        std::vector<std::string> keysToRemove;
        for (const auto& pair : *formationPositions) {
            keysToRemove.push_back(pair.first);
        }
        for (const auto& name : keysToRemove) {
            removeComponent(name);
        }

        // Load positions from JSON
        for (const QJsonValue& value : positionsArray) {
            QJsonObject posObj = value.toObject();
            if (posObj.contains("positionName") && posObj["positionName"].isString()) {
                std::string positionName = posObj["positionName"].toString().toStdString();

                // Create the formation position
                addComponent(positionName);

                // Get the position and load its data
                FormationPosition* position = (*formationPositions)[positionName];
                if (position) {
                    position->fromJson(posObj);

                    // Resolve entity reference
                    resolveEntityReference(position, posObj);

                    // If we have offset data in JSON, use it (otherwise use default from formationCreate)
                    if (posObj.contains("Offset") && posObj["Offset"].isObject()) {
                        QJsonObject offsetObj = posObj["Offset"].toObject();
                        if (position->Offset) {
                            position->Offset->x = offsetObj["x"].toDouble();
                            position->Offset->y = offsetObj["y"].toDouble();
                            position->Offset->z = offsetObj["z"].toDouble();
                        }
                    }
                }
            }
        }
    }

    // Deserialize formation type
    if (obj.contains("Formation_Type") && obj["Formation_Type"].isObject()) {
        QJsonObject typeObj = obj["Formation_Type"].toObject();
        QString typeVal = typeObj["value"].toString();
        formationType = stringToFormationType(typeVal);

        // Only create default formation if we don't have positions loaded from JSON
        if (!obj.contains("formationPositions") || !obj["formationPositions"].isArray() ||
            obj["formationPositions"].toArray().isEmpty()) {
            formationCreate();
        }
    }
}

void Formation::formationCreate() {
    // 1. Cleanup: Safely remove existing positions
    std::vector<std::string> keysToRemove;
    for (const auto& pair : *formationPositions) {
        keysToRemove.push_back(pair.first);
    }
    for (const auto& name : keysToRemove) {
        removeComponent(name);
    }
    if (formationPositions) {
        formationPositions->clear();
    }

    // 2. Dynamic Generation
    // float spacing = 0.5f;

    // for (int i = 0; i < count; i++) {
    //     std::string name = "ally_" + std::to_string(i);
    //     addComponent(name);

    //     FormationPosition* pos = (*formationPositions)[name];
    //     if (!pos || !pos->Offset) continue;

    //     pos->Offset->y = 0;

    //     // 3. Per-Type Geometry
    //     switch (formationType) {
    //     case Constants::FormationType::Line: {
    //         float sign = (i % 2 == 0) ? -1.0f : 1.0f;
    //         float dist = ((i + 1) / 2) * spacing;
    //         pos->Offset->x = sign * dist;
    //         pos->Offset->z = 0;
    //         break;
    //     }
    //     case Constants::FormationType::V: {
    //         float sign = (i % 2 == 0) ? -1.0f : 1.0f;
    //         float dist = ((i + 1) / 2) * spacing;
    //         pos->Offset->x = sign * dist;
    //         pos->Offset->z = -dist;
    //         break;
    //     }
    //     case Constants::FormationType::Diamond: {
    //         int half = count / 2;
    //         if (i < half) {
    //             float sign = (i % 2 == 0) ? -1.0f : 1.0f;
    //             float dist = ((i + 1) / 2) * spacing;
    //             pos->Offset->x = sign * dist;
    //             pos->Offset->z = -dist;
    //         } else {
    //             int j = i - half;
    //             float sign = (j % 2 == 0) ? -1.0f : 1.0f;
    //             float dist = ((j + 1) / 2) * spacing;
    //             pos->Offset->x = sign * dist;
    //             pos->Offset->z = dist * 2;
    //         }
    //         break;
    //     }
    //     case Constants::FormationType::Square: {
    //         int side = static_cast<int>(std::sqrt(count)) + 1;
    //         int row = i / side;
    //         int col = i % side;
    //         pos->Offset->x = (col - side / 2) * spacing;
    //         pos->Offset->z = (row - side / 2) * spacing;
    //         break;
    //     }
    //     case Constants::FormationType::Column: {
    //         pos->Offset->x = 0;
    //         pos->Offset->z = -(i + 1) * spacing;
    //         break;
    //     }
    //     case Constants::FormationType::EchelonLeft: {
    //         pos->Offset->x = -(i + 1) * spacing;
    //         pos->Offset->z = -(i + 1) * spacing;
    //         break;
    //     }
    //     case Constants::FormationType::EchelonRight: {
    //         pos->Offset->x = (i + 1) * spacing;
    //         pos->Offset->z = -(i + 1) * spacing;
    //         break;
    //     }
    //     case Constants::FormationType::StaggeredColumn: {
    //         float sign = (i % 2 == 0) ? -1.0f : 1.0f;
    //         pos->Offset->x = sign * spacing * 0.5f;
    //         pos->Offset->z = -(i + 1) * spacing;
    //         break;
    //     }
    //     case Constants::FormationType::Wedge: {
    //         float sign = (i % 2 == 0) ? -1.0f : 1.0f;
    //         float dist = ((i + 1) / 2) * spacing;
    //         pos->Offset->x = sign * dist;
    //         pos->Offset->z = -(i + 1) * spacing;
    //         break;
    //     }
    //     }
    // }
    // 2. Dynamic Generation
    float spacing = 0.5f;
    for (int i = 0; i < count; i++) {
        std::string name = "ally_" + std::to_string(i);
        addComponent(name);
        FormationPosition* pos = (*formationPositions)[name];
        if (!pos || !pos->Offset) continue;
        pos->Offset->y = 0;
        // 1. LINE (Horizontal spread)
        if (formationType == Constants::FormationType::Line) {
            float side = (i % 2 == 0) ? -1.0f : 1.0f;
            float multiplier = (i / 2) + 1;
            pos->Offset->x = side * spacing * multiplier;
            pos->Offset->z = 0;
        }
        // 2. V (Trailing edges)
        else if (formationType == Constants::FormationType::V) {
            float side = (i % 2 == 0) ? -1.0f : 1.0f;
            float multiplier = (i / 2) + 1;
            pos->Offset->x = side * spacing * multiplier;
            pos->Offset->z = -spacing * multiplier;
        }
        // --- DIAMOND FORMATION (HOLLOW / OUTLINE) ---
        else if (formationType == Constants::FormationType::Diamond) {
            // Minimum points to form a diamond
            if (count == 1) {
                pos->Offset->x = 0;
                pos->Offset->z = 0;
                return;
            }
            float radius = spacing * (count / 4.0f + 1);
            // Normalize index to [0, 1)
            float t = (float)i / (float)count;
            float x = 0.0f;
            float z = 0.0f;
            if (t < 0.25f) {
                // Top → Right
                float u = t / 0.25f;
                x =  u * radius;
                z = -radius + u * radius;
            }
            else if (t < 0.50f) {
                // Right → Bottom
                float u = (t - 0.25f) / 0.25f;
                x =  radius - u * radius;
                z =  u * radius;
            }
            else if (t < 0.75f) {
                // Bottom → Left
                float u = (t - 0.50f) / 0.25f;
                x = -u * radius;
                z =  radius - u * radius;
            }
            else {
                // Left → Top
                float u = (t - 0.75f) / 0.25f;
                x = -radius + u * radius;
                z = -u * radius;
            }
            pos->Offset->x = x;
            pos->Offset->z = z;
        }
        // 5. SQUARE (FIXED: Added float x, z declaration)
        else if (formationType == Constants::FormationType::Square) {
            int pointsPerSide = std::ceil((float)count / 4.0f);
            float sideLength = pointsPerSide * spacing;
            float halfSide = sideLength / 2.0f;
            int side = i / pointsPerSide;
            int indexOnSide = i % pointsPerSide;
            float step = (float)indexOnSide / (float)pointsPerSide * sideLength;
            // Variables declare karna zaroori hai
            float x = 0;
            float z = 0;
            switch (side) {
            case 0: // TOP
                x = -halfSide + step;
                z = halfSide;
                break;
            case 1: // RIGHT
                x = halfSide;
                z = halfSide - step;
                break;
            case 2: // BOTTOM
                x = halfSide - step;
                z = -halfSide;
                break;
            case 3: // LEFT
                x = -halfSide;
                z = -halfSide + step;
                break;
            }
            pos->Offset->x = x;
            pos->Offset->z = z;
        }
        // 4. COLUMN (Single file)
        else if (formationType == Constants::FormationType::Column) {
            pos->Offset->x = 0;
            pos->Offset->z = -spacing * (i + 1);
        }
        // 5. ECHELON LEFT (Staircase Left)
        else if (formationType == Constants::FormationType::EchelonLeft) {
            pos->Offset->x = spacing * (i + 1);
            pos->Offset->z = -spacing * (i + 1);
        }
        // 6. ECHELON RIGHT (Staircase Right)
        else if (formationType == Constants::FormationType::EchelonRight) {
            pos->Offset->x = -spacing * (i + 1);
            pos->Offset->z = -spacing * (i + 1);
        }
        // 7. STAGGERED COLUMN (Zig-Zag)
        else if (formationType == Constants::FormationType::StaggeredColumn) {
            pos->Offset->x = (i % 2 == 0) ? spacing : -spacing;
            pos->Offset->z = -spacing * (i + 1);
        }
        else if (formationType == Constants::FormationType::Wedge) {
            // Determine side: Even indices (0, 2, 4...) go Left (-1), Odd (1, 3, 5...) go Right (1)
            float side = (i % 2 == 0) ? -1.0f : 1.0f;

            int row = (i / 2) + 1;
            pos->Offset->x = side * spacing * row;
            pos->Offset->z = -spacing * row;
            pos->Offset->y = 0;
        }
    }
}

void Formation::addComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    FormationPosition* fp = new FormationPosition();

    // FIXED: Generate unique component ID instead of using "ID"
    std::string componentID = ID + "_" + name;

    if(name == "mothership"){
        mothership = fp;
    }else{
        (*formationPositions)[name] = fp;
    }

    // FIXED: Pass the unique componentID instead of hardcoded "ID"
    emit parent->componentAdded(QString::fromStdString(ID), QString::fromStdString(componentID), QString::fromStdString(name));

}

// Removes a formation position component
void Formation::removeComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    (*formationPositions).erase(name);
    emit parent->componentRemoved(QString::fromStdString(ID), QString::fromStdString(name));
}
// Returns a component's serialized data by name
// QJsonObject Formation::getComponent(std::string name) {
//     if(name == "mothership"){
//         return mothership->toJson();
//     }else{
//         if (formationPositions->find(name) != formationPositions->end()) {
//             // exists
//             return (*formationPositions)[name]->toJson();
//         }
//     }

//     return QJsonObject();
// }

QJsonObject Formation::getComponent(std::string name) {
    if(name == "mothership"){
        qDebug() << "Getting mothership component";
        return mothership->toJson();
    }else{
        if (formationPositions->find(name) != formationPositions->end()) {
            // exists
            FormationPosition* pos = (*formationPositions)[name];
            qDebug() << "Getting component:" << name.c_str();
            qDebug() << "Position has Offset:" << (pos->Offset ? "Yes" : "No");
            if (pos->Offset) {
                qDebug() << "Offset values: x=" << pos->Offset->x
                         << " y=" << pos->Offset->y
                         << " z=" << pos->Offset->z;
            }
            qDebug() << "JSON output:" << pos->toJson();
            return pos->toJson();
        }
    }

    return QJsonObject();
}

void Formation::updateComponent(QString name, const QJsonObject& obj) {
    // 1. Handle Formation Type change
    if (name == "type") {
        if (obj.contains("value")) {
            formationType = stringToFormationType(obj["value"].toString());
            formationCreate(); // Re-apply geometric logic to existing count
        }
        return;
    }

    // 2. Handle Count change (Integration for increasing allies)
    if (name == "count") {
        if (obj.contains("value")) {
            int newCount = obj["value"].toString().toInt();
            if (newCount >= 0) {
                this->count = newCount;
                formationCreate(); // Dynamically add/remove ally slots
            }
        }
        return;
    }

    // 3. Handle Mothership updates
    if (name == "mothership") {
        if (obj.contains("entity") && obj["entity"].isObject()) {
            QJsonObject entityObj = obj["entity"].toObject();
            if (entityObj.contains("id")) {
                std::string id = entityObj["id"].toString().toStdString();
                Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
                if (parent && parent->Entities->find(id) != parent->Entities->end()) {
                    mothership->entity = (*parent->Entities)[id];
                }
            }
        }
        mothership->fromJson(obj);
        return;
    }

    // 4. Handle Individual Ally updates (Assignment of entities to slots)
    if (!formationPositions) return;
    std::string key = name.toStdString();
    auto it = formationPositions->find(key);
    if (it == formationPositions->end()) return;

    FormationPosition* pos = it->second;
    if (obj.contains("entity") && obj["entity"].isObject()) {
        QJsonObject entityObj = obj["entity"].toObject();
        if (entityObj.contains("id")) {
            std::string id = entityObj["id"].toString().toStdString();
            Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            if (parent && parent->Entities->find(id) != parent->Entities->end()) {
                pos->entity = (*parent->Entities)[id];

                // Link the platform's dynamic model to the formation logic
                Platform* platform = dynamic_cast<Platform*>(pos->entity);
                Platform* mother = dynamic_cast<Platform*>(mothership->entity);
                if (platform && platform->dynamicModel && mother) {
                    platform->dynamicModel->followEntity = mother;
                    platform->dynamicModel->formationPosition = pos;
                    platform->dynamicModel->follow = true;
                }
            }
        }
    }
    pos->fromJson(obj);
}
// Converts formation enum to display string
QString Formation::formationTypeToString(Constants::FormationType type) const {
    switch (type) {
    case Constants::FormationType::Line:    return "Line";
    case Constants::FormationType::V:       return "V";
    case Constants::FormationType::Diamond: return "Diamond";
    case Constants::FormationType::Square:  return "Square"; // Added
    case Constants::FormationType::Column:  return "Column";
    case Constants::FormationType::EchelonLeft: return "Echelon Left";
    case Constants::FormationType::EchelonRight: return "Echelon Right"; // Add this
    case Constants::FormationType::StaggeredColumn: return "Staggered Column";
    case Constants::FormationType::Wedge: return "Wedge";
    default: return "Line";
    }
}
// Converts display string back to formation enum
Constants::FormationType Formation::stringToFormationType(QString str) const {
    if (str == "Line")    return Constants::FormationType::Line;
    if (str == "V")       return Constants::FormationType::V;
    if (str == "Diamond") return Constants::FormationType::Diamond;
    if (str == "Square")  return Constants::FormationType::Square; // Added
    if (str == "Column")  return Constants::FormationType::Column;
    if (str == "Echelon Left") return Constants::FormationType::EchelonLeft;
    if (str == "Echelon Right") return Constants::FormationType::EchelonRight; // Add this
    if (str == "Staggered Column") return Constants::FormationType::StaggeredColumn;
    if (str == "Wedge") return Constants::FormationType::Wedge;
    return Constants::FormationType::Line;
}
// Returns available formation type options
QStringList Formation::formationTypeOptions() const {
    return {"Line", "V", "Diamond", "Square","Column", "Echelon Left", "Echelon Right", "Staggered Column", "Wedge"};
}
// Resolves entity references after loading from JSON
// Links formation positions to actual entities and enables follow behavior
void Formation::resolveEntityReference(FormationPosition* position, const QJsonObject& obj) {
    if (obj.contains("entity") && obj["entity"].isObject()) {
        QJsonObject entityObj = obj["entity"].toObject();
        std::string entityId = entityObj["id"].toString().toStdString();
        std::string entityName = entityObj["name"].toString().toStdString();

        // Only resolve if it's not a dummy ID
        if (entityId != "dummy" && !entityId.empty()) {
            Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            if (parent && parent->Entities->find(entityId) != parent->Entities->end()) {
                position->entity = (*parent->Entities)[entityId];

                // If this is a formation position (not mothership), setup follow behavior
                if (position != mothership && mothership && mothership->entity) {
                    Platform* platform = dynamic_cast<Platform*>(position->entity);
                    Platform* mother = dynamic_cast<Platform*>(mothership->entity);
                    if (platform && platform->dynamicModel && mother) {
                        platform->dynamicModel->followEntity = mother;
                        platform->dynamicModel->formationPosition = position;
                        platform->dynamicModel->follow = true;
                    }
                }
            } else {
                // Entity not found in hierarchy, create placeholder
                if (!position->entity || position->entity->ID == "dummy") {
                    if (position->entity) delete position->entity;
                    position->entity = new Platform(nullptr);
                    position->entity->ID = entityId;
                    position->entity->Name = entityName.empty() ? "Unknown Entity" : entityName;
                }
            }
        }
    }
}
