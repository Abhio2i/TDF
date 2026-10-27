/* =============================================================================
 * FILE:         hierarchyconnector.cpp
 * MODULE:       Hierarchy UI Connector
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the HierarchyConnector class, a singleton that
 *               provides connectivity between the core Hierarchy data model
 *               and UI components (HierarchyTree, TacticalDisplay, Inspector,
 *               QMainWindow). It manages signal/slot connections, file
 *               operations (new/open/save), copy/paste, library data
 *               initialisation, dummy data generation, recent projects,
 *               and feedback data collection.
 *
 * REQUIREMENTS: Implements REQ-CONN-010 through REQ-CONN-019
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-CONN-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#include "hierarchyconnector.h"                     // For hierarchy connector class
#include <QToolBar>                                // For toolbar handling
#include <QAction>                                 // For action handling
#include <QFileDialog>                             // For file dialog
#include <QJsonDocument>                           // For JSON document handling
#include <QJsonParseError>                         // For JSON parse errors
#include <QMessageBox>                             // For message box
#include <QStandardPaths>                          // For standard paths
#include "GUI/Hierarchytree/contextmenu.h"         // For context menu
#include "GUI/Menubars/menubar.h"                  // For menu bar
#include <QUuid>                                   // For UUID generation
#include "GUI/Editors/databaseeditor.h"            // For database editor
#include "GUI/Editors/scenarioeditor.h"            // For scenario editor
#include "GUI/Editors/runtimeeditor.h"             // For runtime editor
#include "GUI/Hierarchytree/contextmenu.h"         // For context menu
#include "core/Hierarchy/EntityProfiles/weapon.h"  // For Weapon type (weapon attach)
#include "core/Hierarchy/EntityProfiles/weapons/bomb.h"
#include "core/Hierarchy/EntityProfiles/weapons/missile.h"
#include "core/Hierarchy/EntityProfiles/weapons/sonobuoy.h"
#include "core/Hierarchy/EntityProfiles/weapons/torpedo.h"
#include "core/Hierarchy/EntityProfiles/weapons/artillery.h"
#include "core/Hierarchy/EntityProfiles/weapons/rocket.h"
#include "core/Hierarchy/EntityProfiles/weapons/flare.h"
#include "core/Hierarchy/EntityProfiles/weapons/chaff.h"
#include "core/Hierarchy/EntityProfiles/platform.h" // For Platform type (weapon attach)
#include "qapplication.h"                          // For application instance
#include "GUI/Tacticaldisplay/canvaswidget.h"      // For canvas widget
#include <QSettings>                               // For settings storage
#include <core/Utility/uuid.h>                     // For UUID utilities
#include <QRandomGenerator>                        // For random number generation
#include <core/Hierarchy/Utils/entityutils.h>      // For entity utilities
#include <GUI/Hierarchytree/addformationdialog.h>
#include <GUI/Settings/applicationdialog.h>
#include "Setup.h"
#include "GUI/Hierarchytree/addweapondialog.h"
#include "GUI/Tacticaldisplay/entityinfodialog.h"
#include "GUI/mainwindow.h"
#include <QStandardPaths>
#include <QFileInfo>
#include "core/Hierarchy/ECS/Struct/Meta_Data_Component.h"
#include "GUI/Canvas/map_with_canvas.h"
#include "GUI/dialogwindowutils.h"

// %%% Static Instance %%%

static void changeFormationMothership(Formation* form, const QString& newMotherId)
{
    if (!form || !form->mothership || !form->root) return;
    Hierarchy* root = form->root;
    auto newIt = root->Entities.find(newMotherId.toStdString());
    if (newIt == root->Entities.end()) return;

    Entity* newMotherEntity = newIt->second;
    Platform* newMother = dynamic_cast<Platform*>(newMotherEntity);
    if (!newMother) return;

    // 1. Clear old mothership
    Platform* oldMother = nullptr;
    if (form->mothership->entity) {
        oldMother = dynamic_cast<Platform*>(form->mothership->entity);
        if (oldMother) {
            oldMother->dynamicModel.formation = nullptr;
            oldMother->dynamicModel.follow = false;
            oldMother->dynamicModel.followEntity = nullptr;
            oldMother->dynamicModel.formationPosition = nullptr;
        }
    }

    // 2. Free slot if new mother was an ally
    std::string freedSlot;
    if (form->formationPositions) {
        for (auto& pair : *form->formationPositions) {
            if (pair.second && pair.second->entity &&
                pair.second->entity->ID == newMotherId.toStdString()) {
                pair.second->entity = nullptr;
                freedSlot = pair.first;
                break;
            }
        }
    }

    // 3. Set new mothership
    form->mothership->entity = newMotherEntity;
    newMother->dynamicModel.formation = form;
    newMother->dynamicModel.follow = false;
    newMother->dynamicModel.followEntity = nullptr;
    newMother->dynamicModel.formationPosition = nullptr;

    // 4. Old mother → empty ally slot
    if (oldMother && form->formationPositions) {
        FormationPosition* targetSlot = nullptr;
        if (!freedSlot.empty()) {
            auto it = form->formationPositions->find(freedSlot);
            if (it != form->formationPositions->end())
                targetSlot = it->second;
        }
        if (!targetSlot) {
            for (auto& pair : *form->formationPositions) {
                if (pair.second && !pair.second->entity) {
                    targetSlot = pair.second;
                    break;
                }
            }
        }
        if (targetSlot) {
            targetSlot->entity = oldMother;
            oldMother->dynamicModel.followEntity = newMother;
            oldMother->dynamicModel.formationPosition = targetSlot;
            oldMother->dynamicModel.follow = true;
        }
    }

    // 5. Re-link remaining allies
    if (form->formationPositions) {
        for (auto& pair : *form->formationPositions) {
            if (!pair.second || !pair.second->entity) continue;
            Platform* p = dynamic_cast<Platform*>(pair.second->entity);
            if (!p) continue;
            p->dynamicModel.followEntity = newMother;
            p->dynamicModel.formationPosition = pair.second;
            p->dynamicModel.follow = true;
        }
    }
    // 6. Refresh geometry
    form->FormationTypeUpdate();
}
double dmsToDecimal(const QString& dms)
{
    if (dms.isEmpty())
        return 0.0;
    QChar direction = dms.at(0); // N, S, E, W
    QString value = dms.mid(1);
    QStringList parts = value.split(':');
    if (parts.size() != 3)
        return 0.0;
    double degrees = parts[0].toDouble();
    double minutes = parts[1].toDouble();
    double seconds = parts[2].toDouble();
    double decimal =
        degrees +
        (minutes / 60.0) +
        (seconds / 3600.0);
    // South & West negative hote hain
    if (direction == 'S' || direction == 'W')
        decimal = -decimal;
    return decimal;
}

/* Singleton instance */
HierarchyConnector* HierarchyConnector::m_instance = nullptr;

// %%% Constructor %%%
/* Initialize hierarchy connector */
HierarchyConnector::HierarchyConnector(QObject* parent)
    : QObject(parent), hierarchy(nullptr), library(nullptr), libTreeView(nullptr)
{
    // Initialize pointers to null
}

// %%% Singleton Instance Accessor %%%
/* Get singleton instance */
HierarchyConnector* HierarchyConnector::instance()
{
    // Create instance if null
    if (!m_instance) {
        m_instance = new HierarchyConnector();
    }
    return m_instance;
}

// %%% File Path Management %%%
/* Get last saved file path for editor */
QString HierarchyConnector::getLastSavedFilePath(QMainWindow* parent)
{
    // Check for DatabaseEditor
    if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(parent)) {
        return dbEditor->lastSavedFilePath;
    }
    // Check for ScenarioEditor
    else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(parent)) {
        return scEditor->lastSavedFilePath;
    }
    // Check for RuntimeEditor
    else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(parent)) {
        return rtEditor->lastSavedFilePath;
    }
    // Return empty string if no match
    return QString();
}

static Weapon* createWeapon(const QString& typeName, Hierarchy* h)
{
    if (typeName == "Bomb")      return new Bomb(h);
    if (typeName == "Sonobuoy")  return new Sonobuoy(h);
    if (typeName == "Torpedo")   return new Torpedo(h);
    if (typeName == "Artillery") return new Artillery(h);
    if (typeName == "Rocket")    return new Rocket(h);
    if (typeName == "Flare")     return new Flare(h);
    if (typeName == "Chaff")     return new Chaff(h);
    return new Missile(h);
}
// %%% Signal Connections %%%
/* Connect signals for hierarchy and UI components */
void HierarchyConnector::connectSignals(Hierarchy* hierarchy, Hierarchy* library, HierarchyTree* treeView,
                                        TacticalDisplay* tactical, Inspector* inspector,
                                        MapWithCanvas* apiMapWidget)
{
    if (!hierarchy || !treeView) {
        return;
    }
    treeView->getContextMenu()->m_hierarchy = hierarchy;
    // ── Helper: Entity type → Profile type string ─────────────────────────
    auto getEntityProfileType = [](Hierarchy* src, const QString& sourceId) -> QString {
        if (!src) return "";
        auto it = src->Entities.find(sourceId.toStdString());
        if (it == src->Entities.end() || !it->second) return "";
        switch (it->second->type) {
        case Constants::EntityType::Platform:    return "Platform";
        case Constants::EntityType::Sensor:      return "Sensor";
        case Constants::EntityType::Radio:       return "Radio";
        case Constants::EntityType::IFF:         return "IFF";
        case Constants::EntityType::Weapon:      return "Weapon";
        case Constants::EntityType::Formation:   return "Formation_blocked";
        case Constants::EntityType::FixedPoint:  return "FixedPoints";
        case Constants::EntityType::SpecialZone: return "SpecialZone";
        default: return "";
        }
    };

    // ── Helper: Target profile type string ───────────────────────────────
    auto getTargetProfileType = [](Hierarchy* hier, const QString& targetId,
                                   bool isProfile, bool isFolder) -> QString {
        if (!hier) return "";
        auto getTypeStr = [](Constants::EntityType t) -> QString {
            switch (t) {
            case Constants::EntityType::Platform:    return "Platform";
            case Constants::EntityType::Sensor:      return "Sensor";
            case Constants::EntityType::Radio:       return "Radio";
            case Constants::EntityType::IFF:         return "IFF";
            case Constants::EntityType::Weapon:      return "Weapon";
            case Constants::EntityType::Formation:   return "Formation_blocked";
            case Constants::EntityType::FixedPoint:  return "FixedPoints";
            case Constants::EntityType::SpecialZone: return "SpecialZone";
            default: return "";
            }
        };
        if (isProfile) {
            auto it = hier->ProfileCategories.find(targetId.toStdString());
            if (it == hier->ProfileCategories.end() || !it->second) return "";
            return getTypeStr(it->second->type);
        }
        if (isFolder) {
            auto fit = hier->Folders.find(targetId.toStdString());
            if (fit == hier->Folders.end() || !fit->second) return "";
            std::string parentId = fit->second->parentID;
            auto pit = hier->ProfileCategories.find(parentId);
            if (pit == hier->ProfileCategories.end()) {
                auto ffit = hier->Folders.find(parentId);
                if (ffit != hier->Folders.end() && ffit->second) {
                    parentId = ffit->second->parentID;
                    pit = hier->ProfileCategories.find(parentId);
                }
            }
            if (pit == hier->ProfileCategories.end() || !pit->second) return "";
            return getTypeStr(pit->second->type);
        }
        return "";
    };

    // ── Hierarchy → TreeView ─────────────────────────────────────────────
    connect(hierarchy, &Hierarchy::profileAdded, treeView, &HierarchyTree::profileAdded);
    connect(hierarchy, &Hierarchy::folderAdded, treeView, &HierarchyTree::folderAdded);
    connect(hierarchy, &Hierarchy::entityAdded, treeView,
            [=](QString parentID, QString ID, QString entityName) {
                treeView->entityAdded(parentID, ID, entityName);
                QTimer::singleShot(0, treeView, [=]() {
                    auto it = hierarchy->Entities.find(ID.toStdString());
                    if (it != hierarchy->Entities.end()) {
                        if (!it->second->Active) {
                            treeView->setEntityActiveState(ID, false);
                        }
                    }
                });
            });

    connect(hierarchy, &Hierarchy::profileRemoved,    treeView, &HierarchyTree::profileRemoved);
    connect(hierarchy, &Hierarchy::folderRemoved,     treeView, &HierarchyTree::folderRemoved);
    connect(hierarchy, &Hierarchy::entityRemoved,     treeView, &HierarchyTree::entityRemoved);
    connect(hierarchy, &Hierarchy::componentAdded,    treeView, &HierarchyTree::componentAdded);
    connect(hierarchy, &Hierarchy::componentRemoved,  treeView, &HierarchyTree::componentRemoved);
    connect(hierarchy, &Hierarchy::subComponentAdded, treeView, &HierarchyTree::subComponentAdded);
    connect(hierarchy, &Hierarchy::subComponentRemoved, treeView, &HierarchyTree::subComponentRemoved);
    connect(hierarchy, &Hierarchy::subComponentRenamed, treeView, &HierarchyTree::subComponentRenamed);

    // ── ContextMenu → Hierarchy (profile / folder / subcomponent) ────────
    connect(treeView->getContextMenu(), &ContextMenu::addProfileRequested,
            hierarchy, &Hierarchy::addProfileCategaory);
    connect(treeView->getContextMenu(), &ContextMenu::removeProfileRequested,
            hierarchy, &Hierarchy::removeProfileCategaory);
    connect(treeView->getContextMenu(), &ContextMenu::addFolderRequested,
            hierarchy, &Hierarchy::addFolder);
    connect(treeView->getContextMenu(), &ContextMenu::removeFolderRequested,
            hierarchy, &Hierarchy::removeFolder);
    connect(treeView->getContextMenu(), &ContextMenu::removeSubComponentRequested,
            hierarchy, &Hierarchy::removeSubComponent);
    connect(treeView->getContextMenu(), &ContextMenu::removeEntityRequested,
            hierarchy, &Hierarchy::removeEntity);
    connect(treeView->getContextMenu(), &ContextMenu::removeComponentRequested,
            hierarchy, &Hierarchy::removeComponent);

    // ── Add Entity ───────────────────────────────────────────────────────
    connect(treeView->getContextMenu(), &ContextMenu::addEntityRequested,
            this, [=](QString parentId, QString entityName, bool isProfileParent,
                QVariantMap components, AddItemDialog* dialog, QString sensortype,
                double initLat, double initlon, float heading) {

                if (!dialog) return;
                if (!confirmAndLoadAssociatedDatabase(hierarchy, treeView)) {
                    return;
                }
                QString sensorType = dialog->getSensorType();
                Entity* newEntity = hierarchy->addEntity(parentId, entityName, isProfileParent, sensorType);

                if (!newEntity) return;

                if (!dialog->getSelectedEntityId().isEmpty()) {
                    Entity* copyentity = library->getEntityById(dialog->getSelectedEntityId());
                    if (copyentity) {
                        newEntity->category = copyentity->category;
                        newEntity->airCategory = copyentity->airCategory;
                        newEntity->groundCategory = copyentity->groundCategory;
                        newEntity->marineCategory = copyentity->marineCategory;
                    }
                }

                if (sensorType != "Generic" && newEntity) {
                    Sensor* sensorEntity = dynamic_cast<Sensor*>(newEntity);
                    if (sensorEntity) {
                        Sensor::SubType subTypeEnum = Sensor::getSubTypeFromString(sensorType);
                        sensorEntity->subType = subTypeEnum;
                    }
                }

                QSet<QString> addedComponents;
                for (const auto& component : components.keys()) {
                    if (components.value(component).toBool() && !addedComponents.contains(component)) {
                        try {
                            newEntity->addComponent(component.toStdString());
                            addedComponents.insert(component);
                        } catch (const std::exception& e) {}
                    }
                }

                addedComponents.insert("transform");
                addedComponents.insert("rigidbody");
                if (!dialog->getSelectedEntityId().isEmpty()) {
                    Entity* copyentity = library->getEntityById(dialog->getSelectedEntityId());
                    if (copyentity) {
                        std::string id         = newEntity->ID;
                        std::string parentID   = newEntity->parentID;
                        std::string originalName = newEntity->Name;
                        newEntity->fromJson(copyentity->toJson());
                        newEntity->ID        = id;
                        newEntity->parentID  = parentID;
                        newEntity->Name      = originalName;
                        newEntity->ReferenceID = copyentity->ID;
                    }
                }

                auto* data = newEntity->root->world.GetComponent<ECS::Entity_Data>(newEntity->ecsID);
                if(data  && !hierarchy->isDatabase){
                    newEntity->Active = false;
                    data->active = false;
                }

                Platform* platform = dynamic_cast<Platform*>(newEntity);

                if (platform) {
                    if (initLat < 190) {
                        platform->transform.setGeoCord(initLat, initlon);
                    }

                    if (dialog->isScEnabled()) {
                        auto* data = platform->root->world.GetComponent<ECS::Entity_Data>(platform->ecsID);
                        data->active = true;
                        platform->Active = true;
                        const float range = dialog->getRange() / 2;
                        QPointF city = dialog->getCity();
                        double lat = city.x() + ((QRandomGenerator::global()->generateDouble() - 0.5) * 2 * (range / 111.111));
                        double lon = city.y() + ((QRandomGenerator::global()->generateDouble() - 0.5) * 2 * (range / 100));
                        double heading = 360 * QRandomGenerator::global()->generateDouble();

                        if (initLat < 190) {
                            lat = initLat;
                            lon = initlon;
                        }
                        platform->transform.setGeoCord(lat, lon);
                        platform->transform.setFromEulerAngles(QVector3D(0, heading, 0));

                        QString trajectoryType   = dialog->customDialog.getShape();
                        int     waypointcount    = dialog->customDialog.getWaypointsCount();
                        float   trajectorylength = dialog->customDialog.getLength();
                        if (trajectoryType == "Circle" || trajectoryType == "Spiral")
                            trajectorylength = dialog->customDialog.getRadius();

                        float waypointoffsetlength = trajectorylength / waypointcount;
                        float minSpeed    = dialog->customDialog.getMinSpeed();
                        float maxSpeed    = dialog->customDialog.getMaxSpeed();
                        float speed       = minSpeed;
                        float speedOffset = (maxSpeed - minSpeed) / waypointcount;
                        float minAltitude  = dialog->customDialog.getMinAltitude();
                        float maxAltitude  = dialog->customDialog.getMaxAltitude();
                        float Altitude     = minAltitude;
                        float AltitudeOffset = (maxAltitude - minAltitude) / waypointcount;
                        float head       = heading;
                        float headOffset = 360 / waypointcount;

                        // if (platform->category == Entity::SubMarineCategory::Submarine) {
                        //     Altitude = 0;
                        // }

                        Altitude = 0;
                        if (waypointcount > 1) {
                            Waypoints* newWaypoint = new Waypoints();
                            newWaypoint->position  = new Vector(lat, Altitude, lon);
                            newWaypoint->speed     = speed;
                            platform->trajectory.addTrajectory(newWaypoint);
                        }
                        if (platform->category == Entity::Category::Marine && platform->marineCategory == Entity::SubMarineCategory::Submarine) {
                            Altitude -= AltitudeOffset;
                        }
                        if (platform->category == Entity::Category::Air ) {
                            Altitude += AltitudeOffset;
                        }
                        for (int i = 0; i < waypointcount; i++) {
                            auto [latAtRadius, lonAtRadius] = calculateNewLatLong(lat, lon, head, waypointoffsetlength);
                            Waypoints* newWaypoint = new Waypoints();
                            newWaypoint->position  = new Vector(latAtRadius, Altitude, lonAtRadius);
                            newWaypoint->speed     = speed;
                            platform->trajectory.addTrajectory(newWaypoint);
                            speed    += speedOffset;
                            if (platform->category == Entity::Category::Marine) {
                                Altitude -= AltitudeOffset;
                                Altitude = Altitude > 0 ? 0 : Altitude;
                                if(platform->marineCategory != Entity::SubMarineCategory::Submarine){
                                    Altitude = 0;
                                }
                            }else
                            if (platform->category == Entity::Category::Ground) {
                                Altitude = 0;
                            }
                            else {
                                Altitude += AltitudeOffset;
                            }

                            if (dialog && trajectoryType == "Zigzag") {
                                lat = latAtRadius + (QRandomGenerator::global()->generateDouble() - 0.5) * 0.1;
                                lon = lonAtRadius + (QRandomGenerator::global()->generateDouble() - 0.5) * 0.1;
                            } else if (trajectoryType == "Circle") {
                                if (dialog->customDialog.getStartPoint() == "intial") { lat = latAtRadius; lon = lonAtRadius; }
                                if (dialog->customDialog.getCirclePoint() == "Clockwise") head += headOffset;
                                else head -= headOffset;
                            } else if (trajectoryType == "Spiral") {
                                if (dialog->customDialog.getStartPoint() == "intial") { lat = latAtRadius; lon = lonAtRadius; }
                                if (dialog->customDialog.getCirclePoint() == "Clockwise") head += headOffset * (1 + waypointcount / 100);
                                else head -= headOffset * (1 + waypointcount / 100);
                                waypointoffsetlength *= 1.02;
                            } else {
                                lat = latAtRadius;
                                lon = lonAtRadius;
                            }
                        }
                        emit hierarchy->trajectoryisAdded(platform->ID.c_str(), &platform->trajectory);

                        if (dialog->isScSensorsEnabled()) {
                            std::string id = platform->sensors.ID;
                            hierarchy->addSubComponent(QString::fromStdString(id), "Aesa_" + entityName, "AESA");
                            hierarchy->addSubComponent(QString::fromStdString(id), "ESM_"   + entityName, "ESM");
                            hierarchy->addSubComponent(QString::fromStdString(id), "CSM_"   + entityName, "CSM");
                        }

                        if (dialog->isScIffEnabled()) {
                            std::string id = platform->iffs.ID;
                            hierarchy->addSubComponent(QString::fromStdString(id), "IFF_" + entityName);
                        }

                        if (dialog->isScRadioEnabled()) {
                            std::string id = platform->radios.ID;
                            hierarchy->addSubComponent(QString::fromStdString(id), "Radio" + entityName);
                        }
                        // platform->dynamicModel.moveSpeed = 0;
                        platform->dynamicModel.Altitude  = 0;

                        if (dialog->isScSensorsEnabled()) {
                            const int minRadRng = dialog->getMinRadarRange();
                            const int maxRadRng = dialog->getMaxRadarRange();
                            for (auto const& pair : platform->sensors.sensors) {
                                Sensor* s = pair.second;
                                s->range = minRadRng + (QRandomGenerator::global()->generateDouble() * (maxRadRng - minRadRng));
                            }
                        }

                        if (dialog->isScRadioEnabled()) {
                            const int minRadioRng = dialog->getMinRadioRange();
                            const int maxRadioRng = dialog->getMaxRadioRange();
                            for (auto const& pair : platform->radios.radios) {
                                Radio* r = pair.second;
                                r->Range = minRadioRng + (QRandomGenerator::global()->generateDouble() * (maxRadioRng - minRadioRng));
                            }
                        }
                        QCoreApplication::processEvents();
                    }
                }

                QString selectedTeam = dialog->getSelectedTeam();
                if (!selectedTeam.isEmpty() && newEntity) {
                    QJsonObject delta;
                    QJsonArray options;
                    const QStringList teamList = {"RedTeam","BlueTeam","GreenTeam","YellowTeam",
                                                  "GreyTeam","AlphaTeam","BetaTeam","GammaTeam"};
                    for (const QString& t : teamList) options.append(t);
                    QJsonObject teamObj;
                    teamObj["type"]    = "option";
                    teamObj["options"] = options;
                    teamObj["value"]   = selectedTeam;
                    delta["Team"] = teamObj;
                    hierarchy->UpdateComponent(QString::fromStdString(newEntity->ID), "_self", delta);
                }
            });

    // ── Add Component (Radio / Sensor / IFF / Weapon) from component node ─
    connect(treeView->getContextMenu(), &ContextMenu::addComponentRequested, this,
            [=](QString entityID, QString componentType, QString componentName,
                QString sensorType, QString sensorProfileId) {
                if (componentType == "iffs") {
                    if (sensorProfileId.isEmpty()) return;
                    if (!confirmAndLoadAssociatedDatabase(hierarchy, treeView)) return;
                    QJsonObject obj = (library->Iffs)[sensorProfileId.toStdString()]->toJson();
                    hierarchy->addSubComponent(entityID,
                                               componentName, sensorType, sensorProfileId, obj);
                } else if (componentType == "sensors") {
                    if (sensorProfileId.isEmpty()) return;
                    if (!confirmAndLoadAssociatedDatabase(hierarchy, treeView)) return;
                    QJsonObject obj = (library->Sensors)[sensorProfileId.toStdString()]->toJson();
                    hierarchy->addSubComponent(entityID,
                                               componentName, sensorType, sensorProfileId, obj);
                    hierarchy->attachSensors(entityID, componentName, sensorType);
                } else if (componentType == "radios") {
                    if (sensorProfileId.isEmpty()) return;
                    if (!confirmAndLoadAssociatedDatabase(hierarchy, treeView)) return;
                    QJsonObject obj = (library->Radios)[sensorProfileId.toStdString()]->toJson();
                    hierarchy->addSubComponent(entityID,
                                               componentName, sensorType, sensorProfileId, obj);
                } else if (componentType == "weapons") {
                    try {
                        if (entityID.isEmpty() || componentName.isEmpty()) {
                            QMessageBox::warning(nullptr, "Validation Error", "Invalid weapon data provided");
                            return;
                        }

                        auto it = hierarchy->Entities.find(entityID.toStdString());
                        if (it == hierarchy->Entities.end()) {
                            QMessageBox::warning(nullptr, "Error", "Parent entity not found");
                            return;
                        }
                        Platform* platform = dynamic_cast<Platform*>(it->second);
                        if (!platform) {
                            QMessageBox::critical(nullptr, "Type Error", "Parent entity must be a Platform");
                            return;
                        }

                        Weapon* weapon = createWeapon(sensorType, hierarchy);
                        if (!weapon) {
                            QMessageBox::critical(nullptr, "Error", "Failed to create weapon");
                            return;
                        }

                        weapon->Name         = componentName.toStdString();
                        weapon->parentEntity = platform;
                        weapon->ID           = Uuid::generateShortUniqueID();
                        if (/*platform->weapons &&*/ platform->weapons.weapons) {
                            platform->weapons.weapons->insert({weapon->ID, weapon});
                            hierarchy->Weapons.insert({weapon->ID, weapon});
                            hierarchy->Entities.insert({weapon->ID, weapon});

                            emit hierarchy->subComponentAdded(
                                QString::fromStdString(platform->weapons.ID),
                                QString::fromStdString(weapon->ID),
                                componentName);

                            QString weaponQID = QString::fromStdString(weapon->ID);
                            for (const std::string& compName : weapon->getSupportedComponents()) {
                                std::string compID;
                                if      (compName == "transform"    )      compID = weapon->transform.ID;
                                else if (compName == "rigidbody"    )      compID = weapon->rigidbody.ID;
                                else if (compName == "collider"     )       compID = weapon->collider.ID;
                                else if (compName == "trajectory"   )     compID = weapon->trajectory.ID;
                                else if (compName == "bitmap"       ) compID = weapon->meshRenderer2d.ID;
                                else if (compName == "dynamicModel" )   compID = weapon->dynamicModel.ID;
                                else if (compName == "crossSection" )   compID = weapon->crossSection.ID;
                                else continue;
                                emit hierarchy->componentAdded(
                                    weaponQID,
                                    QString::fromStdString(compID),
                                    QString::fromStdString(compName));
                            }
                        } else {
                            QMessageBox::critical(nullptr, "Error", "Failed to initialize weapon system");
                            delete weapon;
                        }
                    } catch (const std::exception& e) {
                        QMessageBox::critical(nullptr, "Error",
                                              QString("Failed to create weapon: %1").arg(e.what()));
                    }
                }
            });
    // ── Single entity context menu → bulk handlers ───────────────────────
    connect(treeView->getContextMenu(), &ContextMenu::setEntityActiveRequested,
            this, [=](QString entityID, bool active) {
                auto it = hierarchy->Entities.find(entityID.toStdString());
                if (it == hierarchy->Entities.end() || !it->second) return;

                QJsonObject delta;
                delta["active"] = active;
                hierarchy->UpdateComponent(entityID, "_self", delta);
                treeView->setEntityActiveState(entityID, active);

                Entity* entity = it->second;
                entity->Active = active;
                if (Platform* p = dynamic_cast<Platform*>(entity))
                    p->Active = active;
                if (ECS::Entity_Data* data =
                    hierarchy->world.GetComponent<ECS::Entity_Data>(entity->ecsID)) {
                    data->active = active;
                }
                if (apiMapWidget)
                    apiMapWidget->refreshAllLayers();
            });

    connect(treeView->getContextMenu(), &ContextMenu::addWeaponToEntityRequested,
            this, [=](QVariantMap entityData) {
                QList<QVariantMap> entities;
                entities.append(entityData);
                emit treeView->addWeaponToEntitiesRequested(entities);
            });

    connect(treeView->getContextMenu(), &ContextMenu::addSensorToEntityRequested,
            this, [=](QVariantMap entityData) {
                QList<QVariantMap> entities;
                entities.append(entityData);
                emit treeView->addSensorToEntitiesRequested(entities);
            });

    connect(treeView->getContextMenu(), &ContextMenu::addIFFToEntityRequested,
            this, [=](QVariantMap entityData) {
                QList<QVariantMap> entities;
                entities.append(entityData);
                emit treeView->addIFFToEntitiesRequested(entities);
            });

    connect(treeView->getContextMenu(), &ContextMenu::addRadioToEntityRequested,
            this, [=](QVariantMap entityData) {
                QList<QVariantMap> entities;
                entities.append(entityData);
                emit treeView->addRadioToEntitiesRequested(entities);
            });

    connect(treeView->getContextMenu(), &ContextMenu::addTeamToEntityRequested,
            this, [=](QVariantMap entityData, QString team) {
                QList<QVariantMap> entities;
                entities.append(entityData);
                emit treeView->addTeamToEntitiesRequested(entities, team);
            });

    // ── Bulk: Copy / Paste ───────────────────────────────────────────────
    connect(treeView, &HierarchyTree::copyItemsRequested, this,
            [this, hierarchy](QList<QVariantMap> dataList) {
                if (dataList.isEmpty()) return;
                copySource = hierarchy;
            });

    // ── Shared paste logic ───────────────────────────────────────────────
    auto executePaste = [this, hierarchy, getEntityProfileType, getTargetProfileType]
        (QVariantMap targetData, QList<QVariantMap> itemsToPaste) {
            if (!copySource) {
                QMessageBox::warning(nullptr, "Paste Error", "No items copied. Please copy items first.");
                return;
            }
            if (itemsToPaste.isEmpty()) return;
            QString targetId, targetType;
            if (targetData["type"].type() == QVariant::Map) {
                QVariantMap typeData = targetData["type"].toMap();
                targetType = (typeData.contains("type") && typeData["type"].toString() == "option")
                                 ? "profile" : targetData["type"].toString();
            } else {
                targetType = targetData["type"].toString();
            }
            targetId = targetData["ID"].toString();
            bool isProfile = (targetType == "profile");
            bool isFolder  = (targetType == "folder");
            QString targetProfileType = getTargetProfileType(hierarchy, targetId, isProfile, isFolder);
            QApplication::processEvents();
            for (const QVariantMap& itemData : itemsToPaste) {
                QString sourceId = itemData["ID"].toString();
                QString sourceProfileType = getEntityProfileType(copySource, sourceId);
                if (sourceProfileType == "Formation_blocked") {
                    QMessageBox::warning(nullptr, "Paste Not Allowed",
                                         "Formation entities cannot be copied or pasted.");
                    return;
                }
                if (targetProfileType == "Formation_blocked") {
                    QMessageBox::warning(nullptr, "Paste Not Allowed",
                                         "Cannot paste into Formation category.");
                    return;
                }
                if (!sourceProfileType.isEmpty() && !targetProfileType.isEmpty()
                    && sourceProfileType != targetProfileType) {
                    QMessageBox::warning(nullptr, "Paste Error",
                                         QString("Cannot paste '%1' entity into '%2' category.\n"
                                                 "Only '%3' entities can be pasted here.")
                                             .arg(sourceProfileType)
                                             .arg(targetProfileType)
                                             .arg(targetProfileType));
                    return;
                }
                try {
                    auto entityIt = copySource->Entities.find(sourceId.toStdString());
                    if (entityIt != copySource->Entities.end()) {
                        QJsonObject entityJson = entityIt->second->toJson();
                        entityJson["id"]        = QString::fromStdString(Uuid::generateShortUniqueID());
                        entityJson["parent_id"] = targetId;
                        hierarchy->addEntityFromJson(targetId, entityJson, isProfile);
                    }
                } catch (const std::exception& e) {}
                QApplication::processEvents();
            }
        };

    // ── Both connects now just delegate ─────────────────────────────────
    connect(treeView, &HierarchyTree::pasteItemsRequested, this,
            [executePaste](QVariantMap targetData, QList<QVariantMap> itemsToPaste) {
                executePaste(targetData, itemsToPaste);
            });

    connect(treeView->getContextMenu(), &ContextMenu::pasteItemsRequested, this,
            [executePaste](QVariantMap targetData, QList<QVariantMap> itemsToPaste) {
                executePaste(targetData, itemsToPaste);
            });

    // ── Bulk: Remove entities ────────────────────────────────────────────
    connect(treeView, &HierarchyTree::removeEntitiesRequested,
            this, [=](QList<QPair<QString, QString>> entityInfoList) {
                for (const auto& entityInfo : entityInfoList) {
                    try {
                        hierarchy->removeEntity(entityInfo.first, entityInfo.second);
                    } catch (const std::exception& e) {}
                }
            });

    // ── Bulk: Set Active / Inactive ──────────────────────────────────────
    connect(treeView, &HierarchyTree::setEntitiesActiveRequested,
            this, [=](QList<QVariantMap> entities, bool active) {
                for (const QVariantMap& data : entities) {
                    QString entityID = data["ID"].toString();
                    auto it = hierarchy->Entities.find(entityID.toStdString());
                    if (it == hierarchy->Entities.end() || !it->second) continue;

                    QJsonObject delta;
                    delta["active"] = active;
                    hierarchy->UpdateComponent(entityID, "_self", delta);
                    treeView->setEntityActiveState(entityID, active);

                    Entity* entity = it->second;
                    entity->Active = active;
                    if (Platform* p = dynamic_cast<Platform*>(entity))
                        p->Active = active;
                    if (ECS::Entity_Data* data =
                        hierarchy->world.GetComponent<ECS::Entity_Data>(entity->ecsID)) {
                        data->active = active;
                    }
                    if (apiMapWidget)
                        apiMapWidget->refreshAllLayers();
                }
            });

    // ── Bulk: Add Weapon ─────────────────────────────────────────────────
    connect(treeView, &HierarchyTree::addWeaponToEntitiesRequested, this,
            [=](QList<QVariantMap> entities) {
                if (entities.isEmpty()) return;

                Hierarchy* dbHierarchy = library ? library : hierarchy;
                AddWeaponDialog dlg(treeView, dbHierarchy, false);
                if (dlg.exec() != QDialog::Accepted) return;

                QString selectedId = dlg.selectedEntityId();
                QString weaponName = dlg.weaponName().trimmed();
                if (weaponName.isEmpty()) return;
                if (!confirmAndLoadAssociatedDatabase(hierarchy, treeView))
                    return;

                QString typeName = "Missile";
                if (!selectedId.isEmpty() && dbHierarchy) {
                    auto dbIt = dbHierarchy->Weapons.find(selectedId.toStdString());
                    if (dbIt != dbHierarchy->Weapons.end() && dbIt->second)
                        typeName = dbIt->second->weaponTypeName();
                } else {
                    typeName = dlg.weaponTypeStr();
                }

                QJsonObject hoistedCfg = dlg.configJson();
                hoistedCfg["weaponTypeName"] = typeName;

                for (const QVariantMap& data : entities) {
                    QCoreApplication::processEvents();
                    QString entityID = data["ID"].toString();
                    auto it = hierarchy->Entities.find(entityID.toStdString());
                    if (it == hierarchy->Entities.end()) continue;

                    Platform* platform = dynamic_cast<Platform*>(it->second);
                    if (!platform) continue;
                    int i = 1;
                    if (dlg.weaponType() == Weapon::WeaponType::Sonobuoy && dlg.m_count) {
                        i = dlg.m_count->value();
                    }
                    for (int k = 0; k < i; k++) {
                        QCoreApplication::processEvents();
                        Weapon* weapon       = createWeapon(typeName, hierarchy);
                        weapon->Name         = weaponName.toStdString() + QString::number(k).toStdString();
                        weapon->parentEntity = platform;
                        weapon->ID           = Uuid::generateShortUniqueID();
                        weapon->parentID     = platform->ID;
                        if (/*!platform->weapons ||*/ !platform->weapons.weapons) {
                            delete weapon;
                            continue;
                        }
                        weapon->init();
                        if (!selectedId.isEmpty() && dbHierarchy) {
                            auto dbIt = dbHierarchy->Weapons.find(selectedId.toStdString());
                            if (dbIt != dbHierarchy->Weapons.end() && dbIt->second) {
                                std::string savedId     = weapon->ID;
                                std::string savedName   = weapon->Name;
                                std::string savedParent = weapon->parentID;
                                weapon->fromJson(dbIt->second->toJson());
                                weapon->ID     = savedId;
                                weapon->Name   = savedName;
                                weapon->parentID = savedParent;
                            }
                        } else {
                            weapon->fromJson(hoistedCfg);
                            weapon->Name = weaponName.toStdString() + QString::number(k).toStdString();
                        }

                        weapon->syncComponentsFromWeaponData();
                        platform->weapons.weapons->insert({weapon->ID, weapon});
                        hierarchy->Weapons.insert({weapon->ID, weapon});
                        hierarchy->Entities.insert({weapon->ID, weapon});

                        emit hierarchy->subComponentAdded(
                            QString::fromStdString(platform->weapons.ID),
                            QString::fromStdString(weapon->ID),
                            weaponName + QString::number(k));

                        QString weaponQID = QString::fromStdString(weapon->ID);
                        for (const std::string& compName : weapon->getSupportedComponents()) {
                            std::string compID;
                            if      (compName == "transform"    /*&& weapon->transform*/)      compID = weapon->transform.ID;
                            else if (compName == "rigidbody"    /*&& weapon->rigidbody*/)      compID = weapon->rigidbody.ID;
                            else if (compName == "collider"     /*&& weapon->collider*/)       compID = weapon->collider.ID;
                            else if (compName == "trajectory"   /*&& weapon->trajectory*/)     compID = weapon->trajectory.ID;
                            else if (compName == "bitmap"       /*&& weapon->meshRenderer2d*/) compID = weapon->meshRenderer2d.ID;
                            else if (compName == "dynamicModel" /*&& weapon->dynamicModel*/)   compID = weapon->dynamicModel.ID;
                            else if (compName == "crossSection" /*&& weapon->crossSection*/)   compID = weapon->crossSection.ID;
                            else continue;
                            emit hierarchy->componentAdded(
                                weaponQID,
                                QString::fromStdString(compID),
                                QString::fromStdString(compName));
                        }
                    }
                }
            });

    // ── Bulk: Add Sensor ─────────────────────────────────────────────────
    connect(treeView, &HierarchyTree::addSensorToEntitiesRequested, this,
            [=](QList<QVariantMap> entities) {
                if (entities.isEmpty()) return;
                Hierarchy* dbHierarchy = library ? library : hierarchy;
                AddItemDialog dialog(AddItemDialog::EntityType, "sensors",
                                     AddItemDialog::ComponentSensorMode, dbHierarchy, treeView);
                if (dialog.exec() != QDialog::Accepted) return;
                if (dialog.getName().isEmpty()) return;

                QString sensorName = dialog.getName();
                QString sensorType = dialog.getSensorType();
                QString profileId  = dialog.getProfileId();
                if (!confirmAndLoadAssociatedDatabase(hierarchy, treeView))
                    return;

                for (const QVariantMap& data : entities) {
                    QString entityID = data["ID"].toString();
                    auto it = hierarchy->Entities.find(entityID.toStdString());
                    if (it == hierarchy->Entities.end()) continue;

                    Platform* platform = dynamic_cast<Platform*>(it->second);
                    if (!platform ) continue;

                    QJsonObject obj;
                    if (!profileId.isEmpty() && dbHierarchy->Sensors.count(profileId.toStdString())) {
                        obj = dbHierarchy->Sensors[profileId.toStdString()]->toJson();
                    }
                    hierarchy->addSubComponent(QString::fromStdString(platform->sensors.ID),
                                               sensorName, sensorType, profileId, obj);
                }
            });

    // ── Bulk: Add IFF ────────────────────────────────────────────────────
    connect(treeView, &HierarchyTree::addIFFToEntitiesRequested, this,
            [=](QList<QVariantMap> entities) {
                if (entities.isEmpty()) return;

                Hierarchy* dbHierarchy = library ? library : hierarchy;
                AddItemDialog dialog(AddItemDialog::EntityType, "iffs",
                                     AddItemDialog::ComponentIFFMode, dbHierarchy, treeView);
                if (dialog.exec() != QDialog::Accepted) return;
                if (dialog.getName().isEmpty()) return;

                QString iffName   = dialog.getName();
                QString profileId = dialog.getProfileId();
                if (!confirmAndLoadAssociatedDatabase(hierarchy, treeView))
                    return;

                for (const QVariantMap& data : entities) {
                    QString entityID = data["ID"].toString();
                    auto it = hierarchy->Entities.find(entityID.toStdString());
                    if (it == hierarchy->Entities.end()) continue;

                    Platform* platform = dynamic_cast<Platform*>(it->second);
                    if (!platform /*|| !platform->iffs*/) continue;

                    QJsonObject obj;
                    if (!profileId.isEmpty() && dbHierarchy->Iffs.count(profileId.toStdString())) {
                        obj = dbHierarchy->Iffs[profileId.toStdString()]->toJson();
                    }
                    hierarchy->addSubComponent(QString::fromStdString(platform->iffs.ID),
                                               iffName, "", profileId, obj);
                }
            });

    // ── Bulk: Add Radio ──────────────────────────────────────────────────
    connect(treeView, &HierarchyTree::addRadioToEntitiesRequested, this,
            [=](QList<QVariantMap> entities) {
                if (entities.isEmpty()) return;

                Hierarchy* dbHierarchy = library ? library : hierarchy;
                AddItemDialog dialog(AddItemDialog::EntityType, "radios",
                                     AddItemDialog::ComponentRadioMode, dbHierarchy, treeView);
                if (dialog.exec() != QDialog::Accepted) return;
                if (dialog.getName().isEmpty()) return;

                QString radioName = dialog.getName();
                QString profileId = dialog.getProfileId();
                QString radioType = dialog.getRadioType();
                if (!confirmAndLoadAssociatedDatabase(hierarchy, treeView))
                    return;

                for (const QVariantMap& data : entities) {
                    QString entityID = data["ID"].toString();
                    auto it = hierarchy->Entities.find(entityID.toStdString());
                    if (it == hierarchy->Entities.end()) continue;

                    Platform* platform = dynamic_cast<Platform*>(it->second);
                    if (!platform /*|| !platform->radios*/) continue;

                    QJsonObject obj;
                    if (!profileId.isEmpty() && dbHierarchy->Radios.count(profileId.toStdString())) {
                        obj = dbHierarchy->Radios[profileId.toStdString()]->toJson();
                    }
                    hierarchy->addSubComponent(QString::fromStdString(platform->radios.ID),
                                               radioName, radioType, profileId, obj);
                }
            });

    // ── Bulk: Set Team ───────────────────────────────────────────────────
    connect(treeView, &HierarchyTree::addTeamToEntitiesRequested, this,
            [=](QList<QVariantMap> entities, QString team) {
                for (const QVariantMap& data : entities) {
                    QString entityID = data["ID"].toString();
                    // if (!hierarchy->Entities) continue;
                    auto it = hierarchy->Entities.find(entityID.toStdString());
                    if (it == hierarchy->Entities.end()) continue;

                    QJsonObject entityJson = it->second->toJson();
                    QJsonObject teamObj;
                    if (entityJson.contains("Team") && entityJson["Team"].isObject()) {
                        teamObj = entityJson["Team"].toObject();
                        teamObj["value"] = team;
                    } else {
                        QJsonArray options;
                        for (const QString& t : {"RedTeam","BlueTeam","GreenTeam","YellowTeam",
                                                 "GreyTeam","AlphaTeam","BetaTeam","GammaTeam"})
                            options.append(t);
                        teamObj["type"]    = "option";
                        teamObj["options"] = options;
                        teamObj["value"]   = team;
                    }
                    QJsonObject delta;
                    delta["Team"] = teamObj;
                    hierarchy->UpdateComponent(entityID, "_self", delta);
                }
            });

    // ── Single entity category ────────────────────────────────────────────
    connect(treeView->getContextMenu(), &ContextMenu::setCategoryToEntityRequested,
            this, [=](QVariantMap entityData, QString category, QString subCategory) {
                QList<QVariantMap> entities;
                entities.append(entityData);
                emit treeView->setCategoryToEntitiesRequested(entities, category, subCategory);
            });

    // ── Bulk: Set Category ────────────────────────────────────────────────
    connect(treeView, &HierarchyTree::setCategoryToEntitiesRequested, this,
            [=](QList<QVariantMap> entities, QString category, QString subCategory) {
                for (const QVariantMap& data : entities) {
                    QString entityID = data["ID"].toString();
                    auto it = hierarchy->Entities.find(entityID.toStdString());
                    if (it == hierarchy->Entities.end()) continue;
                    QJsonObject entityJson = it->second->toJson();
                    // Update Category
                    QJsonObject categoryObj;
                    if (entityJson.contains("Category") && entityJson["Category"].isObject()) {
                        categoryObj = entityJson["Category"].toObject();
                        categoryObj["value"] = category;
                    } else {
                        QJsonArray options;
                        for (const QString& opt : {"Air", "Ground", "Marine"})
                            options.append(opt);
                        categoryObj["type"]    = "option";
                        categoryObj["options"] = options;
                        categoryObj["value"]   = category;
                    }

                    // Update SubCategory options based on category
                    QJsonObject subCategoryObj;
                    QMap<QString, QStringList> subCatMap = {
                        {"Air",    QStringList{"Aircraft", "Helicopter", "UAV", "FighterAircraft", "TransportAircraft"}},
                        {"Ground", QStringList{"Tank", "GroundRadar", "Human", "BattleVehicle", "MilitaryGroundVehicle", "TransportVehicle"}},
                        {"Marine", QStringList{"Ship", "Frigate", "Submarine", "AircraftCarrier", "GuidedMissileCruiser", "Destroyer", "SmallBoat"}}
                    };
                    QJsonArray subOptions;
                    for (const QString& opt : subCatMap.value(category))
                        subOptions.append(opt);

                    if (entityJson.contains("SubCategory") && entityJson["SubCategory"].isObject()) {
                        subCategoryObj = entityJson["SubCategory"].toObject();
                    } else {
                        subCategoryObj["type"]     = "option";
                        subCategoryObj["editable"] = false;
                    }
                    subCategoryObj["options"] = subOptions;
                    subCategoryObj["value"]   = subCategory;

                    QJsonObject delta;
                    delta["Category"]    = categoryObj;
                    delta["SubCategory"] = subCategoryObj;
                    hierarchy->UpdateComponent(entityID, "_self", delta);
                }
            });

    // ── Copy / Paste (single item) ───────────────────────────────────────
    connect(treeView->getContextMenu(), &ContextMenu::copyItemRequested,
            treeView, &HierarchyTree::copyItemRequested);
    connect(treeView->getContextMenu(), &ContextMenu::pasteItemRequested,
            treeView, &HierarchyTree::pasteItemRequested);

    connect(treeView, &HierarchyTree::copyItemRequested, this,
            [this, hierarchy, treeView](QVariantMap data) {
                if (data["type"].toString() != "entity") return;
                copydata = data;
                copySource = hierarchy;
                treeView->copiedItems.clear();
                if (treeView->getContextMenu())
                    treeView->getContextMenu()->m_copiedItems.clear();
            });

    connect(treeView, &HierarchyTree::copyItemsRequested, this,
            [this, hierarchy](QList<QVariantMap> dataList) {
                if (dataList.isEmpty()) return;
                copydata.clear();
                copySource = hierarchy;
            });

    connect(treeView, &HierarchyTree::pasteItemRequested, this,
            [this, hierarchy, getEntityProfileType, getTargetProfileType](QVariantMap targetData) {
                if (copydata.isEmpty() || !copySource) return;
                if (copydata["type"].toString() != "entity") return;

                QString targetType, targetId = targetData["ID"].toString();
                bool isFolder = false;
                if (targetData["type"].type() == QVariant::Map) {
                    QVariantMap typeData = targetData["type"].toMap();
                    if (typeData.contains("type") && typeData["type"].toString() == "option")
                        targetType = "profile";
                    else return;
                } else {
                    targetType = targetData["type"].toString();
                    isFolder = (targetType == "folder");
                }
                bool isProfile = (targetType == "profile");
                QString sourceId = copydata["ID"].toString();
                QString sourceProfileType = getEntityProfileType(copySource, sourceId);
                QString targetProfileType = getTargetProfileType(hierarchy, targetId, isProfile, isFolder);

                if (sourceProfileType == "Formation_blocked") {
                    QMessageBox::warning(nullptr, "Paste Not Allowed",
                                         "Formation entities cannot be copied or pasted.");
                    return;
                }
                if (targetProfileType == "Formation_blocked") {
                    QMessageBox::warning(nullptr, "Paste Not Allowed",
                                         "Cannot paste into Formation category.");
                    return;
                }
                if (!sourceProfileType.isEmpty() && !targetProfileType.isEmpty()
                    && sourceProfileType != targetProfileType) {
                    QMessageBox::warning(nullptr, "Paste Error",
                                         QString("Cannot paste '%1' entity into '%2' category.\n"
                                                 "Only '%3' entities can be pasted here.")
                                             .arg(sourceProfileType).arg(targetProfileType).arg(targetProfileType));
                    return;
                }

                try {
                    auto entityIt = copySource->Entities.find(sourceId.toStdString());
                    if (entityIt == copySource->Entities.end()) return;
                    QJsonObject entityJson = entityIt->second->toJson();
                    entityJson["id"] = QString::fromStdString(Uuid::generateShortUniqueID());
                    hierarchy->addEntityFromJson(targetId, entityJson, isProfile);
                    copydata.clear();
                    copySource = nullptr;
                } catch (const std::exception& e) {}
            });

  // ── Tactical Display ─────────────────────────────────────────────────
    if (tactical) {
        connect(hierarchy, &Hierarchy::entityRemoved, tactical, &TacticalDisplay::removeMesh);
        connect(hierarchy, &Hierarchy::entityRemoved, this, [=](const QString& entityId) {
            if (tactical->canvas && tactical->canvas->entityInfoDialog) {
                tactical->canvas->entityInfoDialog->onEntityInvalidated(entityId);
            }
        });
    }

    // ── Inspector ────────────────────────────────────────────────────────
    if (inspector) {
        connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
        connect(inspector, &Inspector::valueChanged, this,
                [=](QString entityID, QString componentName, QJsonObject delta) {
                    if (!componentName.contains("_self")) return;
                    if (!delta.contains("active")) return;

                    const bool active = delta["active"].toBool();
                    treeView->setEntityActiveState(entityID, active);

                    auto it = hierarchy->Entities.find(entityID.toStdString());
                    if (it == hierarchy->Entities.end() || !it->second)
                        return;

                    Entity* entity = it->second;
                    entity->Active = active;
                    if (Platform* p = dynamic_cast<Platform*>(entity))
                        p->Active = active;

                    if (ECS::Entity_Data* data =
                        hierarchy->world.GetComponent<ECS::Entity_Data>(entity->ecsID)) {
                        data->active = active;
                    }
                    if (apiMapWidget)
                        apiMapWidget->refreshAllLayers();
                });
    }
    if (apiMapWidget) {
        if (EditorCanvas* canvas = apiMapWidget->editorCanvas()) {
            connect(canvas, &EditorCanvas::entityActiveChanged,
                    this, [=](const QString& entityID, bool active) {
                        treeView->setEntityActiveState(entityID, active);
                        apiMapWidget->refreshAllLayers();
                    });
        }
    }
    // ── Rename signals ───────────────────────────────────────────────────
    connect(hierarchy, &Hierarchy::profileRenamed, treeView, &HierarchyTree::profileRenamed);
    connect(hierarchy, &Hierarchy::folderRenamed,  treeView, &HierarchyTree::folderRenamed);
    connect(hierarchy, &Hierarchy::entityRenamed,  treeView, &HierarchyTree::entityRenamed);
    connect(treeView->getContextMenu(), &ContextMenu::renameItemRequested,
            hierarchy, [=](QVariantMap data) {
                QString type     = data["type"].toString();
                QString id       = data["ID"].toString();
                QString name     = data["name"].toString();
                QString parentID = data["parentId"].toString();
                if      (type == "profile")      hierarchy->renameProfileCategaory(id, name);
                else if (type == "folder")       hierarchy->renameFolder(id, name);
                else if (type == "entity")       hierarchy->renameEntity(id, name);
                else if (type == "subcomponent") hierarchy->renameSubComponent(parentID, id, name);
            });

    // ── Formation ────────────────────────────────────────────────────────
    connect(treeView, &HierarchyTree::addFormationRequested, this,
            [=](QList<QVariantMap> selectedEntities) {

                if (selectedEntities.size() < 2) {
                    QMessageBox::warning(nullptr, "Cannot Create Formation",
                                         "Need at least 2 entities to create a formation.");
                    return;
                }

                // Speed / already-in-formation checks
                float max = 0;
                float min = 10000;
                for (const auto& entity : selectedEntities) {
                    std::string id = entity["ID"].toString().toStdString();
                    auto it = hierarchy->Platforms.find(id);
                    if (it != hierarchy->Platforms.end()) {
                        max = it->second->dynamicModel.maxSpeed > max ? it->second->dynamicModel.maxSpeed : max;
                        min = it->second->dynamicModel.maxSpeed < min ? it->second->dynamicModel.maxSpeed : min;
                        if ((max - min) > 500) {
                            QMessageBox::warning(nullptr, "Cannot Create Formation",
                                                 "entities Speed too unmachable");
                            return;
                        }
                        if (it->second->dynamicModel.formation || it->second->dynamicModel.followEntity) {
                            QMessageBox::warning(nullptr, "Cannot Create Formation",
                                                 "entities already part of Formation");
                            return;
                        }
                    }
                }

                // ── Dialog ────────────────────────────────────────────────────
                AddFormationDialog dialog(selectedEntities, nullptr);
                if (dialog.exec() != QDialog::Accepted)
                    return;

                QString formationName = dialog.getFormationName();
                QString mothershipId  = dialog.getMothershipId();
                QString formationType = dialog.getFormationType();
                QList<QVariantMap> allies = dialog.getAllies();
                int alliesCount = dialog.getAlliesCount();

                if (formationName.isEmpty()) {
                    QMessageBox::warning(nullptr, "Invalid Input", "Formation name cannot be empty.");
                    return;
                }

                // ── Custom offsets from Database ──────────────────────────────
                // Format: [{x, y}, ...]  where y = formation Z
                QJsonArray customPositions;

                if (dialog.isCustomFromDatabase()) {
                    QString customId = dialog.getSelectedCustomFormationId();
                    MainWindow *mw = MainWindow::instance();
                    Hierarchy *dbHier = (mw && mw->databaseEditor) ? mw->databaseEditor->hierarchy : nullptr;

                    if (dbHier && !customId.isEmpty()) {
                        auto it = dbHier->Entities.find(customId.toStdString());
                        if (it != dbHier->Entities.end() && it->second) {

                            // 1) Memory se (sabse reliable)
                            Formation *dbForm = dynamic_cast<Formation*>(it->second);
                            if (dbForm && dbForm->formationPositions) {
                                for (const auto& pair : *dbForm->formationPositions) {
                                    FormationPosition *pos = pair.second;
                                    if (!pos || !pos->Offset) continue;
                                    QJsonObject pt;
                                    pt["x"] = pos->Offset->x;
                                    pt["y"] = pos->Offset->z;   // formation Z → canvas Y
                                    customPositions.append(pt);
                                }
                            }

                            // 2) Fallback: toJson
                            if (customPositions.isEmpty()) {
                                QJsonObject j = it->second->toJson();
                                QJsonArray raw;
                                if (j.contains("formationPositions") && j["formationPositions"].isArray())
                                    raw = j["formationPositions"].toArray();
                                else if (j.contains("FormationPositions") && j["FormationPositions"].isArray())
                                    raw = j["FormationPositions"].toArray();

                                for (const QJsonValue& v : raw) {
                                    QJsonObject posObj = v.toObject();
                                    QJsonObject pt;
                                    if (posObj.contains("Offset") && posObj["Offset"].isObject()) {
                                        QJsonObject off = posObj["Offset"].toObject();
                                        pt["x"] = off["x"].toDouble();
                                        pt["y"] = off["z"].toDouble();
                                    } else if (posObj.contains("x")) {
                                        pt["x"] = posObj["x"].toDouble();
                                        pt["y"] = posObj["y"].toDouble();
                                    } else {
                                        continue;
                                    }
                                    customPositions.append(pt);
                                }
                            }

                            if (!customPositions.isEmpty())
                                formationType = "Custom";

                            qDebug() << "[AddFormation] Custom offsets loaded:" << customPositions.size();
                        }
                    }
                }

                // ── Formation Profile ─────────────────────────────────────────
                QString formationProfileId;
                for (const auto& [id, profile] : hierarchy->ProfileCategories) {
                    if (profile->type == Constants::EntityType::Formation) {
                        formationProfileId = QString::fromStdString(id);
                        break;
                    }
                }
                if (formationProfileId.isEmpty()) {
                    ProfileCategaory* fp = hierarchy->addProfileCategaory("Formation");
                    if (fp) {
                        fp->setProfileType(Constants::EntityType::Formation);
                        formationProfileId = QString::fromStdString(fp->ID);
                    }
                }
                if (formationProfileId.isEmpty()) {
                    QMessageBox::critical(nullptr, "Error", "Failed to find or create Formation profile.");
                    return;
                }

                // ── Create Formation entity ───────────────────────────────────
                Entity* fe = hierarchy->addEntity(formationProfileId, formationName, true);
                if (!fe) {
                    QMessageBox::critical(nullptr, "Error", "Failed to create formation entity.");
                    return;
                }
                hierarchy->removeEntity(formationProfileId, QString::fromStdString(fe->ID));

                Formation* formation  = new Formation(hierarchy);
                formation->Name       = formationName.toStdString();
                formation->ID         = Uuid::generateShortUniqueID();
                formation->parentID   = formationProfileId.toStdString();
                formation->type       = Constants::EntityType::Formation;
                hierarchy->ProfileCategories[formationProfileId.toStdString()]->addEntityWithObject(formation);
                hierarchy->Entities.insert({formation->ID, formation});

                // ── Type enum ─────────────────────────────────────────────────
                Constants::FormationType typeEnum = Constants::FormationType::V;
                if      (formationType == "Line")             typeEnum = Constants::FormationType::Line;
                else if (formationType == "V")                typeEnum = Constants::FormationType::V;
                else if (formationType == "Diamond")          typeEnum = Constants::FormationType::Diamond;
                else if (formationType == "Square")           typeEnum = Constants::FormationType::Square;
                else if (formationType == "Column")           typeEnum = Constants::FormationType::Column;
                else if (formationType == "Echelon Left")     typeEnum = Constants::FormationType::EchelonLeft;
                else if (formationType == "Echelon Right")    typeEnum = Constants::FormationType::EchelonRight;
                else if (formationType == "Staggered Column") typeEnum = Constants::FormationType::StaggeredColumn;
                else if (formationType == "Wedge")            typeEnum = Constants::FormationType::Wedge;
                else if (formationType == "Coil")             typeEnum = Constants::FormationType::Coil;
                else if (formationType == "Herringbone")      typeEnum = Constants::FormationType::Herringbone;
                else if (formationType == "Skirmish")         typeEnum = Constants::FormationType::Skirmish;
                else if (formationType == "Box")              typeEnum = Constants::FormationType::BoxFormation;
                else if (formationType == "Custom")           typeEnum = Constants::FormationType::Custom;
                formation->formationType = typeEnum;
                formation->count         = alliesCount;

                // ── Mothership ────────────────────────────────────────────────
                auto motherIt = hierarchy->Entities.find(mothershipId.toStdString());
                if (motherIt != hierarchy->Entities.end()) {
                    Entity* motherEntity = motherIt->second;
                    QJsonObject motherJson, entityRef;
                    entityRef["type"] = "reference";
                    entityRef["name"] = QString::fromStdString(motherEntity->Name);
                    entityRef["id"]   = QString::fromStdString(motherEntity->ID);
                    motherJson["entity"] = entityRef;
                    formation->updateComponent("mothership", motherJson);
                    formation->mothership->entity = motherEntity;
                    Platform* p = dynamic_cast<Platform*>(motherEntity);
                    if (p) {
                        p->dynamicModel.formation = formation;
                    }
                }

                // Predefined slots generate
                formation->formationCreate();

                // ── Custom offsets OVERRIDE (x → Offset.x, y → Offset.z) ──────
                if (!customPositions.isEmpty() && formation->formationPositions) {
                    int idx = 0;
                    for (const QJsonValue &val : customPositions) {
                        if (idx >= alliesCount) break;

                        QJsonObject posObj = val.toObject();
                        double dx = posObj.value("x").toDouble(0.0);
                        double dy = posObj.value("y").toDouble(0.0);  // canvas Y

                        std::string positionName = "ally_" + std::to_string(idx);
                        auto pit = formation->formationPositions->find(positionName);
                        if (pit != formation->formationPositions->end()) {
                            FormationPosition* position = pit->second;
                            if (position && position->Offset) {
                                position->Offset->x = static_cast<float>(dx);
                                position->Offset->y = 0.0f;
                                position->Offset->z = static_cast<float>(dy);
                            }
                        }
                        idx++;
                    }
                }

                // ── Allies assign ─────────────────────────────────────────────
                int allyIndex = 0;
                for (const auto& allyData : allies) {
                    QString allyId = allyData["ID"].toString();
                    auto allyIt = hierarchy->Entities.find(allyId.toStdString());
                    if (allyIt == hierarchy->Entities.end() || allyIndex >= alliesCount) continue;

                    Entity* allyEntity = allyIt->second;
                    std::string positionName = "ally_" + std::to_string(allyIndex);

                    if (formation->formationPositions->find(positionName) != formation->formationPositions->end()) {
                        FormationPosition* position = (*formation->formationPositions)[positionName];
                        position->entity = allyEntity;

                        float offsetX = 0, offsetY = 0, offsetZ = 0;
                        if (position->Offset) {
                            offsetX = position->Offset->x;
                            offsetY = position->Offset->y;
                            offsetZ = position->Offset->z;
                        }

                        double geoLat = 0, geoLon = 0, geoAlt = 0, geoHead = 0;
                        if (position->geoOffset) {
                            geoLat  = position->geoOffset->latitude;
                            geoLon  = position->geoOffset->longitude;
                            geoAlt  = position->geoOffset->altitude;
                            geoHead = position->geoOffset->Heading;
                        }

                        QJsonObject positionJson, entityRef, offsetObj, geoOffsetObj;
                        entityRef["type"] = "reference";
                        entityRef["name"] = QString::fromStdString(allyEntity->Name);
                        entityRef["id"]   = QString::fromStdString(allyEntity->ID);
                        positionJson["entity"] = entityRef;

                        offsetObj["type"] = "section";
                        offsetObj["x"] = offsetX;
                        offsetObj["y"] = offsetY;
                        offsetObj["z"] = offsetZ;
                        positionJson["Offset"] = offsetObj;

                        geoOffsetObj["type"] = "section";
                        geoOffsetObj["lat"] = geoLat;
                        geoOffsetObj["lon"] = geoLon;
                        geoOffsetObj["alt"] = geoAlt;
                        geoOffsetObj["heading"] = geoHead;
                        positionJson["geoOffset"] = geoOffsetObj;

                        formation->updateComponent(QString::fromStdString(positionName), positionJson);

                        Platform* plt    = dynamic_cast<Platform*>(allyEntity);
                        Platform* mother = dynamic_cast<Platform*>(formation->mothership->entity);
                        if (plt && mother) {
                            plt->dynamicModel.followEntity      = mother;
                            plt->dynamicModel.formationPosition = position;
                            plt->dynamicModel.follow            = true;
                        }
                    }
                    allyIndex++;
                }
            });
    connect(treeView->getContextMenu(), &ContextMenu::changeMothershipRequested,
            this, [=](const QString& formationId) {
                auto it = hierarchy->Entities.find(formationId.toStdString());
                if (it == hierarchy->Entities.end()) return;

                Formation* form = dynamic_cast<Formation*>(it->second);
                if (!form || !form->mothership) return;

                // Build member list
                QStringList names;
                QStringList ids;

                if (form->mothership->entity) {
                    names << QString::fromStdString(form->mothership->entity->Name) + " (current lead)";
                    ids   << QString::fromStdString(form->mothership->entity->ID);
                }
                if (form->formationPositions) {
                    for (const auto& pair : *form->formationPositions) {
                        if (!pair.second || !pair.second->entity) continue;
                        names << QString::fromStdString(pair.second->entity->Name);
                        ids   << QString::fromStdString(pair.second->entity->ID);
                    }
                }
                if (names.isEmpty()) return;

                bool ok = false;
                QString chosen = QInputDialog::getItem(
                    nullptr, "Change Mothership",
                    "Select new lead:", names, 0, false, &ok);
                if (!ok || chosen.isEmpty()) return;
                int idx = names.indexOf(chosen);
                if (idx < 0 || idx >= ids.size()) return;
                changeFormationMothership(form, ids[idx]);
            });
    // ── Leave Formation (ally only) ─────────────────────────────────────
    connect(treeView->getContextMenu(),
            &ContextMenu::removeEntityFromFormationRequested,
            this, [=](const QString& formationId, const QString& entityId) {
                auto it = hierarchy->Entities.find(formationId.toStdString());
                if (it == hierarchy->Entities.end()) return;
                Formation* form = dynamic_cast<Formation*>(it->second);
                if (!form) return;
                form->removeEntityFromFormation(entityId);
            });

    // ── Add Entity to Formation (Option B dialog) ───────────────────────
    connect(treeView->getContextMenu(),
            &ContextMenu::addEntityToFormationRequested,
            this, [=](const QString& formationId) {
                auto it = hierarchy->Entities.find(formationId.toStdString());
                if (it == hierarchy->Entities.end()) return;
                Formation* form = dynamic_cast<Formation*>(it->second);
                if (!form || !form->mothership || !form->mothership->entity) {
                    QMessageBox::warning(nullptr, "Add Entity",
                                         "Formation has no mothership. Set a lead first.");
                    return;
                }
                QStringList names;
                QStringList ids;
                for (const auto& pair : hierarchy->Platforms) {
                    Platform* p = pair.second;
                    if (!p) continue;
                    if (p->dynamicModel.formation || p->dynamicModel.formationPosition)
                        continue;
                    if (form->mothership->entity->ID == p->ID)
                        continue;
                    names << QString::fromStdString(p->Name);
                    ids   << QString::fromStdString(p->ID);
                }

                if (names.isEmpty()) {
                    QMessageBox::information(nullptr, "Add Entity",
                                             "No free Entity available to add to this formation.");
                    return;
                }

                bool ok = false;
                QString chosen = QInputDialog::getItem(
                    nullptr,
                    "Add Entity to Formation",
                    "Select platform:",
                    names, 0, false, &ok);
                if (!ok || chosen.isEmpty()) return;

                const int idx = names.indexOf(chosen);
                if (idx < 0 || idx >= ids.size()) return;

                auto eit = hierarchy->Entities.find(ids[idx].toStdString());
                if (eit == hierarchy->Entities.end() || !eit->second) return;

                if (!form->addEntityToFormation(eit->second)) {
                    QMessageBox::warning(nullptr, "Add Entity",
                                         "Could not add entity to formation.");
                }
            });
    connect(treeView->getContextMenu(),
            &ContextMenu::breakFormationRequested,
            this, [=](const QString& formationId) {
                auto it = hierarchy->Entities.find(formationId.toStdString());
                if (it == hierarchy->Entities.end()) return;
                if (Formation* form = dynamic_cast<Formation*>(it->second))
                    form->breakFormation();
            });

    connect(treeView->getContextMenu(),
            &ContextMenu::resumeFormationRequested,
            this, [=](const QString& formationId) {
                auto it = hierarchy->Entities.find(formationId.toStdString());
                if (it == hierarchy->Entities.end()) return;
                if (Formation* form = dynamic_cast<Formation*>(it->second))
                    form->resumeFormation();
            });

    // ── Edit SubComponent Sonobuoy parameters from DB) ─────────────
    connect(treeView->getContextMenu(), &ContextMenu::editSubComponentRequested,
            this, [=](QString parentID, QString subCompID, QString subCompName, QString subCompType) {
                Hierarchy* dbHierarchy = library ? library : hierarchy;
                if (!dbHierarchy) return;
                QStringList displayNames;
                QMap<QString, QString> displayToId;
                for (const auto& [profileId, profile] : dbHierarchy->ProfileCategories) {
                    if (!profile) continue;
                    QString profileName = QString::fromStdString(profile->Name);
                    if (profileName.compare("Weapon", Qt::CaseInsensitive) != 0) continue;
                    auto collectFromMap = [&](auto& entityMap) {
                        for (const auto& [eid, entity] : entityMap) {
                            if (!entity) continue;
                            Weapon* dbW = dynamic_cast<Weapon*>(entity);
                            if (!dbW) continue;
                            if (dbW->weaponTypeName().compare(subCompType, Qt::CaseInsensitive) != 0) continue;
                            QString eName = QString::fromStdString(entity->Name).trimmed();
                            if (eName.isEmpty()) continue;
                            QString display = eName + " (" + dbW->weaponTypeName() + ")";
                            if (!displayToId.contains(display)) {
                                displayNames.append(display);
                                displayToId[display] = QString::fromStdString(eid);
                            }
                        }
                    };
                    collectFromMap(profile->Entities);
                    for (const auto& [fid, folder] : profile->Folders) {
                        if (folder) collectFromMap(folder->Entities);
                    }
                }
                if (displayNames.isEmpty()) return;
                displayNames.sort(Qt::CaseInsensitive);
                bool ok = false;
                QString selected = QInputDialog::getItem(
                    treeView,
                    "Select " + subCompType + " from DB",
                    "Choose a " + subCompType + " to apply parameters from:",
                    displayNames, 0, false, &ok);
                if (!ok || selected.isEmpty()) return;
                QString dbWeaponId = displayToId.value(selected);
                if (dbWeaponId.isEmpty()) return;
                auto dbIt = dbHierarchy->Weapons.find(dbWeaponId.toStdString());
                if (dbIt == dbHierarchy->Weapons.end() || !dbIt->second) return;
                QJsonObject sourceJson = dbIt->second->toJson();
                auto sceneIt = hierarchy->Weapons.find(subCompID.toStdString());
                if (sceneIt == hierarchy->Weapons.end() || !sceneIt->second) return;
                Weapon* targetWeapon = sceneIt->second;
                std::string savedId     = targetWeapon->ID;
                std::string savedName   = targetWeapon->Name;
                std::string savedParent = targetWeapon->parentID;
                targetWeapon->fromJson(sourceJson);
                targetWeapon->ID       = savedId;
                targetWeapon->Name     = savedName;
                targetWeapon->parentID = savedParent;
                targetWeapon->syncComponentsFromWeaponData();
            });
    connect(treeView->getContextMenu(), &ContextMenu::editMultipleSubComponentsRequested,
            this, [=](QList<QVariantMap> subComponents) {
                if (subComponents.isEmpty()) return;
                Hierarchy* dbHier = library ? library : hierarchy;
                if (!dbHier) return;
                QString subCompType;
                {
                    QString firstID = subComponents.first()["ID"].toString();
                    auto firstIt = hierarchy->Weapons.find(firstID.toStdString());
                    if (firstIt == hierarchy->Weapons.end() || !firstIt->second) return;
                    subCompType = firstIt->second->weaponTypeName();
                }
                if (subCompType.isEmpty()) return;
                QStringList displayNames;
                QMap<QString, QString> displayToId;
                for (auto& [profileId, profile] : dbHier->ProfileCategories) {
                    if (!profile) continue;
                    if (QString::fromStdString(profile->Name).compare("Weapon", Qt::CaseInsensitive) != 0) continue;
                    // Profile entities
                    for (auto& [eid, entity] : profile->Entities) {
                        if (!entity) continue;
                        Weapon* dbW = dynamic_cast<Weapon*>(entity);
                        if (!dbW) continue;
                        if (dbW->weaponTypeName().compare(subCompType, Qt::CaseInsensitive) != 0) continue;
                        QString eName = QString::fromStdString(entity->Name).trimmed();
                        if (eName.isEmpty()) continue;
                        QString display = eName + " (" + dbW->weaponTypeName() + ")";
                        if (!displayToId.contains(display)) {
                            displayNames.append(display);
                            displayToId[display] = QString::fromStdString(eid);
                        }
                    }
                    // Folder entities
                    for (auto& [fid, folder] : profile->Folders) {
                        if (!folder) continue;
                        for (auto& [eid, entity] : folder->Entities) {
                            if (!entity) continue;
                            Weapon* dbW = dynamic_cast<Weapon*>(entity);
                            if (!dbW) continue;
                            if (dbW->weaponTypeName().compare(subCompType, Qt::CaseInsensitive) != 0) continue;
                            QString eName = QString::fromStdString(entity->Name).trimmed();
                            if (eName.isEmpty()) continue;
                            QString display = eName + " (" + dbW->weaponTypeName() + ")";
                            if (!displayToId.contains(display)) {
                                displayNames.append(display);
                                displayToId[display] = QString::fromStdString(eid);
                            }
                        }
                    }
                }
                if (displayNames.isEmpty()) return;
                displayNames.sort(Qt::CaseInsensitive);
                bool ok = false;
                QString selected = QInputDialog::getItem(
                    treeView,
                    "Select " + subCompType + " from DB",
                    "Choose a " + subCompType + " to apply parameters from:",
                    displayNames, 0, false, &ok
                    );
                if (!ok || selected.isEmpty()) return;
                QString dbWeaponId = displayToId.value(selected);
                if (dbWeaponId.isEmpty()) return;
                auto dbIt = dbHier->Weapons.find(dbWeaponId.toStdString());
                if (dbIt == dbHier->Weapons.end() || !dbIt->second) return;
                QJsonObject sourceJson = dbIt->second->toJson();
                // Sab pe apply karo
                for (const QVariantMap& data : subComponents) {
                    auto sceneIt = hierarchy->Weapons.find(data["ID"].toString().toStdString());
                    if (sceneIt == hierarchy->Weapons.end() || !sceneIt->second) continue;
                    Weapon* w = sceneIt->second;
                    std::string savedId     = w->ID;
                    std::string savedName   = w->Name;
                    std::string savedParent = w->parentID;
                    w->fromJson(sourceJson);
                    w->ID       = savedId;
                    w->Name     = savedName;
                    w->parentID = savedParent;
                    w->syncComponentsFromWeaponData();
                }
            });
    connect(treeView->getContextMenu(), &ContextMenu::editEntityRequested,
            this, [=](QVariantMap entityData) {
                QString entityID = entityData["ID"].toString();
                auto sceneIt = hierarchy->Entities.find(entityID.toStdString());
                if (sceneIt == hierarchy->Entities.end()) return;
                Platform* targetPlatform = dynamic_cast<Platform*>(sceneIt->second);
                if (!targetPlatform) return;
                Hierarchy* dbHier = library ? library : hierarchy;
                if (!dbHier) return;
                QStringList displayNames;
                QMap<QString, QString> displayToId;
                for (auto& [profileId, profile] : dbHier->ProfileCategories) {
                    if (!profile) continue;
                    if (profile->type != Constants::EntityType::Platform) continue;
                    auto collectEntities = [&](auto& entityMap) {
                        for (auto& [eid, entity] : entityMap) {
                            if (!entity) continue;
                            Platform* dbP = dynamic_cast<Platform*>(entity);
                            if (!dbP) continue;
                            QString eName = QString::fromStdString(entity->Name).trimmed();
                            if (eName.isEmpty()) continue;
                            QString catInfo = "";
                            QJsonObject ej = entity->toJson();
                            if (ej.contains("Category") && ej["Category"].isObject())
                                catInfo = ej["Category"].toObject()["value"].toString();
                            if (ej.contains("SubCategory") && ej["SubCategory"].isObject())
                                catInfo += "/" + ej["SubCategory"].toObject()["value"].toString();
                            QString display = eName + (catInfo.isEmpty() ? "" : " (" + catInfo + ")");
                            if (!displayToId.contains(display)) {
                                displayNames.append(display);
                                displayToId[display] = QString::fromStdString(eid);
                            }
                        }
                    };
                    collectEntities(profile->Entities);
                    for (auto& [fid, folder] : profile->Folders) {
                        if (folder) collectEntities(folder->Entities);
                    }
                }
                if (displayNames.isEmpty()) {
                    QMessageBox::information(treeView, "No Platforms", "No platform entities found in database.");
                    return;
                }
                displayNames.sort(Qt::CaseInsensitive);

                QInputDialog dialog(treeView);
                dialog.setWindowTitle("Edit Entity — Select from DB");
                dialog.setLabelText("Choose a platform to apply parameters from:");
                dialog.setComboBoxItems(displayNames);
                dialog.setComboBoxEditable(false);
                dialog.setWindowFlags(dialog.windowFlags()
                                      | Qt::WindowMinimizeButtonHint
                                      | Qt::WindowMaximizeButtonHint
                                      | Qt::WindowCloseButtonHint);
                makeDialogResizable(&dialog);
                if (dialog.exec() != QDialog::Accepted) return;
                QString selected = dialog.textValue();
                if (selected.isEmpty()) return;

                QString dbEntityId = displayToId.value(selected);
                if (dbEntityId.isEmpty()) return;
                auto dbIt = dbHier->Entities.find(dbEntityId.toStdString());
                if (dbIt == dbHier->Entities.end() || !dbIt->second) return;
                QJsonObject sourceJson = dbIt->second->toJson();
                std::string savedId     = targetPlatform->ID;
                std::string savedName   = targetPlatform->Name;
                std::string savedParent = targetPlatform->parentID;
                if (sourceJson.contains("dynamicModel") && sourceJson["dynamicModel"].isObject()) {
                    QJsonObject dmDelta = sourceJson["dynamicModel"].toObject();
                    dmDelta.remove("id");
                    dmDelta.remove("type");
                    dmDelta.remove("takeoff");
                    targetPlatform->dynamicModel.fromJson(dmDelta);
                }
                QJsonObject delta;
                if (sourceJson.contains("Category") && sourceJson["Category"].isObject()) {
                    delta["Category"] = sourceJson["Category"].toObject();
                }
                if (sourceJson.contains("SubCategory") && sourceJson["SubCategory"].isObject()) {
                    delta["SubCategory"] = sourceJson["SubCategory"].toObject();
                }
                if (!delta.isEmpty()) {
                    hierarchy->UpdateComponent(entityID, "_self", delta);
                }
            });
    connect(treeView->getContextMenu(), &ContextMenu::editMultipleEntitiesRequested,
            this, [=](QList<QVariantMap> entitiesData) {
                if (entitiesData.isEmpty()) return;

                Hierarchy* dbHier = library ? library : hierarchy;
                if (!dbHier) return;

                QStringList displayNames;
                QMap<QString, QString> displayToId;

                for (auto& [profileId, profile] : dbHier->ProfileCategories) {
                    if (!profile) continue;
                    if (profile->type != Constants::EntityType::Platform) continue;

                    auto collectEntities = [&](auto& entityMap) {
                        for (auto& [eid, entity] : entityMap) {
                            if (!entity) continue;
                            Platform* dbP = dynamic_cast<Platform*>(entity);
                            if (!dbP) continue;
                            QString eName = QString::fromStdString(entity->Name).trimmed();
                            if (eName.isEmpty()) continue;
                            QString catInfo = "";
                            QJsonObject ej = entity->toJson();
                            if (ej.contains("Category") && ej["Category"].isObject())
                                catInfo = ej["Category"].toObject()["value"].toString();
                            if (ej.contains("SubCategory") && ej["SubCategory"].isObject())
                                catInfo += "/" + ej["SubCategory"].toObject()["value"].toString();
                            QString display = eName + (catInfo.isEmpty() ? "" : " (" + catInfo + ")");
                            if (!displayToId.contains(display)) {
                                displayNames.append(display);
                                displayToId[display] = QString::fromStdString(eid);
                            }
                        }
                    };

                    collectEntities(profile->Entities);
                    for (auto& [fid, folder] : profile->Folders) {
                        if (folder) collectEntities(folder->Entities);
                    }
                }
                if (displayNames.isEmpty()) {
                    QMessageBox::information(treeView, "No Platforms",
                                             "No platform entities found in database.");
                    return;
                }

                displayNames.sort(Qt::CaseInsensitive);

                bool ok = false;
                QString selected = QInputDialog::getItem(
                    treeView,
                    "Edit Entities — Select from DB",
                    "Choose a platform to apply parameters from:",
                    displayNames, 0, false, &ok
                    );
                if (!ok || selected.isEmpty()) return;

                QString dbEntityId = displayToId.value(selected);
                if (dbEntityId.isEmpty()) return;

                auto dbIt = dbHier->Entities.find(dbEntityId.toStdString());
                if (dbIt == dbHier->Entities.end() || !dbIt->second) return;
                QJsonObject sourceJson = dbIt->second->toJson();
                for (const QVariantMap& entityData : entitiesData) {
                    QString entityID = entityData["ID"].toString();
                    auto sceneIt = hierarchy->Entities.find(entityID.toStdString());
                    if (sceneIt == hierarchy->Entities.end()) continue;
                    Platform* targetPlatform = dynamic_cast<Platform*>(sceneIt->second);
                    if (!targetPlatform) continue;
                    // dynamicModel apply
                    if (sourceJson.contains("dynamicModel") && sourceJson["dynamicModel"].isObject()) {
                        QJsonObject dmDelta = sourceJson["dynamicModel"].toObject();
                        dmDelta.remove("id");
                        dmDelta.remove("type");
                        // if (targetPlatform->dynamicModel) {
                            targetPlatform->dynamicModel.fromJson(dmDelta);
                        // }
                    }
                    // Category + SubCategory apply
                    QJsonObject delta;
                    if (sourceJson.contains("Category") && sourceJson["Category"].isObject())
                        delta["Category"] = sourceJson["Category"].toObject();
                    if (sourceJson.contains("SubCategory") && sourceJson["SubCategory"].isObject())
                        delta["SubCategory"] = sourceJson["SubCategory"].toObject();
                    if (!delta.isEmpty()) {
                        hierarchy->UpdateComponent(entityID, "_self", delta);
                    }
                }
            });
}
// %%% Library Signal Connections %%%
/* Connect signals for library and tree view */
void HierarchyConnector::connectLibrarySignals(Hierarchy* library, HierarchyTree* libTree)
{
    // Validate inputs
    if (!library || !libTree) {
        return;
    }
    // Connect library to tree view
    connect(library, &Hierarchy::profileAdded, libTree, &HierarchyTree::profileAdded);
    connect(library, &Hierarchy::folderAdded, libTree, &HierarchyTree::folderAdded);
    connect(library, &Hierarchy::entityAdded, libTree, &HierarchyTree::entityAdded);
    connect(library, &Hierarchy::componentAdded, libTree, &HierarchyTree::componentAdded);
    connect(library, &Hierarchy::subComponentAdded, libTree, &HierarchyTree::subComponentAdded);
    connect(library, &Hierarchy::profileRemoved, libTree, &HierarchyTree::profileRemoved);
    connect(library, &Hierarchy::folderRemoved, libTree, &HierarchyTree::folderRemoved);
    connect(library, &Hierarchy::entityRemoved, libTree, &HierarchyTree::entityRemoved);
    connect(libTree->getContextMenu(), &ContextMenu::addProfileRequested,
            library, &Hierarchy::addProfileCategaory);
    connect(libTree->getContextMenu(), &ContextMenu::removeProfileRequested,
            library, &Hierarchy::removeProfileCategaory);
    connect(libTree->getContextMenu(), &ContextMenu::addFolderRequested,
            library, &Hierarchy::addFolder);
    connect(libTree->getContextMenu(), &ContextMenu::removeFolderRequested,
            library, &Hierarchy::removeFolder);
    // Connect add entity action
    connect(libTree->getContextMenu(), &ContextMenu::addEntityRequested, this,
            [=](QString parentId, QString entityName, bool isProfileParent, QVariantMap components) {
                Entity* newEntity = library->addEntity(parentId, entityName, isProfileParent);
                QSet<QString> addedComponents;
                for (const auto& component : components.keys()) {
                    if (components.value(component).toBool() && !addedComponents.contains(component)) {
                        try {
                            newEntity->addComponent(component.toStdString());
                            addedComponents.insert(component);
                            emit library->componentAdded(QString::fromStdString(newEntity->ID), "ID", component);
                        } catch (const std::exception& e) {

                        }
                    }
                }
            });
    // Connect remove entity action
    connect(libTree->getContextMenu(), &ContextMenu::removeEntityRequested,
            library, &Hierarchy::removeEntity);
    // Connect rename signals
    connect(library, &Hierarchy::profileRenamed, libTree, &HierarchyTree::profileRenamed);
    connect(library, &Hierarchy::folderRenamed, libTree, &HierarchyTree::folderRenamed);
    connect(library, &Hierarchy::entityRenamed, libTree, &HierarchyTree::entityRenamed);
    // Connect context menu rename action
    connect(libTree->getContextMenu(), &ContextMenu::renameItemRequested,
            library, [=](QVariantMap data) {
                QString type = data["type"].toString();
                QString id = data["ID"].toString();
                QString name = data["name"].toString();
                if (type == "profile") {
                    library->renameProfileCategaory(id, name);
                } else if (type == "folder") {
                    library->renameFolder(id, name);
                } else if (type == "entity") {
                    library->renameEntity(id, name);
                }
            });
    // Connect copy-paste signals
    connect(libTree->getContextMenu(), &ContextMenu::copyItemRequested,
            libTree, &HierarchyTree::copyItemRequested);
    connect(libTree->getContextMenu(), &ContextMenu::pasteItemRequested,
            libTree, &HierarchyTree::pasteItemRequested);
    // Handle copy action
    connect(libTree, &HierarchyTree::copyItemRequested, this,
            [this, library](QVariantMap data) {
                QString type = data["type"].toString();
                if (type != "entity") {
                    return;
                }
                copydata = data;
                copySource = library;
            });
    // Handle paste action
    connect(libTree, &HierarchyTree::pasteItemRequested, this,
            [this, library](QVariantMap targetData) {
                if (copydata.isEmpty()) {
                    return;
                }
                if (!copySource) {
                    return;
                }
                QString type = copydata["type"].toString();
                QString id = copydata["ID"].toString();
                QString targetType = targetData["type"].toString();
                QString targetId = targetData["ID"].toString();
                if (type != "entity") {
                    return;
                }
                try {
                    auto entityIt = copySource->Entities.find(id.toStdString());
                    if (entityIt == copySource->Entities.end()) {
                        return;
                    }
                    QJsonObject entityJson = entityIt->second->toJson();
                    QString newId = QUuid::createUuid().toString();
                    entityJson["id"] = newId;
                    QStringList componentNames;
                    for (auto it = entityJson.begin(); it != entityJson.end(); ++it) {
                        QString key = it.key();
                        if (key != "id" && key != "name" && key != "parent_id" && key != "branch" &&
                            key != "active" && key != "parameters" && key != "type") {
                            componentNames << key;
                        }
                    }
                    Entity* newEntity = library->addEntityFromJson(targetId, entityJson, targetType == "profile");
                    if (newEntity) {
                        for (const QString& compName : componentNames) {
                            emit library->componentAdded(QString::fromStdString(newEntity->ID), "ID", compName);
                        }
                    }
                    copydata.clear();
                    copySource = nullptr;
                } catch (const std::exception& e) {
                }
            });

}

void HierarchyConnector::initializeLibraryData(Hierarchy* library)
{
    if (!library) {
        return;
    }
    ProfileCategaory* platform = library->addProfileCategaory("Platform");
    if (platform) platform->setProfileType(Constants::EntityType::Platform);

    ProfileCategaory* sensor = library->addProfileCategaory("Sensor");
    if (sensor) sensor->setProfileType(Constants::EntityType::Sensor);

    ProfileCategaory* radio = library->addProfileCategaory("Radio");
    if (radio) radio->setProfileType(Constants::EntityType::Radio);

    ProfileCategaory* iff = library->addProfileCategaory("IFF");
    if (iff) iff->setProfileType(Constants::EntityType::IFF);

    ProfileCategaory* weapon = library->addProfileCategaory("Weapon");
    if (weapon) weapon->setProfileType(Constants::EntityType::Weapon);

    ProfileCategaory* specialZone = library->addProfileCategaory("SpecialZone");
    if (specialZone) specialZone->setProfileType(Constants::EntityType::SpecialZone);

    ProfileCategaory* fixedPoints = library->addProfileCategaory("FixedPoints");
    if (fixedPoints) fixedPoints->setProfileType(Constants::EntityType::FixedPoint);

    ProfileCategaory* formation = library->addProfileCategaory("Formation");
    if (formation) formation->setProfileType(Constants::EntityType::Formation);

    if (!ApplicationDialog::getGlobalDatabaseEnabled()) {
        return;
    }
    QString aircraftDbPath = TDFManager::instance()->getAircraftDbPath();
    QFile jsonFile(aircraftDbPath);
    if (jsonFile.open(QIODevice::ReadOnly)) {
        QByteArray data = jsonFile.readAll();
        jsonFile.close();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("hierarchy")) {
                QJsonObject hierarchyObj = obj["hierarchy"].toObject();
                library->fromJson(hierarchyObj);
                return;
            } else {
                qWarning() << "JSON file does not contain 'hierarchy' key";
            }
        } else {
            qWarning() << "Failed to parse JSON:" << parseError.errorString();
        }
    } else {
        qWarning() << "[HierarchyConnector] Could not open aircraft DB file:" << aircraftDbPath;
    }
    qWarning() << "Using fallback default library data";
    if (platform) {
        Folder* air = platform->addFolder("Air");
        if (air) {
            Entity* fighterJet = air->addEntity("FighterJet");
            if (fighterJet) {
                try {
                    fighterJet->addComponent("transform");
                    emit library->componentAdded(
                        QString::fromStdString(fighterJet->ID), "ID", "transform");
                } catch (const std::exception& e) {
                    Q_UNUSED(e)
                }
            }
        }
    }
}

/* Initialize dummy data for hierarchy */
void HierarchyConnector::initializeDummyData(Hierarchy* hierarchy)
{
    // Add profile categories
    ProfileCategaory* platform = hierarchy->addProfileCategaory("Platform");
    platform->setProfileType(Constants::EntityType::Platform);
    hierarchy->addProfileCategaory("SpecialZone")->setProfileType(Constants::EntityType::SpecialZone);
    hierarchy->addProfileCategaory("Radio")->setProfileType(Constants::EntityType::Radio);
    hierarchy->addProfileCategaory("Sensor")->setProfileType(Constants::EntityType::Sensor);
    hierarchy->addProfileCategaory("Weapon")->setProfileType(Constants::EntityType::Weapon);
    hierarchy->addProfileCategaory("IFF")->setProfileType(Constants::EntityType::IFF);
    hierarchy->addProfileCategaory("Formation")->setProfileType(Constants::EntityType::Formation);
    hierarchy->addProfileCategaory("FixedPoints")->setProfileType(Constants::EntityType::FixedPoint);
    QFile file(":/jsonData/DB/jsonData/entity_DB.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
}
// %%% Library Operations %%%
/* Load JSON to library */
void HierarchyConnector::loadToLibrary(QMainWindow* parent)
{
    if (!parent) {
        return;
    }
    Hierarchy* targetLibrary = nullptr;
    HierarchyTree* targetLibTreeView = nullptr;
    if (MainWindow* mw = MainWindow::instance()) {
        targetLibrary = mw->getDatabaseHierarchy();
    }
    if (ScenarioEditor* se = qobject_cast<ScenarioEditor*>(parent)) {
        targetLibTreeView = se->libTreeView;
    } else if (RuntimeEditor* re = qobject_cast<RuntimeEditor*>(parent)) {
        targetLibTreeView = re->libTreeView;
    }
    if (!targetLibrary) {
        QMessageBox::critical(parent, "Error", "Database hierarchy not available");
        return;
    }
    if (!targetLibrary) {
        QMessageBox::critical(parent, "Error", "Library not available");
        return;
    }
    QString homeDir = QDir::homePath();
    QString tdfPath = homeDir + "/TDF";
    QString databasePath = tdfPath + "/Database";
    QDir dir;
    if (!dir.exists(tdfPath)) {
        dir.mkpath(tdfPath);
    }
    if (!dir.exists(databasePath)) {
        dir.mkpath(databasePath);
    }
    QString filePath = QFileDialog::getOpenFileName(
        parent,
        "Open File to Library",
        databasePath,
        "Supported Files (*.db *.json);;Database Files (*.db);;JSON Files (*.json);;All Files (*.*)"
        );
    if (filePath.isEmpty()) {
        return;
    }
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(parent, "Error", "Cannot open file");
        return;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::critical(parent, "Error",
                              QString("Invalid JSON file: %1").arg(err.errorString()));
        return;
    }
    QJsonObject obj = doc.object();
    if (!obj.contains("hierarchy")) {
        QMessageBox::critical(parent, "Error", "No hierarchy data in file");
        return;
    }
    QJsonObject hier = obj["hierarchy"].toObject();
    try {
        targetLibrary->fromJson(hier);
        // Update UI
        if (targetLibTreeView) {
            targetLibTreeView->setLibraryFileName(filePath);
            targetLibTreeView->getTreeWidget()->clearSelection();
            targetLibTreeView->getTreeWidget()->update();
        }
        RecentProjectsManager::instance()->addToRecentProjects(filePath,
                                                               RecentProjectsManager::LibraryData);
    } catch (const std::exception& e) {
        QMessageBox::critical(parent, "Error",
                              QString("Failed to load library: %1").arg(e.what()));
    }
}

void HierarchyConnector::openXmlFile(Hierarchy* hierarchy,QString fullPath)
{
    if (hierarchy) {
        QDomElement root = getDom(fullPath);
        QFileInfo fileInfo(fullPath);
        QString folderPath = fileInfo.absolutePath();
        QDomElement ptfDbList;

        // Find <Field> whose <Name> is "ptf"
        QDomNodeList fields = root.elementsByTagName("Field");

        for (int i = 0; i < fields.count(); ++i) {
            QDomElement field = fields.at(i).toElement();

            QDomElement nameElement =
                field.firstChildElement("Name");

            if (nameElement.text().trimmed() == "ptf") {
                ptfDbList = field.firstChildElement("DBList");
                break;
            }
        }

        if (ptfDbList.isNull()) {
            qDebug() << "PTF DBList not found";
            return;
        }

        QDomElement child = ptfDbList.firstChildElement("Datablock_Instance");

        QList<QDomElement> profiles;
        int count = 0;

        while (!child.isNull()) {
            profiles.append(child);
            QString name = child.attribute("Name");
            QString path = child.text().trimmed();

            qDebug() << "PTF:" << name << path;

            ++count;
            child = child.nextSiblingElement("Datablock_Instance");
        }

        qDebug() << "TOTAL:" << count;

        // QDomNodeList profiles =
        // ptfDbList.elementsByTagName("Datablock_Instance");


        // QDomNodeList profiles = root.elementsByTagName(QStringLiteral("Datablock_Instance"));
        ProfileCategaory *profile = nullptr;
        for (const auto& [key, profilePtr] : hierarchy->ProfileCategories) {
            QString name = QString::fromStdString(profilePtr->Name).toLower();
            if(profilePtr->type == Constants::EntityType::Platform){
                profile = profilePtr;
                break;
            }
        }
        if(!profile || profile->type != Constants::EntityType::Platform)return;
        for (int i = 0; i < profiles.count(); ++i) {
            QDomElement el = profiles.at(i).toElement();
            if (!el.isNull()) {
                QString name = el.attribute("Name");
                QString path = el.text();
                Entity* entity = profile->addEntity(name.toStdString());
                entity->fromJson(QJsonObject());
                QCoreApplication::processEvents();
                Platform* plf = dynamic_cast<Platform*>(entity);
                QDomElement platform = getDom(folderPath+"/"+path);
                // QDomNodeList comp = platform.childNodes();

                QList<QDomElement> comp;

                QDomElement initialField;
                QDomElement trajectoryField;

                QDomElement field =
                    platform.firstChildElement("Field");

                while (!field.isNull()) {
                    comp.append(field);

                    // Direct child Name check
                    QDomElement nameEl = field.firstChildElement("Name");

                    if (nameEl.text().trimmed() == "initial") {
                        initialField = field;
                    }else
                        if (nameEl.text().trimmed() == "trajectory") {
                            trajectoryField = field;
                        }
                    field = field.nextSiblingElement("Field");
                }

                std::vector<std::string>supportedComponents =  plf->getSupportedComponents();
                for (const std::string &component : supportedComponents) {
                    plf->addComponent(component);
                }

                ///intial position
                if (!initialField.isNull()) {

                    QDomElement datablock =
                        initialField.firstChildElement("Datablock");

                    // initial ke Datablock ke direct Fields
                    double speed = 0.0;
                    double heading = 0.0;
                    double pitch = 0.0;
                    double roll = 0.0;

                    for (QDomElement field = datablock.firstChildElement("Field");
                         !field.isNull();
                         field = field.nextSiblingElement("Field"))
                    {
                        QString fieldName =
                            field.firstChildElement("Name").text().trimmed();

                        if (fieldName == "pos") {

                            QDomElement latLon =
                                field.firstChildElement("LatLon");

                            QString lat =
                                latLon.firstChildElement("Lat").text().trimmed();

                            QString lon =
                                latLon.firstChildElement("Lon").text().trimmed();

                            double latitude = dmsToDecimal(lat);
                            double longitude = dmsToDecimal(lon);

                            plf->transform.setGeoCord(latitude,longitude);
                        }
                        else if (fieldName == "speed") {

                            speed = field.firstChildElement("Value")
                                        .text().trimmed().toDouble();

                            plf->dynamicModel.moveSpeed = speed;
                        }
                        else if (fieldName == "heading") {

                            heading = field.firstChildElement("Value")
                                          .text().trimmed().toDouble();

                            plf->transform.setHeading(heading);
                        }
                        else if (fieldName == "pitch") {

                            pitch = field.firstChildElement("Value")
                                        .text().trimmed().toDouble();

                            //plf->Pitch = pitch;
                        }
                        else if (fieldName == "roll") {

                            roll = field.firstChildElement("Value")
                                       .text().trimmed().toDouble();

                            //plf->Roll = roll;
                        }
                    }
                }

                ///trajectory data
                if (!trajectoryField.isNull()) {

                    QDomElement trajectoryBlock =
                        trajectoryField.firstChildElement("Datablock");

                    QDomElement wayPointsField;

                    // 2. way_points Field find karo
                    for (QDomElement field = trajectoryBlock.firstChildElement("Field");
                         !field.isNull();
                         field = field.nextSiblingElement("Field"))
                    {
                        QString fieldName =
                            field.firstChildElement("Name").text().trimmed();

                        if (fieldName == "way_points") {
                            wayPointsField = field;
                            break;
                        }
                    }

                    if (!wayPointsField.isNull()) {

                        QDomElement dbList =
                            wayPointsField.firstChildElement("DBList");

                        // 3. Har DBList_Instance = ek waypoint
                        for (QDomElement waypoint =
                             dbList.firstChildElement("DBList_Instance");
                             !waypoint.isNull();
                             waypoint =
                             waypoint.nextSiblingElement("DBList_Instance"))
                        {
                            QString waypointName =
                                waypoint.attribute("Name");

                            double latitude = 0.0;
                            double longitude = 0.0;
                            double speed = 0.0;

                            // 4. Waypoint ke fields read karo
                            for (QDomElement field =
                                 waypoint.firstChildElement("Field");
                                 !field.isNull();
                                 field = field.nextSiblingElement("Field"))
                            {
                                QString fieldName =
                                    field.firstChildElement("Name")
                                        .text()
                                        .trimmed();

                                // Speed
                                if (fieldName == "spd") {

                                    speed =
                                        field.firstChildElement("Value")
                                            .text()
                                            .trimmed()
                                            .toDouble();
                                }

                                // Position
                                else if (fieldName == "pos") {

                                    QDomElement latLon =
                                        field.firstChildElement("LatLon");

                                    QString lat =
                                        latLon.firstChildElement("Lat")
                                            .text()
                                            .trimmed();

                                    QString lon =
                                        latLon.firstChildElement("Lon")
                                            .text()
                                            .trimmed();

                                    latitude = dmsToDecimal(lat);
                                    longitude = dmsToDecimal(lon);
                                }
                            }

                            Waypoints* newWaypoint = new Waypoints();
                            newWaypoint->position  = new Vector(latitude, 1000, longitude);
                            newWaypoint->speed     = speed;
                            plf->trajectory.addTrajectory(newWaypoint);
                            qDebug() << "Waypoint:" << waypointName;
                            qDebug() << "Lat:" << latitude;
                            qDebug() << "Lon:" << longitude;
                            qDebug() << "Speed:" << speed;
                        }
                    }
                }


                for (int j = 0; j < comp.count(); ++j) {
                    QDomElement fieldElement = comp.at(j).toElement();
                    if (!fieldElement.isNull()) {
                        QDomElement compName = fieldElement.elementsByTagName("Name").at(0).toElement();
                        if(compName.text().contains("dynamic") /*&& plf->dynamicModel*/){
                            QDomNodeList valueName = compName.parentNode().toElement().elementsByTagName("Name");
                            for (int k = 0; k < valueName.count(); ++k) {
                                QDomElement Name = valueName.at(k).toElement();
                                if(Name.text().contains("altitude")){
                                    QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                    plf->dynamicModel.Altitude = value.toFloat();
                                }else
                                    if(Name.text().contains("turn_rate")){
                                        QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                        plf->dynamicModel.turnRate = value.toFloat();
                                    }else
                                        if(Name.text().contains("acceleration")){
                                            QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                            plf->dynamicModel.Acceleration = value.toFloat();
                                        }else
                                            if(Name.text().contains("deceleration")){
                                                QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                                plf->dynamicModel.Decceleration = value.toFloat();
                                            }else
                                                if(Name.text().contains("climb_rate")){
                                                    QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                                    plf->dynamicModel.climbRate = value.toFloat();
                                                }else
                                                    if(Name.text().contains("dive_rate")){
                                                        QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                                        plf->dynamicModel.diveRate = value.toFloat();
                                                    }
                            }
                        }

                        if(compName.text().contains("physical") /*&& plf->collider*/){
                            QDomNodeList valueName = compName.parentNode().toElement().elementsByTagName("Name");
                            for (int k = 0; k < valueName.count(); ++k) {
                                QDomElement Name = valueName.at(k).toElement();
                                if(Name.text().contains("width")){
                                    QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                    plf->collider.getDimension().Width = value.toFloat();
                                }else
                                    if(Name.text().contains("length")){
                                        QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                        plf->collider.getDimension().Length = value.toFloat();
                                    }else
                                        if(Name.text().contains("height")){
                                            QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                            plf->collider.getDimension().Height = value.toFloat();
                                        }

                            }
                        }
                        if(compName.text().contains("sensors") /*&& plf->sensors*/){
                            QDomNodeList valueName = fieldElement.elementsByTagName("Name");
                            QDomNode n = fieldElement.firstChild();
                            while(!n.isNull()) {
                                QDomElement e = n.toElement();
                                if(!e.isNull() && e.tagName() == "DBList") {
                                    QString nm = e.firstChild().toElement().attribute("Name");
                                    if(nm.isEmpty())break;
                                    hierarchy->addSubComponent(QString::fromStdString(plf->sensors.ID),
                                                               nm, "Generic");
                                }
                                n = n.nextSibling();
                            }
                        }
                        if(compName.text().contains("radios") /*&& plf->radios*/){
                            QDomNodeList valueName = fieldElement.elementsByTagName("Name");
                            QDomNode n = fieldElement.firstChild();
                            while(!n.isNull()) {
                                QDomElement e = n.toElement();
                                if(!e.isNull() && e.tagName() == "DBList") {
                                    QString nm = e.firstChild().toElement().attribute("Name");
                                    if(nm.isEmpty())break;
                                    hierarchy->addSubComponent(QString::fromStdString(plf->radios.ID),
                                                               nm);
                                }
                                n = n.nextSibling();
                            }
                        }

                        if(compName.text().contains("iffs") /*&& plf->iffs*/){
                            QDomNodeList valueName = fieldElement.elementsByTagName("Name");
                            QDomNode n = fieldElement.firstChild();
                            while(!n.isNull()) {
                                QDomElement e = n.toElement();
                                if(!e.isNull() && e.tagName() == "DBList") {
                                    QString nm = e.firstChild().toElement().attribute("Name");
                                    if(nm.isEmpty())break;
                                    hierarchy->addSubComponent(QString::fromStdString(plf->iffs.ID),
                                                               nm);
                                }
                                n = n.nextSibling();
                            }
                        }

                    }
                }
            }
        }
    } else {
    }
}
QDomElement HierarchyConnector::getDom(QString filePath){
    QFile file(filePath);
    QDomDocument doc;

    if (!file.open(QIODevice::ReadOnly) || !doc.setContent(&file)) {
        file.close();
        return doc.documentElement();
    }
    file.close();
    return doc.documentElement();

}
bool HierarchyConnector::confirmAndLoadAssociatedDatabase(Hierarchy* targetHierarchy, QWidget* parentWidget) const
{
    MainWindow* mw = MainWindow::instance();
    if (!mw || !mw->databaseEditor)
        return true;

    const bool isScenarioHier = (mw->scenarioEditor
                                 && targetHierarchy == mw->scenarioEditor->hierarchy);
    const bool isRuntimeHier  = (mw->runtimeEditor
                                && targetHierarchy == mw->runtimeEditor->hierarchy);
    if (!isScenarioHier && !isRuntimeHier)
        return true;

    const QString currentDbPath = mw->databaseEditor->lastSavedFilePath;
    QString associatedDbPath = mw->associatedDatabasePathForHierarchy(targetHierarchy);

    if (!currentDbPath.isEmpty() && !associatedDbPath.isEmpty()
        && QFileInfo(currentDbPath).absoluteFilePath()
               == QFileInfo(associatedDbPath).absoluteFilePath()) {
        return true;
    }

    if (!currentDbPath.isEmpty() && associatedDbPath.isEmpty())
        return true;

    if (!associatedDbPath.isEmpty() && QFileInfo::exists(associatedDbPath)) {
        if (!currentDbPath.isEmpty()
            && QFileInfo(currentDbPath).absoluteFilePath()
                   == QFileInfo(associatedDbPath).absoluteFilePath()) {
            return true;
        }

        QMessageBox msgBox(parentWidget);
        msgBox.setWindowTitle("Load Associated Database");
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(
            QString("This item is sourced from the database:\n%1\n\n"
                    "That database is not currently loaded in the Database Editor.\n"
                    "Load it now before adding the entity / component?")
                .arg(QFileInfo(associatedDbPath).fileName()));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);

        if (msgBox.exec() != QMessageBox::Ok)
            return false;

        if (parentWidget) parentWidget->setEnabled(false);
        mw->showLoadingOverlay(
            QString("Loading Database: %1...").arg(QFileInfo(associatedDbPath).fileName()));
        QCoreApplication::processEvents();

        mw->databaseEditor->loadFromJsonFile(associatedDbPath);
        if (isScenarioHier && mw->scenarioEditor)
            mw->scenarioEditor->setLoadedDatabaseFile(associatedDbPath);
        if (isRuntimeHier && mw->runtimeEditor)
            mw->runtimeEditor->setLoadedDatabaseFile(associatedDbPath);

        mw->hideLoadingOverlay();
        if (parentWidget) parentWidget->setEnabled(true);

        return true;
    }

    if (currentDbPath.isEmpty()) {
        QMessageBox msgBox(parentWidget);
        msgBox.setWindowTitle("Database Not Loaded");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(
            "No database is currently loaded in the Database Editor.\n\n"
            "Please load a .db file in the Database Editor first,\n"
            "then try adding the entity / component again.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return false;
    }

    return true;
}
