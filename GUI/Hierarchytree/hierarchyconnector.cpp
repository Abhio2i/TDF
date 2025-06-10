
#include "hierarchyconnector.h"
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMessageBox>
#include <QDebug>
#include "GUI/Hierarchytree/contextmenu.h"
#include "GUI/Menubars/menubar.h"

HierarchyConnector* HierarchyConnector::m_instance = nullptr;

HierarchyConnector::HierarchyConnector(QObject* parent) : QObject(parent) {}

HierarchyConnector* HierarchyConnector::instance()
{
    if (!m_instance) {
        m_instance = new HierarchyConnector();
    }
    return m_instance;
}

void HierarchyConnector::connectSignals(Hierarchy* hierarchy, HierarchyTree* treeView,
                                        TacticalDisplay* tactical, Inspector* inspector)
{
    if (!hierarchy || !treeView) {
        qWarning() << "Cannot connect signals - null hierarchy or treeView";
        return;
    }

    // Connect hierarchy to tree view
    connect(hierarchy, &Hierarchy::profileAdded,
            treeView, &HierarchyTree::profileAdded);
    connect(hierarchy, &Hierarchy::folderAdded,
            treeView, &HierarchyTree::folderAdded);
    connect(hierarchy, &Hierarchy::entityAdded,
            treeView, &HierarchyTree::entityAdded);
    connect(hierarchy, &Hierarchy::profileRemoved,
            treeView, &HierarchyTree::profileRemoved);
    connect(hierarchy, &Hierarchy::folderRemoved,
            treeView, &HierarchyTree::folderRemoved);
    connect(hierarchy, &Hierarchy::entityRemoved,
            treeView, &HierarchyTree::entityRemoved);
    connect(hierarchy, &Hierarchy::componentAdded,
            treeView, &HierarchyTree::componentAdded);
    connect(hierarchy, &Hierarchy::componentRemoved,
            treeView, &HierarchyTree::componentRemoved);

    // Connect tree view to hierarchy
    connect(treeView->getContextMenu(), &ContextMenu::addProfileRequested,
            hierarchy, &Hierarchy::addProfileCategaory);

    connect(treeView->getContextMenu(), &ContextMenu::removeProfileRequested,
            hierarchy, &Hierarchy::removeProfileCategaory);
    connect(treeView->getContextMenu(), &ContextMenu::addFolderRequested,
            hierarchy, &Hierarchy::addFolder);
    connect(treeView->getContextMenu(), &ContextMenu::removeFolderRequested,
            hierarchy, &Hierarchy::removeFolder);

    connect(treeView->getContextMenu(), &ContextMenu::addEntityRequested,
            this, [=](QString parentId, QString entityName, bool isProfileParent, QVariantMap components) {
                qDebug() << "Creating entity:" << entityName;

                // Create entity (Transform will be added automatically)
                Entity* newEntity = hierarchy->addEntity(parentId, entityName, isProfileParent);

                // Add only non-Transform components that were selected
                for (const auto& component : components.keys()) {
                    if (component != "transform" && components[component].toBool()) {
                        try {
                            newEntity->addComponent(component.toStdString());
                            qDebug() << "Added component:" << component;
                        } catch (...) {
                            qWarning() << "Failed to add component:" << component;
                        }
                    }
                }

                qDebug() << "Entity created with ID:" << QString::fromStdString(newEntity->ID);
            });
    connect(treeView->getContextMenu(), &ContextMenu::removeEntityRequested,
            hierarchy, &Hierarchy::removeEntity);
    connect(treeView->getContextMenu(), &ContextMenu::removeComponentRequested,
            hierarchy, &Hierarchy::removeComponent);
    // Connect tactical display if provided
    if (tactical) {
        connect(hierarchy, &Hierarchy::entityRemoved,
                tactical, &TacticalDisplay::removeMesh);
    }

    // Connect inspector if provided
    if (inspector) {
        connect(inspector, &Inspector::valueChanged,
                hierarchy, &Hierarchy::UpdateComponent);
    }

    // Connect rename signals
    connect(hierarchy, &Hierarchy::profileRenamed,
            treeView, &HierarchyTree::profileRenamed);
    connect(hierarchy, &Hierarchy::folderRenamed,
            treeView, &HierarchyTree::folderRenamed);
    connect(hierarchy, &Hierarchy::entityRenamed,
            treeView, &HierarchyTree::entityRenamed);

    // Connect context menu rename signal to hierarchy
    connect(treeView->getContextMenu(), &ContextMenu::renameItemRequested,
            [hierarchy](QVariantMap data) {
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

    connect(treeView, &HierarchyTree::copyItemRequested, this,
            [this, hierarchy](QVariantMap data) {
                qDebug() << "Copy received - Type:" << data["type"].toString()
                << "Name:" << data["name"].toString();
                copydata = data;
                copySource = hierarchy;
            });

    connect(treeView, &HierarchyTree::pasteItemRequested, this,
            [this, hierarchy](QVariantMap targetData) {
                qDebug() << "Paste requested - Target Type:" << targetData["type"].toString()
                << "Target Name:" << targetData["name"].toString();

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
                    qWarning() << "Can only paste entities!";
                    return;
                }

                try {
                    QJsonObject entityJson = (*copySource->Entities)[id.toStdString()]->toJson();

                    if (targetType == "profile") {
                        hierarchy->addEntityFromJson(targetId, entityJson, true);
                        qDebug() << "Entity pasted under profile";
                    }
                    else if (targetType == "folder") {
                        hierarchy->addEntityFromJson(targetId, entityJson, false);
                        qDebug() << "Entity pasted under folder";
                    }
                    else {
                        qWarning() << "Invalid paste target type:" << targetType;
                    }
                }
                catch (const std::exception& e) {
                    qCritical() << "Paste failed:" << e.what();
                }
            });


}

void HierarchyConnector::connectLibrarySignals(Hierarchy* library, HierarchyTree* libTree)
{
    if (!library || !libTree) {
        qWarning() << "Cannot connect signals - null library or libTree";
        return;
    }
    // Connect library to tree view
    connect(library, &Hierarchy::profileAdded,
            libTree, &HierarchyTree::profileAdded);
    connect(library, &Hierarchy::folderAdded,
            libTree, &HierarchyTree::folderAdded);
    connect(library, &Hierarchy::entityAdded,
            libTree, &HierarchyTree::entityAdded);
    connect(library, &Hierarchy::componentAdded,
            libTree, &HierarchyTree::componentAdded);
    connect(library, &Hierarchy::profileRemoved,
            libTree, &HierarchyTree::profileRemoved);
    connect(library, &Hierarchy::folderRemoved,
            libTree, &HierarchyTree::folderRemoved);
    connect(library, &Hierarchy::entityRemoved,
            libTree, &HierarchyTree::entityRemoved);

    // Connect tree view to library
    connect(libTree->getContextMenu(), &ContextMenu::addProfileRequested,
            library, &Hierarchy::addProfileCategaory);
    connect(libTree->getContextMenu(), &ContextMenu::removeProfileRequested,
            library, &Hierarchy::removeProfileCategaory);
    connect(libTree->getContextMenu(), &ContextMenu::addFolderRequested,
            library, &Hierarchy::addFolder);
    connect(libTree->getContextMenu(), &ContextMenu::removeFolderRequested,
            library, &Hierarchy::removeFolder);
    connect(libTree->getContextMenu(), &ContextMenu::addEntityRequested,
            library, &Hierarchy::addEntity);
    connect(libTree->getContextMenu(), &ContextMenu::removeEntityRequested,
            library, &Hierarchy::removeEntity);

    // Connect rename signals
    connect(library, &Hierarchy::profileRenamed,
            libTree, &HierarchyTree::profileRenamed);
    connect(library, &Hierarchy::folderRenamed,
            libTree, &HierarchyTree::folderRenamed);
    connect(library, &Hierarchy::entityRenamed,
            libTree, &HierarchyTree::entityRenamed);

    // Connect context menu rename signal to library
    connect(libTree->getContextMenu(), &ContextMenu::renameItemRequested,
            [library](QVariantMap data) {
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

    // Connect library copy-paste signals
    connect(libTree->getContextMenu(), &ContextMenu::copyItemRequested,
            libTree, &HierarchyTree::copyItemRequested);

    connect(libTree->getContextMenu(), &ContextMenu::pasteItemRequested,
            libTree, &HierarchyTree::pasteItemRequested);

    connect(libTree, &HierarchyTree::copyItemRequested, this,
            [this, library](QVariantMap data) {
                qDebug() << "Library copy received - Type:" << data["type"].toString();
                copydata = data;
                copySource = library;
            });

    connect(libTree, &HierarchyTree::pasteItemRequested, this,
            [this, library](QVariantMap targetData) {
                qDebug() << "Library paste requested";

                if (copydata.isEmpty() || !copySource) {
                    qWarning() << "Cannot paste - no data or source!";
                    return;
                }

                QString type = copydata["type"].toString();
                QString id = copydata["ID"].toString();
                QString targetType = targetData["type"].toString();
                QString targetId = targetData["ID"].toString();

                if (type == "entity") {
                    try {
                        QJsonObject entityJson = (*copySource->Entities)[id.toStdString()]->toJson();

                        if (targetType == "profile") {
                            library->addEntityFromJson(targetId, entityJson, true);
                        }
                        else if (targetType == "folder") {
                            library->addEntityFromJson(targetId, entityJson, false);
                        }
                    }
                    catch (...) {
                        qCritical() << "Failed to paste library entity!";
                    }
                }
            });

}
void HierarchyConnector::initializeLibraryData(Hierarchy* library)
{
    qDebug() << "Initializing library data...";
    ProfileCategaory* platform = library->addProfileCategaory("Platform");
    Folder* air = platform->addFolder("Air");
    air->addEntity("FighterJet");

}
void HierarchyConnector::initializeDummyData(Hierarchy* hierarchy)
{
    ProfileCategaory* platform = hierarchy->addProfileCategaory("Platform");
    hierarchy->addProfileCategaory("Radio");
    hierarchy->addProfileCategaory("Sensor");
    hierarchy->addProfileCategaory("Weapon");
    hierarchy->addProfileCategaory("IFF");
    Folder* air = platform->addFolder("Air");
    air->addFolder("Fighterjet");
    platform->addFolder("Ground");
    platform->addFolder("Sea");
    Entity* aircraft = air->addEntity("Aircraft");
    aircraft->addComponent("transform");
    aircraft->addComponent("trajectory");
    aircraft->addComponent("rigidbody");
    aircraft->addComponent("dynamicModel");
    aircraft->addComponent("collider");
    aircraft->addComponent("meshRenderer2d");
    // Entity* aircraft = air->addEntity("Aircraft");
}
void HierarchyConnector::setupFileOperations(QMainWindow* parent, Hierarchy* hierarchy)
{
    MenuBar* menuBar = qobject_cast<MenuBar*>(parent->menuBar());
    if (!menuBar) return;
    QAction* loadAction = menuBar->getLoadAction();
    QAction* saveAction = menuBar->getSaveAction();
    connect(loadAction, &QAction::triggered, parent, [=]() {
        QString filePath = QFileDialog::getOpenFileName(parent, "Open JSON", "", "JSON Files (*.json)");
        if (!filePath.isEmpty()) {
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                file.close();
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(data, &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    hierarchy->fromJson(doc.object());
                } else {
                    QMessageBox::warning(parent, "Error", "Invalid JSON File");
                }
            }
        }
    });
    connect(saveAction, &QAction::triggered, parent, [=]() {
        QString filePath = QFileDialog::getSaveFileName(parent, "Save JSON", "", "JSON Files (*.json)");
        if (!filePath.isEmpty()) {
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                QJsonDocument doc(hierarchy->toJson());
                file.write(doc.toJson(QJsonDocument::Indented));
                file.close();
                QMessageBox::information(parent, "Saved", "JSON saved successfully");
            }
        }
    });
}
void HierarchyConnector::handleLibraryToHierarchyDrop(QVariantMap sourceData, QVariantMap targetData)
{
    if (!hierarchy || !library) {
        qWarning() << "Invalid hierarchy or library pointers";
        return;
    }
    QString sourceType = sourceData["type"].toString();
    QString targetType = targetData["type"].toString();
    qDebug() << "Drop operation - Source type:" << sourceType << "Target type:" << targetType;
    // Only allow dropping entities
    if (sourceType != "entity") {
        qWarning() << "Invalid drag source - only entities can be dragged (got:" << sourceType << ")";
        return;
    }
    // Only allow dropping onto profiles or folders
    if (targetType != "profile" && targetType != "folder") {
        qWarning() << "Invalid drop target - can only drop on profiles or folders (got:" << targetType << ")";
        return;
    }
    QString sourceId = sourceData["ID"].toString();
    QString targetId = targetData["ID"].toString();
    qDebug() << "Processing drop - Source ID:" << sourceId << "Target ID:" << targetId;
    try {
        QJsonObject entityJson = (*library->Entities)[sourceId.toStdString()]->toJson();
        qDebug() << "Entity JSON:" << entityJson;

        if (targetType == "profile") {
            hierarchy->addEntityFromJson(targetId, entityJson, true);
            qDebug() << "Successfully added entity to profile";
        } else {
            hierarchy->addEntityFromJson(targetId, entityJson, false);
            qDebug() << "Successfully added entity to folder";
        }
    } catch (const std::exception& e) {
        qCritical() << "Drop operation failed:" << e.what();
    }
}
void HierarchyConnector::handleHierarchyToLibraryDrop(QVariantMap sourceData, QVariantMap targetData)
{
    if (!hierarchy || !library) return;

    QString sourceType = sourceData["type"].toString();
    QString targetType = targetData["type"].toString();
    // Only allow dragging entities
    if (sourceType != "entity") {
        qDebug() << "Drag source is not an entity - ignoring drop";
        return;
    }
    // Only allow dropping onto library profiles or folders
    if (targetType != "profile" && targetType != "folder") {
        qDebug() << "Can only drop onto profiles or folders - ignoring drop";
        return;
    }
    QString sourceId = sourceData["ID"].toString();
    QString targetId = targetData["ID"].toString();
    try {
        QJsonObject entityJson = (*hierarchy->Entities)[sourceId.toStdString()]->toJson();

        if (targetType == "profile") {
            library->addEntityFromJson(targetId, entityJson, true);
        } else {
            library->addEntityFromJson(targetId, entityJson, false);
        }
    } catch (const std::exception& e) {
        qCritical() << "Hierarchy to Library drop failed:" << e.what();
    }
}
