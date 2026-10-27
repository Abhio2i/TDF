#include "hierarchytree.h"
#include <QIcon>
#include <QInputDialog>
#include <QContextMenuEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QMouseEvent>
#include <QMimeData>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <GUI/Hierarchytree/contextmenu.h>
#include <QApplication>
#include <QMessageBox>
#include <GUI/Hierarchytree/addformationdialog.h>
#include <QFileInfo>
#include "hierarchy-styles.h"
#include "qheaderview.h"
#include "GUI/mainwindow.h"
#include <QPainter>
#include <QStyleOptionViewItem>
#include "GUI/dialogwindowutils.h"

namespace HIT = HierarchyItemType;
namespace {
class NoAutoRemoveTreeWidget : public QTreeWidget {
public:
    using QTreeWidget::QTreeWidget;

protected:
    void startDrag(Qt::DropActions supportedActions) override {
        Q_UNUSED(supportedActions);
        const QModelIndexList indexes = selectedIndexes();
        if (indexes.isEmpty())
            return;
        QMimeData *mimeData = model()->mimeData(indexes);
        if (!mimeData)
            return;
        QRect boundingRect;
        for (const QModelIndex &index : indexes)
            boundingRect = boundingRect.united(visualRect(index));
        QPixmap pixmap(boundingRect.size());
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        for (const QModelIndex &index : indexes) {
            QStyleOptionViewItem opt;
            opt.initFrom(this);
            opt.rect = visualRect(index).translated(-boundingRect.topLeft());
            opt.text = index.data(Qt::DisplayRole).toString();
            opt.icon = index.data(Qt::DecorationRole).value<QIcon>();
            opt.features |= QStyleOptionViewItem::HasDisplay;
            if (!opt.icon.isNull())
                opt.features |= QStyleOptionViewItem::HasDecoration;
            style()->drawControl(QStyle::CE_ItemViewItem, &opt, &painter, this);
        }
        painter.end();
        QDrag *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
        drag->exec(Qt::CopyAction, Qt::CopyAction);
    }
};
} // namespace

HierarchyTree::HierarchyTree(QWidget *parent)
    : QWidget(parent)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(5);
    filterLayout->setContentsMargins(5, 5, 5, 5);
    QWidget *filterContainer = new QWidget(this);
    filterContainer->setStyleSheet(HierarchyStyles::FilterLayout);
    filterContainer->setLayout(filterLayout);
    searchBar = new QLineEdit(this);
    searchBar->setPlaceholderText("Search entities...");
    searchBar->setClearButtonEnabled(true);
    searchBar->setFixedHeight(30);
    searchBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    searchBar->setStyleSheet(HierarchyStyles::SearchBar);
    profileFilterCombo = new QComboBox(this);
    profileFilterCombo->addItem("All Profiles");
    profileFilterCombo->setFixedHeight(30);
    profileFilterCombo->setFixedWidth(110);
    profileFilterCombo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    profileFilterCombo->setStyleSheet(HierarchyStyles::ProfileDropdown);
    filterLayout->addWidget(searchBar);
    filterLayout->addWidget(profileFilterCombo);
    mainLayout->addWidget(filterContainer);
    tree = new NoAutoRemoveTreeWidget(this);
    // tree = new QTreeWidget(this);
    tree->setHeaderLabel("Hierarchy");
    tree->setEditTriggers(QAbstractItemView::DoubleClicked);
    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    tree->expandAll();
    tree->setStyleSheet(HierarchyStyles::TreeWidget);
    tree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree->setDragEnabled(true);
    setAcceptDrops(true);
    QHeaderView* header = tree->header();
    header->setStyleSheet(HierarchyStyles::TreeHeader);
    mainLayout->addWidget(tree, 1);
    connect(searchBar, &QLineEdit::textChanged, this, &HierarchyTree::onSearchTextChanged);
    connect(profileFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HierarchyTree::onProfileFilterChanged);
    connect(tree, &QTreeWidget::itemClicked, this, [=](QTreeWidgetItem* item, int column) {
        Q_UNUSED(column);
        QVariantMap data = item->data(0, Qt::UserRole).toMap();
        QString type;
        if (data["type"].type() == QVariant::Map) {
            QVariantMap typeData = data["type"].toMap();
            if (typeData.contains("type") && typeData["type"].toString() == HIT::OptionTag) {
                type = HIT::ProfileItem;
                data["type"] = type;
            } else {
                return;
            }
        } else {
            type = data["type"].toString();
        }
        if (type == HIT::ComponentItem) {
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
    });
    connect(tree, &QTreeWidget::itemSelectionChanged, this, [=]() {
        QList<QTreeWidgetItem*> selectedItems = tree->selectedItems();
        QList<QVariantMap> selectedDataList;
        for (QTreeWidgetItem* item : selectedItems) {
            QVariantMap data = item->data(0, Qt::UserRole).toMap();
            if (!data.isEmpty()) {
                selectedDataList.append(data);
            }
        }
        if (!selectedDataList.isEmpty()) {
            emit itemsSelected(selectedDataList);
        }
    });
    contextMenu = new ContextMenu(this);
    connect(tree, &QTreeWidget::customContextMenuRequested,
            this, &HierarchyTree::showContextMenu);
    connect(contextMenu, &ContextMenu::removeComponentRequested,
            this, &HierarchyTree::componentRemoved);
    connect(contextMenu, &ContextMenu::selectFormationWithEntityRequested,
            this, &HierarchyTree::formationSelectWithEntity, Qt::UniqueConnection);
}

HierarchyTree::~HierarchyTree()
{
    delete contextMenu;
}

QStringList HierarchyTree::getAllProfileNames() const
{
    QStringList profiles;
    for (const QString& profileName : profileMap.values()) {
        profiles.append(profileName);
    }
    return profiles;
}

void HierarchyTree::updateProfileDropdown()
{
    QString currentFullName = profileFilterCombo->currentData(Qt::UserRole).toString();
    if (currentFullName.isEmpty()) {
        currentFullName = profileFilterCombo->currentText();
    }
    profileFilterCombo->clear();
    profileFilterCombo->addItem("All Profiles");
    profileFilterCombo->setItemData(0, "All Profiles", Qt::UserRole);
    QStringList profiles = getAllProfileNames();
    profiles.sort();
    for (const QString& profile : profiles) {
        profileFilterCombo->addItem(profile);
        int lastIndex = profileFilterCombo->count() - 1;
        profileFilterCombo->setItemData(lastIndex, profile, Qt::UserRole);
    }
    for (int i = 0; i < profileFilterCombo->count(); ++i) {
        QString fullName = profileFilterCombo->itemData(i, Qt::UserRole).toString();
        if (fullName == currentFullName) {
            profileFilterCombo->setCurrentIndex(i);
            break;
        }
    }
}

void HierarchyTree::onSearchTextChanged(const QString& text)
{
    QString profileFilter = "All Profiles";
    if (profileFilterCombo->currentIndex() > 0) {
        profileFilter = profileFilterCombo->currentData(Qt::UserRole).toString();
        if (profileFilter.isEmpty()) {
            profileFilter = profileFilterCombo->currentText();
        }
    }
    filterHierarchy(profileFilter, text);
    emit searchFilterChanged(text);
}

void HierarchyTree::onProfileFilterChanged(int index)
{
    Q_UNUSED(index);
    QString profileFilter = "All Profiles";
    if (profileFilterCombo->currentIndex() > 0) {
        profileFilter = profileFilterCombo->currentData(Qt::UserRole).toString();
        if (profileFilter.isEmpty()) {
            profileFilter = profileFilterCombo->currentText();
        }
    }
    filterHierarchy(profileFilter, searchBar->text());
    emit profileFilterChanged(profileFilter);
}

void HierarchyTree::filterHierarchy(const QString& profileFilter, const QString& searchText)
{
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* topItem = tree->topLevelItem(i);
        applyFilters(topItem, profileFilter, searchText);
    }
}
void HierarchyTree::applyFilters(QTreeWidgetItem* item, const QString& profileFilter, const QString& searchText)
{
    if (!item) return;

    bool shouldShow = true;
    if (profileFilter != "All Profiles" && !profileFilter.isEmpty()) {
        shouldShow = itemBelongsToProfile(item, profileFilter);
    }
    bool selfMatched = false;
    if (shouldShow && !searchText.isEmpty()) {
        selfMatched = itemMatchesSearch(item, searchText);
        if (!selfMatched) {
            bool childMatches = false;
            for (int i = 0; i < item->childCount(); ++i) {
                applyFilters(item->child(i), profileFilter, searchText);
                if (!item->child(i)->isHidden())
                    childMatches = true;
            }
            item->setHidden(!childMatches);
            return;
        }
    }
    item->setHidden(!shouldShow);
    if (selfMatched) {
        for (int i = 0; i < item->childCount(); ++i)
            applyFilters(item->child(i), profileFilter, QString());
    } else {
        for (int i = 0; i < item->childCount(); ++i)
            applyFilters(item->child(i), profileFilter, searchText);
    }
}

bool HierarchyTree::itemMatchesSearch(QTreeWidgetItem* item, const QString& searchText)
{
    if (!item) return false;
    QString itemText = item->text(0);
    QVariantMap data = item->data(0, Qt::UserRole).toMap();
    QString type = data["type"].toString();

    if (data["type"].type() == QVariant::Map) {
        QVariantMap typeData = data["type"].toMap();
        if (typeData.contains("type") && typeData["type"].toString() == HIT::OptionTag) {
            type = HIT::ProfileItem;
        }
    }

    if (type != HIT::EntityItem)
        return false;

    if (itemText.contains(searchText, Qt::CaseInsensitive))
        return true;

    auto normalize = [](QString s) {
        s = s.toLower();
        s.remove(QChar(' '));
        s.remove(QChar('-'));
        s.remove(QChar('_'));
        return s;
    };

    const QString normItem   = normalize(itemText);
    const QString normSearch = normalize(searchText);
    if (normSearch.isEmpty())
    return true;
    return normItem.contains(normSearch);
}

bool HierarchyTree::itemBelongsToProfile(QTreeWidgetItem* item, const QString& profileName)
{
    if (!item) return false;
    QVariantMap data = item->data(0, Qt::UserRole).toMap();
    if (data["type"].type() == QVariant::Map) {
        QVariantMap typeData = data["type"].toMap();
        if (typeData.contains("type") && typeData["type"].toString() == HIT::OptionTag) {
            return data["name"].toString() == profileName;
        }
    } else if (data["type"].toString() == HIT::ProfileItem) {
        return data["name"].toString() == profileName;
    }
    QTreeWidgetItem* parent = item->parent();
    while (parent) {
        QVariantMap parentData = parent->data(0, Qt::UserRole).toMap();
        if (parentData["type"].type() == QVariant::Map) {
            QVariantMap typeData = parentData["type"].toMap();
            if (typeData.contains("type") && typeData["type"].toString() == HIT::OptionTag) {
                return parentData["name"].toString() == profileName;
            }
        } else if (parentData["type"].toString() == HIT::ProfileItem) {
            return parentData["name"].toString() == profileName;
        }
        parent = parent->parent();
    }
    return false;
}

void HierarchyTree::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_C) {
            QList<QVariantMap> selectedEntities = getSelectedEntities();
            if (!selectedEntities.isEmpty()) {
                QList<QVariantMap> nonFormationEntities;
                for (const QVariantMap& data : selectedEntities) {
                    QString id = data["ID"].toString();
                    bool isFormation = false;
                    if (contextMenu && contextMenu->m_hierarchy) {
                        auto it = contextMenu->m_hierarchy->Entities.find(id.toStdString());
                        if (it != contextMenu->m_hierarchy->Entities.end()) {
                            isFormation = dynamic_cast<Formation*>(it->second) != nullptr;
                        }
                    }
                    if (!isFormation) {
                        nonFormationEntities.append(data);
                    }
                }
                if (!nonFormationEntities.isEmpty()) {
                    copiedItems = nonFormationEntities;
                    if (contextMenu) contextMenu->m_copiedItems = nonFormationEntities;
                    emit copyItemsRequested(nonFormationEntities);
                }
            }
            event->accept();
            return;
        }
        else if (event->key() == Qt::Key_V) {
            QList<QTreeWidgetItem*> selectedItems = tree->selectedItems();
            if (!selectedItems.isEmpty() && !copiedItems.isEmpty()) {
                QTreeWidgetItem* targetItem = selectedItems.first();
                QVariantMap targetData = targetItem->data(0, Qt::UserRole).toMap();
                emit pasteItemsRequested(targetData, copiedItems);
            }
            event->accept();
            return;
        }
    }
    else if (event->key() == Qt::Key_Delete) {
        QList<QTreeWidgetItem*> selectedItems = tree->selectedItems();
        if (selectedItems.isEmpty()) {
            event->accept();
            return;
        }
        bool hasValidItems = false;
        for (QTreeWidgetItem* item : selectedItems) {
            QVariantMap data = item->data(0, Qt::UserRole).toMap();
            QString type = data["type"].toString();
            if (type == HIT::EntityItem || type == HIT::FolderItem || type == HIT::SubComponentItem) {
                hasValidItems = true;
                break;
            }
        }
        if (!hasValidItems) {
            event->accept();
            return;
        }
        if (QMessageBox::question(this, "Confirm Delete",
                                  "Are you sure you want to delete the selected items?",
                                  QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            event->accept();
            return;
        }
        for (QTreeWidgetItem* item : selectedItems) {
            if (!item) continue;
            QVariantMap data = item->data(0, Qt::UserRole).toMap();
            QString type     = data["type"].toString();
            QString id       = data["ID"].toString();
            QString parentId = data["parentId"].toString();
            QString name     = data["name"].toString();
            if (type == HIT::SubComponentItem && contextMenu) {
                emit contextMenu->removeSubComponentRequested(parentId, id, name);
            }
        }
        removeSelectedEntities();
        event->accept();
        return;
    }
    else if (event->key() == Qt::Key_F2) {
        QList<QTreeWidgetItem*> selectedItems = tree->selectedItems();
        if (selectedItems.size() == 1) {
            QTreeWidgetItem* item = selectedItems.first();
            QVariantMap data = item->data(0, Qt::UserRole).toMap();
            QString currentName = data["name"].toString();
            QString type = data["type"].toString();
            if (type == HIT::ComponentItem) {
                event->accept();
                return;
            }
            QInputDialog dialog(this);
            makeDialogResizable(&dialog);
            dialog.setWindowTitle("Rename");
            dialog.setLabelText("Enter New Name:");
            dialog.setTextValue(currentName);
            if (dialog.exec() == QDialog::Accepted) {
                QString newName = dialog.textValue();
                if (!newName.trimmed().isEmpty()) {
                    QVariantMap modifiedData = data;
                    modifiedData["name"] = newName;
                    emit contextMenu->renameItemRequested(modifiedData);
                }
            }
        }
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

QTreeWidget* HierarchyTree::getTreeWidget()
{
    return tree;
}

void HierarchyTree::profileAdded(QString ID, QString profileName)
{
    profileMap[ID] = profileName;
    updateProfileDropdown();
    QTreeWidgetItem *category = new QTreeWidgetItem(tree);
    category->setText(0, profileName);
    category->setIcon(0, QIcon(":/icons/images/category.png"));
    Items.insert(ID, category);
    QVariantMap data;
    data["ID"] = ID;
    data["parentId"] = "";
    data["name"] = profileName;
    QVariantMap typeData;
    typeData["options"] = QStringList({"Platform", "Radio", "Sensor", "SpecialZone", "Weapon", "IFF", "Supply", "FixedPoints"});
    typeData["type"] = HIT::OptionTag;
    typeData["value"] = profileName;
    data["type"] = typeData;
    category->setData(0, Qt::UserRole, data);
    category->setFlags(category->flags() & ~Qt::ItemIsDragEnabled);
}

void HierarchyTree::folderAdded(QString parentID, QString ID, QString folderName)
{
    if (!Items.contains(parentID)) {
        qWarning() << "HierarchyTree::folderAdded: unknown parentID" << parentID;
        return;
    }
    QTreeWidgetItem *folder = new QTreeWidgetItem(Items[parentID]);
    folder->setText(0, folderName);
    folder->setIcon(0, QIcon(":/icons/images/folder.png"));
    Items.insert(ID, folder);
    QVariantMap data;
    data["ID"] = ID;
    data["parentId"] = parentID;
    data["name"] = folderName;
    data["type"] = HIT::FolderItem;
    folder->setData(0, Qt::UserRole, data);
    folder->setFlags(folder->flags() & ~Qt::ItemIsDragEnabled);
}

void HierarchyTree::entityAdded(QString parentID, QString ID, QString entityName)
{
    if (!Items.contains(parentID)) {
        qWarning() << "HierarchyTree::entityAdded: unknown parentID" << parentID;
        return;
    }
    QTreeWidgetItem *entity = new QTreeWidgetItem(Items[parentID]);
    entity->setText(0, entityName);
    entity->setIcon(0, QIcon(":/icons/images/entity.png"));
    Items.insert(ID, entity);
    QVariantMap data;
    data["ID"] = ID;
    data["parentId"] = parentID;
    data["name"] = entityName;
    data["type"] = HIT::EntityItem;
    entity->setData(0, Qt::UserRole, data);
    entity->setFlags(entity->flags() | Qt::ItemIsDragEnabled);
}

// void HierarchyTree::componentAdded(QString parentID, QString ID, QString componentName)
// {
//     if (!Items.contains(parentID)) {
//         return;
//     }
//     QTreeWidgetItem *parentItem = Items[parentID];
//     if (!parentItem || parentItem->treeWidget() != tree || !isItemInTree(parentItem)) {
//         Items.remove(parentID);
//         return;
//     }
//     QString displayName = componentName;
//     if (!displayName.isEmpty()) {
//         displayName[0] = displayName[0].toUpper();
//     }
//     QTreeWidgetItem *component = new QTreeWidgetItem(parentItem);
//     component->setText(0, displayName);
//     component->setIcon(0, QIcon(":/icons/images/component.png"));
//     Items.insert(ID, component);
//     QVariantMap data;
//     data["ID"] = ID;
//     data["parentId"] = parentID;
//     data["name"] = componentName;
//     data["type"] = HIT::ComponentItem;
//     component->setData(0, Qt::UserRole, data);
//     component->setFlags(component->flags() & ~Qt::ItemIsDragEnabled);
// }

//fix by yogesh
void HierarchyTree::componentAdded(QString parentID, QString ID, QString componentName)
{
    QTreeWidgetItem *parentItem = Items[parentID];

    // 1. Parent valid hai ya nahi
    if (!parentItem ||
        parentItem->treeWidget() != tree ||
        !isItemInTree(parentItem)) {

        Items.remove(parentID);
        return;
    }

    // 2. Check karo ki ye component already tree mein hai ya nahi
    for (int i = 0; i < parentItem->childCount(); ++i) {

        QTreeWidgetItem *child = parentItem->child(i);

        QVariantMap data =
            child->data(0, Qt::UserRole).toMap();

        if (data["type"].toString() == HIT::ComponentItem &&
            data["ID"].toString() == ID) {

            // Component already exists → duplicate mat banao
            return;
        }
    }
    QString displayName = componentName;
    if (!displayName.isEmpty()) {
        displayName[0] = displayName[0].toUpper();
    }
    QTreeWidgetItem *component = new QTreeWidgetItem(parentItem);
    component->setText(0, displayName);
    component->setIcon(0, QIcon(":/icons/images/component.png"));
    Items.insert(ID, component);
    QVariantMap data;
    data["ID"] = ID;
    data["parentId"] = parentID;
    data["name"] = componentName;
    data["type"] = HIT::ComponentItem;
    component->setData(0, Qt::UserRole, data);
    component->setFlags(component->flags() & ~Qt::ItemIsDragEnabled);
}



void HierarchyTree::subComponentAdded(QString parentID, QString ID, QString subComponentName)
{
    if (!Items.contains(parentID)) {
        return;
    }
    QString displayName = subComponentName;
    if (!displayName.isEmpty()) {
        displayName[0] = displayName[0].toUpper();
    }
    QTreeWidgetItem *subcomponent = new QTreeWidgetItem(Items[parentID]);
    subcomponent->setText(0, displayName);
    subcomponent->setIcon(0, QIcon(":/icons/images/subcomponent.png"));
    Items.insert(ID, subcomponent);
    QVariantMap data;
    data["ID"] = ID;
    data["parentId"] = parentID;
    data["name"] = subComponentName;
    data["type"] = HIT::SubComponentItem;
    subcomponent->setData(0, Qt::UserRole, data);
    subcomponent->setFlags(subcomponent->flags() & ~Qt::ItemIsDragEnabled);
}

void HierarchyTree::profileRemoved(QString ID)
{
    if (Items.contains(ID)) {
        profileMap.remove(ID);
        updateProfileDropdown();
        delete Items[ID];
        Items.remove(ID);
    }
}

void HierarchyTree::folderRemoved(QString ID)
{
    if (Items.contains(ID)) {
        delete Items[ID];
        Items.remove(ID);
    }
}

//fix by yogesh
void HierarchyTree::removeItemFromMapRecursively(QTreeWidgetItem* item)
{
    if (!item)
        return;

    // First remove all children from Items
    for (int i = 0; i < item->childCount(); ++i) {
        removeItemFromMapRecursively(item->child(i));
    }

    // Remove this item's pointer from Items
    for (auto it = Items.begin(); it != Items.end(); ) {
        if (it.value() == item) {
            it = Items.erase(it);
        } else {
            ++it;
        }
    }
}



// void HierarchyTree::entityRemoved(QString ID)
// {
//     if (Items.contains(ID)) {
//         delete Items[ID];
//         Items.remove(ID);
//     }
//     emit entityRemovedNotify(ID);
// }



//fix by yogesh
void HierarchyTree::entityRemoved(QString ID)
{
    auto it = Items.find(ID);

    if (it != Items.end()) {
        QTreeWidgetItem *item = it.value();

        removeItemFromMapRecursively(item);

        delete item;
    }

    emit entityRemovedNotify(ID);
}



// void HierarchyTree::componentRemoved(QString entityID, QString componentName)
// {
//     if (Items.contains(entityID)) {
//         QTreeWidgetItem *entityItem = Items[entityID];

//         if (!entityItem) return;
//         for (int i = 0; i < entityItem->childCount(); ++i) {
//             QTreeWidgetItem *child = entityItem->child(i);
//             if (!child) continue;
//             QVariantMap data = child->data(0, Qt::UserRole).toMap();

//             if (data["type"].toString() == HIT::ComponentItem && data["name"].toString() == componentName) {
//                 QString keyToRemove = Items.key(child);
//                 if (!keyToRemove.isEmpty()) {
//                     Items.remove(keyToRemove);
//                 }
//                 entityItem->removeChild(child);
//                 delete child;
//                 break;

//             }
//         }
//     }
//     emit removeComponentRequested(entityID, componentName);
// }


//fix by yogesh
void HierarchyTree::componentRemoved(QString entityID, QString componentName)
{
    if (Items.contains(entityID)) {

        QTreeWidgetItem *entityItem = Items[entityID];

        for (int i = 0; i < entityItem->childCount(); ++i) {

            QTreeWidgetItem *child = entityItem->child(i);

            QVariantMap data =
                child->data(0, Qt::UserRole).toMap();

            if (data["type"].toString() == HIT::ComponentItem &&
                data["name"].toString() == componentName) {

                removeItemFromMapRecursively(child);

                delete child;

                break;
            }
        }
    }

    emit removeComponentRequested(entityID, componentName);
}







// void HierarchyTree::subComponentRemoved(QString CompID,QString subCompID,QString componentName)
// {
//     if (!Items.contains(subCompID) || !Items.contains(CompID)) {
//         return;
//     }
//     QTreeWidgetItem *item = Items[subCompID];
//     QTreeWidgetItem *parentItem = Items[CompID];

//     Items.remove(subCompID);
//     if (item && item->parent() == parentItem) {
//         parentItem->removeChild(item);
//         delete item;
//     }
// }


//fix by yogesh.
void HierarchyTree::subComponentRemoved(
    QString CompID,
    QString subCompID,
    QString componentName)
{
    auto it = Items.find(subCompID);
    if (it != Items.end()) {
        QTreeWidgetItem *item = it.value();
        removeItemFromMapRecursively(item);
        delete item;
    }
}

void HierarchyTree::profileRenamed(QString ID, QString name)
{
    if (Items.contains(ID)) {
        if (profileMap.contains(ID)) {
            profileMap[ID] = name;
            updateProfileDropdown();
        }
        Items[ID]->setText(0, name);
        QVariantMap data = Items[ID]->data(0, Qt::UserRole).toMap();
        data["name"] = name;
        if (data["type"].type() == QVariant::Map) {
            QVariantMap typeData = data["type"].toMap();
            typeData["value"] = name;
            data["type"] = typeData;
        }
        Items[ID]->setData(0, Qt::UserRole, data);
    }
}

void HierarchyTree::folderRenamed(QString ID, QString name)
{
    if (Items.contains(ID)) {
        Items[ID]->setText(0, name);
        QVariantMap data = Items[ID]->data(0, Qt::UserRole).toMap();
        data["name"] = name;
        Items[ID]->setData(0, Qt::UserRole, data);
    }
}

void HierarchyTree::entityRenamed(QString ID, QString name)
{
    if (Items.contains(ID)) {
        Items[ID]->setText(0, name);
        QVariantMap data = Items[ID]->data(0, Qt::UserRole).toMap();
        data["name"] = name;
        Items[ID]->setData(0, Qt::UserRole, data);
    }
}

ContextMenu *HierarchyTree::getContextMenu() const { return contextMenu; }

QString HierarchyTree::multiSelectMenuStyleSheet() const
{
    return R"(
    QMenu {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        font-size: 12px;
        padding: 4px 0;
    }
    QMenu::item {
        padding: 6px 20px 6px 8px;
        color: white;
        background-color: transparent;
        border: none;
        margin: 1px 4px;
    }
    QMenu::item:selected {
        background-color: #0078D4;
        color: white;
        border-radius: 2px;
    }
    QMenu::icon {
        margin-left: 4px;
        margin-right: 4px;
    }
    QMenu::item:disabled {
        color: #666666;
    }
    QMenu::separator {
        height: 1px;
        background-color: #27446d;
        margin: 4px 0;
    }
)";
}

void HierarchyTree::showContextMenu(const QPoint &pos)
{
    if (islib) return;
    QList<QTreeWidgetItem*> selectedItems = tree->selectedItems();

    if (selectedItems.size() > 1) {
        QMenu contextMenu(this);
        contextMenu.setStyleSheet(multiSelectMenuStyleSheet());

        bool allArePlatformEntities = true;
        bool allAreSubComponents = true;
        bool allAreComponents = true;
        QList<QVariantMap> selectedEntities;
        QList<QVariantMap> selectedSubComponents;

        for (QTreeWidgetItem* item : selectedItems) {
            QVariantMap data = item->data(0, Qt::UserRole).toMap();
            QString type = data["type"].toString();

            if (type == HIT::SubComponentItem) {
                selectedSubComponents.append(data);
            } else {
                allAreSubComponents = false;
            }
            if (type != HIT::ComponentItem) {
                allAreComponents = false;
            }
            if (type == HIT::EntityItem) {
                QString parentId = data["parentId"].toString();
                if (Items.contains(parentId)) {
                    QVariantMap parentData = Items[parentId]->data(0, Qt::UserRole).toMap();
                    if (parentData["type"].type() == QVariant::Map) {
                        QVariantMap typeData = parentData["type"].toMap();
                        if (typeData.contains("type") && typeData["type"].toString() == HIT::OptionTag
                            && typeData.contains("value")
                            && typeData["value"].toString() == "Platform") {
                            selectedEntities.append(data);
                            continue;
                        }
                    }
                    else if (parentData["type"].toString() == HIT::FolderItem) {
                        QString grandParentId = parentData["parentId"].toString();
                        if (Items.contains(grandParentId)) {
                            QVariantMap gpData = Items[grandParentId]->data(0, Qt::UserRole).toMap();
                            if (gpData["type"].type() == QVariant::Map) {
                                QVariantMap gpTypeData = gpData["type"].toMap();
                                if (gpTypeData.contains("type") && gpTypeData["type"].toString() == HIT::OptionTag
                                    && gpTypeData.contains("value")
                                    && gpTypeData["value"].toString() == "Platform") {
                                    selectedEntities.append(data);
                                    continue;
                                }
                            }
                        }
                    }
                }
            }
            allArePlatformEntities = false;
        }
        QString editorCtx = getCurrentEditorContext();
        QAction *copyAction = nullptr;
        if (!allAreSubComponents) {
            bool hasFormation = false;
            if (this->contextMenu && this->contextMenu->m_hierarchy) {
                for (QTreeWidgetItem* item : selectedItems) {
                    QVariantMap data = item->data(0, Qt::UserRole).toMap();
                    if (data["type"].toString() == HIT::EntityItem) {
                        QString id = data["ID"].toString();
                        auto it = this->contextMenu->m_hierarchy->Entities.find(id.toStdString());
                        if (it != this->contextMenu->m_hierarchy->Entities.end()
                            && dynamic_cast<Formation*>(it->second) != nullptr) {
                            hasFormation = true;
                            break;
                        }
                    }
                }
            }
            if (!hasFormation) {
                copyAction = contextMenu.addAction(QIcon(":/icons/images/copy.png"), "Copy");
            }
        }
        QAction *deleteAction = nullptr;
        if (!allAreComponents) {
            deleteAction = contextMenu.addAction(QIcon(":/icons/images/delete.png"), "Delete");
        }
        QAction *editSubCompAction = nullptr;
        if (allAreSubComponents && !selectedSubComponents.isEmpty()) {
            bool allAreWeapons = false;
            if (this->contextMenu && this->contextMenu->m_hierarchy) {
                allAreWeapons = true;
                for (const QVariantMap& data : selectedSubComponents) {
                    QString subID = data["ID"].toString();
                    auto it = this->contextMenu->m_hierarchy->Weapons.find(subID.toStdString());
                    if (it == this->contextMenu->m_hierarchy->Weapons.end()) {
                        allAreWeapons = false;
                        break;
                    }
                }
            }
            if (allAreWeapons && editorCtx != "database") {
                editSubCompAction = contextMenu.addAction(QIcon(":/icons/images/edit.png"), "Edit");
            }
        }
        QAction *editEntitiesAction = nullptr;
        if (allArePlatformEntities && !selectedEntities.isEmpty() && editorCtx != "database") {
            editEntitiesAction = contextMenu.addAction(QIcon(":/icons/images/edit.png"), "Edit");
        }
        if (editEntitiesAction) {
            connect(editEntitiesAction, &QAction::triggered, this, [=]() {
                if (this->contextMenu) {
                    emit this->contextMenu->editMultipleEntitiesRequested(selectedEntities);
                }
            });
        }

        QAction *setActiveAction    = nullptr;
        QAction *setInactiveAction  = nullptr;
        QMenu   *addCompMenu        = nullptr;
        QAction *addWeaponAction    = nullptr;
        QAction *addSensorAction    = nullptr;
        QAction *addIFFAction       = nullptr;
        QAction *addRadioAction     = nullptr;
        QAction *addFormationAction = nullptr;

        if (allArePlatformEntities) {
            setActiveAction   = contextMenu.addAction(QIcon(":/icons/images/enable.png"),  "Set Active");
            setInactiveAction = contextMenu.addAction(QIcon(":/icons/images/disable.png"), "Set Inactive");
            addCompMenu = contextMenu.addMenu(QIcon(":/icons/images/add.png"), "Add");
            addCompMenu->setStyleSheet(contextMenu.styleSheet());
            addWeaponAction = addCompMenu->addAction(QIcon(":/icons/images/bio-weapon.png"),     "Add Weapon");
            addSensorAction = addCompMenu->addAction(QIcon(":/icons/images/database (1).png"),   "Add Sensor");
            addIFFAction    = addCompMenu->addAction(QIcon(":/icons/images/identification.png"), "Add IFF");
            addRadioAction  = addCompMenu->addAction(QIcon(":/icons/images/radio.png"),          "Add Radio");
            if (selectedEntities.size() >= 2) {
                addFormationAction = addCompMenu->addAction(QIcon(":/icons/images/add.png"), "Add Formation");
            }
            QMenu *setTeamSubMenu = contextMenu.addMenu(QIcon(":/icons/images/seteam.png"), "Set Team");
            setTeamSubMenu->setStyleSheet(contextMenu.styleSheet());
            const QStringList teamsList = {"RedTeam", "BlueTeam", "GreenTeam", "YellowTeam",
                                           "GreyTeam", "AlphaTeam", "BetaTeam", "GammaTeam"};
            for (const QString& team : teamsList) {
                QAction *teamAction = setTeamSubMenu->addAction(team);
                connect(teamAction, &QAction::triggered, this, [=]() {
                    emit addTeamToEntitiesRequested(getSelectedEntities(), team);
                });
            }
            if (editorCtx == "database") {
                QMenu *setCategorySubMenu = contextMenu.addMenu(QIcon(":/icons/images/set.png"), "Set Category");
                setCategorySubMenu->setStyleSheet(contextMenu.styleSheet());
                QMap<QString, QStringList> categoryMap = {
                    {"Air",    QStringList{"Aircraft", "Helicopter", "UAV", "FighterAircraft", "TransportAircraft"}},
                    {"Ground", QStringList{"Tank", "GroundRadar", "Human", "BattleVehicle", "MilitaryGroundVehicle", "TransportVehicle"}},
                    {"Marine", QStringList{"Ship", "Frigate", "Submarine", "AircraftCarrier", "GuidedMissileCruiser", "Destroyer", "SmallBoat"}}
                };
                for (auto it = categoryMap.begin(); it != categoryMap.end(); ++it) {
                    const QString category    = it.key();
                    const QStringList subCats = it.value();
                    QMenu* subCatMenu = setCategorySubMenu->addMenu(category);
                    subCatMenu->setStyleSheet(contextMenu.styleSheet());
                    for (const QString& subCat : subCats) {
                        const QString subCategory = subCat;
                        QAction* subCatAction = subCatMenu->addAction(subCategory);
                        connect(subCatAction, &QAction::triggered, this, [=]() {
                            emit setCategoryToEntitiesRequested(getSelectedEntities(), category, subCategory);
                        });
                    }
                }
            }
        }
        if (copyAction) {
            connect(copyAction, &QAction::triggered, this, [=]() {
                QList<QVariantMap> sel = getSelectedEntities();
                if (!sel.isEmpty()) {
                    copiedItems = sel;
                    if (this->contextMenu) this->contextMenu->m_copiedItems = sel;
                    emit copyItemsRequested(sel);
                }
            });
        }
        if (editSubCompAction) {
            connect(editSubCompAction, &QAction::triggered, this, [=]() {
                if (selectedSubComponents.isEmpty()) return;
                if (this->contextMenu) {
                    emit this->contextMenu->editMultipleSubComponentsRequested(selectedSubComponents);
                }
            });
        }
        if (deleteAction) {
            connect(deleteAction, &QAction::triggered, this, [=]() {
                if (QMessageBox::question(this, "Confirm Delete",
                                          "Are you sure you want to delete the selected items?",
                                          QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
                    return;
                }
                for (const QVariantMap& data : selectedSubComponents) {
                    QString id       = data["ID"].toString();
                    QString parentId = data["parentId"].toString();
                    QString name     = data["name"].toString();
                    if (this->contextMenu) {
                        emit this->contextMenu->removeSubComponentRequested(parentId, id, name);
                    }
                }
                if (!selectedSubComponents.isEmpty() &&
                    selectedSubComponents.size() == selectedItems.size()) {
                    return;
                }
                removeSelectedEntities();
            });
        }
        if (allArePlatformEntities) {
            connect(setActiveAction, &QAction::triggered, this, [=]() {
                emit setEntitiesActiveRequested(getSelectedEntities(), true);
            });
            connect(setInactiveAction, &QAction::triggered, this, [=]() {
                emit setEntitiesActiveRequested(getSelectedEntities(), false);
            });
            connect(addWeaponAction, &QAction::triggered, this, [=]() {
                emit addWeaponToEntitiesRequested(getSelectedEntities());
            });
            connect(addSensorAction, &QAction::triggered, this, [=]() {
                emit addSensorToEntitiesRequested(getSelectedEntities());
            });
            connect(addIFFAction, &QAction::triggered, this, [=]() {
                emit addIFFToEntitiesRequested(getSelectedEntities());
            });
            connect(addRadioAction, &QAction::triggered, this, [=]() {
                emit addRadioToEntitiesRequested(getSelectedEntities());
            });
            if (addFormationAction) {
                connect(addFormationAction, &QAction::triggered, this, [=]() {
                    emit addFormationRequested(selectedEntities);
                });
            }
        }

        contextMenu.exec(tree->viewport()->mapToGlobal(pos));
    }
    else {
        QTreeWidgetItem* item = tree->itemAt(pos);
        if (item) {
            contextMenu->setupMenu(item);
            contextMenu->exec(tree->viewport()->mapToGlobal(pos));
        }
    }
}

void HierarchyTree::contextMenuEvent(QContextMenuEvent *event)
{
    if (islib) {
        event->ignore();
        return;
    }
    showContextMenu(event->pos());
}

void HierarchyTree::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist") ||
        event->mimeData()->hasFormat("application/x-entity")) {
        event->acceptProposedAction();
        QTreeWidgetItem *item = tree->currentItem();
        if (!item) {
            dragsourceData.clear();
            return;
        }
        QVariantMap data = item->data(0, Qt::UserRole).toMap();
        data["islib"] = islib;
        if (dragsourceData.isEmpty())
            dragsourceData = data;
    } else {
        event->ignore();
    }
}

void HierarchyTree::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist") ||
        event->mimeData()->hasFormat("application/x-entity")) {
        QTreeWidgetItem *item = tree->itemAt(event->pos());
        if (item) {
            QVariantMap data = item->data(0, Qt::UserRole).toMap();
            QString type;
            if (data["type"].type() == QVariant::Map) {
                QVariantMap typeData = data["type"].toMap();
                if (typeData.contains("type") && typeData["type"].toString() == HIT::OptionTag) {
                    type = HIT::ProfileItem;
                } else {
                    event->ignore();
                    return;
                }
            } else {
                type = data["type"].toString();
            }
            if (type == HIT::ProfileItem || type == HIT::FolderItem || type == HIT::EntityItem || type == HIT::ComponentItem) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void HierarchyTree::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist") &&
        !event->mimeData()->hasFormat("application/x-entity")) {
        event->ignore();
        return;
    }

    QPoint viewportPos = tree->viewport()->mapFrom(this, event->pos());
    QTreeWidgetItem *targetItem = tree->itemAt(viewportPos);
    if (!targetItem) {
        event->ignore();
        return;
    }
    QVariantMap targetData = targetItem->data(0, Qt::UserRole).toMap();
    QString targetType;
    if (targetData["type"].type() == QVariant::Map) {
        QVariantMap typeData = targetData["type"].toMap();
        if (typeData.contains("type") && typeData["type"].toString() == HIT::OptionTag) {
            targetType = HIT::ProfileItem;
        } else {
            event->ignore();
            return;
        }
    } else {
        targetType = targetData["type"].toString();
    }
    if (targetType == HIT::EntityItem || targetType == HIT::SubComponentItem) {
        event->ignore();
        return;
    }

    QVariantMap sourceData = dragsourceData;
    dragsourceData.clear();
    if (sourceData["type"].toString() != HIT::EntityItem) {
        event->ignore();
        return;
    }
    emit itemDropped(sourceData, targetData);
    event->acceptProposedAction();
}

QList<QVariantMap> HierarchyTree::getSelectedEntities() const
{
    QList<QVariantMap> entities;
    QList<QTreeWidgetItem*> selectedItems = tree->selectedItems();
    for (QTreeWidgetItem* item : selectedItems) {
        QVariantMap data = item->data(0, Qt::UserRole).toMap();
        if (data["type"].toString() == HIT::EntityItem) {
            entities.append(data);
        }
    }
    return entities;
}

void HierarchyTree::removeSelectedEntities()
{
    QList<QPair<QString, QString>> entityInfoList;
    QList<QTreeWidgetItem*> selectedItems = tree->selectedItems();

    for (QTreeWidgetItem* item : selectedItems) {
        if (!item || !isItemInTree(item)) continue;

        QVariantMap data = item->data(0, Qt::UserRole).toMap();
        if (data.isEmpty()) continue;

        QString type     = data["type"].toString();
        QString id       = data["ID"].toString();
        QString parentID = data["parentId"].toString();
        if (id.isEmpty()) continue;

        if (type == HIT::SubComponentItem) {
            if (Items.contains(id)) {
                QTreeWidgetItem* subItem = Items[id];
                Items.remove(id);
                delete subItem;
            }
            continue;
        }

        if (type == HIT::EntityItem) {
            entityInfoList.append(qMakePair(parentID, id));
            if (Items.contains(id)) {
                QTreeWidgetItem* entityItem = Items[id];
                removeDescendantsFromMap(entityItem);
                Items.remove(id);
                delete entityItem;
            }
        }
        else if (type == HIT::FolderItem) {
            bool isProfileParent = false;
            if (Items.contains(parentID)) {
                QVariantMap parentData = Items[parentID]->data(0, Qt::UserRole).toMap();
                if (parentData["type"].type() == QVariant::Map) {
                    QVariantMap typeData = parentData["type"].toMap();
                    if (typeData.contains("type") && typeData["type"].toString() == HIT::OptionTag) {
                        isProfileParent = true;
                    }
                }
            }
            if (contextMenu) {
                emit contextMenu->removeFolderRequested(parentID, id, isProfileParent);
            }
            if (Items.contains(id)) {
                QTreeWidgetItem* folderItem = Items[id];
                removeDescendantsFromMap(folderItem);
                Items.remove(id);
                delete folderItem;
            }
        }
    }

    if (!entityInfoList.isEmpty()) {
        emit removeEntitiesRequested(entityInfoList);
    }
}

void HierarchyTree::selectEntityById(const QString& entityId)
{
    if (Items.contains(entityId)) {
        QTreeWidgetItem* item = Items[entityId];
        tree->setCurrentItem(item);
        tree->scrollToItem(item);
    }
}

void HierarchyTree::renameEntityById(const QString& entityId, const QString& newName)
{
    if (!Items.contains(entityId) || newName.trimmed().isEmpty())
        return;

    QVariantMap data = Items[entityId]->data(0, Qt::UserRole).toMap();
    if (data["type"].toString() == HIT::ComponentItem)
        return;

    QVariantMap modifiedData = data;
    modifiedData["name"] = newName;
    emit contextMenu->renameItemRequested(modifiedData);
}

void HierarchyTree::removeEntityById(const QString& entityId)
{
    if (!Items.contains(entityId))
        return;

    if (QMessageBox::question(this, "Confirm Delete",
                              "Are you sure you want to delete this item?",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    tree->blockSignals(true);
    tree->clearSelection();
    Items[entityId]->setSelected(true);
    tree->blockSignals(false);

    removeSelectedEntities();
}


void HierarchyTree::setLibraryFileName(const QString& fileName)
{
    m_libraryFilePath = fileName;
    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        emit libraryFileNameChanged(fileInfo.fileName());
    } else {
        emit libraryFileNameChanged("Library");
    }
}

void HierarchyTree::selectMultipleEntitiesInTree(const QList<QString>& entityIds)
{
    if (!tree || entityIds.isEmpty()) return;
    tree->blockSignals(true);
    tree->clearSelection();
    bool scrolledToFirst = false;
    for (const QString& entityId : entityIds) {
        if (Items.contains(entityId)) {
            QTreeWidgetItem* item = Items[entityId];
            if (item) {
                item->setSelected(true);
                if (!scrolledToFirst) {
                    tree->scrollToItem(item);
                    scrolledToFirst = true;
                }
            }
        }
    }

    tree->blockSignals(false);
    QList<QVariantMap> selectedDataList;
    for (const QString& entityId : entityIds) {
        if (Items.contains(entityId)) {
            QTreeWidgetItem* item = Items[entityId];
            if (item) {
                QVariantMap itemData = item->data(0, Qt::UserRole).toMap();
                if (!itemData.isEmpty()) {
                    selectedDataList.append(itemData);
                }
            }
        }
    }
    if (!selectedDataList.isEmpty()) {
        emit itemsSelected(selectedDataList);
    }
}

void HierarchyTree::subComponentRenamed(QString componentId, QString subCompId, QString newName)
{
    Q_UNUSED(componentId);
    QTreeWidgetItemIterator it(getTreeWidget());
    while (*it) {
        QVariantMap data = (*it)->data(0, Qt::UserRole).toMap();
        if (data["ID"].toString() == subCompId && data["type"].toString() == HIT::SubComponentItem) {
            (*it)->setText(0, newName);
            break;
        }
        ++it;
    }
}

void HierarchyTree::setEntityActiveState(const QString& entityId, bool active)
{
    if (!Items.contains(entityId)) return;
    QTreeWidgetItem* item = Items[entityId];
    if (!item) return;

    if (active) {
        item->setForeground(0, QColor(255, 255, 255));
        item->setToolTip(0, "");
    } else {
        item->setForeground(0, QColor(120, 120, 120));
        item->setToolTip(0, "Inactive");
    }
}

void HierarchyTree::removeDescendantsFromMap(QTreeWidgetItem* item)
{
    if (!item) return;
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem* child = item->child(i);
        removeDescendantsFromMap(child);
        QVariantMap data = child->data(0, Qt::UserRole).toMap();
        QString id = data["ID"].toString();
        if (!id.isEmpty()) {
            Items.remove(id);
        }
    }
}

bool HierarchyTree::isItemInTree(QTreeWidgetItem* item) const
{
    if (!item) return false;
    QTreeWidgetItem* current = item;
    while (current->parent()) {
        current = current->parent();
    }
    return tree->invisibleRootItem()->indexOfChild(current) != -1;
}

QString HierarchyTree::getCurrentEditorContext() const
{
    MainWindow* mw = MainWindow::instance();
    if (!mw) return "";
    QWidget *cur = mw->stackedWidget->currentWidget();
    if (cur == mw->databaseEditor) return "database";
    if (cur == mw->scenarioEditor) return "scenario";
    if (cur == mw->runtimeEditor)  return "runtime";
    return "";
}
