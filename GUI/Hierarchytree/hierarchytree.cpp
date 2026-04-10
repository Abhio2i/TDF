
/* ========================================================================= */
/* File: hierarchytree.cpp                                                  */
/* Purpose: Implements hierarchy tree widget with drag-and-drop support      */
//               Written by Arti Rajpoot
/* ========================================================================= */

#include "hierarchytree.h"                         // For hierarchy tree class
#include <QIcon>                                   // For icons
#include <QInputDialog>                            // For input dialog
#include <QContextMenuEvent>                       // For context menu events
#include <QVBoxLayout>                             // For vertical layout
#include <QHBoxLayout>                             // For horizontal layout
#include <QLineEdit>                               // For search bar
#include <QComboBox>                               // For profile dropdown
#include <QLabel>                                  // For labels
#include <QMouseEvent>                             // For mouse events
#include <QMimeData>                               // For MIME data handling
#include <QDrag>                                   // For drag operations
#include <QDragEnterEvent>                         // For drag enter events
#include <QDragMoveEvent>                          // For drag move events
#include <QDropEvent>                              // For drop events
#include <GUI/Hierarchytree/contextmenu.h>         // For context menu
#include <QApplication>                            // For application instance
#include <QMessageBox>
#include <GUI/Hierarchytree/addformationdialog.h>
#include <QFileInfo>
#include "hierarchy-styles.h"
#include "qheaderview.h"


HierarchyTree::HierarchyTree(QWidget *parent)
    : QWidget(parent)
{
    // Create main layout
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Create filter layout
    filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(5);
    filterLayout->setContentsMargins(5, 5, 5, 5);

    // Dark background for filter container
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

    // Add widgets to filter layout
    filterLayout->addWidget(searchBar);
    filterLayout->addWidget(profileFilterCombo);


    // Add filter container to main layout
    mainLayout->addWidget(filterContainer);

    // Create tree widget
    tree = new QTreeWidget(this);
    tree->setHeaderLabel("Hierarchy");
    tree->setEditTriggers(QAbstractItemView::DoubleClicked);
    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    tree->expandAll();
    tree->setStyleSheet(HierarchyStyles::TreeWidget);
    tree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Enable drag and drop
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree->setDragEnabled(true);
    setAcceptDrops(true);

    // Style header
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
            if (typeData.contains("type") && typeData["type"].toString() == "option") {
                type = "profile";
                data["type"] = type;
            } else {
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
/* Get all profile names */
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

    // Restore selection using full name
    for (int i = 0; i < profileFilterCombo->count(); ++i) {
        QString fullName = profileFilterCombo->itemData(i, Qt::UserRole).toString();
        if (fullName == currentFullName) {
            profileFilterCombo->setCurrentIndex(i);
            break;
        }
    }
}
/* Handle search text change */

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

/* Handle profile filter change */
void HierarchyTree::onProfileFilterChanged(int index)
{
    Q_UNUSED(index);

    QString profileFilter = "All Profiles";

    if (profileFilterCombo->currentIndex() > 0) {
        // Get FULL NAME from UserRole
        profileFilter = profileFilterCombo->currentData(Qt::UserRole).toString();

        if (profileFilter.isEmpty()) {
            // Fallback to display text
            profileFilter = profileFilterCombo->currentText();
        }
    }
    filterHierarchy(profileFilter, searchBar->text());
    emit profileFilterChanged(profileFilter);
}
/* Filter hierarchy based on profile and search text */
void HierarchyTree::filterHierarchy(const QString& profileFilter, const QString& searchText)
{
    // Start from root
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* topItem = tree->topLevelItem(i);
        applyFilters(topItem, profileFilter, searchText);
    }
}

/* Recursively apply filters */
void HierarchyTree::applyFilters(QTreeWidgetItem* item, const QString& profileFilter, const QString& searchText)
{
    if (!item) return;

    bool shouldShow = true;

    // Check profile filter
    if (profileFilter != "All Profiles" && !profileFilter.isEmpty()) {
        shouldShow = itemBelongsToProfile(item, profileFilter);
    }

    if (shouldShow && !searchText.isEmpty()) {
        shouldShow = itemMatchesSearch(item, searchText);
        if (!shouldShow) {
            bool childMatches = false;
            for (int i = 0; i < item->childCount(); ++i) {
                QTreeWidgetItem* child = item->child(i);
                applyFilters(child, profileFilter, searchText);
                if (!child->isHidden()) {
                    childMatches = true;
                }
            }
            shouldShow = childMatches;
        }
    }
    item->setHidden(!shouldShow);
    // Apply filters to children
    for (int i = 0; i < item->childCount(); ++i) {
        applyFilters(item->child(i), profileFilter, searchText);
    }
}
/* Check if item matches search criteria */
bool HierarchyTree::itemMatchesSearch(QTreeWidgetItem* item, const QString& searchText)
{
    if (!item) return false;

    QString itemText = item->text(0);
    QVariantMap data = item->data(0, Qt::UserRole).toMap();
    QString type = data["type"].toString();

    // For profiles with complex type data
    if (data["type"].type() == QVariant::Map) {
        QVariantMap typeData = data["type"].toMap();
        if (typeData.contains("type") && typeData["type"].toString() == "option") {
            type = "profile";
        }
    }

    // Only search entities by default, but you can modify to search other types
    if (type == "entity") {
        return itemText.contains(searchText, Qt::CaseInsensitive);
    }

    return false;
}

/* Check if item belongs to profile */
bool HierarchyTree::itemBelongsToProfile(QTreeWidgetItem* item, const QString& profileName)
{
    if (!item) return false;

    // Get item data
    QVariantMap data = item->data(0, Qt::UserRole).toMap();

    // Check if this is a profile item
    if (data["type"].type() == QVariant::Map) {
        QVariantMap typeData = data["type"].toMap();
        if (typeData.contains("type") && typeData["type"].toString() == "option") {
            // This is a profile item
            return data["name"].toString() == profileName;
        }
    } else if (data["type"].toString() == "profile") {
        return data["name"].toString() == profileName;
    }

    // For non-profile items, check their parent hierarchy
    QTreeWidgetItem* parent = item->parent();
    while (parent) {
        QVariantMap parentData = parent->data(0, Qt::UserRole).toMap();
        if (parentData["type"].type() == QVariant::Map) {
            QVariantMap typeData = parentData["type"].toMap();
            if (typeData.contains("type") && typeData["type"].toString() == "option") {
                return parentData["name"].toString() == profileName;
            }
        } else if (parentData["type"].toString() == "profile") {
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
            // Copy
            QList<QVariantMap> selectedEntities = getSelectedEntities();
            if (!selectedEntities.isEmpty()) {
                copiedItems = selectedEntities;
                emit copyItemsRequested(selectedEntities);
            }
            event->accept();
            return;
        }
        else if (event->key() == Qt::Key_V) {
            // Paste
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
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

/* Get tree widget */
QTreeWidget* HierarchyTree::getTreeWidget()
{
    return tree;
}

// %%% Tree Item Management %%%
/* Add profile to tree */
void HierarchyTree::profileAdded(QString ID, QString profileName)
{
    // Store profile in map
    profileMap[ID] = profileName;

    // Update profile dropdown
    updateProfileDropdown();

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
}

/* Add component to tree */
void HierarchyTree::componentAdded(QString parentID, QString ID, QString componentName)
{
    // Validate parent ID
    if (!Items.contains(parentID)) {
        return;
    }

    QString displayName = componentName;
    if (!displayName.isEmpty()) {
        displayName[0] = displayName[0].toUpper();
    }

    // Create component item
    QTreeWidgetItem *component = new QTreeWidgetItem(Items[parentID]);
    component->setText(0, displayName);
    component->setIcon(0, QIcon(":/icons/images/component.png"));
    Items.insert(ID, component);

    // Set item data
    QVariantMap data;
    data["ID"] = ID;
    data["parentId"] = parentID;
    data["name"] = componentName;
    data["type"] = "component";
    component->setData(0, Qt::UserRole, data);
    component->setFlags(component->flags() & ~Qt::ItemIsDragEnabled);
}

void HierarchyTree::subComponentAdded(QString parentID, QString ID, QString subComponentName)
{
    // Validate parent ID
    if (!Items.contains(parentID)) {
        return;
    }

    QString displayName = subComponentName;
    if (!displayName.isEmpty()) {
        displayName[0] = displayName[0].toUpper();
    }
    // Create component item
    QTreeWidgetItem *subcomponent = new QTreeWidgetItem(Items[parentID]);
    subcomponent->setText(0, displayName);
    subcomponent->setIcon(0, QIcon(":/icons/images/subcomponent.png"));
    Items.insert(ID, subcomponent);

    // Set item data
    QVariantMap data;
    data["ID"] = ID;
    data["parentId"] = parentID;
    data["name"] = subComponentName;
    data["type"] = "subcomponent";
    subcomponent->setData(0, Qt::UserRole, data);
    subcomponent->setFlags(subcomponent->flags() & ~Qt::ItemIsDragEnabled);
}

/* Remove profile from tree */
void HierarchyTree::profileRemoved(QString ID)
{
    // Check if profile exists
    if (Items.contains(ID)) {
        // Remove from profile map
        profileMap.remove(ID);
        // Update profile dropdown
        updateProfileDropdown();
        // Remove from tree
        delete Items[ID];
        Items.remove(ID);
    }
}

/* Remove folder from tree */
void HierarchyTree::folderRemoved(QString ID)
{
    // Check if folder exists
    if (Items.contains(ID)) {
        delete Items[ID];
        Items.remove(ID);
    }
}

/* Remove entity from tree */
void HierarchyTree::entityRemoved(QString ID)
{
    // Check if entity exists
    if (Items.contains(ID)) {
        delete Items[ID];
        Items.remove(ID);
    }
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
                break;
            }
        }
    }
    // Emit remove component signal
    emit removeComponentRequested(entityID, componentName);
}

/* Remove subcomponent from tree */
void HierarchyTree::subComponentRemoved(QString compID,QString subCompID, QString componentName)
{
    // Check if entity exists
    if (Items.contains(subCompID)) {
        QTreeWidgetItem *entityItem = Items[subCompID];
        Items.remove(subCompID);
        delete entityItem;
    }
}

void HierarchyTree::profileRenamed(QString ID, QString name)
{
    // Check if profile exists
    if (Items.contains(ID)) {
        // Update profile map
        if (profileMap.contains(ID)) {
            profileMap[ID] = name;
            updateProfileDropdown();
        }
        // Update tree item
        Items[ID]->setText(0, name);
        QVariantMap data = Items[ID]->data(0, Qt::UserRole).toMap();
        data["name"] = name;
        // Update nested type data for profile
        if (data["type"].type() == QVariant::Map) {
            QVariantMap typeData = data["type"].toMap();
            typeData["value"] = name;
            data["type"] = typeData;
        }
        Items[ID]->setData(0, Qt::UserRole, data);
    }
}

/* Rename folder in tree */
void HierarchyTree::folderRenamed(QString ID, QString name)
{
    // Check if folder exists
    if (Items.contains(ID)) {
        Items[ID]->setText(0, name);
        QVariantMap data = Items[ID]->data(0, Qt::UserRole).toMap();
        data["name"] = name;
        Items[ID]->setData(0, Qt::UserRole, data);
    }
}

/* Rename entity in tree */
void HierarchyTree::entityRenamed(QString ID, QString name)
{
    // Check if entity exists
    if (Items.contains(ID)) {
        Items[ID]->setText(0, name);
        QVariantMap data = Items[ID]->data(0, Qt::UserRole).toMap();
        data["name"] = name;
        Items[ID]->setData(0, Qt::UserRole, data);
    }
}

ContextMenu *HierarchyTree::getContextMenu() const { return contextMenu; }
void HierarchyTree::showContextMenu(const QPoint &pos)
{
    if (islib) return;
    QList<QTreeWidgetItem*> selectedItems = tree->selectedItems();
    if (selectedItems.size() > 1) {
        QMenu contextMenu(this);
        contextMenu.setStyleSheet(R"(
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
)");

        // ── Check karo sab Platform entities hain ya nahi ──────────────────
        bool allArePlatformEntities = true;
        QList<QVariantMap> selectedEntities;

        for (QTreeWidgetItem* item : selectedItems) {
            QVariantMap data = item->data(0, Qt::UserRole).toMap();
            if (data["type"].toString() == "entity") {
                QString parentId = data["parentId"].toString();
                if (Items.contains(parentId)) {
                    QVariantMap parentData = Items[parentId]->data(0, Qt::UserRole).toMap();

                    // Case 1: Direct profile parent
                    if (parentData["type"].type() == QVariant::Map) {
                        QVariantMap typeData = parentData["type"].toMap();
                        if (typeData.contains("type") && typeData["type"].toString() == "option"
                            && typeData.contains("value")
                            && typeData["value"].toString() == "Platform") {
                            selectedEntities.append(data);
                            continue;
                        }
                    }
                    // Case 2: Folder parent — grandparent profile check karo
                    else if (parentData["type"].toString() == "folder") {
                        QString grandParentId = parentData["parentId"].toString();
                        if (Items.contains(grandParentId)) {
                            QVariantMap gpData = Items[grandParentId]->data(0, Qt::UserRole).toMap();
                            if (gpData["type"].type() == QVariant::Map) {
                                QVariantMap gpTypeData = gpData["type"].toMap();
                                if (gpTypeData.contains("type") && gpTypeData["type"].toString() == "option"
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
            break;
        }

        // ── Har case mein Copy aur Delete ─────────────────────────────────
        QAction *copyAction   = contextMenu.addAction(QIcon(":/icons/images/copy.png"),   "Copy");
        QAction *deleteAction = contextMenu.addAction(QIcon(":/icons/images/delete.png"), "Delete");

        // ── Sirf Platform entities hain to extra options ───────────────────
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
                addFormationAction = addCompMenu->addAction(
                    QIcon(":/icons/images/add.png"), "Add Formation");
            }

            // ── Submenu: Set Team ──────────────────────────────────────────
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

            // ── Submenu: Set Category ──────────────────────────────────────
            QMenu *setCategorySubMenu = contextMenu.addMenu(QIcon(":/icons/images/set.png"), "Set Category");
            setCategorySubMenu->setStyleSheet(contextMenu.styleSheet());
            const QStringList categoriesList = {"Aircraft", "Helicopter", "Ship", "Submarine", "Tank"};
            for (const QString& category : categoriesList) {
                QAction *categoryAction = setCategorySubMenu->addAction(category);
                connect(categoryAction, &QAction::triggered, this, [=]() {
                    emit setCategoryToEntitiesRequested(getSelectedEntities(), category);
                });
            }
        }

        // ── Connects ──────────────────────────────────────────────────────
        connect(copyAction, &QAction::triggered, this, [=]() {
            QList<QVariantMap> sel = getSelectedEntities();
            if (!sel.isEmpty()) {
                copiedItems = sel;
                if (this->contextMenu) this->contextMenu->m_copiedItems = sel;
                emit copyItemsRequested(sel);
            }
        });
        connect(deleteAction, &QAction::triggered, this, [=]() {
            if (QMessageBox::question(this, "Confirm Delete",
                                      "Are you sure you want to delete the selected items?",
                                      QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                removeSelectedEntities();
            }
        });

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

    } else {
        QTreeWidgetItem *item = tree->itemAt(pos);
        if (item) {
            contextMenu->setupMenu(item);
            contextMenu->exec(tree->viewport()->mapToGlobal(pos));
        }
    }
}

/* Handle context menu event */
void HierarchyTree::contextMenuEvent(QContextMenuEvent *event)
{
    if (islib) {
        event->ignore();
        return;
    }
    // Show context menu at event position
    showContextMenu(event->pos());
}

/* Handle drag enter event */
void HierarchyTree::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept drag if MIME data is entity
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist") ||
        event->mimeData()->hasFormat("application/x-entity")) {
        event->acceptProposedAction();
        QTreeWidgetItem *item = tree->currentItem();
        if (!item) {
            dragsourceData.clear();
            return;
        }
        if (!item) {
            dragsourceData.clear();
            return;
        }
        QVariantMap data = item->data(0, Qt::UserRole).toMap();
        data["islib"] = islib;
        if(dragsourceData.isEmpty())
            dragsourceData = data;
    } else {
        event->ignore();
    }
}

/* Handle drag move event */
void HierarchyTree::dragMoveEvent(QDragMoveEvent *event)
{
    // Check MIME data
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist") ||
        event->mimeData()->hasFormat("application/x-entity")) {
        QTreeWidgetItem *item = tree->itemAt(event->pos());
        if (item) {
            QVariantMap data = item->data(0, Qt::UserRole).toMap();
            QString type;
            if (data["type"].type() == QVariant::Map) {
                QVariantMap typeData = data["type"].toMap();
                if (typeData.contains("type") && typeData["type"].toString() == "option") {
                    type = "profile";
                } else {
                    event->ignore();
                    return;
                }
            } else {
                type = data["type"].toString();
            }
            if (type == "profile" || type == "folder" || type == "entity" || type == "component") {
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
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist") ||
        event->mimeData()->hasFormat("application/x-entity")) {
        // Get target item
        QPoint viewportPos = tree->viewport()->mapFrom(this, event->pos());
        // 2. Ab item find karein
        QTreeWidgetItem *targetItem = tree->itemAt(viewportPos);
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
                event->ignore();
                return;
            }
        } else {
            targetType = targetData["type"].toString();
        }
        // Validate target type
        if (targetType == "entity" || targetType == "subcomponent") {
            event->ignore();
            return;
        }
        // Read source data
        QByteArray itemData = event->mimeData()->data("application/x-entity");
        QDataStream stream(&itemData, QIODevice::ReadOnly);
        QVariantMap sourceData = dragsourceData;
        dragsourceData.clear();
        // stream >> sourceData;
        // Validate source type
        if (sourceData["type"].toString() != "entity") {
            event->ignore();
            return;
        }
        // Emit drop signal
        emit itemDropped(sourceData, targetData);
        event->acceptProposedAction();
    }else{
        event->ignore();
        return;
    }
}
QList<QVariantMap> HierarchyTree::getSelectedEntities() const
{
    QList<QVariantMap> entities;
    QList<QTreeWidgetItem*> selectedItems = tree->selectedItems();
    for (QTreeWidgetItem* item : selectedItems) {
        QVariantMap data = item->data(0, Qt::UserRole).toMap();
        QString type = data["type"].toString();
        if (type == "entity") {
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
        QVariantMap data = item->data(0, Qt::UserRole).toMap();
        if (data["type"].toString() == "entity") {
            QString entityID = data["ID"].toString();
            QString parentID = data["parentId"].toString();
            entityInfoList.append(qMakePair(parentID, entityID));
            // Tree se remove
            if (Items.contains(entityID)) {
                delete Items[entityID];
                Items.remove(entityID);
            }
        }
    }
    if (!entityInfoList.isEmpty()) {
        emit removeEntitiesRequested(entityInfoList);
    }
}

/* Select entity by ID */
void HierarchyTree::selectEntityById(const QString& entityId)
{
    // Check if entity exists
    if (Items.contains(entityId)) {
        QTreeWidgetItem* item = Items[entityId];
        tree->setCurrentItem(item); // Select item
        tree->scrollToItem(item);   // Scroll to item
    }
}
void HierarchyTree::setLibraryFileName(const QString& fileName)
{
    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        QString displayName = fileInfo.fileName();
        emit libraryFileNameChanged(displayName);
    } else {
        emit libraryFileNameChanged("Library"); // Default
    }
}
/* Select multiple entities in tree by IDs */
void HierarchyTree::selectMultipleEntitiesInTree(const QList<QString>& entityIds)
{
    if (!tree || entityIds.isEmpty()) return;
    // Block signals to prevent recursive calls
    tree->blockSignals(true);
    // Clear current selection
    tree->clearSelection();
    // Track if we scrolled to first item
    bool scrolledToFirst = false;
    // Find and select all items with matching IDs
    for (const QString& entityId : entityIds) {
        if (Items.contains(entityId)) {
            QTreeWidgetItem* item = Items[entityId];
            if (item) {
                item->setSelected(true);
                // Scroll to first selected item only
                if (!scrolledToFirst) {
                    tree->scrollToItem(item);
                    scrolledToFirst = true;
                }
            }
        }
    }
    // Re-enable signals
    tree->blockSignals(false);
    // Manually trigger itemsSelected signal with all selected data
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
    // Emit the multi-selection signal
    if (!selectedDataList.isEmpty()) {
        emit itemsSelected(selectedDataList);
    }
}
void HierarchyTree::subComponentRenamed(QString componentId, QString subCompId, QString newName)
{
    // Tree mein subCompId wala item dhundho aur naam update karo
    QTreeWidgetItemIterator it(getTreeWidget());
    while (*it) {
        QVariantMap data = (*it)->data(0, Qt::UserRole).toMap();
        if (data["ID"].toString() == subCompId && data["type"].toString() == "subcomponent") {
            (*it)->setText(0, newName);
            break;
        }
        ++it;
    }
}
void HierarchyTree::setEntityActiveState(const QString& entityId, bool active)
{
    if (!Items.contains(entityId))
        return;
    QTreeWidgetItem* item = Items[entityId];
    if (!item)
        return;
    if (active) {
        // Restore normal white text
        item->setForeground(0, QColor(255, 255, 255));
        item->setToolTip(0, "");
    } else {
        // Gray out to signal inactive state
        item->setForeground(0, QColor(120, 120, 120));
        item->setToolTip(0, "Inactive");
    }
}
