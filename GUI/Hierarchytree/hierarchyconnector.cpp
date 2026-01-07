/* ========================================================================= */
/* File: hierarchyconnector.cpp                                             */
/* Purpose: Manages connections between hierarchy, UI, and file operations   */
/* ========================================================================= */

#include "hierarchyconnector.h"                     // For hierarchy connector class
#include <QToolBar>                                // For toolbar handling
#include <QAction>                                 // For action handling
#include <QFileDialog>                             // For file dialog
#include <QJsonDocument>                           // For JSON document handling
#include <QJsonParseError>                         // For JSON parse errors
#include <QMessageBox>                             // For message box
#include <QDebug>                                  // For debug output
#include <QStandardPaths>                          // For standard paths
#include "GUI/Hierarchytree/contextmenu.h"         // For context menu
#include "GUI/Menubars/menubar.h"                  // For menu bar
#include <QUuid>                                   // For UUID generation
#include "GUI/Editors/databaseeditor.h"            // For database editor
#include "GUI/Editors/scenarioeditor.h"            // For scenario editor
#include "GUI/Editors/runtimeeditor.h"             // For runtime editor
#include "GUI/Hierarchytree/contextmenu.h"
#include "qapplication.h"
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include <QSettings>
#include <core/Utility/uuid.h>
#include <QRandomGenerator>
#include <core/Hierarchy/Utils/entityutils.h>





// %%% Static Instance %%%
/* Singleton instance */
HierarchyConnector* HierarchyConnector::m_instance = nullptr;

/* Constructor */
HierarchyConnector::HierarchyConnector(QObject* parent)
    : QObject(parent), hierarchy(nullptr), library(nullptr), libTreeView(nullptr)
{
    // Initialize pointers to null
}

/* Get singleton instance */
HierarchyConnector* HierarchyConnector::instance()
{
    // Create instance if null
    if (!m_instance) {
        m_instance = new HierarchyConnector();
    }
    return m_instance;
}

/* Get last saved file path */
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

/* Connect signals for hierarchy and UI components */
void HierarchyConnector::connectSignals(Hierarchy* hierarchy, HierarchyTree* treeView,
                                        TacticalDisplay* tactical, Inspector* inspector)
{
    // Validate inputs
    if (!hierarchy || !treeView) {
        qWarning() << "Cannot connect signals - null hierarchy or treeView";
        return;
    }
    treeView->getContextMenu()->m_hierarchy = hierarchy;
    // Connect hierarchy to tree view
    connect(hierarchy, &Hierarchy::profileAdded, treeView, &HierarchyTree::profileAdded);
    connect(hierarchy, &Hierarchy::folderAdded, treeView, &HierarchyTree::folderAdded);
    connect(hierarchy, &Hierarchy::entityAdded, treeView, &HierarchyTree::entityAdded);
    connect(hierarchy, &Hierarchy::profileRemoved, treeView, &HierarchyTree::profileRemoved);
    connect(hierarchy, &Hierarchy::folderRemoved, treeView, &HierarchyTree::folderRemoved);
    connect(hierarchy, &Hierarchy::entityRemoved, treeView, &HierarchyTree::entityRemoved);
    connect(hierarchy, &Hierarchy::componentAdded, treeView, &HierarchyTree::componentAdded);
    connect(hierarchy, &Hierarchy::componentRemoved, treeView, &HierarchyTree::componentRemoved);
    connect(hierarchy, &Hierarchy::subComponentAdded, treeView, &HierarchyTree::subComponentAdded);
    connect(hierarchy, &Hierarchy::subComponentRemoved, treeView, &HierarchyTree::subComponentRemoved);

    // Connect tree view context menu to hierarchy
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
    connect(treeView->getContextMenu(), &ContextMenu::addEntityRequested,
            this, [=](QString parentId, QString entityName, bool isProfileParent, QVariantMap components,AddItemDialog* dialog) {

                // ✅ DEBUG: Dialog check
                if (!dialog) {
                    qDebug() << "❌ Dialog is NULL!";
                    return;
                }
                QString sensorType = dialog->getSensorType();
                qDebug() << "Dialog Sensor Type:" << sensorType;

                qDebug() << "Creating entity:" << entityName;
                Entity* newEntity = hierarchy->addEntity(parentId, entityName, isProfileParent);
                if (sensorType != "Generic" && newEntity) {
                    Sensor* sensorEntity = dynamic_cast<Sensor*>(newEntity);
                    if (sensorEntity) {
                        Sensor::SubType subTypeEnum = Sensor::getSubTypeFromString(sensorType);
                        sensorEntity->subType = subTypeEnum;
                        qDebug() << "✅ Sensor Type set to:" << sensorType;
                    }
                }
                QSet<QString> addedComponents;
                for (const auto& component : components.keys()) {
                    if (components.value(component).toBool() && !addedComponents.contains(component)) {
                        try {
                            newEntity->addComponent(component.toStdString());
                            addedComponents.insert(component);
                            qDebug() << "Added user-specified component:" << component
                                     << "to entity:" << QString::fromStdString(newEntity->ID);
                        } catch (const std::exception& e) {
                            qWarning() << "Failed to add component" << component
                                       << "to entity:" << e.what();
                        }
                    }
                }
                addedComponents.insert("transform");
                addedComponents.insert("rigidbody");
                Platform* platform = dynamic_cast<Platform*>(newEntity);
                if(platform && dialog->isScEnabled()){
                    const float range = dialog->getRange()/2;
                    double lat = 28.6139+((QRandomGenerator::global()->generateDouble()-0.5)*2*(range/111.111));
                    double lon = 77.2090 +((QRandomGenerator::global()->generateDouble()-0.5)*2*(range/100));
                    double heading = 360 * QRandomGenerator::global()->generateDouble();
                    //Delhi 28.6139∘ N 77.2090∘
                    platform->transform->setGeoCord(lat,lon);
                    // platform->transform->setTranslation(QVector3D(lat,0,lon));
                    platform->transform->setFromEulerAngles(QVector3D(0,heading,0));

                    for(int i=0;i<10;i++){
                        auto [latAtRadius, lonAtRadius] = calculateNewLatLong(lat,lon,heading,5);
                        Waypoints* newWaypoint = new Waypoints();
                        newWaypoint->position = new Vector(latAtRadius, 0, lonAtRadius);
                        platform->trajectory->addTrajectory(newWaypoint);
                        if(dialog && dialog->getTrajectory() == "Zigzag"){
                            lat = latAtRadius+(QRandomGenerator::global()->generateDouble()-0.5)*0.1;
                            lon = lonAtRadius+(QRandomGenerator::global()->generateDouble()-0.5)*0.1;
                        }else{
                            lat = latAtRadius;
                            lon = lonAtRadius;
                        }
                    }


                    std::string id = platform->sensors->ID;
                    //Sensor
                    hierarchy->addSubComponent(QString::fromStdString(id),ComponentType::SensorProfile,"Radar_"+entityName,"Generic");
                    hierarchy->addSubComponent(QString::fromStdString(id),ComponentType::SensorProfile,"ESM_"+entityName,"ESM");
                    hierarchy->addSubComponent(QString::fromStdString(id),ComponentType::SensorProfile,"CSM_"+entityName,"CSM");
                    //iff
                    id = platform->iffs->ID;
                    hierarchy->addSubComponent(QString::fromStdString(id),ComponentType::IFFProfile,"IFF_"+entityName);
                    //Radio
                    id = platform->radios->ID;
                    hierarchy->addSubComponent(QString::fromStdString(id),ComponentType::RadioProfile,"Radio"+entityName);

                    if(dialog){
                        //set Speed
                        const int minspd = dialog->getMinPlaneSpeed();
                        const int maxspd = dialog->getMinPlaneSpeed();
                        const int minturn = dialog->getMinPlaneSpeed();
                        const int maxturn = dialog->getMaxTurnRadius();
                        const int speed = minspd +(QRandomGenerator::global()->generateDouble() * (maxspd-minspd));
                        const int turnRad = minturn +(QRandomGenerator::global()->generateDouble() * (maxturn-minturn));

                        platform->dynamicModel->minSpeed = minspd;
                        platform->dynamicModel->maxSpeed = maxspd;
                        platform->dynamicModel->moveSpeed = speed;
                        platform->dynamicModel->turnRadius = turnRad;

                        //set RadarRange
                        const int minRadRng= dialog->getMinRadarRange();
                        const int maxRadRng = dialog->getMaxRadarRange();
                        for (auto const& pair :*platform->sensors->sensors) {
                            Sensor* s = pair.second;
                            const int RadarRng = minRadRng +(QRandomGenerator::global()->generateDouble() * (maxRadRng-minRadRng));
                            s->range = RadarRng;
                        }

                        //set RadioRange
                        const int minRadioRng= dialog->getMinRadioRange();
                        const int maxRadioRng = dialog->getMaxRadioRange();
                        for (auto const& pair :*platform->radios->radios) {
                            Radio* r = pair.second;
                            const int RadioRng = minRadioRng +(QRandomGenerator::global()->generateDouble()* (maxRadioRng-minRadioRng));
                            r->Range = RadioRng;
                        }
                    }

                    QCoreApplication::processEvents();
                }
            });

    // Connect remove entity and component actions
    connect(treeView->getContextMenu(), &ContextMenu::removeEntityRequested,
            hierarchy, &Hierarchy::removeEntity);
    connect(treeView->getContextMenu(), &ContextMenu::removeComponentRequested,
            hierarchy, &Hierarchy::removeComponent);
    connect(treeView->getContextMenu(), &ContextMenu::addComponentRequested, this,
            [=](QString entityID, QString componentType, QString componentName,
                QString sensorType, QString sensorProfileId = "") {  // ✅ NEW parameter
                if (componentType == "iffs") {
                    hierarchy->addSubComponent(entityID, ComponentType::IFFProfile, componentName, sensorType, sensorProfileId);
                    // hierarchy->attchedIff(entityID, componentName);
                } else if (componentType == "sensors") {
                    hierarchy->addSubComponent(entityID, ComponentType::SensorProfile, componentName, sensorType, sensorProfileId);


                    if (!sensorProfileId.isEmpty()) {
                        // Here you can copy settings from the selected sensor profile
                        qDebug() << "Using sensor profile ID:" << sensorProfileId;
                        // You can implement copying of sensor parameters from profile
                    }

                    hierarchy->attachSensors(entityID, componentName, sensorType);
                }
                else if (componentType == "radios") {
                    hierarchy->addSubComponent(entityID, ComponentType::RadioProfile, componentName, sensorType, sensorProfileId);
                    // hierarchy->attachRadios(entityID, componentName);
                } else {
                    qWarning() << "Unsupported component type for addComponentRequested:" << componentType;
                }
            });

    // Connect tactical display if provided
    if (tactical) {
        connect(hierarchy, &Hierarchy::entityRemoved, tactical, &TacticalDisplay::removeMesh);
    }

    // Connect inspector if provided
    if (inspector) {
        connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
    }

    // Connect rename signals
    connect(hierarchy, &Hierarchy::profileRenamed, treeView, &HierarchyTree::profileRenamed);
    connect(hierarchy, &Hierarchy::folderRenamed, treeView, &HierarchyTree::folderRenamed);
    connect(hierarchy, &Hierarchy::entityRenamed, treeView, &HierarchyTree::entityRenamed);

    // Connect context menu rename action
    connect(treeView->getContextMenu(), &ContextMenu::renameItemRequested,
            hierarchy, [=](QVariantMap data) {
                QString type = data["type"].toString();
                QString id = data["ID"].toString();
                QString name = data["name"].toString();
                if (type == "profile") {
                    hierarchy->renameProfileCategaory(id, name);
                } else if (type == "folder") {
                    hierarchy->renameFolder(id, name);
                } else if (type == "entity") {
                    hierarchy->renameEntity(id, name);
                }
            });

    // Connect copy-paste signals
    connect(treeView->getContextMenu(), &ContextMenu::copyItemRequested,
            treeView, &HierarchyTree::copyItemRequested);
    connect(treeView->getContextMenu(), &ContextMenu::pasteItemRequested,
            treeView, &HierarchyTree::pasteItemRequested);

    // Handle copy action
    connect(treeView, &HierarchyTree::copyItemRequested, this,
            [this, hierarchy](QVariantMap data) {
                QString type = data["type"].toString();
                if (type != "entity") {
                    return;
                }
                qDebug()<<data["ID"];
                copydata = data;
                copySource = hierarchy;
            });

    // Handle paste action
    connect(treeView, &HierarchyTree::pasteItemRequested, this, [this, hierarchy](QVariantMap targetData) {
        if (copydata.isEmpty()) {
            qWarning() << "Cannot paste - nothing copied!";
            return;
        }
        if (!copySource) {
            qWarning() << "Cannot paste - no source hierarchy set!";
            return;
        }
        QString type = copydata["type"].toString();
        QString id = copydata["ID"].toString();
        QString targetType;
        if (targetData["type"].type() == QVariant::Map) {
            QVariantMap typeData = targetData["type"].toMap();
            if (typeData.contains("type") && typeData["type"].toString() == "option") {
                targetType = "profile";
            } else {
                qWarning() << "Invalid nested type structure in pasteItemRequested:" << targetData["type"];
                return;
            }
        } else {
            targetType = targetData["type"].toString();
        }
        QString targetId = targetData["ID"].toString();
        qDebug()<<targetId;
        if (type != "entity") {
            qWarning() << "Can only paste entities! Type was:" << type;
            return;
        }
        try {
            auto entityIt = copySource->Entities->find(id.toStdString());
            if (entityIt == copySource->Entities->end()) {
                qCritical() << "Entity not found in Entities map for ID:" << id;
                return;
            }
            QJsonObject entityJson = entityIt->second->toJson();
            QString newId = QString::fromStdString( Uuid::generateShortUniqueID());
            entityJson["id"] = newId;
            Entity* newEntity = hierarchy->addEntityFromJson(targetId, entityJson, targetType == "profile");
            copydata.clear();
            copySource = nullptr;
        } catch (const std::exception& e) {
            qCritical() << "Paste failed:" << e.what();
        }
    });

    // Handle drag-and-drop
    connect(treeView, &HierarchyTree::itemDropped, this, [=](QVariantMap sourceData, QVariantMap targetData) {
        if (sourceData["type"].toString() == "entity") {
            QString sourceId = sourceData["ID"].toString();
            QString targetId = targetData["ID"].toString();
            bool isProfile = (targetData["type"].toString() == "profile")||targetData["type"].type() == QVariant::Map;

            try {
                if (!copySource) {
                    qWarning() << "No copy source available for drop!";
                    return;
                }
                auto entityIt = copySource->Entities->find(sourceId.toStdString());
                if (entityIt == copySource->Entities->end()) {
                    qCritical() << "Entity not found in Entities map for ID:" << sourceId;
                    return;
                }
                QJsonObject entityJson = entityIt->second->toJson();
                Constants::EntityType sourcetype = entityIt->second->type;
                Constants::EntityType type;
                ////////////////////////////////////////
                bool iscomp = targetData["type"].toString() == "component";
                QString trname = targetData["name"].toString();
                if(!iscomp){
                    if(isProfile){
                        type = copySource->ProfileCategories[targetId.toStdString()]->type;
                    }else{
                        type = (*copySource->Folders)[targetId.toStdString()]->type;
                    }
                }

                if((trname == "sensors" || type == Constants::EntityType::Platform) && sourcetype == Constants::EntityType::Sensor){
                    hierarchy->addSubComponent(targetId,ComponentType::SensorProfile,"Radar_","Generic",sourceId);
                }else
                if((trname == "iffs" || type == Constants::EntityType::Platform) && sourcetype == Constants::EntityType::IFF){
                    hierarchy->addSubComponent(targetId,ComponentType::IFFProfile,"IFF_","",sourceId);
                }else
                if((trname == "radios" || type == Constants::EntityType::Platform) && sourcetype == Constants::EntityType::Radio){
                    hierarchy->addSubComponent(targetId,ComponentType::RadioProfile,"Radio_","",sourceId);
                }
                if(iscomp)return;
                ///////////////////////////////////////

                if(sourcetype != type)return;
                Hierarchy* parent = copySource;
                Entity *entity;
                if(type == Constants::EntityType::Radio){
                    entity = new Radio(parent);
                }else
                if(type == Constants::EntityType::Sensor){
                    entity = new Sensor(parent);
                }else
                if(type == Constants::EntityType::FixedPoint){
                    entity = new FixedPoints(parent);
                }else
                if(type == Constants::EntityType::Formation){
                    entity = new Formation(parent);
                }else
                if(type == Constants::EntityType::SpecialZone){
                    entity = new Specialzone(parent);
                }else
                if(type == Constants::EntityType::IFF){
                    entity = new IFF(parent);
                }else{
                    entity = new Platform(parent);
                }
                entityJson["id"] = QString::fromStdString(Uuid::generateShortUniqueID());
                entityJson["parent_id"] = targetId;
                entity->Name = entityJson["name"].toString().toStdString();
                entity->ID = entityJson["id"].toString().toStdString();
                entity->parentID = entityJson["parent_id"].toString().toStdString();
                if (entity) {
                    if(isProfile){
                        copySource->ProfileCategories[targetId.toStdString()]->addEntityWithObject(entity);
                    }else{
                        (*copySource->Folders)[targetId.toStdString()]->addEntityWithObject(entity);
                    }
                    entity->fromJson(entityJson);
                    QCoreApplication::processEvents();
                }
            } catch (const std::exception& e) {
                qCritical() << "Failed to drop entity:" << e.what();
            }
        }
    });
}

/* Connect signals for library and tree view */
void HierarchyConnector::connectLibrarySignals(Hierarchy* library, HierarchyTree* libTree)
{
    // Validate inputs
    if (!library || !libTree) {
        qWarning() << "Cannot connect signals - null library or libTree";
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
                        } catch (const std::exception& e) {
                            qWarning() << "Failed to add component" << component
                                       << "to library entity:" << e.what();
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
                    qWarning() << "Cannot paste - nothing copied!";
                    return;
                }
                if (!copySource) {
                    qWarning() << "Cannot paste - no source hierarchy set!";
                    return;
                }
                QString type = copydata["type"].toString();
                QString id = copydata["ID"].toString();
                QString targetType = targetData["type"].toString();
                QString targetId = targetData["ID"].toString();
                if (type != "entity") {
                    qWarning() << "Can only paste entities! Type was:" << type;
                    return;
                }
                try {
                    auto entityIt = copySource->Entities->find(id.toStdString());
                    if (entityIt == copySource->Entities->end()) {
                        qCritical() << "Library Entity not found in Entities map for ID:" << id;
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
                            emit library->componentAdded(QString::fromStdString(newEntity->ID),"ID", compName);
                        }
                    } else {
                        qWarning() << "Failed to create new entity during paste";
                    }
                    copydata.clear();
                    copySource = nullptr;
                } catch (const std::exception& e) {
                    qCritical() << "Failed to paste library entity:" << e.what();
                }
            });

    // Handle drag-and-drop
    connect(libTree, &HierarchyTree::itemDropped, this, [=](QVariantMap sourceData, QVariantMap targetData) {
        return;
        if (sourceData["type"].toString() == "entity") {
            QString sourceId = sourceData["ID"].toString();
            QString targetId = targetData["ID"].toString();
            bool isProfile = (targetData["type"].toString() == "profile");
            try {
                auto entityIt = library->Entities->find(sourceId.toStdString());
                if (entityIt == library->Entities->end()) {
                    qCritical() << "Library Entity not found in Entities map for ID:" << sourceId;
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
                Entity* newEntity = library->addEntityFromJson(targetId, entityJson, isProfile);
                if (newEntity) {
                    for (const QString& compName : componentNames) {
                        emit library->componentAdded(QString::fromStdString(newEntity->ID),"ID", compName);
                    }
                } else {
                    qWarning() << "Failed to create new entity during drop";
                }
            } catch (const std::exception& e) {
                qCritical() << "Failed to drop library entity:" << e.what();
            }
        }
    });
}

/* Initialize library data */
void HierarchyConnector::initializeLibraryData(Hierarchy* library)
{
    // Validate input
    if (!library) {
        return;
    }
    // Add platform profile
    ProfileCategaory* platform = library->addProfileCategaory("Platform");

    if (!platform) {
        return;
    }
    platform->setProfileType(Constants::EntityType::Platform);
    // Add air folder
    Folder* air = platform->addFolder("Air");
    if (!air) {
        return;
    }
    // Add fighter jet entity
    Entity* fighterJet = air->addEntity("FighterJet");
    if (!fighterJet) {
        return;
    }
    // Add components to fighter jet
    QStringList components = {"transform"};
    QSet<QString> addedComponents;
    for (const QString& comp : components) {
        try {
            fighterJet->addComponent(comp.toStdString());
            addedComponents.insert(comp);
            emit library->componentAdded(QString::fromStdString(fighterJet->ID),"ID", comp);
        } catch (const std::exception& e) {
            // Ignore exceptions
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
}


void HierarchyConnector::setupFileOperations(QMainWindow* parent, Hierarchy* hierarchy, TacticalDisplay* tacticalDisplay)
{
    MenuBar* menuBar = qobject_cast<MenuBar*>(parent->menuBar());
    if (!menuBar) {
        qWarning() << "MenuBar not found for file operations setup";
        return;
    }

    // Determine editor type
    RecentProjectsManager::EditorType editorType;
    if (qobject_cast<DatabaseEditor*>(parent)) {
        editorType = RecentProjectsManager::DatabaseEditor;
    } else if (qobject_cast<ScenarioEditor*>(parent)) {
        editorType = RecentProjectsManager::ScenarioEditor;
    } else if (qobject_cast<RuntimeEditor*>(parent)) {
        editorType = RecentProjectsManager::RuntimeEditor;
    } else {
        qWarning() << "Unknown editor type";
        return;
    }

    QAction* loadAction = menuBar->getLoadAction();
    QAction* loadToLibraryAction = menuBar->getLoadToLibraryAction();
    QAction* saveAction = menuBar->getSaveAction();
    QAction* sameSaveAction = menuBar->getSameSaveAction();

    // Connect load action
    connect(loadAction, &QAction::triggered, this, [=]() {
        QString filePath = QFileDialog::getOpenFileName(parent, "Open JSON",
                                                        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                        "JSON Files (*.json)");

        if (!filePath.isEmpty()) {
            // Call editor-specific load function
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

    // Connect save action
    connect(saveAction, &QAction::triggered, this, [=]() {
        QString filePath = QFileDialog::getSaveFileName(parent, "Save JSON",
                                                        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                        "JSON Files (*.json)");

        if (!filePath.isEmpty()) {
            // Save file
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                QJsonObject obj;
                obj["hierarchy"] = hierarchy->toJson();
                if (tacticalDisplay != nullptr) {
                    obj["tactical"] = tacticalDisplay->canvas->toJson();
                } else {
                    obj["tactical"] = QJsonObject();
                }

                QJsonDocument doc(obj);
                file.write(doc.toJson(QJsonDocument::Indented));
                file.close();

                // Add to appropriate recent projects list
                RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);

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

    // Connect same save action
    connect(sameSaveAction, &QAction::triggered, this, [=]() {
        QString filePath = getLastSavedFilePath(parent);
        if (filePath.isEmpty()) {
            emit saveAction->triggered();
            return;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::warning(parent, "Error", "Failed to save file");
            return;
        }

        QJsonObject obj;
        obj["hierarchy"] = hierarchy->toJson();
        if (tacticalDisplay != nullptr) {
            obj["tactical"] = tacticalDisplay->canvas->toJson();
        }

        QJsonDocument doc(obj);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        // Add to appropriate recent projects list
        RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);
    });
}
/* Load JSON to library */
void HierarchyConnector::loadToLibrary(QMainWindow* parent)
{
    // Validate input
    if (!parent) {
        qWarning() << "Cannot load to library - parent is null";
        return;
    }
    // Determine target library
    Hierarchy* targetLibrary = nullptr;
    if (ScenarioEditor* se = qobject_cast<ScenarioEditor*>(parent)) {
        targetLibrary = se->library;
    } else if (RuntimeEditor* re = qobject_cast<RuntimeEditor*>(parent)) {
        targetLibrary = re->library;
    }
    if (!targetLibrary) {
        qWarning() << "Cannot load to library - unsupported parent type";
        return;
    }
    // Open file dialog
    QString filePath = QFileDialog::getOpenFileName(parent, "Open JSON to Library", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "JSON Files (*.json)");
    if (filePath.isEmpty()) {
        return;
    }
    // Read JSON file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(parent, "Error", QString("Failed to open file: %1").arg(file.errorString()));
        return;
    }
    QByteArray data = file.readAll();
    file.close();
    // Parse JSON
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(parent, "Error", QString("Failed to parse JSON: %1").arg(err.errorString()));
        return;
    }
    // Load hierarchy data
    QJsonObject obj = doc.object();
    if (obj.contains("hierarchy")) {
        QJsonObject hier = obj["hierarchy"].toObject();
        targetLibrary->fromJson(hier);
        if (libTreeView) {
            libTreeView->getTreeWidget()->update();
        }
        qDebug() << "JSON loaded into target library successfully";
    } else {
        QMessageBox::warning(parent, "Error", "JSON does not contain 'hierarchy' key");
    }
}

/* Handle drag-and-drop from library to hierarchy */
void HierarchyConnector::handleLibraryToHierarchyDrop(QVariantMap sourceData, QVariantMap targetData)
{
    // Validate hierarchy and library
    if (!hierarchy || !library) {
        qWarning() << "Invalid hierarchy or library pointers";
        return;
    }
    // Determine source and target types
    QString sourceType = sourceData["type"].toString();
    QString targetType;
    if (targetData["type"].type() == QVariant::Map) {
        QVariantMap typeData = targetData["type"].toMap();
        if (typeData.contains("type") && typeData["type"].toString() == "option") {
            targetType = "profile";
        } else {
            qWarning() << "Invalid nested type structure in handleLibraryToHierarchyDrop:" << targetData["type"];
            return;
        }
    } else {
        targetType = targetData["type"].toString();
    }
    // Validate source type
    if (sourceType != "entity") {
        qWarning() << "Invalid drag source - only entities can be dragged (got:" << sourceType << ")";
        return;
    }
    // Validate target type
    if (targetType != "profile" && targetType != "folder") {
        qWarning() << "Invalid drop target - can only drop on profiles or folders (got:" << targetType << ")";
        return;
    }
    // Get IDs
    QString sourceId = sourceData["ID"].toString();
    QString targetId = targetData["ID"].toString();
    // Process drop
    try {
        auto entityIt = library->Entities->find(sourceId.toStdString());
        if (entityIt == library->Entities->end()) {
            qCritical() << "Library Entity not found in Entities map for ID:" << sourceId;
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
        Entity* newEntity = hierarchy->addEntityFromJson(targetId, entityJson, targetType == "profile");
        if (newEntity) {
            for (const QString& compName : componentNames) {
                emit hierarchy->componentAdded(QString::fromStdString(newEntity->ID),"ID", compName);
            }
        } else {
            qWarning() << "Failed to create new entity during drop";
        }
    } catch (const std::exception& e) {
        qCritical() << "Drop operation failed:" << e.what();
    }
}

/* Handle drag-and-drop from hierarchy to library */
void HierarchyConnector::handleHierarchyToLibraryDrop(QVariantMap sourceData, QVariantMap targetData)
{
    // Validate hierarchy and library
    if (!hierarchy || !library) {
        qWarning() << "Invalid hierarchy or library pointers";
        return;
    }
    // Get source and target types
    QString sourceType = sourceData["type"].toString();
    QString targetType = targetData["type"].toString();
    // Validate source type
    if (sourceType != "entity") {
        qWarning() << "Drag source is not an entity - ignoring drop (got:" << sourceType << ")";
        return;
    }
    // Validate target type
    if (targetType != "profile" && targetType != "folder") {
        qWarning() << "Can only drop onto profiles or folders - ignoring drop (got:" << targetType << ")";
        return;
    }
    // Get IDs
    QString sourceId = sourceData["ID"].toString();
    QString targetId = targetData["ID"].toString();
    // Process drop
    try {
        auto entityIt = hierarchy->Entities->find(sourceId.toStdString());
        if (entityIt == hierarchy->Entities->end()) {
            qCritical() << "Hierarchy Entity not found in Entities map for ID:" << sourceId;
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
                emit library->componentAdded(QString::fromStdString(newEntity->ID),"ID", compName);
            }
        } else {
            qWarning() << "Failed to create new entity during drop";
        }
    } catch (const std::exception& e) {
        qCritical() << "Hierarchy to Library drop failed:" << e.what();
    }
}

/* Generate feedback data for hierarchy */
QJsonObject HierarchyConnector::getFeedbackData(Hierarchy* hierarchy)
{
    // Initialize feedback data
    QJsonObject feedbackData;
    if (!hierarchy) {
        qWarning() << "Hierarchy is null, cannot generate feedback data";
        return feedbackData;
    }
    try {
        // System overview data
        QJsonObject overviewData;
        overviewData["systemStatus"] = "System: ONLINE  Sim: RUNNING  RTC: 2025-10-15";
        overviewData["uptime"] = "Uptime: 02:15:30";
        overviewData["feedbackEvents"] = "Accumulated Feedback Events: 156";
        overviewData["cpuUsage"] = 65;
        int mainEntities = 0;
        for (const auto& [id, e] : *hierarchy->Entities) {
            if (hierarchy->ProfileCategories.find(e->parentID) == hierarchy->ProfileCategories.end()) {
                mainEntities++;
            }
        }
        overviewData["entities"] = mainEntities;
        feedbackData["overview"] = overviewData;

        // Storage data
        QJsonObject storageData;
        storageData["mongoDb"] = "MongoDB: 2.4 GB";
        storageData["logs"] = "Logs: 1.1 GB";
        storageData["scenarios"] = "Scenarios: 0.8 GB";
        storageData["totalStorage"] = "Total: 4.3 GB";
        feedbackData["storage"] = storageData;

        // Sensors data
        QJsonObject sensorsData;
        QJsonArray sensorList;
        for (const auto& [id, e] : *hierarchy->Entities) {
            QJsonObject entityJson = e->toJson();
            if (entityJson.contains("sensors")) {
                QJsonArray sensors = entityJson["sensors"].toArray();
                for (const auto& sensorValue : sensors) {
                    QJsonObject sensor = sensorValue.toObject();
                    QJsonObject sensorItem;
                    sensorItem["type"] = sensor.contains("name") ? sensor["name"].toString() : "Unknown";
                    sensorItem["status"] = sensor.contains("status") ? sensor["status"].toString() : "Unknown";
                    sensorList.append(sensorItem);
                    if (sensor.contains("type") && sensor["type"].toString() == "RADAR") {
                        sensorsData["radarFeedback"] = "RADAR: " + sensor["status"].toString();
                    }
                }
            }
        }
        sensorsData["sensorList"] = sensorList;
        sensorsData["iffFeedback"] = "IFF: Active";
        feedbackData["sensors"] = sensorsData;

        // Radio data
        QJsonObject radioData;
        radioData["radioSystem"] = "Radio System: UHF/VHF";
        radioData["frequency"] = "Frequency: 243.0 MHz";
        radioData["signalStrength"] = "Signal Strength: 85%";
        feedbackData["radio"] = radioData;

        // Network data
        QJsonObject networkData;
        networkData["connectivity"] = "Connectivity: Stable";
        networkData["bandwidth"] = "Bandwidth Usage: 45%";
        networkData["latency"] = "Latency: 25ms";
        feedbackData["network"] = networkData;

        // Logs data
        feedbackData["logs"] = "System logs loaded successfully\nRTC: 2025-10-15 10:30:00\nSimulation running normally";

        // Canvas interactions data
        QJsonArray interactions;
        QJsonObject interaction1;
        interaction1["time"] = "10:25:30";
        interaction1["id"] = "Entity_001";
        interaction1["geoCoords"] = "35.6895, 139.6917";
        interaction1["fixedPoints"] = "FP1, FP2, FP3";
        interactions.append(interaction1);
        feedbackData["interactions"] = interactions;

        // Entity data
        QJsonObject entityData;
        int totalEntities = 0;
        int activeEntities = 0;
        QJsonArray entityList;
        for (const auto& [id, e] : *hierarchy->Entities) {
            if (hierarchy->ProfileCategories.find(e->parentID) != hierarchy->ProfileCategories.end()) {
                continue;
            }
            QJsonObject entityJson = e->toJson();
            totalEntities++;
            bool isActive = entityJson.contains("active") && entityJson["active"].toBool();
            if (isActive) activeEntities++;
            QJsonObject entityItem;
            entityItem["id"] = QString::fromStdString(id);
            entityItem["name"] = QString::fromStdString(e->Name);
            entityItem["active"] = isActive;
            entityList.append(entityItem);
        }
        entityData["total"] = totalEntities;
        entityData["active"] = activeEntities;
        entityData["entities"] = entityList;
        feedbackData["entities"] = entityData;

        // IFF data
        QJsonObject iffData;
        int totalIffs = 0;
        int activeIffs = 0;
        QJsonArray iffList;
        for (const auto& [mainId, mainE] : *hierarchy->Entities) {
            if (hierarchy->ProfileCategories.find(mainE->parentID) != hierarchy->ProfileCategories.end()) {
                continue;
            }
            QJsonObject entityJson = mainE->toJson();
            if (entityJson.contains("iffList")) {
                QJsonArray iffArray = entityJson["iffList"].toArray();
                for (const auto& iffValue : iffArray) {
                    QJsonObject iffJson = iffValue.toObject();
                    totalIffs++;
                    QString mode = iffJson.contains("operationalMode") ? iffJson["operationalMode"].toString() : "";
                    if (mode == "Active") activeIffs++;
                    QJsonObject iffItem;
                    iffItem["id"] = iffJson.contains("ID") ? iffJson["ID"].toString() : "";
                    iffItem["name"] = iffJson.contains("Name") ? iffJson["Name"].toString() : "Unknown";
                    iffItem["parentEntity"] = QString::fromStdString(mainE->Name);
                    iffItem["mode"] = mode;
                    iffList.append(iffItem);
                }
            }
        }
        iffData["total"] = totalIffs;
        iffData["active"] = activeIffs;
        iffData["iffs"] = iffList;
        feedbackData["iffs"] = iffData;

        // FixedPoint data
        QJsonObject fixedPointData;
        int totalFixedPoints = 0;
        int activeFixedPoints = 0;
        QJsonArray fixedPointList;
        for (const auto& [id, e] : *hierarchy->Entities) {
            QJsonObject entityJson = e->toJson();
            if (entityJson.contains("type") && entityJson["type"].toString() == "FixedPoint") {
                totalFixedPoints++;
                bool isActive = entityJson.contains("active") && entityJson["active"].toBool();
                if (isActive) activeFixedPoints++;
                QJsonObject fpItem;
                fpItem["id"] = QString::fromStdString(id);
                fpItem["name"] = QString::fromStdString(e->Name);
                fpItem["active"] = isActive;
                fixedPointList.append(fpItem);
            }
        }
        fixedPointData["total"] = totalFixedPoints;
        fixedPointData["active"] = activeFixedPoints;
        fixedPointData["fixedPoints"] = fixedPointList;
        feedbackData["fixedPoints"] = fixedPointData;

        // Weapon data
        QJsonObject weaponData;
        int totalWeapons = 0;
        int activeWeapons = 0;
        QJsonArray weaponList;
        for (const auto& [id, e] : *hierarchy->Entities) {
            QJsonObject entityJson = e->toJson();
            if (entityJson.contains("weaponType")) {
                totalWeapons++;
                bool isActive = entityJson.contains("active") && entityJson["active"].toBool();
                if (isActive) activeWeapons++;
                QJsonObject weaponItem;
                weaponItem["id"] = QString::fromStdString(id);
                weaponItem["name"] = QString::fromStdString(e->Name);
                weaponItem["active"] = isActive;
                weaponItem["type"] = entityJson["weaponType"].toString();
                weaponList.append(weaponItem);
            }
        }
        weaponData["total"] = totalWeapons;
        weaponData["active"] = activeWeapons;
        weaponData["weapons"] = weaponList;
        feedbackData["weapons"] = weaponData;

    } catch (const std::exception& e) {
        qCritical() << "Error generating feedback data:" << e.what();
    }

    return feedbackData;
}
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
    qDebug() << "🗑️ Recent projects list cleared";
}
