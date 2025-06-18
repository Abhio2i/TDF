
// #include "hierarchytree.h"
// #include <QDebug>
// #include <QIcon>
// #include <QInputDialog>
// #include <QContextMenuEvent>
// #include <QVBoxLayout>
// #include <QMouseEvent>
// #include <QMimeData>
// #include <QDrag>
// #include <QDragEnterEvent>
// #include <QDragMoveEvent>
// #include <QDropEvent>
// #include <GUI/Hierarchytree/contextmenu.h>
// #include <QMimeData>
// #include <QApplication>
// #include <QDragEnterEvent>

// HierarchyTree::HierarchyTree(QWidget *parent)
//     : QWidget(parent)
// {
//     tree = new QTreeWidget(this);
//     tree->setHeaderLabel("Hierarchy");
//     tree->setEditTriggers(QAbstractItemView::DoubleClicked);
//     tree->setContextMenuPolicy(Qt::CustomContextMenu);
//     tree->expandAll();

//     // Enable drag and drop
//     tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
//     tree->setDragDropMode(QAbstractItemView::DragDrop);
//     tree->setDragEnabled(true);
//     tree->viewport()->setAcceptDrops(true);
//     tree->setDropIndicatorShown(true);
//     tree->setDefaultDropAction(Qt::IgnoreAction);

//     // Install event filter for drop handling
//     tree->viewport()->installEventFilter(this);
//     connect(tree, &QTreeWidget::itemClicked, this, [=](QTreeWidgetItem* item, int column) {
//         Q_UNUSED(column);
//         QVariantMap data = item->data(0, Qt::UserRole).toMap();
//         emit itemSelected(data);
//     });
//     connect(tree, &QTreeWidget::itemPressed, this, [=](QTreeWidgetItem *item, int col){
//         QVariantMap storedData = item->data(0, Qt::UserRole).toMap();
//         emit copyItemRequested(storedData);
//         qDebug() << "Pressed:" << item->text(0);
//     });
//     QVBoxLayout *layout = new QVBoxLayout(this);
//     layout->addWidget(tree);
//     setLayout(layout);

//     contextMenu = new ContextMenu(this);
//     connect(tree, &QTreeWidget::customContextMenuRequested,
//             this, &HierarchyTree::showContextMenu);
//     connect(contextMenu, &ContextMenu::removeComponentRequested,
//             this, &HierarchyTree::componentRemoved);
// }

// HierarchyTree::~HierarchyTree()
// {
//     delete contextMenu;
//     delete tree;
// }

// QTreeWidget* HierarchyTree::getTreeWidget() {
//     return tree;
// }

// void HierarchyTree::profileAdded(QString ID, QString profileName) {
//     QTreeWidgetItem *category = new QTreeWidgetItem(tree);
//     category->setText(0, profileName);
//     category->setIcon(0, QIcon(":/icons/images/category.png"));
//     Items.insert(ID, category);

//     QVariantMap data;
//     data["ID"] = ID;
//     data["parentId"] = "";
//     data["name"] = profileName;
//     data["type"] = "profile";
//     category->setData(0, Qt::UserRole, data);
//     category->setFlags(category->flags() & ~Qt::ItemIsDragEnabled);
// }

// void HierarchyTree::folderAdded(QString parentID, QString ID, QString folderName) {
//     QTreeWidgetItem *folder = new QTreeWidgetItem(Items[parentID]);
//     folder->setText(0, folderName);
//     folder->setIcon(0, QIcon(":/icons/images/folder.png"));
//     Items.insert(ID, folder);

//     QVariantMap data;
//     data["ID"] = ID;
//     data["parentId"] = parentID;
//     data["name"] = folderName;
//     data["type"] = "folder";
//     folder->setData(0, Qt::UserRole, data);
//     folder->setFlags(folder->flags() & ~Qt::ItemIsDragEnabled);
// }

// void HierarchyTree::entityAdded(QString parentID, QString ID, QString entityName) {
//     QTreeWidgetItem *entity = new QTreeWidgetItem(Items[parentID]);
//     entity->setText(0, entityName);
//     entity->setIcon(0, QIcon(":/icons/images/entity.png"));
//     Items.insert(ID, entity);

//     QVariantMap data;
//     data["ID"] = ID;
//     data["parentId"] = parentID;
//     data["name"] = entityName;
//     data["type"] = "entity";
//     entity->setData(0, Qt::UserRole, data);
//     entity->setFlags(entity->flags() | Qt::ItemIsDragEnabled);
// }

// void HierarchyTree::componentAdded(QString parentID, QString componentName) {
//     QTreeWidgetItem *component = new QTreeWidgetItem(Items[parentID]);
//     component->setText(0, componentName);
//     component->setIcon(0, QIcon(":/icons/images/component.png"));
//     QVariantMap data;
//     data["ID"] = "";
//     data["parentId"] = parentID;
//     data["name"] = componentName;
//     data["type"] = "component";
//     component->setData(0, Qt::UserRole, data);
// }

// void HierarchyTree::profileRemoved(QString ID) {
//     delete Items[ID];
//     Items.remove(ID);
// }

// void HierarchyTree::folderRemoved(QString ID) {
//     delete Items[ID];
//     Items.remove(ID);
// }

// void HierarchyTree::entityRemoved(QString ID) {
//     delete Items[ID];
//     Items.remove(ID);
// }

// void HierarchyTree::profileRenamed(QString ID, QString name) {
//     if (Items.contains(ID)) {
//         Items[ID]->setText(0, name);
//     }
// }

// void HierarchyTree::folderRenamed(QString ID, QString name) {
//     if (Items.contains(ID)) {
//         Items[ID]->setText(0, name);
//     }
// }

// void HierarchyTree::entityRenamed(QString ID, QString name) {
//     if (Items.contains(ID)) {
//         Items[ID]->setText(0, name);
//     }
// }

// void HierarchyTree::showContextMenu(const QPoint &pos) {
//     QTreeWidgetItem *item = tree->itemAt(pos);
//     if (item) {
//         contextMenu->setupMenu(item);
//         contextMenu->exec(tree->viewport()->mapToGlobal(pos));
//     }
// }

// void HierarchyTree::contextMenuEvent(QContextMenuEvent *event) {
//     showContextMenu(event->pos());
// }
// bool HierarchyTree::eventFilter(QObject *watched, QEvent *event) {
//     if (watched == tree->viewport() && event->type() == QEvent::Drop) {
//         QDropEvent *dropEvent = static_cast<QDropEvent *>(event);
//         QPoint pos = dropEvent->position().toPoint();
//         QTreeWidgetItem *targetItem = tree->itemAt(pos);

//         if (targetItem) {
//             QVariantMap storedData = targetItem->data(0, Qt::UserRole).toMap();
//             emit pasteItemRequested(storedData);
//         }

//         dropEvent->setDropAction(Qt::IgnoreAction);
//         dropEvent->accept();
//         return true;
//     }
//     return QWidget::eventFilter(watched, event);
// }
// void HierarchyTree::componentRemoved(QString entityID, QString componentName)
// {
//     if (Items.contains(entityID)) {
//         QTreeWidgetItem *entityItem = Items[entityID];
//         for (int i = 0; i < entityItem->childCount(); ++i) {
//             QTreeWidgetItem *child = entityItem->child(i);
//             QVariantMap data = child->data(0, Qt::UserRole).toMap();
//             if (data["type"].toString() == "component" && data["name"].toString() == componentName) {
//                 delete child; // Remove the component from the tree
//                 break;
//             }
//         }
//     }
//     emit removeComponentRequested(entityID, componentName);
// }



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
#include <QDragEnterEvent>

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
    tree->setDefaultDropAction(Qt::IgnoreAction);

    // Install event filter for drop handling
    tree->viewport()->installEventFilter(this);
    connect(tree, &QTreeWidget::itemClicked, this, [=](QTreeWidgetItem* item, int column) {
        Q_UNUSED(column);
        QVariantMap data = item->data(0, Qt::UserRole).toMap();
        if (data["type"].toString() == "component") {
            // For components, include entity ID and component name (match JSON key)
            data["entityID"] = data["parentId"];
            QString componentName = data["name"].toString();
            // Map display name to JSON key
            if (componentName.compare("Trajectories", Qt::CaseInsensitive) == 0) {
                componentName = "trajectory";
            } else {
                componentName = componentName.toLower();
            }
            data["componentName"] = componentName;
        }
        emit itemSelected(data);
        qDebug() << "Item clicked: name=" << data["name"] << "type=" << data["type"] << "entityID=" << data["entityID"] << "componentName=" << data["componentName"];
    });
    connect(tree, &QTreeWidget::itemPressed, this, [=](QTreeWidgetItem *item, int col){
        QVariantMap storedData = item->data(0, Qt::UserRole).toMap();
        emit copyItemRequested(storedData);
        qDebug() << "Pressed:" << item->text(0);
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
    delete tree;
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
    category->setData(0, Qt::UserRole, data);
    category->setFlags(category->flags() & ~Qt::ItemIsDragEnabled);
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
}

void HierarchyTree::componentAdded(QString parentID, QString componentName) {
    QTreeWidgetItem *component = new QTreeWidgetItem(Items[parentID]);
    component->setText(0, componentName);
    component->setIcon(0, QIcon(":/icons/images/component.png"));
    QVariantMap data;
    data["ID"] = "";
    data["parentId"] = parentID;
    data["name"] = componentName;
    data["type"] = "component";
    component->setData(0, Qt::UserRole, data);
}

void HierarchyTree::profileRemoved(QString ID) {
    delete Items[ID];
    Items.remove(ID);
}

void HierarchyTree::folderRemoved(QString ID) {
    delete Items[ID];
    Items.remove(ID);
}

void HierarchyTree::entityRemoved(QString ID) {
    delete Items[ID];
    Items.remove(ID);
}

void HierarchyTree::profileRenamed(QString ID, QString name) {
    if (Items.contains(ID)) {
        Items[ID]->setText(0, name);
    }
}

void HierarchyTree::folderRenamed(QString ID, QString name) {
    if (Items.contains(ID)) {
        Items[ID]->setText(0, name);
    }
}

void HierarchyTree::entityRenamed(QString ID, QString name) {
    if (Items.contains(ID)) {
        Items[ID]->setText(0, name);
    }
}

void HierarchyTree::showContextMenu(const QPoint &pos) {
    QTreeWidgetItem *item = tree->itemAt(pos);
    if (item) {
        contextMenu->setupMenu(item);
        contextMenu->exec(tree->viewport()->mapToGlobal(pos));
    }
}

void HierarchyTree::contextMenuEvent(QContextMenuEvent *event) {
    showContextMenu(event->pos());
}

bool HierarchyTree::eventFilter(QObject *watched, QEvent *event) {
    if (watched == tree->viewport() && event->type() == QEvent::Drop) {
        QDropEvent *dropEvent = static_cast<QDropEvent *>(event);
        QPoint pos = dropEvent->position().toPoint();
        QTreeWidgetItem *targetItem = tree->itemAt(pos);

        if (targetItem) {
            QVariantMap storedData = targetItem->data(0, Qt::UserRole).toMap();
            emit pasteItemRequested(storedData);
        }

        dropEvent->setDropAction(Qt::IgnoreAction);
        dropEvent->accept();
        return true;
    }
    return QWidget::eventFilter(watched, event);
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
                break;
            }
        }
    }
    emit removeComponentRequested(entityID, componentName);
}
