#include "formation.h"
#include "qjsonarray.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/GlobalRegistry.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Debug/console.h>

Formation::Formation(Hierarchy* h)
    : Entity(h), formationType(Constants::FormationType::Line), count(0),
    mothership(nullptr), formationPositions(new std::unordered_map<std::string,FormationPosition*>())
{
}
void Formation::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
    addComponent("mothership");

}

std::vector<std::string> Formation::getSupportedComponents() {
    std::vector<std::string> supported;
    return supported;
}

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
    obj["type"] = entityObj;

    return obj;
}

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
    if (obj.contains("type") && obj["type"].isObject()) {
        QJsonObject typeObj = obj["type"].toObject();
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
    // First, safely remove existing positions
    std::vector<std::string> keysToRemove;
    for (const auto& pair : *formationPositions) {
        keysToRemove.push_back(pair.first);
    }

    for (const auto& name : keysToRemove) {
        removeComponent(name);
    }

    // Clear the map to ensure no dangling pointers
    if (formationPositions) {
        formationPositions->clear();
    }

    // Create formation based on type
    if (formationType == Constants::FormationType::V) {
        float rearDistance = 0.01f;
        float sideDistance = 0.01f;

        // Ally_0: Behind and LEFT of mothership
        std::string name = "ally_" + std::to_string(0);
        addComponent(name);

        // SAFETY CHECK: Ensure the position was created
        if (formationPositions->find(name) != formationPositions->end()) {
            FormationPosition* pos = (*formationPositions)[name];
            if (pos) {
                // Ensure Offset exists
                if (!pos->Offset) {
                    pos->Offset = new Vector();
                }
                pos->Offset->x = -sideDistance;
                pos->Offset->y = 0;
                pos->Offset->z = -rearDistance;
            }
        }

        // Ally_1: Behind and RIGHT of mothership
        name = "ally_" + std::to_string(1);
        addComponent(name);

        // SAFETY CHECK: Ensure the position was created
        if (formationPositions->find(name) != formationPositions->end()) {
            FormationPosition* pos = (*formationPositions)[name];
            if (pos) {
                // Ensure Offset exists
                if (!pos->Offset) {
                    pos->Offset = new Vector();
                }
                pos->Offset->x = sideDistance;
                pos->Offset->y = 0;
                pos->Offset->z = -rearDistance;
            }
        }

        count = 2;
    }
    else if (formationType == Constants::FormationType::Line) {
        // Line formation: aircraft side by side
        float horizontalSpacing = 0.01f;

        // Ally_0 on LEFT side of mothership
        std::string name = "ally_" + std::to_string(0);
        addComponent(name);

        // SAFETY CHECK
        if (formationPositions->find(name) != formationPositions->end()) {
            FormationPosition* pos = (*formationPositions)[name];
            if (pos) {
                if (!pos->Offset) {
                    pos->Offset = new Vector();
                }
                pos->Offset->x = -horizontalSpacing;
                pos->Offset->y = 0;
                pos->Offset->z = 0;
            }
        }

        // Ally_1 on RIGHT side of mothership
        name = "ally_" + std::to_string(1);
        addComponent(name);

        // SAFETY CHECK
        if (formationPositions->find(name) != formationPositions->end()) {
            FormationPosition* pos = (*formationPositions)[name];
            if (pos) {
                if (!pos->Offset) {
                    pos->Offset = new Vector();
                }
                pos->Offset->x = horizontalSpacing;
                pos->Offset->y = 0;
                pos->Offset->z = 0;
            }
        }

        count = 2;
    }
    else if (formationType == Constants::FormationType::Diamond) {
        float firstRowDistance = 0.01f;
        float secondRowDistance = 0.02f;
        float sideDistance = 0.01f;

        // Ally_0: First row, LEFT side
        std::string name = "ally_" + std::to_string(0);
        addComponent(name);

        // SAFETY CHECK
        if (formationPositions->find(name) != formationPositions->end()) {
            FormationPosition* pos = (*formationPositions)[name];
            if (pos) {
                if (!pos->Offset) {
                    pos->Offset = new Vector();
                }
                pos->Offset->x = -sideDistance;
                pos->Offset->y = 0;
                pos->Offset->z = -firstRowDistance;
            }
        }

        // Ally_1: First row, RIGHT side
        name = "ally_" + std::to_string(1);
        addComponent(name);

        // SAFETY CHECK
        if (formationPositions->find(name) != formationPositions->end()) {
            FormationPosition* pos = (*formationPositions)[name];
            if (pos) {
                if (!pos->Offset) {
                    pos->Offset = new Vector();
                }
                pos->Offset->x = sideDistance;
                pos->Offset->y = 0;
                pos->Offset->z = -firstRowDistance;
            }
        }

        // Ally_2: Second row, CENTER (behind Ally_0 and Ally_1)
        name = "ally_" + std::to_string(2);
        addComponent(name);

        // SAFETY CHECK
        if (formationPositions->find(name) != formationPositions->end()) {
            FormationPosition* pos = (*formationPositions)[name];
            if (pos) {
                if (!pos->Offset) {
                    pos->Offset = new Vector();
                }
                pos->Offset->x = 0;
                pos->Offset->y = 0;
                pos->Offset->z = -secondRowDistance;
            }
        }

        count = 3;
    }
    else if(formationType == Constants::FormationType::Column) {
        float spacing = -0.01f; // Adjust this value for the distance behind

        for(int i = 0; i < 2; i++) {
            std::string name = "ally_" + std::to_string(i);
            addComponent(name);

            if (formationPositions->count(name) && (*formationPositions)[name]) {
                FormationPosition* pos = (*formationPositions)[name];
                if (!pos->Offset) pos->Offset = new Vector();

                // EXPLICITLY set every value to clear "Line" values
                pos->Offset->x = 0;             // Center horizontally
                pos->Offset->y = 0;             // Same height
                pos->Offset->z = spacing * (i + 1); // Move BEHIND (Z axis)
            }
        }
        count = 2;
    }
    // else if (formationType == Constants::FormationType::EchelonLeft) {
    //     float sideDistance = -0.015f; // Negative X moves to the Left
    //     float rearDistance = -0.015f; // Negative Z moves Behind

    //     for (int i = 0; i < 3; i++) {
    //         std::string name = "ally_" + std::to_string(i);
    //         addComponent(name);

    //         if (formationPositions->count(name) && (*formationPositions)[name]) {
    //             FormationPosition* pos = (*formationPositions)[name];
    //             if (!pos->Offset) pos->Offset = new Vector();

    //             // Each ally moves further left and further back than the one before it
    //             pos->Offset->x = sideDistance * (i + 1);
    //             pos->Offset->y = 0;
    //             pos->Offset->z = rearDistance * (i + 1);
    //         }
    //     }
    //     count = 3;
    // }
    else if (formationType == Constants::FormationType::EchelonLeft) {
        // If -0.015 put them on the right, then +0.015 will put them on the LEFT
        float sideDistance = 0.015f;
        float rearDistance = -0.015f; // Keep negative to stay BEHIND

        for (int i = 0; i < 3; i++) {
            std::string name = "ally_" + std::to_string(i);
            addComponent(name);

            if (formationPositions->count(name) && (*formationPositions)[name]) {
                FormationPosition* pos = (*formationPositions)[name];
                if (!pos->Offset) pos->Offset = new Vector();

                // Each aircraft steps further Left and further Back
                pos->Offset->x = sideDistance * (i + 1);
                pos->Offset->y = 0;
                pos->Offset->z = rearDistance * (i + 1);
            }
        }
        count = 3;
    }
    else if (formationType == Constants::FormationType::EchelonRight) {
        // sideDistance is negative because your engine uses Positive X for Left
        float sideDistance = -0.015f;
        float rearDistance = -0.015f;

        for (int i = 0; i < 3; i++) {
            std::string name = "ally_" + std::to_string(i);
            addComponent(name);

            if (formationPositions->count(name) && (*formationPositions)[name]) {
                FormationPosition* pos = (*formationPositions)[name];
                if (!pos->Offset) pos->Offset = new Vector();

                // Ally 0: x = -0.015, z = -0.015 (Right and Back)
                // Ally 1: x = -0.030, z = -0.030 (Further Right and Back)
                pos->Offset->x = sideDistance * (i + 1);
                pos->Offset->y = 0;
                pos->Offset->z = rearDistance * (i + 1);
            }
        }
        count = 3;
    }

    else if (formationType == Constants::FormationType::StaggeredColumn) {
        float sideOffset = 0.010f;  // Distance to the side
        float rearSpacing = -0.020f; // Distance behind

        for (int i = 0; i < 3; i++) {
            std::string name = "ally_" + std::to_string(i);
            addComponent(name);

            if (formationPositions->count(name) && (*formationPositions)[name]) {
                FormationPosition* pos = (*formationPositions)[name];
                if (!pos->Offset) pos->Offset = new Vector();

                // Alternate sides: Ally 0 (Left), Ally 1 (Right), Ally 2 (Left)
                // Based on your engine: Positive X = Left, Negative X = Right
                if (i % 2 == 0) {
                    pos->Offset->x = sideOffset;     // Left side
                } else {
                    pos->Offset->x = -sideOffset;    // Right side
                }

                pos->Offset->y = 0;
                pos->Offset->z = rearSpacing * (i + 1); // Progressively further back
            }
        }
        count = 3;
    }
    else if (formationType == Constants::FormationType::Wedge) {
        float sideSpacing = 0.010f;  // Horizontal distance
        float rearSpacing = -0.010f; // Distance behind leader

        for (int i = 0; i < 3; i++) {
            std::string name = "ally_" + std::to_string(i);
            addComponent(name);

            if (formationPositions->count(name) && (*formationPositions)[name]) {
                FormationPosition* pos = (*formationPositions)[name];
                if (!pos->Offset) pos->Offset = new Vector();

                pos->Offset->y = 0;

                if (i == 0) {
                    // First ally: Left and Back
                    pos->Offset->x = sideSpacing;
                    pos->Offset->z = rearSpacing;
                }
                else if (i == 1) {
                    // Second ally: Right and Back
                    pos->Offset->x = -sideSpacing;
                    pos->Offset->z = rearSpacing;
                }
                else if (i == 2) {
                    // Third ally: Further Left and Further Back
                    pos->Offset->x = sideSpacing * 2;
                    pos->Offset->z = rearSpacing * 2;
                }
            }
        }
        count = 3;
    }

    // else {
    //     // Default to line formation if unknown type
    //     qDebug() << "Unknown formation type, defaulting to Line";
    //     formationType = Constants::FormationType::Line;

    //     // Recursively call with default type
    //     formationCreate();
    // }
}


void Formation::addComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    FormationPosition* fp = new FormationPosition();
    if(name == "mothership"){
        mothership = fp;
    }else{
        (*formationPositions)[name] = fp;
    }
    emit parent->componentAdded(QString::fromStdString(ID),"ID", QString::fromStdString(name));

}
void Formation::removeComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    (*formationPositions).erase(name);
    emit parent->componentRemoved(QString::fromStdString(ID), QString::fromStdString(name));
}



QJsonObject Formation::getComponent(std::string name) {
    if(name == "mothership"){
        return mothership->toJson();
    }else{
        if (formationPositions->find(name) != formationPositions->end()) {
            // exists
            return (*formationPositions)[name]->toJson();
        }
    }

    return QJsonObject();
}

void Formation::updateComponent(QString name, const QJsonObject& obj) {
    // 1. Handle Formation Type change from UI Dropdown
    if (name == "type") {
        if (obj.contains("value")) {
            // Translate string "Column" to the Enum value
            formationType = stringToFormationType(obj["value"].toString());
            // Force recreation of allies with new Column offsets (X=0, Y=-0.1)
            formationCreate();
        }
        return;
    }

    // 2. Handle Mothership updates
    if(name == "mothership"){
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

    // 3. Handle Ally updates
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

// formation.cpp

QString Formation::formationTypeToString(Constants::FormationType type) const {
    switch (type) {
    case Constants::FormationType::Line:    return "Line";
    case Constants::FormationType::V:       return "V";
    case Constants::FormationType::Diamond: return "Diamond";
    case Constants::FormationType::Column:  return "Column";
    case Constants::FormationType::EchelonLeft: return "Echelon Left";
    case Constants::FormationType::EchelonRight: return "Echelon Right"; // Add this
    case Constants::FormationType::StaggeredColumn: return "Staggered Column";
    case Constants::FormationType::Wedge: return "Wedge";
    default: return "Line";
    }
}

Constants::FormationType Formation::stringToFormationType(QString str) const {
    if (str == "Line")    return Constants::FormationType::Line;
    if (str == "V")       return Constants::FormationType::V;
    if (str == "Diamond") return Constants::FormationType::Diamond;
    if (str == "Column")  return Constants::FormationType::Column;
    if (str == "Echelon Left") return Constants::FormationType::EchelonLeft;
    if (str == "Echelon Right") return Constants::FormationType::EchelonRight; // Add this
    if (str == "Staggered Column") return Constants::FormationType::StaggeredColumn;
    if (str == "Wedge") return Constants::FormationType::Wedge;
    return Constants::FormationType::Line;
}

QStringList Formation::formationTypeOptions() const {
    return {"Line", "V", "Diamond", "Column", "Echelon Left", "Echelon Right", "Staggered Column", "Wedge"};
}

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
