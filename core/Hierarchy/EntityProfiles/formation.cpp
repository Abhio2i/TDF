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
#include <cmath>

/**
 * @brief Constructs a Formation entity.
 * @param h Pointer to the parent Hierarchy.
 */
Formation::Formation(Hierarchy* h)
    : Entity(h), formationType(Constants::FormationType::Line), count(0),
    mothership(nullptr), formationPositions(new std::unordered_map<std::string,FormationPosition*>())
{
}

Formation::~Formation(){
    clear();
}

void Formation::clear(){
    // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if(mothership){
        if(mothership->entity){
            if (root && root->Entities.find(mothership->entity->ID) != root->Entities.end()) {
                Platform* p = dynamic_cast<Platform*>(mothership->entity);
                // if(p->dynamicModel)
                    p->dynamicModel.formation = nullptr;
            }else{
                qDebug()<<"this entity not exist";
            }
        }
        emit root->componentRemoved(QString::fromStdString(ID), QString::fromStdString("mothership"));
        delete mothership;
        mothership = nullptr;
    }
    if(!formationPositions)return;
    for ( auto& pair : *formationPositions) {
        if(pair.second && pair.second->entity){
            if (root && root->Entities.find(pair.second->entity->ID) != root->Entities.end()) {
                Platform* platform = dynamic_cast<Platform*>(pair.second->entity);
                if (platform /*&& platform->dynamicModel */) {
                    platform->dynamicModel.followEntity = nullptr;
                    platform->dynamicModel.formationPosition = nullptr;
                    platform->dynamicModel.follow = false;
                }
            }
            emit root->componentRemoved(QString::fromStdString(ID), QString::fromStdString(pair.first));
            delete pair.second;
            pair.second = nullptr; // Dangling pointer safety
        }
    }
    formationPositions->clear(); // Container ko reset karein
    delete formationPositions;
    formationPositions = nullptr;
}

/**
 * @brief Spawns the formation entity into the hierarchy.
 * Registers the entity and creates the mothership component.
 */
void Formation::spawn() {
    // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit root->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
    addComponent("mothership");
}

/**
 * @brief Returns supported components for the formation (currently none exposed).
 * @return Empty vector.
 */
std::vector<std::string> Formation::getSupportedComponents() {
    std::vector<std::string> supported;
    return supported;
}

void Formation::addSupportedcomponent(){

}

/**
 * @brief Serializes the formation state into JSON.
 * Includes formation metadata, mothership, positions, and formation type.
 * @return QJsonObject containing the formation data.
 */
QJsonObject Formation::toJson() const
{
    QJsonObject obj;
    obj["name"]      = QString::fromStdString(Name);
    obj["branch"]    = QString::fromStdString("Entity");
    obj["id"]        = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"]    = Active;
    obj["count"]     = QString::number(count);

    // ── Mothership ────────────────────────────────────────────────────
    if (mothership) {
        if (mothership->entity &&
            root &&
            root->Entities.find(mothership->entity->ID) != root->Entities.end()) {
            obj["mothership"] = mothership->toJson();
        } else if (mothership->Offset) {
            // entity link nahi – sirf offset (rare)
            QJsonObject ms;
            QJsonObject offsetObj;
            offsetObj["x"] = mothership->Offset->x;
            offsetObj["y"] = mothership->Offset->y;
            offsetObj["z"] = mothership->Offset->z;
            offsetObj["type"] = "vector";
            ms["Offset"] = offsetObj;
            obj["mothership"] = ms;
        }
    }

    // ── Formation positions (allies) ──────────────────────────────────
    if (formationPositions) {
        QJsonArray formationArray;

        for (const auto& pair : *formationPositions) {
            const std::string& positionName = pair.first;
            FormationPosition* position = pair.second;
            if (!position)
                continue;

            QJsonObject posObj;

            // Case A: entity linked hai (Scenario / Runtime)
            if (position->entity &&
                root &&
                root->Entities.find(position->entity->ID) != root->Entities.end()) {
                posObj = position->toJson();
            }
            // Case B: template mode (Database Editor canvas) – sirf Offset
            else if (position->Offset) {
                QJsonObject offsetObj;
                offsetObj["x"] = position->Offset->x;
                offsetObj["y"] = position->Offset->y;
                offsetObj["z"] = position->Offset->z;
                offsetObj["type"] = "vector";
                posObj["Offset"] = offsetObj;
            } else {
                continue;
            }

            posObj["positionName"] = QString::fromStdString(positionName);
            formationArray.append(posObj);
        }

        obj["formationPositions"] = formationArray;
    }

    // ── Formation type ────────────────────────────────────────────────
    QJsonObject entityObj;
    entityObj["type"] = "option";
    QJsonArray optionsArray;
    for (const QString& opt : formationTypeOptions())
        optionsArray.append(opt);
    entityObj["options"] = optionsArray;
    entityObj["value"]   = formationTypeToString(formationType);
    obj["Formation_Type"] = entityObj;

    return obj;
}
void Formation::removeformationPosition(FormationPosition* pos){

    if(!formationPositions)return;
    for (auto& pair : *formationPositions) {
        if(pair.second == pos){
            delete pair.second;
            pair.second = nullptr;
            formationPositions->erase(pair.first);
            // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            emit root->componentRemoved(QString::fromStdString(ID), QString::fromStdString(pair.first));
            break;
        }
    }
}

/**
 * @brief Deserializes formation data from JSON.
 * Restores entity links, formation positions, offsets, and formation type.
 * @param obj JSON object containing formation data.
 */
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
        if(formationPositions){
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
    }
    // Deserialize formation type
    if (obj.contains("Formation_Type") && obj["Formation_Type"].isObject()) {
        QJsonObject typeObj = obj["Formation_Type"].toObject();
        QString typeVal = typeObj["value"].toString();
        Constants::FormationType last  = formationType;
        formationType = stringToFormationType(typeVal);
        if(formationType !=last){
            FormationTypeUpdate();
        }

    }
}

void Formation::FormationTypeUpdate()
{
    if (!formationPositions)
        return;

    if (formationType == Constants::FormationType::Custom)
        return;

    float spacing = 0.5f;
    int i = 0;

    for (auto& pair : *formationPositions) {
        FormationPosition* pos = pair.second;
        if (!pos || !pos->Offset)
            continue;
        pos->Offset->y = 0;
        // 1. LINE (Horizontal spread)
        if (formationType == Constants::FormationType::Line) {
            float side = (i % 2 == 0) ? -1.0f : 1.0f;
            float multiplier = (i / 2) + 1;
            pos->Offset->x = side * spacing * multiplier;
            pos->Offset->z = 0;
            pos->Offset->z += 2.2f;
        }
        // 2. V (Trailing edges)
        else if (formationType == Constants::FormationType::V) {
            float side = (i % 2 == 0) ? -1.0f : 1.0f;
            float multiplier = (i / 2) + 1;
            pos->Offset->x = side * spacing * multiplier;
            pos->Offset->z = -spacing * multiplier;
        }
        // 3. DIAMOND (Hollow / Outline)
        else if (formationType == Constants::FormationType::Diamond) {
            if (count == 1) {
                pos->Offset->x = 0;
                pos->Offset->z = 0;
                // do NOT return – continue with next positions
            } else {
                float radius = spacing * (count / 4.0f + 1);
                float t = (float)i / (float)count;
                float x = 0.0f;
                float z = 0.0f;

                if (t < 0.25f) {               // Top → Right
                    float u = t / 0.25f;
                    x =  u * radius;
                    z = -radius + u * radius;
                }
                else if (t < 0.50f) {          // Right → Bottom
                    float u = (t - 0.25f) / 0.25f;
                    x =  radius - u * radius;
                    z =  u * radius;
                }
                else if (t < 0.75f) {          // Bottom → Left
                    float u = (t - 0.50f) / 0.25f;
                    x = -u * radius;
                    z =  radius - u * radius;
                }
                else {
                    float u = (t - 0.75f) / 0.25f;
                    x = -radius + u * radius;
                    z = -u * radius;
                }
                pos->Offset->x = x;
                pos->Offset->z = z;
            }
        }
        // 4. SQUARE (Outline)
        else if (formationType == Constants::FormationType::Square) {
            int pointsPerSide = std::ceil((float)count / 4.0f);
            if (pointsPerSide < 1) pointsPerSide = 1;
            float sideLength = pointsPerSide * spacing;
            float halfSide   = sideLength / 2.0f;
            int side         = i / pointsPerSide;
            int indexOnSide  = i % pointsPerSide;
            float step       = (float)indexOnSide / (float)pointsPerSide * sideLength;
            float x = 0, z = 0;

            switch (side) {
            case 0: // TOP
                x = -halfSide + step;
                z =  halfSide;
                break;
            case 1: // RIGHT
                x =  halfSide;
                z =  halfSide - step;
                break;
            case 2: // BOTTOM
                x =  halfSide - step;
                z = -halfSide;
                break;
            case 3: // LEFT
                x = -halfSide;
                z = -halfSide + step;
                break;
            default: // safety
                x = 0; z = 0;
                break;
            }
            pos->Offset->x = x;
            pos->Offset->z = z;
        }
        // 5. COLUMN (Single file)
        else if (formationType == Constants::FormationType::Column) {
            pos->Offset->x = 0;
            pos->Offset->z = -spacing * (i + 1);
            pos->Offset->z += 2.1f;
        }
        // 6. ECHELON LEFT
        else if (formationType == Constants::FormationType::EchelonLeft) {
            pos->Offset->x =  spacing * (i + 1);
            pos->Offset->z = -spacing * (i + 1);
            pos->Offset->z += 2.1f;
        }
        // 7. ECHELON RIGHT
        else if (formationType == Constants::FormationType::EchelonRight) {
            pos->Offset->x = -spacing * (i + 1);
            pos->Offset->z = -spacing * (i + 1);
            pos->Offset->z += 2.1f;
        }
        // 8. STAGGERED COLUMN
        else if (formationType == Constants::FormationType::StaggeredColumn) {
            pos->Offset->x = (i % 2 == 0) ? spacing : -spacing;
            pos->Offset->z = -spacing * (i + 1);
            pos->Offset->z += 2.0f;
        }
        // 9. WEDGE
        else if (formationType == Constants::FormationType::Wedge) {
            float side = (i % 2 == 0) ? -1.0f : 1.0f;
            int row = (i / 2) + 1;
            pos->Offset->x = side * spacing * row;
            pos->Offset->z = -spacing * row;
            pos->Offset->z += 2.0f;
        }
        // 10. COIL (Archimedean spiral)
        else if (formationType == Constants::FormationType::Coil) {
            const float angleStep    = 0.7f;
            const float startRadius  = 1.2f;
            const float radiusGrowth = 0.55f;

            float angle  = i * angleStep;
            float radius = startRadius + i * radiusGrowth;

            pos->Offset->x =  radius * std::cos(angle);
            pos->Offset->z = -radius * std::sin(angle);
        }
        // 11. HERRINGBONE
        else if (formationType == Constants::FormationType::Herringbone) {
            int   row  = i / 2;
            float side = (i % 2 == 0) ? -1.0f : 1.0f;
            float width = (row % 2 == 0) ? 1.6f : 0.7f;
            pos->Offset->x = side * spacing * width * (row + 1);
            pos->Offset->z = -spacing * (row + 1);
        }
        // 12. SKIRMISH (loose irregular line)
        else if (formationType == Constants::FormationType::Skirmish) {
            float side       = (i % 2 == 0) ? -1.0f : 1.0f;
            float multiplier = (i / 2) + 1;
            pos->Offset->x = side * spacing * multiplier * 1.3f;
            pos->Offset->z = -spacing * 0.4f * (1 + (i % 3));
        }
        // 13. BOX (filled rectangle)
        else if (formationType == Constants::FormationType::BoxFormation) {
            int cols = std::max(2, (int)std::ceil(std::sqrt((float)count)));
            int rows = (count + cols - 1) / cols;
            int col  = i % cols;
            int row  = i / cols;
            float halfW = (cols - 1) * spacing * 0.5f;
            float halfD = (rows - 1) * spacing * 0.5f;
            pos->Offset->x = -halfW + col * spacing;
            pos->Offset->z = -halfD + row * spacing;
        }

        i++;
    }
}

/**
 * @brief Dynamically creates formation positions based on the current type and count.
 * Calculates offsets for each ally slot and adds them as components.
 */
void Formation::formationCreate()
{
    if (!formationPositions)
        return;

    // 1. Cleanup
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
    float spacing = 0.5f;
    for (int i = 0; i < count; i++) {
        std::string name = "ally_" + std::to_string(i);
        addComponent(name);
        FormationPosition* pos = (*formationPositions)[name];
        if (!pos || !pos->Offset)
            continue;

        pos->Offset->y = 0;

        if (formationType == Constants::FormationType::Line) {
            float side = (i % 2 == 0) ? -1.0f : 1.0f;
            float multiplier = (i / 2) + 1;
            pos->Offset->x = side * spacing * multiplier;
            pos->Offset->z = 0;
        }
        else if (formationType == Constants::FormationType::V) {
            float side = (i % 2 == 0) ? -1.0f : 1.0f;
            float multiplier = (i / 2) + 1;
            pos->Offset->x = side * spacing * multiplier;
            pos->Offset->z = -spacing * multiplier;
        }
        else if (formationType == Constants::FormationType::Diamond) {
            if (count == 1) {
                pos->Offset->x = 0;
                pos->Offset->z = 0;
            } else {
                float radius = spacing * (count / 4.0f + 1);
                float t = (float)i / (float)count;
                float x = 0.0f, z = 0.0f;

                if (t < 0.25f) {
                    float u = t / 0.25f;
                    x =  u * radius;
                    z = -radius + u * radius;
                }
                else if (t < 0.50f) {
                    float u = (t - 0.25f) / 0.25f;
                    x =  radius - u * radius;
                    z =  u * radius;
                }
                else if (t < 0.75f) {
                    float u = (t - 0.50f) / 0.25f;
                    x = -u * radius;
                    z =  radius - u * radius;
                }
                else {
                    float u = (t - 0.75f) / 0.25f;
                    x = -radius + u * radius;
                    z = -u * radius;
                }
                pos->Offset->x = x;
                pos->Offset->z = z;
            }
        }
        else if (formationType == Constants::FormationType::Square) {
            int pointsPerSide = std::ceil((float)count / 4.0f);
            if (pointsPerSide < 1) pointsPerSide = 1;
            float sideLength = pointsPerSide * spacing;
            float halfSide   = sideLength / 2.0f;
            int side         = i / pointsPerSide;
            int indexOnSide  = i % pointsPerSide;
            float step       = (float)indexOnSide / (float)pointsPerSide * sideLength;
            float x = 0, z = 0;
            switch (side) {
            case 0: x = -halfSide + step; z =  halfSide; break;
            case 1: x =  halfSide;        z =  halfSide - step; break;
            case 2: x =  halfSide - step; z = -halfSide; break;
            case 3: x = -halfSide;        z = -halfSide + step; break;
            default: x = 0; z = 0; break;
            }
            pos->Offset->x = x;
            pos->Offset->z = z;
        }
        else if (formationType == Constants::FormationType::Column) {
            pos->Offset->x = 0;
            pos->Offset->z = -spacing * (i + 1);
        }
        else if (formationType == Constants::FormationType::EchelonLeft) {
            pos->Offset->x =  spacing * (i + 1);
            pos->Offset->z = -spacing * (i + 1);
        }
        else if (formationType == Constants::FormationType::EchelonRight) {
            pos->Offset->x = -spacing * (i + 1);
            pos->Offset->z = -spacing * (i + 1);
        }
        else if (formationType == Constants::FormationType::StaggeredColumn) {
            pos->Offset->x = (i % 2 == 0) ? spacing : -spacing;
            pos->Offset->z = -spacing * (i + 1);
        }
        else if (formationType == Constants::FormationType::Wedge) {
            float side = (i % 2 == 0) ? -1.0f : 1.0f;
            int row = (i / 2) + 1;
            pos->Offset->x = side * spacing * row;
            pos->Offset->z = -spacing * row;
        }
        // 10. COIL (Archimedean spiral)
        else if (formationType == Constants::FormationType::Coil) {
            const float angleStep    = 0.7f;
            const float startRadius  = 1.2f;
            const float radiusGrowth = 0.55f;

            float angle  = i * angleStep;
            float radius = startRadius + i * radiusGrowth;

            pos->Offset->x =  radius * std::cos(angle);
            pos->Offset->z = -radius * std::sin(angle);
        }
        else if (formationType == Constants::FormationType::Herringbone) {
            int   row  = i / 2;
            float side = (i % 2 == 0) ? -1.0f : 1.0f;

            float width = (row % 2 == 0) ? 1.6f : 0.7f;

            pos->Offset->x = side * spacing * width * (row + 1);
            pos->Offset->z = -spacing * (row + 1);
        }
        else if (formationType == Constants::FormationType::Skirmish) {
            float side       = (i % 2 == 0) ? -1.0f : 1.0f;
            float multiplier = (i / 2) + 1;
            pos->Offset->x = side * spacing * multiplier * 1.3f;
            pos->Offset->z = -spacing * 0.4f * (1 + (i % 3));
        }
        else if (formationType == Constants::FormationType::BoxFormation) {
            int cols = std::max(2, (int)std::ceil(std::sqrt((float)count)));
            int rows = (count + cols - 1) / cols;
            int col  = i % cols;
            int row  = i / cols;
            float halfW = (cols - 1) * spacing * 0.5f;
            float halfD = (rows - 1) * spacing * 0.5f;
            pos->Offset->x = -halfW + col * spacing;
            pos->Offset->z = -halfD + row * spacing;
        }
    }
}
/**
 * @brief Adds a formation position component.
 * @param name Name of the position (e.g., "mothership" or "ally_0").
 */
void Formation::addComponent(std::string name) {
    // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    FormationPosition* fp = new FormationPosition();

    // Generate unique component ID using formation ID + position name
    std::string componentID = ID + "_" + name;

    if(name == "mothership"){
        mothership = fp;
        emit root->componentAdded(QString::fromStdString(ID), QString::fromStdString(componentID), QString::fromStdString(name));
    }else{
        if(!formationPositions)return;
        (*formationPositions)[name] = fp;
        emit root->componentAdded(QString::fromStdString(ID), QString::fromStdString(componentID), QString::fromStdString(name));
    }

}

/**
 * @brief Removes a formation position component.
 * @param name Name of the position to remove.
 */
void Formation::removeComponent(std::string name) {
    // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if(!formationPositions)return;
    (*formationPositions).erase(name);
    emit root->componentRemoved(QString::fromStdString(ID), QString::fromStdString(name));
}

/**
 * @brief Returns a component's serialized data by name.
 * @param name Component name ("mothership" or ally name).
 * @return QJsonObject containing the formation position data.
 */
QJsonObject Formation::getComponent(std::string name) {
    if(name == "mothership" && mothership){
        qDebug() << "Getting mothership component";
        return mothership->toJson();
    }else{
        if(!formationPositions)return QJsonObject();
        if (formationPositions->find(name) != formationPositions->end()) {
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

/**
 * @brief Updates a component from JSON data.
 * Handles formation type, count, mothership, and individual ally updates.
 * @param name Component name.
 * @param obj JSON object containing update data.
 */
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
    if (name == "mothership" && mothership) {
        if (obj.contains("entity") && obj["entity"].isObject()) {
            QJsonObject entityObj = obj["entity"].toObject();
            if (entityObj.contains("id")) {
                std::string id = entityObj["id"].toString().toStdString();
                // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
                if (root && root->Entities.find(id) != root->Entities.end()) {
                    mothership->entity = (root->Entities)[id];
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
            // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            if (root && root->Entities.find(id) != root->Entities.end()) {
                pos->entity = root->Entities[id];

                // Link the platform's dynamic model to the formation logic
                Platform* platform = dynamic_cast<Platform*>(pos->entity);
                Platform* mother = dynamic_cast<Platform*>(mothership->entity);
                if (platform /*&& platform->dynamicModel*/ && mother) {
                    platform->dynamicModel.followEntity = mother;
                    platform->dynamicModel.formationPosition = pos;
                    platform->dynamicModel.follow = true;
                }
            }
        }
    }
    pos->fromJson(obj);
}

/**
 * @brief Converts formation enum to display string.
 * @param type Formation type.
 * @return String representation.
 */
QString Formation::formationTypeToString(Constants::FormationType type) const {
    switch (type) {
    case Constants::FormationType::Line:    return "Line";
    case Constants::FormationType::V:       return "V";
    case Constants::FormationType::Diamond: return "Diamond";
    case Constants::FormationType::Square:  return "Square";
    case Constants::FormationType::Column:  return "Column";
    case Constants::FormationType::EchelonLeft: return "Echelon Left";
    case Constants::FormationType::EchelonRight: return "Echelon Right";
    case Constants::FormationType::StaggeredColumn: return "Staggered Column";
    case Constants::FormationType::Wedge: return "Wedge";
    case Constants::FormationType::Coil:            return "Coil";
    case Constants::FormationType::Herringbone:     return "Herringbone";
    case Constants::FormationType::Skirmish:        return "Skirmish";
    case Constants::FormationType::BoxFormation:    return "Box";
    case Constants::FormationType::Custom: return "Custom";
    default: return "Line";
    }
}

/**
 * @brief Converts display string back to formation enum.
 * @param str String representation.
 * @return Corresponding FormationType.
 */

Constants::FormationType Formation::stringToFormationType(QString str) const {
    if (str == "Line")    return Constants::FormationType::Line;
    if (str == "V")       return Constants::FormationType::V;
    if (str == "Diamond") return Constants::FormationType::Diamond;
    if (str == "Square")  return Constants::FormationType::Square;
    if (str == "Column")  return Constants::FormationType::Column;
    if (str == "Echelon Left") return Constants::FormationType::EchelonLeft;
    if (str == "Echelon Right") return Constants::FormationType::EchelonRight;
    if (str == "Staggered Column") return Constants::FormationType::StaggeredColumn;
    if (str == "Wedge") return Constants::FormationType::Wedge;
    if (str == "Coil")             return Constants::FormationType::Coil;
    if (str == "Herringbone")      return Constants::FormationType::Herringbone;
    if (str == "Skirmish")         return Constants::FormationType::Skirmish;
    if (str == "Box")              return Constants::FormationType::BoxFormation;
    if (str == "Custom") return Constants::FormationType::Custom;
    return Constants::FormationType::Line;
}

/**
 * @brief Returns available formation type options for UI.
 * @return QStringList of formation names.
 */
QStringList Formation::formationTypeOptions() const {
    return {
        "Line", "V", "Diamond", "Square", "Column",
        "Echelon Left", "Echelon Right", "Staggered Column", "Wedge",
        "Coil", "Herringbone", "Skirmish", "Box",
        "Custom"
    };
}

/**
 * @brief Resolves entity references after loading from JSON.
 * Links formation positions to actual entities and enables follow behavior.
 * @param position FormationPosition to resolve.
 * @param obj JSON object containing entity reference.
 */
void Formation::resolveEntityReference(FormationPosition* position, const QJsonObject& obj) {
    if (obj.contains("entity") && obj["entity"].isObject()) {
        QJsonObject entityObj = obj["entity"].toObject();
        std::string entityId = entityObj["id"].toString().toStdString();
        std::string entityName = entityObj["name"].toString().toStdString();

        if (entityId != "dummy" && !entityId.empty()) {
            // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            if (root && root->Entities.find(entityId) != root->Entities.end()) {
                position->entity = root->Entities[entityId];

                if (position != mothership && mothership && mothership->entity) {
                    Platform* platform = dynamic_cast<Platform*>(position->entity);
                    Platform* mother = dynamic_cast<Platform*>(mothership->entity);
                    if (platform /*&& platform->dynamicModel*/ && mother) {
                        platform->dynamicModel.followEntity = mother;
                        platform->dynamicModel.formationPosition = position;
                        platform->dynamicModel.follow = true;
                    }
                }else
                    if(mothership && mothership->entity){
                        Platform* mother = dynamic_cast<Platform*>(mothership->entity);
                        if (mother /*&& mother->dynamicModel*/ && mother) {
                            mother->dynamicModel.formation = this;
                        }
                    }
            } else {
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
bool Formation::isMothershipEntity(const QString& entityId) const
{
    if (entityId.isEmpty() || !mothership || !mothership->entity)
        return false;
    return mothership->entity->ID == entityId.toStdString();
}

bool Formation::isAllyEntity(const QString& entityId) const
{
    if (entityId.isEmpty() || !formationPositions)
        return false;
    const std::string id = entityId.toStdString();
    for (const auto& pair : *formationPositions) {
        if (pair.second && pair.second->entity &&
            pair.second->entity->ID == id)
            return true;
    }
    return false;
}

void Formation::removeEntityFromFormation(const QString& entityId)
{
    if (entityId.isEmpty() || !root)
        return;

    if (isMothershipEntity(entityId))
        return;

    if (!formationPositions)
        return;

    const std::string id = entityId.toStdString();
    FormationPosition* posToClear = nullptr;

    for (auto& pair : *formationPositions) {
        if (pair.second && pair.second->entity &&
            pair.second->entity->ID == id) {
            posToClear = pair.second;
            break;
        }
    }
    if (!posToClear)
        return;

    if (Platform* p = dynamic_cast<Platform*>(posToClear->entity)) {
        p->dynamicModel.follow            = false;
        p->dynamicModel.followEntity      = nullptr;
        p->dynamicModel.formationPosition = nullptr;
        p->dynamicModel.formation         = nullptr;
        p->dynamicModel.inActiveTurn      = false;
    }

    posToClear->entity = nullptr;

    emit root->entityUpdate(entityId);
    emit root->entityComponentUpdate(
        QString::fromStdString(ID),
        QStringLiteral("formationPositions"),
        QJsonObject());
}

bool Formation::addEntityToFormation(Entity* entity)
{
    if (!entity || !root || !mothership || !mothership->entity)
        return false;

    Platform* p = dynamic_cast<Platform*>(entity);
    if (!p)
        return false;

    if (p->dynamicModel.formation || p->dynamicModel.formationPosition)
        return false;

    if (mothership->entity->ID == entity->ID)
        return false;

    Platform* mother = dynamic_cast<Platform*>(mothership->entity);
    if (!mother)
        return false;

    if (!formationPositions) {
        formationPositions =
            new std::unordered_map<std::string, FormationPosition*>();
    }

    FormationPosition* freeSlot = nullptr;
    std::string freeName;
    for (auto& pair : *formationPositions) {
        if (pair.second && !pair.second->entity) {
            freeSlot = pair.second;
            freeName = pair.first;
            break;
        }
    }

    if (!freeSlot) {
        freeName = "ally_" + std::to_string(formationPositions->size());
        addComponent(freeName);
        auto it = formationPositions->find(freeName);
        if (it == formationPositions->end() || !it->second)
            return false;
        freeSlot = it->second;
        count = static_cast<int>(formationPositions->size());
        FormationTypeUpdate();
    }

    freeSlot->entity = entity;

    p->dynamicModel.formation         = this;
    p->dynamicModel.followEntity      = mother;
    p->dynamicModel.formationPosition = freeSlot;
    p->dynamicModel.follow            = true;
    p->dynamicModel.inActiveTurn      = false;

    emit root->entityUpdate(QString::fromStdString(entity->ID));
    emit root->entityComponentUpdate(
        QString::fromStdString(ID),
        QStringLiteral("formationPositions"),
        QJsonObject());
    return true;
}
bool Formation::isFormationActive() const
{
    if (!formationPositions)
        return false;
    for (const auto& pair : *formationPositions) {
        if (!pair.second || !pair.second->entity)
            continue;
        if (Platform* p = dynamic_cast<Platform*>(pair.second->entity)) {
            if (p->dynamicModel.follow &&
                p->dynamicModel.formationPosition == pair.second)
                return true;
        }
    }
    return false;
}

void Formation::breakFormation()
{
    if (!root)
        return;

    if (formationPositions) {
        for (auto& pair : *formationPositions) {
            if (!pair.second || !pair.second->entity)
                continue;
            if (Platform* p = dynamic_cast<Platform*>(pair.second->entity)) {
                p->dynamicModel.follow            = false;
                p->dynamicModel.followEntity      = nullptr;
                p->dynamicModel.inActiveTurn      = false;
                emit root->entityUpdate(QString::fromStdString(p->ID));
            }
        }
    }


    emit root->entityUpdate(QString::fromStdString(ID));
    emit root->entityComponentUpdate(
        QString::fromStdString(ID),
        QStringLiteral("formationPositions"),
        QJsonObject());
}

void Formation::resumeFormation()
{
    if (!root || !mothership || !mothership->entity)
        return;

    Platform* mother = dynamic_cast<Platform*>(mothership->entity);
    if (!mother)
        return;

    // Mothership link restore
    mother->dynamicModel.formation = this;

    if (!formationPositions)
        return;

    for (auto& pair : *formationPositions) {
        FormationPosition* pos = pair.second;
        if (!pos || !pos->entity)
            continue;

        Platform* p = dynamic_cast<Platform*>(pos->entity);
        if (!p)
            continue;

        if (p->dynamicModel.formation && p->dynamicModel.formation != this)
            continue;

        p->dynamicModel.formation         = this;
        p->dynamicModel.followEntity      = mother;
        p->dynamicModel.formationPosition = pos;
        p->dynamicModel.follow            = true;
        p->dynamicModel.inActiveTurn      = false;

        emit root->entityUpdate(QString::fromStdString(p->ID));
    }

    emit root->entityUpdate(QString::fromStdString(ID));
    emit root->entityComponentUpdate(
        QString::fromStdString(ID),
        QStringLiteral("formationPositions"),
        QJsonObject());
}
