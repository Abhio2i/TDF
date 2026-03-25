
/* ========================================================================= */
/* File: hierarchyconnector.cpp                                             */
/* Purpose: Manages connections between hierarchy, UI, and file operations   */
// Written by   : Arti Rajpoot
/* ========================================================================= */

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
// %%% Static Instance %%%
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
                                        TacticalDisplay* tactical, Inspector* inspector)
{
    if (!hierarchy || !treeView) {
        return;
    }
    treeView->getContextMenu()->m_hierarchy = hierarchy;

    // ── Hierarchy → TreeView ─────────────────────────────────────────────
    connect(hierarchy, &Hierarchy::profileAdded, treeView, &HierarchyTree::profileAdded);
    connect(hierarchy, &Hierarchy::folderAdded, treeView, &HierarchyTree::folderAdded);
    connect(hierarchy, &Hierarchy::entityAdded, treeView,
            [=](QString parentID, QString ID, QString entityName) {
                treeView->entityAdded(parentID, ID, entityName);
                QTimer::singleShot(0, treeView, [=]() {
                    if (!hierarchy->Entities) return;
                    auto it = hierarchy->Entities->find(ID.toStdString());
                    if (it != hierarchy->Entities->end()) {
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
                QString sensorType = dialog->getSensorType();
                Entity* newEntity = hierarchy->addEntity(parentId, entityName, isProfileParent);

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
                    }
                }

                Platform* platform = dynamic_cast<Platform*>(newEntity);
                if (initLat < 190 && platform) {
                    platform->transform->setGeoCord(initLat, initlon);
                }

                if (platform && dialog->isScEnabled()) {
                    const float range = dialog->getRange() / 2;
                    QPointF city = dialog->getCity();
                    double lat = city.x() + ((QRandomGenerator::global()->generateDouble() - 0.5) * 2 * (range / 111.111));
                    double lon = city.y() + ((QRandomGenerator::global()->generateDouble() - 0.5) * 2 * (range / 100));
                    double heading = 360 * QRandomGenerator::global()->generateDouble();
                    if (initLat < 190 && platform) {
                        lat = initLat;
                        lon = initlon;
                    }
                    platform->transform->setGeoCord(lat, lon);
                    platform->transform->setFromEulerAngles(QVector3D(0, heading, 0));

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

                    if (waypointcount > 1) {
                        Waypoints* newWaypoint = new Waypoints();
                        newWaypoint->position  = new Vector(lat, Altitude, lon);
                        newWaypoint->speed     = speed;
                        platform->trajectory->addTrajectory(newWaypoint);
                    }

                    for (int i = 0; i < waypointcount; i++) {
                        auto [latAtRadius, lonAtRadius] = calculateNewLatLong(lat, lon, head, waypointoffsetlength);
                        Waypoints* newWaypoint = new Waypoints();
                        newWaypoint->position  = new Vector(latAtRadius, Altitude, lonAtRadius);
                        newWaypoint->speed     = speed;
                        platform->trajectory->addTrajectory(newWaypoint);
                        speed    += speedOffset;
                        Altitude += AltitudeOffset;

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

                    emit hierarchy->trajectoryisAdded(platform->ID.c_str(), platform->trajectory);

                    std::string id = platform->sensors->ID;
                    hierarchy->addSubComponent(QString::fromStdString(id), ComponentType::SensorProfile, "Radar_" + entityName, "Generic");
                    hierarchy->addSubComponent(QString::fromStdString(id), ComponentType::SensorProfile, "ESM_"   + entityName, "ESM");
                    hierarchy->addSubComponent(QString::fromStdString(id), ComponentType::SensorProfile, "CSM_"   + entityName, "CSM");

                    id = platform->iffs->ID;
                    hierarchy->addSubComponent(QString::fromStdString(id), ComponentType::IFFProfile, "IFF_" + entityName);

                    id = platform->radios->ID;
                    hierarchy->addSubComponent(QString::fromStdString(id), ComponentType::RadioProfile, "Radio" + entityName);

                    if (dialog) {
                        const int minspd   = dialog->getMinPlaneSpeed();
                        const int maxspd   = dialog->getMaxPlaneSpeed();
                        const int minturn  = dialog->getMinTurnRadius();
                        const int maxturn  = dialog->getMaxTurnRadius();
                        const int spd      = minspd + (QRandomGenerator::global()->generateDouble() * (maxspd - minspd));
                        const int turnRad  = minturn + (QRandomGenerator::global()->generateDouble() * (maxturn - minturn));
                        platform->dynamicModel->moveSpeed = spd;
                        platform->dynamicModel->turnRate  = turnRad;

                        const int minRadRng = dialog->getMinRadarRange();
                        const int maxRadRng = dialog->getMaxRadarRange();
                        for (auto const& pair : *platform->sensors->sensors) {
                            Sensor* s = pair.second;
                            s->range = minRadRng + (QRandomGenerator::global()->generateDouble() * (maxRadRng - minRadRng));
                        }

                        const int minRadioRng = dialog->getMinRadioRange();
                        const int maxRadioRng = dialog->getMaxRadioRange();
                        for (auto const& pair : *platform->radios->radios) {
                            Radio* r = pair.second;
                            r->Range = minRadioRng + (QRandomGenerator::global()->generateDouble() * (maxRadioRng - minRadioRng));
                        }
                    }
                    QCoreApplication::processEvents();
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
                    hierarchy->addSubComponent(entityID, ComponentType::IFFProfile,
                                               componentName, sensorType, sensorProfileId);
                } else if (componentType == "sensors") {
                    hierarchy->addSubComponent(entityID, ComponentType::SensorProfile,
                                               componentName, sensorType, sensorProfileId);
                    hierarchy->attachSensors(entityID, componentName, sensorType);
                } else if (componentType == "radios") {
                    hierarchy->addSubComponent(entityID, ComponentType::RadioProfile,
                                               componentName, sensorType, sensorProfileId);
                } else if (componentType == "weapons") {
                    try {
                        if (entityID.isEmpty() || componentName.isEmpty()) {
                            QMessageBox::warning(nullptr, "Validation Error", "Invalid weapon data provided");
                            return;
                        }
                        if (!hierarchy || !hierarchy->Entities) {
                            QMessageBox::critical(nullptr, "Error", "Hierarchy not available");
                            return;
                        }
                        auto it = hierarchy->Entities->find(entityID.toStdString());
                        if (it == hierarchy->Entities->end()) {
                            QMessageBox::warning(nullptr, "Error", "Parent entity not found");
                            return;
                        }
                        Platform* platform = dynamic_cast<Platform*>(it->second);
                        if (!platform) {
                            QMessageBox::critical(nullptr, "Type Error", "Parent entity must be a Platform");
                            return;
                        }

                        const QString& weaponTypeName = sensorType;
                        Weapon* weapon = nullptr;
                        if      (weaponTypeName == "Bomb")      weapon = new Bomb(hierarchy);
                        else if (weaponTypeName == "Torpedo")   weapon = new Torpedo(hierarchy);
                        else if (weaponTypeName == "Artillery") weapon = new Artillery(hierarchy);
                        else if (weaponTypeName == "Rocket")    weapon = new Rocket(hierarchy);
                        else if (weaponTypeName == "Flare")     weapon = new Flare(hierarchy);
                        else if (weaponTypeName == "Chaff")     weapon = new Chaff(hierarchy);
                        else                                     weapon = new Missile(hierarchy);

                        weapon->Name         = componentName.toStdString();
                        weapon->parentEntity = platform;
                        weapon->ID           = Uuid::generateShortUniqueID();

                        if (!platform->weapons) {
                            WeaponProfile* weapProfile   = new WeaponProfile(hierarchy);
                            weapProfile->ID              = Uuid::generateShortUniqueID();
                            weapProfile->parentID        = entityID.toStdString();
                            weapProfile->parentEntity    = platform;
                            platform->weapons            = weapProfile;
                        }

                        if (platform->weapons && platform->weapons->weapons) {
                            platform->weapons->weapons->insert({weapon->ID, weapon});
                            hierarchy->Weapons->insert({weapon->ID, weapon});
                            hierarchy->Entities->insert({weapon->ID, weapon});

                            emit hierarchy->subComponentAdded(
                                QString::fromStdString(platform->weapons->ID),
                                QString::fromStdString(weapon->ID),
                                componentName);

                            QString weaponQID = QString::fromStdString(weapon->ID);
                            for (const std::string& compName : weapon->getSupportedComponents()) {
                                std::string compID;
                                if      (compName == "transform"    && weapon->transform)      compID = weapon->transform->ID;
                                else if (compName == "rigidbody"    && weapon->rigidbody)      compID = weapon->rigidbody->ID;
                                else if (compName == "collider"     && weapon->collider)       compID = weapon->collider->ID;
                                else if (compName == "trajectory"   && weapon->trajectory)     compID = weapon->trajectory->ID;
                                else if (compName == "bitmap"       && weapon->meshRenderer2d) compID = weapon->meshRenderer2d->ID;
                                else if (compName == "dynamicModel" && weapon->dynamicModel)   compID = weapon->dynamicModel->ID;
                                else if (compName == "crossSection" && weapon->crossSection)   compID = weapon->crossSection->ID;
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
                if (!hierarchy->Entities) return;
                if (hierarchy->Entities->find(entityID.toStdString()) == hierarchy->Entities->end()) return;
                QJsonObject delta;
                delta["active"] = active;
                hierarchy->UpdateComponent(entityID, "_self", delta);
                treeView->setEntityActiveState(entityID, active);
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

    connect(treeView, &HierarchyTree::pasteItemsRequested, this,
            [this, hierarchy](QVariantMap targetData, QList<QVariantMap> itemsToPaste) {
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

                QApplication::processEvents();
                for (const QVariantMap& itemData : itemsToPaste) {
                    QString sourceId = itemData["ID"].toString();
                    try {
                        auto entityIt = copySource->Entities->find(sourceId.toStdString());
                        if (entityIt != copySource->Entities->end()) {
                            QJsonObject entityJson = entityIt->second->toJson();
                            entityJson["id"]        = QString::fromStdString(Uuid::generateShortUniqueID());
                            entityJson["parent_id"] = targetId;
                            hierarchy->addEntityFromJson(targetId, entityJson, isProfile);
                        }
                    } catch (const std::exception& e) {}
                    QApplication::processEvents();
                }
            });

    connect(treeView->getContextMenu(), &ContextMenu::pasteItemsRequested,
            this, [this, hierarchy](QVariantMap targetData, QList<QVariantMap> itemsToPaste) {
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

                QApplication::processEvents();
                for (const QVariantMap& itemData : itemsToPaste) {
                    QString sourceId = itemData["ID"].toString();
                    try {
                        auto entityIt = copySource->Entities->find(sourceId.toStdString());
                        if (entityIt != copySource->Entities->end()) {
                            QJsonObject entityJson = entityIt->second->toJson();
                            entityJson["id"]        = QString::fromStdString(Uuid::generateShortUniqueID());
                            entityJson["parent_id"] = targetId;
                            hierarchy->addEntityFromJson(targetId, entityJson, isProfile);
                        }
                    } catch (const std::exception& e) {}
                    QApplication::processEvents();
                }
            });

    // ── Bulk: Remove entities ────────────────────────────────────────────
    connect(treeView, &HierarchyTree::removeEntitiesRequested,
            this, [=](QList<QPair<QString, QString>> entityInfoList) {
                for (const auto& entityInfo : entityInfoList) {
                    try {
                        hierarchy->removeEntity(entityInfo.first, entityInfo.second, false);
                    } catch (const std::exception& e) {}
                }
            });

    // ── Bulk: Set Active / Inactive ──────────────────────────────────────
    connect(treeView, &HierarchyTree::setEntitiesActiveRequested,
            this, [=](QList<QVariantMap> entities, bool active) {
                for (const QVariantMap& data : entities) {
                    QString entityID = data["ID"].toString();
                    if (!hierarchy->Entities) return;
                    if (hierarchy->Entities->find(entityID.toStdString()) == hierarchy->Entities->end()) continue;
                    QJsonObject delta;
                    delta["active"] = active;
                    hierarchy->UpdateComponent(entityID, "_self", delta);
                    treeView->setEntityActiveState(entityID, active);
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

                QString typeName = "Missile";
                if (!selectedId.isEmpty() && dbHierarchy) {
                    auto dbIt = dbHierarchy->Weapons->find(selectedId.toStdString());
                    if (dbIt != dbHierarchy->Weapons->end() && dbIt->second)
                        typeName = dbIt->second->weaponTypeName();
                } else {
                    typeName = dlg.weaponTypeStr();
                }

                for (const QVariantMap& data : entities) {
                    QString entityID = data["ID"].toString();
                    if (!hierarchy->Entities) continue;
                    auto it = hierarchy->Entities->find(entityID.toStdString());
                    if (it == hierarchy->Entities->end()) continue;

                    Platform* platform = dynamic_cast<Platform*>(it->second);
                    if (!platform) continue;

                    Weapon* weapon       = createWeapon(typeName, hierarchy);
                    weapon->Name         = weaponName.toStdString();
                    weapon->parentEntity = platform;
                    weapon->ID           = Uuid::generateShortUniqueID();

                    if (!platform->weapons) {
                        WeaponProfile* weapProfile = new WeaponProfile(hierarchy);
                        weapProfile->ID            = Uuid::generateShortUniqueID();
                        weapProfile->parentID      = entityID.toStdString();
                        weapProfile->parentEntity  = platform;
                        platform->weapons          = weapProfile;
                    }
                    if (!platform->weapons || !platform->weapons->weapons) { delete weapon; continue; }

                    if (!selectedId.isEmpty() && dbHierarchy) {
                        auto dbIt = dbHierarchy->Weapons->find(selectedId.toStdString());
                        if (dbIt != dbHierarchy->Weapons->end() && dbIt->second) {
                            std::string savedId = weapon->ID, savedName = weapon->Name, savedParent = weapon->parentID;
                            weapon->fromJson(dbIt->second->toJson());
                            weapon->ID = savedId; weapon->Name = savedName; weapon->parentID = savedParent;
                        }
                    } else {
                        QJsonObject cfg = dlg.configJson();
                        cfg["weaponTypeName"] = typeName;
                        weapon->fromJson(cfg);
                        weapon->Name = weaponName.toStdString();
                    }

                    weapon->syncComponentsFromWeaponData();
                    platform->weapons->weapons->insert({weapon->ID, weapon});
                    hierarchy->Weapons->insert({weapon->ID, weapon});
                    hierarchy->Entities->insert({weapon->ID, weapon});

                    emit hierarchy->subComponentAdded(
                        QString::fromStdString(platform->weapons->ID),
                        QString::fromStdString(weapon->ID), weaponName);

                    QString weaponQID = QString::fromStdString(weapon->ID);
                    for (const std::string& compName : weapon->getSupportedComponents()) {
                        std::string compID;
                        if      (compName == "transform"    && weapon->transform)      compID = weapon->transform->ID;
                        else if (compName == "rigidbody"    && weapon->rigidbody)      compID = weapon->rigidbody->ID;
                        else if (compName == "collider"     && weapon->collider)       compID = weapon->collider->ID;
                        else if (compName == "trajectory"   && weapon->trajectory)     compID = weapon->trajectory->ID;
                        else if (compName == "bitmap"       && weapon->meshRenderer2d) compID = weapon->meshRenderer2d->ID;
                        else if (compName == "dynamicModel" && weapon->dynamicModel)   compID = weapon->dynamicModel->ID;
                        else if (compName == "crossSection" && weapon->crossSection)   compID = weapon->crossSection->ID;
                        else continue;
                        emit hierarchy->componentAdded(weaponQID,
                                                       QString::fromStdString(compID), QString::fromStdString(compName));
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

                for (const QVariantMap& data : entities) {
                    QString entityID = data["ID"].toString();
                    if (!hierarchy->Entities) continue;
                    auto it = hierarchy->Entities->find(entityID.toStdString());
                    if (it == hierarchy->Entities->end()) continue;

                    Platform* platform = dynamic_cast<Platform*>(it->second);
                    if (!platform || !platform->sensors) continue;

                    hierarchy->addSubComponent(QString::fromStdString(platform->sensors->ID),
                                               ComponentType::SensorProfile, sensorName, sensorType, profileId);
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

                for (const QVariantMap& data : entities) {
                    QString entityID = data["ID"].toString();
                    if (!hierarchy->Entities) continue;
                    auto it = hierarchy->Entities->find(entityID.toStdString());
                    if (it == hierarchy->Entities->end()) continue;

                    Platform* platform = dynamic_cast<Platform*>(it->second);
                    if (!platform || !platform->iffs) continue;

                    hierarchy->addSubComponent(QString::fromStdString(platform->iffs->ID),
                                               ComponentType::IFFProfile, iffName, "", profileId);
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

                for (const QVariantMap& data : entities) {
                    QString entityID = data["ID"].toString();
                    if (!hierarchy->Entities) continue;
                    auto it = hierarchy->Entities->find(entityID.toStdString());
                    if (it == hierarchy->Entities->end()) continue;

                    Platform* platform = dynamic_cast<Platform*>(it->second);
                    if (!platform || !platform->radios) continue;

                    hierarchy->addSubComponent(QString::fromStdString(platform->radios->ID),
                                               ComponentType::RadioProfile, radioName, "", profileId);
                }
            });

    // ── Bulk: Set Team ───────────────────────────────────────────────────
    connect(treeView, &HierarchyTree::addTeamToEntitiesRequested, this,
            [=](QList<QVariantMap> entities, QString team) {
                for (const QVariantMap& data : entities) {
                    QString entityID = data["ID"].toString();
                    if (!hierarchy->Entities) continue;
                    auto it = hierarchy->Entities->find(entityID.toStdString());
                    if (it == hierarchy->Entities->end()) continue;

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
            this, [=](QVariantMap entityData, QString category) {
                QList<QVariantMap> entities;
                entities.append(entityData);
                emit treeView->setCategoryToEntitiesRequested(entities, category);
            });

    // ── Bulk: Set Category ────────────────────────────────────────────────
    connect(treeView, &HierarchyTree::setCategoryToEntitiesRequested, this,
            [=](QList<QVariantMap> entities, QString category) {
                for (const QVariantMap& data : entities) {
                    QString entityID = data["ID"].toString();
                    if (!hierarchy->Entities) continue;
                    auto it = hierarchy->Entities->find(entityID.toStdString());
                    if (it == hierarchy->Entities->end()) continue;

                    QJsonObject entityJson = it->second->toJson();
                    QJsonObject categoryObj;

                    if (entityJson.contains("Category") && entityJson["Category"].isObject()) {
                        categoryObj = entityJson["Category"].toObject();
                        categoryObj["value"] = category;
                    } else {
                        QJsonArray options;
                        for (const QString& opt : {"Aircraft","Helicopter","Ship","Submarine","Tank"})
                            options.append(opt);
                        categoryObj["type"]    = "option";
                        categoryObj["options"] = options;
                        categoryObj["value"]   = category;
                    }

                    QJsonObject delta;
                    delta["Category"] = categoryObj;
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
            [this, hierarchy](QVariantMap targetData) {
                if (copydata.isEmpty() || !copySource) return;
                if (copydata["type"].toString() != "entity") return;

                QString targetType, targetId = targetData["ID"].toString();
                if (targetData["type"].type() == QVariant::Map) {
                    QVariantMap typeData = targetData["type"].toMap();
                    if (typeData.contains("type") && typeData["type"].toString() == "option")
                        targetType = "profile";
                    else return;
                } else {
                    targetType = targetData["type"].toString();
                }

                try {
                    auto entityIt = copySource->Entities->find(copydata["ID"].toString().toStdString());
                    if (entityIt == copySource->Entities->end()) return;
                    QJsonObject entityJson = entityIt->second->toJson();
                    entityJson["id"] = QString::fromStdString(Uuid::generateShortUniqueID());
                    hierarchy->addEntityFromJson(targetId, entityJson, targetType == "profile");
                    copydata.clear();
                    copySource = nullptr;
                } catch (const std::exception& e) {}
            });

    // ── Drag and Drop ────────────────────────────────────────────────────
    connect(treeView, &HierarchyTree::itemDropped, this,
            [=](QVariantMap sourceData, QVariantMap targetData) {
                if (sourceData["type"].toString() != "entity") return;

                QString sourceId = sourceData["ID"].toString();
                QString targetId = targetData["ID"].toString();
                bool islib       = sourceData["islib"].toBool();
                bool isProfile   = (targetData["type"].toString() == "profile") ||
                                 targetData["type"].type() == QVariant::Map;

                try {
                    copySource = hierarchy;
                    auto entityIt = copySource->Entities->find(sourceId.toStdString());
                    if (islib && library) {
                        entityIt = library->Entities->find(sourceId.toStdString());
                        if (entityIt == library->Entities->end()) return;
                    } else if (entityIt == copySource->Entities->end()) {
                        return;
                    }

                    QJsonObject entityJson = entityIt->second->toJson();
                    Constants::EntityType sourcetype = entityIt->second->type;
                    Constants::EntityType type;
                    bool    iscomp = targetData["type"].toString() == "component";
                    QString srname = sourceData["name"].toString();
                    QString trname = targetData["name"].toString();

                    if (!iscomp) {
                        type = isProfile
                                   ? copySource->ProfileCategories[targetId.toStdString()]->type
                                   : (*copySource->Folders)[targetId.toStdString()]->type;
                    }

                    if ((trname == "sensors" || type == Constants::EntityType::Platform) &&
                        sourcetype == Constants::EntityType::Sensor) {
                        QString senstype = "Generic";
                        auto it = islib ? library->Sensors->find(sourceId.toStdString())
                                        : copySource->Sensors->find(sourceId.toStdString());
                        auto& map = islib ? *library->Sensors : *copySource->Sensors;
                        auto sit = map.find(sourceId.toStdString());
                        if (sit != map.end() && sit->second)
                            senstype = sit->second->subTypeToString(sit->second->subType);
                        hierarchy->addSubComponent(targetId, ComponentType::SensorProfile, srname, senstype, sourceId);
                    } else if ((trname == "iffs" || type == Constants::EntityType::Platform) &&
                               sourcetype == Constants::EntityType::IFF) {
                        hierarchy->addSubComponent(targetId, ComponentType::IFFProfile, srname, "", sourceId);
                    } else if ((trname == "radios" || type == Constants::EntityType::Platform) &&
                               sourcetype == Constants::EntityType::Radio) {
                        hierarchy->addSubComponent(targetId, ComponentType::RadioProfile, srname, "", sourceId);
                    }

                    if (iscomp || sourcetype != type) return;

                    Entity* entity = nullptr;
                    if      (type == Constants::EntityType::Radio)      entity = new Radio(copySource);
                    else if (type == Constants::EntityType::Sensor)     entity = new Sensor(copySource);
                    else if (type == Constants::EntityType::FixedPoint) entity = new FixedPoints(copySource);
                    else if (type == Constants::EntityType::Formation)  entity = new Formation(copySource);
                    else if (type == Constants::EntityType::SpecialZone)entity = new Specialzone(copySource);
                    else if (type == Constants::EntityType::IFF)        entity = new IFF(copySource);
                    else                                                 entity = new Platform(copySource);

                    entityJson["id"]        = QString::fromStdString(Uuid::generateShortUniqueID());
                    entityJson["parent_id"] = targetId;
                    entity->Name     = entityJson["name"].toString().toStdString();
                    entity->ID       = entityJson["id"].toString().toStdString();
                    entity->parentID = entityJson["parent_id"].toString().toStdString();

                    if (isProfile)
                        copySource->ProfileCategories[targetId.toStdString()]->addEntityWithObject(entity);
                    else
                        (*copySource->Folders)[targetId.toStdString()]->addEntityWithObject(entity);

                    entity->fromJson(entityJson);
                    QCoreApplication::processEvents();
                } catch (const std::exception& e) {}
            });

    // ── Tactical Display ─────────────────────────────────────────────────
    if (tactical) {
        connect(hierarchy, &Hierarchy::entityRemoved, tactical, &TacticalDisplay::removeMesh);
    }

    // ── Inspector ────────────────────────────────────────────────────────
    if (inspector) {
        connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
        connect(inspector, &Inspector::valueChanged, this,
                [=](QString entityID, QString componentName, QJsonObject delta) {
                    if (!componentName.contains("_self")) return;
                    if (!delta.contains("active")) return;
                    treeView->setEntityActiveState(entityID, delta["active"].toBool());
                });
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
                AddFormationDialog dialog(selectedEntities, nullptr);
                if (dialog.exec() != QDialog::Accepted) return;

                QString formationName  = dialog.getFormationName();
                QString mothershipId   = dialog.getMothershipId();
                QString formationType  = dialog.getFormationType();
                QList<QVariantMap> allies = dialog.getAllies();
                int alliesCount        = dialog.getAlliesCount();

                if (formationName.isEmpty()) {
                    QMessageBox::warning(nullptr, "Invalid Input", "Formation name cannot be empty.");
                    return;
                }

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

                Entity* fe = hierarchy->addEntity(formationProfileId, formationName, true);
                if (!fe) { QMessageBox::critical(nullptr, "Error", "Failed to create formation entity."); return; }
                hierarchy->removeEntity(formationProfileId, QString::fromStdString(fe->ID), false);

                Formation* formation  = new Formation(hierarchy);
                formation->Name       = formationName.toStdString();
                formation->ID         = Uuid::generateShortUniqueID();
                formation->parentID   = formationProfileId.toStdString();
                formation->type       = Constants::EntityType::Formation;
                hierarchy->ProfileCategories[formationProfileId.toStdString()]->addEntityWithObject(formation);
                hierarchy->Entities->insert({formation->ID, formation});

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
                formation->formationType = typeEnum;
                formation->count         = alliesCount;

                auto motherIt = hierarchy->Entities->find(mothershipId.toStdString());
                if (motherIt != hierarchy->Entities->end()) {
                    Entity* motherEntity = motherIt->second;
                    QJsonObject motherJson, entityRef;
                    entityRef["type"] = "reference";
                    entityRef["name"] = QString::fromStdString(motherEntity->Name);
                    entityRef["id"]   = QString::fromStdString(motherEntity->ID);
                    motherJson["entity"] = entityRef;
                    formation->updateComponent("mothership", motherJson);
                    formation->mothership->entity = motherEntity;
                }

                formation->formationCreate();

                int allyIndex = 0;
                for (const auto& allyData : allies) {
                    QString allyId = allyData["ID"].toString();
                    auto allyIt = hierarchy->Entities->find(allyId.toStdString());
                    if (allyIt == hierarchy->Entities->end() || allyIndex >= alliesCount) continue;

                    Entity* allyEntity = allyIt->second;
                    std::string positionName = "ally_" + std::to_string(allyIndex);

                    if (formation->formationPositions->find(positionName) != formation->formationPositions->end()) {
                        FormationPosition* position = (*formation->formationPositions)[positionName];
                        position->entity = allyEntity;

                        float offsetX = 0, offsetY = 0, offsetZ = 0;
                        if (position->Offset) { offsetX = position->Offset->x; offsetY = position->Offset->y; offsetZ = position->Offset->z; }
                        double geoLat = 0, geoLon = 0, geoAlt = 0, geoHead = 0;
                        if (position->geoOffset) { geoLat = position->geoOffset->latitude; geoLon = position->geoOffset->longitude; geoAlt = position->geoOffset->altitude; geoHead = position->geoOffset->Heading; }

                        QJsonObject positionJson, entityRef, offsetObj, geoOffsetObj;
                        entityRef["type"] = "reference";
                        entityRef["name"] = QString::fromStdString(allyEntity->Name);
                        entityRef["id"]   = QString::fromStdString(allyEntity->ID);
                        positionJson["entity"] = entityRef;

                        offsetObj["type"] = "section"; offsetObj["x"] = offsetX; offsetObj["y"] = offsetY; offsetObj["z"] = offsetZ;
                        positionJson["Offset"] = offsetObj;

                        geoOffsetObj["type"] = "section"; geoOffsetObj["lat"] = geoLat; geoOffsetObj["lon"] = geoLon;
                        geoOffsetObj["alt"] = geoAlt; geoOffsetObj["heading"] = geoHead;
                        positionJson["geoOffset"] = geoOffsetObj;

                        formation->updateComponent(QString::fromStdString(positionName), positionJson);

                        Platform* plt    = dynamic_cast<Platform*>(allyEntity);
                        Platform* mother = dynamic_cast<Platform*>(formation->mothership->entity);
                        if (plt && plt->dynamicModel && mother) {
                            plt->dynamicModel->followEntity       = mother;
                            plt->dynamicModel->formationPosition  = position;
                            plt->dynamicModel->follow             = true;
                        }
                    }
                    allyIndex++;
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

    // Connect tree view context menu to library
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
                            // Component addition failed
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
                    auto entityIt = copySource->Entities->find(id.toStdString());
                    if (entityIt == copySource->Entities->end()) {
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
                    // Paste operation failed
                }
            });
    // Handle drag-and-drop (currently disabled)
    connect(libTree, &HierarchyTree::itemDropped, this, [=](QVariantMap sourceData, QVariantMap targetData) {
        return;

    });
}

void HierarchyConnector::initializeLibraryData(Hierarchy* library)
{
    if (!library) {
        return;
    }

    // Always add default profile categories (structure hamesha chahiye)
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

    // If database disabled in settings — profiles structure ban gayi, Aircraft.db skip
    if (!ApplicationDialog::getGlobalDatabaseEnabled()) {
        qDebug() << "[HierarchyConnector] Default database disabled, skipping Aircraft.db load.";
        return;
    }

    // Database enabled — Aircraft.db se full data load karo
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

                // ✅ FIXED: Don't call clear() - fromJson will replace data
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

    // Fallback — Aircraft.db nahi mila to Platform > Air > FighterJet add karo
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
    return;
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {

        return;
    }
    if (jsonDoc.isObject()) {
        QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.contains("aircraft_list") && jsonObj["aircraft_list"].isArray()) {
            Folder* fold = platform->addFolder("aircraft");
            QJsonArray aircraft_list = jsonObj["aircraft_list"].toArray();
            for (const QJsonValue &value : aircraft_list) {
                // 1. Value ko Object mein convert karein
                QJsonObject entity = value.toObject();

                // 2. Individual data nikalein
                QString name = entity["name"].toString();
                QString category = entity["category"].toString();
                fold->addEntity(name.toStdString());
            }
        }

        if (jsonObj.contains("ground_entities_list") && jsonObj["ground_entities_list"].isArray()) {
            Folder* fold = platform->addFolder("ground");
            QJsonArray ground_entities_list = jsonObj["ground_entities_list"].toArray();
            for (const QJsonValue &value : ground_entities_list) {
                // 1. Value ko Object mein convert karein
                QJsonObject entity = value.toObject();

                // 2. Individual data nikalein
                QString name = entity["name"].toString();
                QString category = entity["category"].toString();
                fold->addEntity(name.toStdString());
            }
        }
    } else {

    }
}
void HierarchyConnector::setupFileOperations(QMainWindow* parent, Hierarchy* hierarchy, TacticalDisplay* tacticalDisplay)
{
    MenuBar* menuBar = qobject_cast<MenuBar*>(parent->menuBar());
    if (!menuBar) {
        return;
    }

    RecentProjectsManager::EditorType editorType;
    QString fileExtension;
    QString fileTypeDescription;
    QString subfolderName;

    if (qobject_cast<DatabaseEditor*>(parent)) {
        editorType = RecentProjectsManager::DatabaseEditor;
        fileExtension = "db";
        fileTypeDescription = "Database Files (*.db)";
        subfolderName = "Database";
    } else if (qobject_cast<ScenarioEditor*>(parent)) {
        editorType = RecentProjectsManager::ScenarioEditor;
        fileExtension = "sc";
        fileTypeDescription = "Scenario Files (*.sc)";
        subfolderName = "Scenario";
    } else if (qobject_cast<RuntimeEditor*>(parent)) {
        editorType = RecentProjectsManager::RuntimeEditor;
        fileExtension = "rn";
        fileTypeDescription = "Runtime Files (*.rn)";
        subfolderName = "Runtime";
    } else {
        return;
    }

    // Helper function to ensure TDF folder structure exists
    auto ensureTDFFolder = [subfolderName]() -> QString {
        QString homeDir = QDir::homePath();
        QString tdfPath = homeDir + "/TDF";
        QString targetPath = tdfPath + "/" + subfolderName;

        QDir dir;
        // Create TDF folder if it doesn't exist
        if (!dir.exists(tdfPath)) {
            dir.mkpath(tdfPath);
        }

        // Create subfolder (Database, Scenario, or Runtime) if it doesn't exist
        if (!dir.exists(targetPath)) {
            dir.mkpath(targetPath);
        }

        return targetPath;
    };

    // Helper function to create scenario instance copy
    auto createScenarioInstanceCopy = [](const QString& runtimeFilePath, const QJsonObject& data) -> bool {
        // Check if this is a .rn file
        QFileInfo runtimeFileInfo(runtimeFilePath);
        if (runtimeFileInfo.suffix().toLower() != "rn") {
            return false;
        }

        // Create paths for scenario instance
        QString homeDir = QDir::homePath();
        QString tdfPath = homeDir + "/TDF";
        QString scenarioPath = tdfPath + "/Scenario";
        QString instanceFolderPath = scenarioPath + "/Scerioinstance";

        // Create scenario instance folder if it doesn't exist
        QDir dir;
        if (!dir.exists(scenarioPath)) {
            dir.mkpath(scenarioPath);
        }
        if (!dir.exists(instanceFolderPath)) {
            dir.mkpath(instanceFolderPath);
        }

        // Create scenario instance file name
        QString runtimeFileName = runtimeFileInfo.completeBaseName();
        QString scenarioFileName;

        // Check if filename starts with "Scenario_" or "Runtime_"
        if (runtimeFileName.startsWith("Scenario_")) {
            // Replace "Scenario_" with "Instance_Scenario_"
            scenarioFileName = "Instance_" + runtimeFileName + ".sc";
        } else if (runtimeFileName.startsWith("Runtime_")) {
            // Replace "Runtime_" with "Instance_Scenario_"
            scenarioFileName = "Instance_" + runtimeFileName.replace("Runtime_", "Scenario_") + ".sc";
        } else {
            // Add "Instance_Scenario_" prefix
            scenarioFileName = "Instance_Scenario_" + runtimeFileName + ".sc";
        }

        QString scenarioFilePath = instanceFolderPath + "/" + scenarioFileName;

        // Save the copy with .sc extension
        QFile scenarioFile(scenarioFilePath);
        if (scenarioFile.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(data);
            scenarioFile.write(doc.toJson(QJsonDocument::Indented));
            scenarioFile.close();
            // qDebug() << "Scenario instance copy saved to:" << scenarioFilePath;
            return true;
        }

        return false;
    };

    QAction* loadAction = menuBar->getLoadAction();
    QAction* loadToLibraryAction = menuBar->getLoadToLibraryAction();
    QAction* openRuntimeInstanceAction = menuBar->getOpenRuntimeInstanceAction();
    QAction* saveAction = menuBar->getSaveAction();
    QAction* sameSaveAction = menuBar->getSameSaveAction();


    connect(openRuntimeInstanceAction, &QAction::triggered, this, [=]() {
        QString homeDir = QDir::homePath();
        QString tdfPath = homeDir + "/TDF";
        QString scenarioPath = tdfPath + "/Scenario";
        QString instanceFolderPath = scenarioPath + "/Scerioinstance";

        // Check if Scerioinstance folder exists
        QDir instanceDir(instanceFolderPath);
        if (!instanceDir.exists()) {
            QMessageBox::information(parent, "Folder Not Found",
                                     "Scerioinstance folder not found at:\n" + instanceFolderPath +
                                         "\n\nPlease save a runtime file first to create the folder.");
            return;
        }

        // Get list of .sc files in the instance folder
        QStringList filters;
        filters << "*.sc";
        instanceDir.setNameFilters(filters);
        QStringList scFiles = instanceDir.entryList(QDir::Files);

        if (scFiles.isEmpty()) {
            QMessageBox::information(parent, "No Files Found",
                                     "No .sc files found in Scerioinstance folder.");
            return;
        }

        // Show file selection dialog
        QString filePath = QFileDialog::getOpenFileName(
            parent,
            "Open Runtime Instance File",
            instanceFolderPath,
            "Scenario Instance Files (*.sc);;All Files (*.*)"
            );

        if (filePath.isEmpty()) {
            return;
        }

        // ✅ Load the selected file in appropriate editor
        if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(parent)) {
            rtEditor->loadFromJsonFile(filePath);
            RecentProjectsManager::instance()->addToRecentProjects(filePath,
                                                                   RecentProjectsManager::RuntimeEditor);

        }
        else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(parent)) {
            scEditor->loadFromJsonFile(filePath);
            RecentProjectsManager::instance()->addToRecentProjects(filePath,
                                                                   RecentProjectsManager::ScenarioEditor);
        }
    });

    // Load action - Each editor should only show relevant files
    connect(loadAction, &QAction::triggered, this, [=]() {
        QString filter;
        QString dialogTitle;
        QString startPath = ensureTDFFolder();

        // Set appropriate filters for each editor type
        if (editorType == RecentProjectsManager::RuntimeEditor) {
            dialogTitle = "Open Runtime/Scenario File";

            filter = "Runtime & Scenario Files (*.rn *.sc);;Runtime Files (*.rn);;Scenario Files (*.sc);;All Files (*)";
        } else if (editorType == RecentProjectsManager::ScenarioEditor) {
            dialogTitle = "Open Scenario File";
            filter = "Scenario Files (*.sc);;JSON Files (*.json);;All Files (*)";
        } else if (editorType == RecentProjectsManager::DatabaseEditor) {
            dialogTitle = "Open Database File";
            filter = "Database Files (*.db);;JSON Files (*.json);;All Files (*)";
        }

        QString filePath = QFileDialog::getOpenFileName(parent, dialogTitle,
                                                        startPath,
                                                        filter);
        if (!filePath.isEmpty()) {
            if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(parent)) {
                dbEditor->loadFromJsonFile(filePath);
                RecentProjectsManager::instance()->addToRecentProjects(filePath,
                                                                       RecentProjectsManager::DatabaseEditor);
            } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(parent)) {
                scEditor->loadFromJsonFile(filePath);
                RecentProjectsManager::instance()->addToRecentProjects(filePath,
                                                                       RecentProjectsManager::ScenarioEditor);
            } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(parent)) {
                rtEditor->loadFromJsonFile(filePath);
                RecentProjectsManager::instance()->addToRecentProjects(filePath,
                                                                       RecentProjectsManager::RuntimeEditor);
            }
        }
    });

    // Load to Library action - This should show database files only
    connect(loadToLibraryAction, &QAction::triggered, this, [=]() {
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
            "Load Database File to Library",
            databasePath,
            "Database Files (*.db);;JSON Files (*.json);;All Files (*.*)"
            );

        if (filePath.isEmpty()) {
            return;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            return;
        }

        QJsonObject obj = doc.object();
        if (obj.contains("hierarchy")) {
            QJsonObject hier = obj["hierarchy"].toObject();

            // Load to appropriate library based on editor type
            if (ScenarioEditor* se = qobject_cast<ScenarioEditor*>(parent)) {
                se->library->clear();
                se->library->fromJson(hier);
                if (se->libTreeView) {
                    se->libTreeView->setLibraryFileName(filePath);
                }
            } else if (RuntimeEditor* re = qobject_cast<RuntimeEditor*>(parent)) {
                re->library->clear();
                re->library->fromJson(hier);
                if (re->libTreeView) {
                    re->libTreeView->setLibraryFileName(filePath);
                }
            }

            RecentProjectsManager::instance()->addToRecentProjects(filePath,
                                                                   RecentProjectsManager::LibraryData);
        }
    });

    // Save As action
    connect(saveAction, &QAction::triggered, this, [=]() {
        QString filter;
        QString dialogTitle;
        QString defaultFileName;
        QString startPath = ensureTDFFolder();
        QString lastFilePath = getLastSavedFilePath(parent);
        QString currentDate = QDate::currentDate().toString("yyyy-MM-dd");
        auto extractBaseNameWithoutDate = [](const QString& baseName) -> QString {
            QRegularExpression datePattern("_\\d{4}-\\d{2}-\\d{2}$|_\\d{2}-\\d{2}-\\d{4}$");
            QString cleanName = baseName;
            cleanName.remove(datePattern);
            return cleanName.isEmpty() ? baseName : cleanName;
        };

        if (editorType == RecentProjectsManager::RuntimeEditor) {
            dialogTitle = "Save Runtime File";
            filter = "Runtime Files (*.rn);;All Files (*.*)";
            if (!lastFilePath.isEmpty()) {
                QFileInfo fileInfo(lastFilePath);
                QString loadedExtension = fileInfo.suffix().toLower();
                QString baseName = fileInfo.completeBaseName();
                if (loadedExtension != "rn" && loadedExtension != "") {
                    QString cleanName = extractBaseNameWithoutDate(baseName);
                    defaultFileName = cleanName + "_" + currentDate + ".rn";
                } else {
                    defaultFileName = baseName + ".rn";
                }
            } else {
                defaultFileName = "Scenario_" + currentDate + ".rn";
            }
        } else if (editorType == RecentProjectsManager::DatabaseEditor) {
            dialogTitle = "Save Database File";
            filter = "Database Files (*.db);;JSON Files (*.json);;All Files (*.*)";
            if (!lastFilePath.isEmpty()) {
                QFileInfo fileInfo(lastFilePath);
                QString loadedExtension = fileInfo.suffix().toLower();
                QString baseName = fileInfo.completeBaseName();
                if (loadedExtension != "db" && loadedExtension != "") {
                    QString cleanName = extractBaseNameWithoutDate(baseName);
                    defaultFileName = cleanName + "_" + currentDate + ".db";
                } else {
                    defaultFileName = baseName + ".db";
                }
            } else {
                defaultFileName = "Database_" + currentDate + ".db";
            }
        } else if (editorType == RecentProjectsManager::ScenarioEditor) {
            dialogTitle = "Save Scenario File";
            filter = "Scenario Files (*.sc);;JSON Files (*.json);;All Files (*.*)";
            if (!lastFilePath.isEmpty()) {
                QFileInfo fileInfo(lastFilePath);
                QString loadedExtension = fileInfo.suffix().toLower();
                QString baseName = fileInfo.completeBaseName();
                if (loadedExtension != "sc" && loadedExtension != "") {
                    QString cleanName = extractBaseNameWithoutDate(baseName);
                    defaultFileName = cleanName + "_" + currentDate + ".sc";
                } else {
                    defaultFileName = baseName + ".sc";
                }
            } else {
                defaultFileName = "Scenario_" + currentDate + ".sc";
            }
        }

        QString filePath = QFileDialog::getSaveFileName(parent, dialogTitle,
                                                        startPath + "/" + defaultFileName,
                                                        filter);
        if (!filePath.isEmpty()) {
            // Get current file extension
            QFileInfo fileInfo(filePath);
            QString currentExtension = fileInfo.suffix().toLower();

            // Auto-add extension if missing
            if (currentExtension.isEmpty()) {
                if (editorType == RecentProjectsManager::RuntimeEditor) {
                    filePath += ".rn";
                } else if (editorType == RecentProjectsManager::DatabaseEditor) {
                    filePath += ".db";
                } else if (editorType == RecentProjectsManager::ScenarioEditor) {
                    filePath += ".sc";
                }
            }

            // Prepare JSON data for saving
            QJsonObject obj;
            obj["hierarchy"] = hierarchy->toJson();
            if (tacticalDisplay != nullptr) {
                obj["tactical"] = tacticalDisplay->canvas->toJson();
            } else {
                obj["tactical"] = QJsonObject();
            }

            // Save the primary file
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                QJsonDocument doc(obj);
                file.write(doc.toJson(QJsonDocument::Indented));
                file.close();
                RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);

                // For Runtime Editor files (.rn), create scenario instance copy
                if (editorType == RecentProjectsManager::RuntimeEditor) {
                    bool copyCreated = createScenarioInstanceCopy(filePath, obj);
                    if (copyCreated) {
                        qDebug() << "Scenario instance copy created successfully";
                    } else {
                        qWarning() << "Failed to create scenario instance copy";
                    }
                }

                // Set last saved path in appropriate editor
                if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(parent)) {
                    dbEditor->lastSavedFilePath = filePath;
                    dbEditor->clearUnsavedChanges();
                } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(parent)) {
                    scEditor->lastSavedFilePath = filePath;
                    scEditor->clearUnsavedChanges();
                } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(parent)) {
                    rtEditor->lastSavedFilePath = filePath;
                    rtEditor->clearUnsavedChanges();
                }
            }
        }
    });

    // Save (same file) action
    connect(sameSaveAction, &QAction::triggered, this, [=]() {
        QString filePath = getLastSavedFilePath(parent);

        // If no previous save, trigger Save As dialog
        if (filePath.isEmpty()) {
            emit saveAction->triggered();
            return;
        }

        // Check file info
        QFileInfo fileInfo(filePath);
        QString currentExtension = fileInfo.suffix().toLower();
        QString tdfPath = QDir::homePath() + "/TDF";

        // Check if file is in correct folder and has correct extension
        QString expectedFolder = tdfPath;
        QString expectedExtension;

        if (editorType == RecentProjectsManager::RuntimeEditor) {
            expectedFolder += "/Runtime";
            expectedExtension = "rn";
        } else if (editorType == RecentProjectsManager::DatabaseEditor) {
            expectedFolder += "/Database";
            expectedExtension = "db";
        } else if (editorType == RecentProjectsManager::ScenarioEditor) {
            expectedFolder += "/Scenario";
            expectedExtension = "sc";
        }

        bool extensionMismatch = (currentExtension != expectedExtension);
        if (!filePath.startsWith(expectedFolder) || extensionMismatch) {
            emit saveAction->triggered();
            return;
        }

        // Prepare JSON data for saving
        QJsonObject obj;
        obj["hierarchy"] = hierarchy->toJson();
        if (tacticalDisplay != nullptr) {
            obj["tactical"] = tacticalDisplay->canvas->toJson();
        }

        // Save the primary file
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::warning(parent, "Error", "Failed to save file");
            return;
        }

        QJsonDocument doc(obj);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);

        // For Runtime Editor files (.rn), create scenario instance copy
        if (editorType == RecentProjectsManager::RuntimeEditor) {
            bool copyCreated = createScenarioInstanceCopy(filePath, obj);
            if (copyCreated) {
                qDebug() << "Scenario instance copy created successfully";
            } else {
                qWarning() << "Failed to create scenario instance copy";
            }
        }

        if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(parent)) {
            dbEditor->lastSavedFilePath = filePath;
        } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(parent)) {
            scEditor->lastSavedFilePath = filePath;
        } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(parent)) {
            rtEditor->lastSavedFilePath = filePath;
        }
    });
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

    if (ScenarioEditor* se = qobject_cast<ScenarioEditor*>(parent)) {
        targetLibrary = se->library;
        targetLibTreeView = se->libTreeView;
    } else if (RuntimeEditor* re = qobject_cast<RuntimeEditor*>(parent)) {
        targetLibrary = re->library;
        targetLibTreeView = re->libTreeView;
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

    // ✅ FIXED: Don't call clear() - it causes crash
    // Just load the JSON directly - it will replace existing data
    try {
        targetLibrary->fromJson(hier);

        // Update UI
        if (targetLibTreeView) {
            targetLibTreeView->setLibraryFileName(filePath);
            // Refresh the tree view
            targetLibTreeView->getTreeWidget()->clearSelection();
            targetLibTreeView->getTreeWidget()->update();
        }

        // Add to recent projects
        RecentProjectsManager::instance()->addToRecentProjects(filePath,
                                                               RecentProjectsManager::LibraryData);

        qDebug() << "[HierarchyConnector] Library loaded successfully from:" << filePath;

    } catch (const std::exception& e) {
        QMessageBox::critical(parent, "Error",
                              QString("Failed to load library: %1").arg(e.what()));
    }
}
// %%% Recent Projects Management %%%
/* Add file to recent projects list */
void HierarchyConnector::addToRecentProjects(const QString& filePath)
{
    if (filePath.isEmpty()) return;

    QSettings settings;
    QStringList recentProjects = settings.value("recentProjects").toStringList();

    // Remove if already exists
    recentProjects.removeAll(filePath);

    // Add to beginning
    recentProjects.prepend(filePath);

    // Keep only last 10 projects
    while (recentProjects.size() > 10) {
        recentProjects.removeLast();
    }

    settings.setValue("recentProjects", recentProjects);
}

/* Get recent projects list */
QStringList HierarchyConnector::getRecentProjects() const
{
    QSettings settings;
    return settings.value("recentProjects").toStringList();
}

/* Clear recent projects list */
void HierarchyConnector::clearRecentProjects()
{
    QSettings settings;
    settings.remove("recentProjects");
}

// %%% New File Creation %%%
/* Create new file with default data */
void HierarchyConnector::newfile(Hierarchy *hierarchy)
{
    hierarchy->fromJson(QJsonObject());
    initializeDummyData(hierarchy);
}

void HierarchyConnector::openXmlFile(Hierarchy* hierarchy,QString fullPath)
{

    if (hierarchy) {
        QDomElement root = getDom(fullPath);
        QFileInfo fileInfo(fullPath);

        QString folderPath = fileInfo.absolutePath();

        QDomNodeList profiles = root.elementsByTagName("Profile_Instance");
        ProfileCategaory *profile = nullptr;
        for (const auto& [key, profilePtr] : hierarchy->ProfileCategories) {
            QString name = QString::fromStdString(profilePtr->Name).toLower();
            if(fullPath.contains(name)){
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
                QDomNodeList comp = platform.childNodes();
                for (int j = 0; j < comp.count(); ++j) {

                    QDomElement fieldElement = comp.at(j).toElement();
                    if (!fieldElement.isNull()) {
                        QDomElement compName = fieldElement.elementsByTagName("Name").at(0).toElement();

                        if(compName.text().contains("dynamic") && plf->dynamicModel){

                            QDomNodeList valueName = compName.parentNode().toElement().elementsByTagName("Name");
                            for (int k = 0; k < valueName.count(); ++k) {
                                QDomElement Name = valueName.at(k).toElement();
                                if(Name.text().contains("altitude")){
                                    QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                    plf->dynamicModel->Altitude = value.toFloat();
                                }else
                                    if(Name.text().contains("turn_rate")){
                                        QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                        plf->dynamicModel->turnRate = value.toFloat();
                                    }else
                                        if(Name.text().contains("acceleration")){
                                            QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                            plf->dynamicModel->Acceleration = value.toFloat();
                                        }else
                                            if(Name.text().contains("deceleration")){
                                                QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                                plf->dynamicModel->Decceleration = value.toFloat();
                                            }else
                                                if(Name.text().contains("climb_rate")){
                                                    QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                                    plf->dynamicModel->climbRate = value.toFloat();
                                                }else
                                                    if(Name.text().contains("dive_rate")){
                                                        QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                                        plf->dynamicModel->diveRate = value.toFloat();
                                                    }
                            }
                        }

                        if(compName.text().contains("physical") && plf->collider){
                            QDomNodeList valueName = compName.parentNode().toElement().elementsByTagName("Name");
                            for (int k = 0; k < valueName.count(); ++k) {
                                QDomElement Name = valueName.at(k).toElement();
                                if(Name.text().contains("width")){
                                    QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                    plf->collider->Width = value.toFloat();
                                }else
                                    if(Name.text().contains("length")){
                                        QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                        plf->collider->Length = value.toFloat();
                                    }else
                                        if(Name.text().contains("height")){
                                            QString value = Name.parentNode().toElement().elementsByTagName("Value").at(0).toElement().text();
                                            plf->collider->Height = value.toFloat();
                                        }

                            }
                        }
                        if(compName.text().contains("sensors") && plf->sensors){
                            QDomNodeList valueName = fieldElement.elementsByTagName("Name");
                            QDomNode n = fieldElement.firstChild();
                            while(!n.isNull()) {
                                QDomElement e = n.toElement();
                                if(!e.isNull() && e.tagName() == "DBList") {
                                    QString nm = e.firstChild().toElement().attribute("Name");
                                    if(nm.isEmpty())break;
                                    hierarchy->addSubComponent(QString::fromStdString(plf->sensors->ID), ComponentType::SensorProfile,
                                                               nm, "Generic");
                                }
                                n = n.nextSibling();
                            }
                        }
                        if(compName.text().contains("radios") && plf->radios){
                            QDomNodeList valueName = fieldElement.elementsByTagName("Name");
                            QDomNode n = fieldElement.firstChild();
                            while(!n.isNull()) {
                                QDomElement e = n.toElement();
                                if(!e.isNull() && e.tagName() == "DBList") {
                                    QString nm = e.firstChild().toElement().attribute("Name");
                                    if(nm.isEmpty())break;
                                    hierarchy->addSubComponent(QString::fromStdString(plf->radios->ID), ComponentType::RadioProfile,
                                                               nm);
                                }
                                n = n.nextSibling();
                            }
                        }

                        if(compName.text().contains("iffs") && plf->iffs){
                            QDomNodeList valueName = fieldElement.elementsByTagName("Name");
                            QDomNode n = fieldElement.firstChild();
                            while(!n.isNull()) {
                                QDomElement e = n.toElement();
                                if(!e.isNull() && e.tagName() == "DBList") {
                                    QString nm = e.firstChild().toElement().attribute("Name");
                                    if(nm.isEmpty())break;
                                    hierarchy->addSubComponent(QString::fromStdString(plf->iffs->ID), ComponentType::IFFProfile,
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
