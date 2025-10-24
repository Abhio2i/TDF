/* ========================================================================= */
/* File: hierarchytree.cpp                                                  */
/* Purpose: Implements hierarchy tree widget with drag-and-drop support      */
/* ========================================================================= */

#include "hierarchytree.h"                         // For hierarchy tree class
#include <QDebug>                                  // For debug output
#include <QIcon>                                   // For icons
#include <QInputDialog>                            // For input dialog
#include <QContextMenuEvent>                       // For context menu events
#include <QVBoxLayout>                             // For vertical layout
#include <QMouseEvent>                             // For mouse events
#include <QMimeData>                               // For MIME data handling
#include <QDrag>                                   // For drag operations
#include <QDragEnterEvent>                         // For drag enter events
#include <QDragMoveEvent>                          // For drag move events
#include <QDropEvent>                              // For drop events
#include <GUI/Hierarchytree/contextmenu.h>         // For context menu
#include <QApplication>                            // For application instance

// %%% Constructor %%%
/* Initialize hierarchy tree widget */
HierarchyTree::HierarchyTree(QWidget *parent)
    : QWidget(parent)
{
    // Create tree widget
    tree = new QTreeWidget(this);
    tree->setHeaderLabel("Hierarchy");
    tree->setEditTriggers(QAbstractItemView::DoubleClicked);
    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    tree->expandAll();

    // Enable drag and drop
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree->setDragDropMode(QAbstractItemView::DragDrop);
    tree->setDragEnabled(true);
    tree->viewport()->setAcceptDrops(true);
    tree->setDropIndicatorShown(true);
    tree->setDefaultDropAction(Qt::CopyAction);

    // Connect item clicked signal
    connect(tree, &QTreeWidget::itemClicked, this, [=](QTreeWidgetItem* item, int column) {
        Q_UNUSED(column);
        QVariantMap data = item->data(0, Qt::UserRole).toMap();
        QString type;
        if (data["type"].type() == QVariant::Map) {
            QVariantMap typeData = data["type"].toMap();
            if (typeData.contains("type") && typeData["type"].toString() == "option") {
                type = "profile";
                data["type"] = type;
            } else {
                qWarning() << "Invalid nested type structure in itemClicked:" << data["type"];
                return;
            }
        } else {
            type = data["type"].toString();
        }
        if (type == "component") {
            data["entityID"] = data["parentId"];
            QString componentName = data["name"].toString();
            if (componentName.compare("Trajectories", Qt::CaseInsensitive) == 0) {
                componentName = "trajectory";
            } else {
                componentName = componentName.toLower();
            }
            data["componentName"] = componentName;
        }
        emit itemSelected(data);
        qDebug() << "Item clicked: name=" << data["name"] << "type=" << type
                 << "entityID=" << data["entityID"] << "componentName=" << data["componentName"];
    });

    // Connect item pressed signal
    connect(tree, &QTreeWidget::itemPressed, this, [=](QTreeWidgetItem *item, int col) {
        Q_UNUSED(col);
        QVariantMap storedData = item->data(0, Qt::UserRole).toMap();
        QString type;
        if (storedData["type"].type() == QVariant::Map) {
            QVariantMap typeData = storedData["type"].toMap();
            if (typeData.contains("type") && typeData["type"].toString() == "option") {
                type = "profile";
            } else {
                qWarning() << "Invalid nested type structure in itemPressed:" << storedData["type"];
                return;
            }
        } else {
            type = storedData["type"].toString();
        }
        if (type == "entity") {
            qDebug() << "Emitting copyItemRequested: type=" << type
                     << "name=" << storedData["name"].toString()
                     << "ID=" << storedData["ID"].toString();
            emit copyItemRequested(storedData);
        } else {
            qDebug() << "Pressed (ignored for copy): type=" << type
                     << "name=" << storedData["name"].toString()
                     << "ID=" << storedData["ID"].toString();
        }
    });

    // Setup layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(tree);
    setLayout(layout);

    // Initialize context menu
    contextMenu = new ContextMenu(this);
    connect(tree, &QTreeWidget::customContextMenuRequested,
            this, &HierarchyTree::showContextMenu);
    connect(contextMenu, &ContextMenu::removeComponentRequested,
            this, &HierarchyTree::componentRemoved);
}

/* Destructor */
HierarchyTree::~HierarchyTree()
{
    // Delete context menu
    delete contextMenu;
    // Tree widget is deleted automatically as a child
}

/* Get tree widget */
QTreeWidget* HierarchyTree::getTreeWidget()
{
    // Return tree widget
    return tree;
}

// %%% Tree Item Management %%%
/* Add profile to tree */
void HierarchyTree::profileAdded(QString ID, QString profileName)
{
    // Create profile item
    QTreeWidgetItem *category = new QTreeWidgetItem(tree);
    category->setText(0, profileName);
    category->setIcon(0, QIcon(":/icons/images/category.png"));
    Items.insert(ID, category);

    // Set item data
    QVariantMap data;
    data["ID"] = ID;
    data["parentId"] = "";
    data["name"] = profileName;
    data["type"] = "profile";
    QVariantMap typeData;
    typeData["options"] = QStringList({"Platform", "Radio", "Sensor", "SpecialZone", "Weapon", "IFF", "Supply", "FixedPoints"});
    typeData["type"] = "option";
    typeData["value"] = profileName;
    data["type"] = typeData;
    category->setData(0, Qt::UserRole, data);
    category->setFlags(category->flags() & ~Qt::ItemIsDragEnabled);
    qDebug() << "Profile added to tree: ID=" << ID << "Name=" << profileName << "Data=" << data;
}

/* Add folder to tree */
void HierarchyTree::folderAdded(QString parentID, QString ID, QString folderName)
{
    // Create folder item
    QTreeWidgetItem *folder = new QTreeWidgetItem(Items[parentID]);
    folder->setText(0, folderName);
    folder->setIcon(0, QIcon(":/icons/images/folder.png"));
    Items.insert(ID, folder);

    // Set item data
    QVariantMap data;
    data["ID"] = ID;
    data["parentId"] = parentID;
    data["name"] = folderName;
    data["type"] = "folder";
    folder->setData(0, Qt::UserRole, data);
    folder->setFlags(folder->flags() & ~Qt::ItemIsDragEnabled);
    qDebug() << "Folder added to tree: ID=" << ID << "Name=" << folderName << "ParentID=" << parentID;
}

/* Add entity to tree */
void HierarchyTree::entityAdded(QString parentID, QString ID, QString entityName)
{
    // Create entity item
    QTreeWidgetItem *entity = new QTreeWidgetItem(Items[parentID]);
    entity->setText(0, entityName);
    entity->setIcon(0, QIcon(":/icons/images/entity.png"));
    Items.insert(ID, entity);

    // Set item data
    QVariantMap data;
    data["ID"] = ID;
    data["parentId"] = parentID;
    data["name"] = entityName;
    data["type"] = "entity";
    entity->setData(0, Qt::UserRole, data);
    entity->setFlags(entity->flags() | Qt::ItemIsDragEnabled);
    qDebug() << "Entity added to tree: ID=" << ID << "Name=" << entityName << "ParentID=" << parentID;
}

/* Add component to tree */
void HierarchyTree::componentAdded(QString parentID, QString componentName)
{
    // Validate parent ID
    if (!Items.contains(parentID)) {
        qWarning() << "Cannot add component: Parent ID" << parentID << "not found in Items";
        return;
    }
    // Create component item
    QTreeWidgetItem *component = new QTreeWidgetItem(Items[parentID]);
    component->setText(0, componentName);
    component->setIcon(0, QIcon(":/icons/images/component.png"));
    // Set item data
    QVariantMap data;
    data["ID"] = "";
    data["parentId"] = parentID;
    data["name"] = componentName;
    data["type"] = "component";
    component->setData(0, Qt::UserRole, data);
    qDebug() << "Component added to tree: Name=" << componentName << "ParentID=" << parentID;
}

/* Remove profile from tree */
void HierarchyTree::profileRemoved(QString ID)
{
    // Check if profile exists
    if (Items.contains(ID)) {
        delete Items[ID];
        Items.remove(ID);
        qDebug() << "Profile removed from tree: ID=" << ID;
    } else {
        qWarning() << "Cannot remove profile: ID" << ID << "not found in Items";
    }
}

/* Remove folder from tree */
void HierarchyTree::folderRemoved(QString ID)
{
    // Check if folder exists
    if (Items.contains(ID)) {
        delete Items[ID];
        Items.remove(ID);
        qDebug() << "Folder removed from tree: ID=" << ID;
    } else {
        qWarning() << "Cannot remove folder: ID" << ID << "not found in Items";
    }
}

/* Remove entity from tree */
void HierarchyTree::entityRemoved(QString ID)
{
    // Check if entity exists
    if (Items.contains(ID)) {
        delete Items[ID];
        Items.remove(ID);
        qDebug() << "Entity removed from tree: ID=" << ID;
    } else {
        qWarning() << "Cannot remove entity: ID" << ID << "not found in Items";
    }
}

/* Rename profile in tree */
void HierarchyTree::profileRenamed(QString ID, QString name)
{
    // Check if profile exists
    if (Items.contains(ID)) {
        Items[ID]->setText(0, name);
        qDebug() << "Profile renamed in tree: ID=" << ID << "New Name=" << name;
    } else {
        qWarning() << "Cannot rename profile: ID" << ID << "not found in Items";
    }
}

/* Rename folder in tree */
void HierarchyTree::folderRenamed(QString ID, QString name)
{
    // Check if folder exists
    if (Items.contains(ID)) {
        Items[ID]->setText(0, name);
        qDebug() << "Folder renamed in tree: ID=" << ID << "New Name=" << name;
    } else {
        qWarning() << "Cannot rename folder: ID" << ID << "not found in Items";
    }
}

/* Rename entity in tree */
void HierarchyTree::entityRenamed(QString ID, QString name)
{
    // Check if entity exists
    if (Items.contains(ID)) {
        Items[ID]->setText(0, name);
        qDebug() << "Entity renamed in tree: ID=" << ID << "New Name=" << name;
    } else {
        qWarning() << "Cannot rename entity: ID" << ID << "not found in Items";
    }
}

/* Show context menu */
void HierarchyTree::showContextMenu(const QPoint &pos)
{
    // Get item at position
    QTreeWidgetItem *item = tree->itemAt(pos);
    if (item) {
        QVariantMap data = item->data(0, Qt::UserRole).toMap();
        QString itemType = data["type"].toString();
        QString specificType = "";
        if (data.contains("type") && data["type"].type() == QVariant::Map) {
            QVariantMap typeData = data["type"].toMap();
            if (typeData.contains("value")) {
                specificType = typeData["value"].toString();
            }
        }
        // Setup and show context menu
        contextMenu->setupMenu(item);
        contextMenu->exec(tree->viewport()->mapToGlobal(pos));
    } else {
        qDebug() << "Context menu requested: No item at position" << pos;
    }
}

/* Handle context menu event */
void HierarchyTree::contextMenuEvent(QContextMenuEvent *event)
{
    // Show context menu at event position
    showContextMenu(event->pos());
}

/* Handle drag enter event */
void HierarchyTree::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept drag if MIME data is entity
    if (event->mimeData()->hasFormat("application/x-entity-data")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

/* Handle drag move event */
void HierarchyTree::dragMoveEvent(QDragMoveEvent *event)
{
    // Check MIME data
    if (event->mimeData()->hasFormat("application/x-entity-data")) {
        QTreeWidgetItem *item = tree->itemAt(event->pos());
        if (item) {
            QVariantMap data = item->data(0, Qt::UserRole).toMap();
            QString type;
            if (data["type"].type() == QVariant::Map) {
                QVariantMap typeData = data["type"].toMap();
                if (typeData.contains("type") && typeData["type"].toString() == "option") {
                    type = "profile";
                } else {
                    qWarning() << "Invalid nested type structure in dragMoveEvent:" << data["type"];
                    event->ignore();
                    return;
                }
            } else {
                type = data["type"].toString();
            }
            // Accept if target is profile or folder
            if (type == "profile" || type == "folder") {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

/* Handle drop event */
void HierarchyTree::dropEvent(QDropEvent *event)
{
    // Validate MIME data
    if (!event->mimeData()->hasFormat("application/x-entity-data")) {
        event->ignore();
        return;
    }
    // Get target item
    QTreeWidgetItem *targetItem = tree->itemAt(event->pos());
    if (!targetItem) {
        event->ignore();
        return;
    }
    // Get target data
    QVariantMap targetData = targetItem->data(0, Qt::UserRole).toMap();
    QString targetType;
    if (targetData["type"].type() == QVariant::Map) {
        QVariantMap typeData = targetData["type"].toMap();
        if (typeData.contains("type") && typeData["type"].toString() == "option") {
            targetType = "profile";
        } else {
            qWarning() << "Invalid nested type structure in dropEvent:" << targetData["type"];
            event->ignore();
            return;
        }
    } else {
        targetType = targetData["type"].toString();
    }
    // Validate target type
    if (targetType != "profile" && targetType != "folder") {
        event->ignore();
        return;
    }
    // Read source data
    QByteArray itemData = event->mimeData()->data("application/x-entity-data");
    QDataStream stream(&itemData, QIODevice::ReadOnly);
    QVariantMap sourceData;
    stream >> sourceData;
    // Validate source type
    if (sourceData["type"].toString() != "entity") {
        event->ignore();
        return;
    }
    // Emit drop signal
    emit itemDropped(sourceData, targetData);
    event->acceptProposedAction();
}

/* Remove component from tree */
void HierarchyTree::componentRemoved(QString entityID, QString componentName)
{
    // Check if entity exists
    if (Items.contains(entityID)) {
        QTreeWidgetItem *entityItem = Items[entityID];
        for (int i = 0; i < entityItem->childCount(); ++i) {
            QTreeWidgetItem *child = entityItem->child(i);
            QVariantMap data = child->data(0, Qt::UserRole).toMap();
            if (data["type"].toString() == "component" && data["name"].toString() == componentName) {
                delete child;
                qDebug() << "Component removed from tree: Name=" << componentName << "EntityID=" << entityID;
                break;
            }
        }
    } else {
        qWarning() << "Cannot remove component: Entity ID" << entityID << "not found in Items";
    }
    // Emit remove component signal
    emit removeComponentRequested(entityID, componentName);
}

/* Select entity by ID */
void HierarchyTree::selectEntityById(const QString& entityId)
{
    // Check if entity exists
    if (Items.contains(entityId)) {
        QTreeWidgetItem* item = Items[entityId];
        tree->setCurrentItem(item); // Select item
        tree->scrollToItem(item);   // Scroll to item
        qDebug() << "Selected entity in tree: ID=" << entityId;
    } else {
        qWarning() << "Cannot select entity: ID" << entityId << "not found in Items";
    }
}
