#include "hierarchytree.h"
#include <QDebug>
#include <QIcon>
#include <QInputDialog>
#include <QContextMenuEvent>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QMimeData>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <GUI/Hierarchytree/contextmenu.h>
#include <QMimeData>
#include <QApplication>

HierarchyTree::HierarchyTree(QWidget *parent)
    : QWidget(parent)
{
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

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(tree);
    setLayout(layout);

    contextMenu = new ContextMenu(this);
    connect(tree, &QTreeWidget::customContextMenuRequested,
            this, &HierarchyTree::showContextMenu);
    connect(contextMenu, &ContextMenu::removeComponentRequested,
            this, &HierarchyTree::componentRemoved);
}

HierarchyTree::~HierarchyTree()
{
    delete contextMenu;
    // tree is deleted automatically as a child of this widget
}

QTreeWidget* HierarchyTree::getTreeWidget() {
    return tree;
}


void HierarchyTree::profileAdded(QString ID, QString profileName) {
    QTreeWidgetItem *category = new QTreeWidgetItem(tree);
    category->setText(0, profileName);
    category->setIcon(0, QIcon(":/icons/images/category.png"));
    Items.insert(ID, category);

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
void HierarchyTree::folderAdded(QString parentID, QString ID, QString folderName) {
    QTreeWidgetItem *folder = new QTreeWidgetItem(Items[parentID]);
    folder->setText(0, folderName);
    folder->setIcon(0, QIcon(":/icons/images/folder.png"));
    Items.insert(ID, folder);

    QVariantMap data;
    data["ID"] = ID;
    data["parentId"] = parentID;
    data["name"] = folderName;
    data["type"] = "folder";
    folder->setData(0, Qt::UserRole, data);
    folder->setFlags(folder->flags() & ~Qt::ItemIsDragEnabled);
    qDebug() << "Folder added to tree: ID=" << ID << "Name=" << folderName << "ParentID=" << parentID;
}

void HierarchyTree::entityAdded(QString parentID, QString ID, QString entityName) {
    QTreeWidgetItem *entity = new QTreeWidgetItem(Items[parentID]);
    entity->setText(0, entityName);
    entity->setIcon(0, QIcon(":/icons/images/entity.png"));
    Items.insert(ID, entity);

    QVariantMap data;
    data["ID"] = ID;
    data["parentId"] = parentID;
    data["name"] = entityName;
    data["type"] = "entity";
    entity->setData(0, Qt::UserRole, data);
    entity->setFlags(entity->flags() | Qt::ItemIsDragEnabled);
    qDebug() << "Entity added to tree: ID=" << ID << "Name=" << entityName << "ParentID=" << parentID;
}

void HierarchyTree::componentAdded(QString parentID, QString componentName) {
    if (!Items.contains(parentID)) {
        qWarning() << "Cannot add component: Parent ID" << parentID << "not found in Items";
        return;
    }
    QTreeWidgetItem *component = new QTreeWidgetItem(Items[parentID]);
    component->setText(0, componentName);
    component->setIcon(0, QIcon(":/icons/images/component.png"));
    QVariantMap data;
    data["ID"] = "";
    data["parentId"] = parentID;
    data["name"] = componentName;
    data["type"] = "component";
    component->setData(0, Qt::UserRole, data);
    qDebug() << "Component added to tree: Name=" << componentName << "ParentID=" << parentID;
}

void HierarchyTree::profileRemoved(QString ID) {
    if (Items.contains(ID)) {
        delete Items[ID];
        Items.remove(ID);
        qDebug() << "Profile removed from tree: ID=" << ID;
    } else {
        qWarning() << "Cannot remove profile: ID" << ID << "not found in Items";
    }
}


void HierarchyTree::folderRemoved(QString ID) {
    if (Items.contains(ID)) {
        delete Items[ID];
        Items.remove(ID);
        qDebug() << "Folder removed from tree: ID=" << ID;
    } else {
        qWarning() << "Cannot remove folder: ID" << ID << "not found in Items";
    }
}

void HierarchyTree::entityRemoved(QString ID) {
    if (Items.contains(ID)) {
        delete Items[ID];
        Items.remove(ID);
        qDebug() << "Entity removed from tree: ID=" << ID;
    } else {
        qWarning() << "Cannot remove entity: ID" << ID << "not found in Items";
    }
}

void HierarchyTree::profileRenamed(QString ID, QString name) {
    if (Items.contains(ID)) {
        Items[ID]->setText(0, name);
        qDebug() << "Profile renamed in tree: ID=" << ID << "New Name=" << name;
    } else {
        qWarning() << "Cannot rename profile: ID" << ID << "not found in Items";
    }
}

void HierarchyTree::folderRenamed(QString ID, QString name) {
    if (Items.contains(ID)) {
        Items[ID]->setText(0, name);
        qDebug() << "Folder renamed in tree: ID=" << ID << "New Name=" << name;
    } else {
        qWarning() << "Cannot rename folder: ID" << ID << "not found in Items";
    }
}

void HierarchyTree::entityRenamed(QString ID, QString name) {
    if (Items.contains(ID)) {
        Items[ID]->setText(0, name);
        qDebug() << "Entity renamed in tree: ID=" << ID << "New Name=" << name;
    } else {
        qWarning() << "Cannot rename entity: ID" << ID << "not found in Items";
    }
}


void HierarchyTree::showContextMenu(const QPoint &pos) {
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

        contextMenu->setupMenu(item);
        contextMenu->exec(tree->viewport()->mapToGlobal(pos));
    } else {
        qDebug() << "Context menu requested: No item at position" << pos;
    }
}

void HierarchyTree::contextMenuEvent(QContextMenuEvent *event) {
    showContextMenu(event->pos());
}

void HierarchyTree::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-entity-data")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}


void HierarchyTree::dragMoveEvent(QDragMoveEvent *event)
{
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
            if (type == "profile" || type == "folder") {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}


void HierarchyTree::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasFormat("application/x-entity-data")) {
        event->ignore();
        return;
    }

    QTreeWidgetItem *targetItem = tree->itemAt(event->pos());
    if (!targetItem) {
        event->ignore();
        return;
    }

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

    if (targetType != "profile" && targetType != "folder") {
        event->ignore();
        return;
    }

    QByteArray itemData = event->mimeData()->data("application/x-entity-data");
    QDataStream stream(&itemData, QIODevice::ReadOnly);

    QVariantMap sourceData;
    stream >> sourceData;

    if (sourceData["type"].toString() != "entity") {
        event->ignore();
        return;
    }

    emit itemDropped(sourceData, targetData);
    event->acceptProposedAction();
}

void HierarchyTree::componentRemoved(QString entityID, QString componentName)
{
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
    emit removeComponentRequested(entityID, componentName);
}
void HierarchyTree::selectEntityById(const QString& entityId) {
    if (Items.contains(entityId)) {
        QTreeWidgetItem* item = Items[entityId];
        tree->setCurrentItem(item); // Select the item in the tree
        tree->scrollToItem(item);   // Ensure the item is visible
        qDebug() << "Selected entity in tree: ID=" << entityId;
    } else {
        qWarning() << "Cannot select entity: ID" << entityId << "not found in Items";
    }
}
