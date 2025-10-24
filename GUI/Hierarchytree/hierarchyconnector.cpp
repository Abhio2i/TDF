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

    // Connect hierarchy to tree view
    connect(hierarchy, &Hierarchy::profileAdded, treeView, &HierarchyTree::profileAdded);
    connect(hierarchy, &Hierarchy::folderAdded, treeView, &HierarchyTree::folderAdded);
    connect(hierarchy, &Hierarchy::entityAdded, treeView, &HierarchyTree::entityAdded);
    connect(hierarchy, &Hierarchy::profileRemoved, treeView, &HierarchyTree::profileRemoved);
    connect(hierarchy, &Hierarchy::folderRemoved, treeView, &HierarchyTree::folderRemoved);
    connect(hierarchy, &Hierarchy::entityRemoved, treeView, &HierarchyTree::entityRemoved);
    connect(hierarchy, &Hierarchy::componentAdded, treeView, &HierarchyTree::componentAdded);
    connect(hierarchy, &Hierarchy::componentRemoved, treeView, &HierarchyTree::componentRemoved);

    // Connect tree view context menu to hierarchy
    connect(treeView->getContextMenu(), &ContextMenu::addProfileRequested,
            hierarchy, &Hierarchy::addProfileCategaory);
    connect(treeView->getContextMenu(), &ContextMenu::removeProfileRequested,
            hierarchy, &Hierarchy::removeProfileCategaory);
    connect(treeView->getContextMenu(), &ContextMenu::addFolderRequested,
            hierarchy, &Hierarchy::addFolder);
    connect(treeView->getContextMenu(), &ContextMenu::removeFolderRequested,
            hierarchy, &Hierarchy::removeFolder);

    // Connect add entity action
    connect(treeView->getContextMenu(), &ContextMenu::addEntityRequested,
            this, [=](QString parentId, QString entityName, bool isProfileParent, QVariantMap components) {
                qDebug() << "Creating entity:" << entityName;
                Entity* newEntity = hierarchy->addEntity(parentId, entityName, isProfileParent);
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
            });

    // Connect remove entity and component actions
    connect(treeView->getContextMenu(), &ContextMenu::removeEntityRequested,
            hierarchy, &Hierarchy::removeEntity);
    connect(treeView->getContextMenu(), &ContextMenu::removeComponentRequested,
            hierarchy, &Hierarchy::removeComponent);

    // Connect add component action
    connect(treeView->getContextMenu(), &ContextMenu::addComponentRequested, this,
            [=](QString entityID, QString componentType, QString componentName) {
                if (componentType == "iff") {
                    hierarchy->attchedIff(entityID, componentName);
                } else if (componentType == "sensors") {
                    hierarchy->attachSensors(entityID, componentName);
                } else if (componentType == "radios") {
                    hierarchy->attachRadios(entityID, componentName);
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
                    emit hierarchy->componentAdded(QString::fromStdString(newEntity->ID), compName);
                }
            } else {
                qWarning() << "Failed to create new entity during paste";
            }
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
            bool isProfile = (targetData["type"].toString() == "profile");
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
                Entity* newEntity = hierarchy->addEntityFromJson(targetId, entityJson, isProfile);
                if (newEntity) {
                    for (const QString& compName : componentNames) {
                        emit hierarchy->componentAdded(QString::fromStdString(newEntity->ID), compName);
                    }
                } else {
                    qWarning() << "Failed to create new entity during drop";
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
                            emit library->componentAdded(QString::fromStdString(newEntity->ID), compName);
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
                        emit library->componentAdded(QString::fromStdString(newEntity->ID), compName);
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
            emit library->componentAdded(QString::fromStdString(fighterJet->ID), comp);
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

/* Setup file operations */
void HierarchyConnector::setupFileOperations(QMainWindow* parent, Hierarchy* hierarchy, TacticalDisplay* tacticalDisplay)
{
    // Get menu bar
    MenuBar* menuBar = qobject_cast<MenuBar*>(parent->menuBar());
    if (!menuBar) {
        qWarning() << "MenuBar not found for file operations setup";
        return;
    }
    // Retrieve actions
    QAction* loadAction = menuBar->getLoadAction();
    QAction* loadToLibraryAction = menuBar->getLoadToLibraryAction();
    QAction* saveAction = menuBar->getSaveAction();
    QAction* sameSaveAction = menuBar->getSameSaveAction();

    // Connect load action
    connect(loadAction, &QAction::triggered, this, [=]() {
        QString filePath = QFileDialog::getOpenFileName(parent, "Open JSON", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "JSON Files (*.json)");
        if (!filePath.isEmpty()) {
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                file.close();
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(data, &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.contains("hierarchy")) {
                        QJsonObject hier = obj["hierarchy"].toObject();
                        hierarchy->fromJson(hier);
                    }
                    if (tacticalDisplay != nullptr) {
                        if (obj.contains("tactical")) {
                            QJsonObject tac = obj["tactical"].toObject();
                            tacticalDisplay->canvas->fromJson(tac);
                        }
                    }
                    qDebug() << "JSON loaded into Hierarchy successfully";
                    if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(parent)) {
                        dbEditor->lastSavedFilePath = filePath;
                        dbEditor->clearUnsavedChanges();
                        qDebug() << "DatabaseEditor lastSavedFilePath set to:" << filePath;
                    } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(parent)) {
                        scEditor->lastSavedFilePath = filePath;
                        scEditor->clearUnsavedChanges();
                        qDebug() << "ScenarioEditor lastSavedFilePath set to:" << filePath;
                    } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(parent)) {
                        rtEditor->lastSavedFilePath = filePath;
                        rtEditor->clearUnsavedChanges();
                        qDebug() << "RuntimeEditor lastSavedFilePath set to:" << filePath;
                    }
                } else {
                    QMessageBox::warning(parent, "Error", QString("Failed to parse JSON: %1").arg(err.errorString()));
                }
            }
        }
    });

    // Connect load to library action
    connect(loadToLibraryAction, &QAction::triggered, this, [=]() {
        this->loadToLibrary(parent);
    });

    // Connect save as action
    connect(saveAction, &QAction::triggered, this, [=]() {
        QString filePath = QFileDialog::getSaveFileName(parent, "Save JSON", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "JSON Files (*.json)");
        if (!filePath.isEmpty()) {
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
                qDebug() << "JSON saved successfully";
                if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(parent)) {
                    dbEditor->lastSavedFilePath = filePath;
                    dbEditor->clearUnsavedChanges();
                    qDebug() << "DatabaseEditor lastSavedFilePath set to:" << filePath;
                } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(parent)) {
                    scEditor->lastSavedFilePath = filePath;
                    scEditor->clearUnsavedChanges();
                    qDebug() << "ScenarioEditor lastSavedFilePath set to:" << filePath;
                } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(parent)) {
                    rtEditor->lastSavedFilePath = filePath;
                    rtEditor->clearUnsavedChanges();
                    qDebug() << "RuntimeEditor lastSavedFilePath set to:" << filePath;
                }
            } else {
                qWarning() << "Failed to open file for writing:" << filePath;
                QMessageBox::warning(parent, "Error", QString("Failed to save JSON: %1").arg(file.errorString()));
            }
        }
    });

    // Connect save action
    connect(sameSaveAction, &QAction::triggered, this, [=]() {
        QString filePath = this->getLastSavedFilePath(parent);
        if (filePath.isEmpty()) {
            qDebug() << "No last saved file path, falling back to Save As";
            emit saveAction->triggered();
            return;
        }
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "Failed to open file for writing:" << file.errorString();
            QMessageBox::warning(parent, "Error", QString("Failed to save JSON: %1").arg(file.errorString()));
            return;
        }
        QJsonObject obj;
        obj["hierarchy"] = hierarchy->toJson();
        obj["tactical"] = tacticalDisplay ? tacticalDisplay->canvas->toJson() : QJsonObject();
        QJsonDocument doc(obj);
        qint64 bytesWritten = file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        if (bytesWritten == -1) {
            qWarning() << "Failed to write JSON to file";
            QMessageBox::warning(parent, "Error", "Failed to write JSON to file");
        } else {
            qDebug() << "JSON saved successfully to:" << filePath;
            if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(parent)) {
                dbEditor->clearUnsavedChanges();
                qDebug() << "DatabaseEditor unsaved changes cleared";
            } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(parent)) {
                scEditor->clearUnsavedChanges();
                qDebug() << "ScenarioEditor unsaved changes cleared";
            } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(parent)) {
                rtEditor->clearUnsavedChanges();
                qDebug() << "RuntimeEditor unsaved changes cleared";
            }
        }
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
                emit hierarchy->componentAdded(QString::fromStdString(newEntity->ID), compName);
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
                emit library->componentAdded(QString::fromStdString(newEntity->ID), compName);
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
