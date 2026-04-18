
/* ========================================================================= */
/* File: hierarchytree.h                                                    */
/* Purpose: Defines widget for displaying hierarchy tree                     */
// Written by   : Arti Rajpoot
/* ========================================================================= */

#ifndef HIERARCHYTREE_H
#define HIERARCHYTREE_H

#include <QWidget>                                // For widget base class
#include <QTreeWidget>                            // For tree widget
#include <QMap>                                   // For key-value mapping
#include <QVariant>                               // For variant data type
#include <QComboBox>                              // For profile dropdown
#include <QLineEdit>                              // For search bar
#include <QHBoxLayout>                            // For horizontal layout
#include "core/Hierarchy/entity.h"                // For entity data structure
#include "qwidgetaction.h"
#include <QLabel>

// %%% Forward Declarations %%%
class ContextMenu;
// In hierarchytree.h, add this class before HierarchyTree class definition
class CompactMenuAction : public QWidgetAction {
public:
    CompactMenuAction(const QIcon& icon, const QString& text, QObject* parent = nullptr)
        : QWidgetAction(parent) {
        QWidget* widget = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(0);  // Zero spacing - NO SPACE between icon and text

        // Icon label
        QLabel* iconLabel = new QLabel();
        if (!icon.isNull()) {
            iconLabel->setPixmap(icon.pixmap(16, 16));
        }
        iconLabel->setFixedSize(16, 16);

        // Text label
        QLabel* textLabel = new QLabel(text);
        textLabel->setStyleSheet("color: white; background: transparent;");
        textLabel->setContentsMargins(0, 0, 0, 0);

        layout->addWidget(iconLabel);
        layout->addWidget(textLabel);
        layout->addStretch();

        widget->setLayout(layout);
        widget->setStyleSheet("background: transparent;");
        setDefaultWidget(widget);
    }
};
// %%% Class Definition %%%
/* Widget for hierarchy tree display */
class HierarchyTree : public QWidget
{
    Q_OBJECT

public:
    // Initialize hierarchy tree
    explicit HierarchyTree(QWidget *parent = nullptr);
    // Clean up resources
    ~HierarchyTree();
    bool islib = false;
    // Get tree widget
    QTreeWidget* getTreeWidget();
    // Add profile to tree
    void profileAdded(QString ID, QString profileName);
    // Add folder to tree
    void folderAdded(QString parentID, QString ID, QString folderName);
    // Add entity to tree
    void entityAdded(QString parentID, QString ID, QString entityName);
    // Add component to entity
    void componentAdded(QString parentID, QString ID, QString componentName);
    // Add subcomponent to entity
    void subComponentAdded(QString parentID, QString ID, QString subComponentName);
    // Remove profile from tree
    void profileRemoved(QString ID);
    // Remove folder from tree
    void folderRemoved(QString ID);
    // Remove entity from tree
    void entityRemoved(QString ID);
    // Remove component from entity
    void componentRemoved(QString entityID, QString componentName);
    // Remove subcomponent from entity
    void subComponentRemoved(QString compID,QString subCompID, QString componentName);
    void subComponentRenamed(QString compID, QString subCompID, QString newName);
    // Rename profile in tree
    void profileRenamed(QString ID, QString name);
    // Rename folder in tree
    void folderRenamed(QString ID, QString name);
    // Rename entity in tree
    void entityRenamed(QString ID, QString name);
    // Get context menu
    ContextMenu* getContextMenu() const;
    // Select entity by ID
    void selectEntityById(const QString& entityId);
    // Get selected entities
    QList<QVariantMap> getSelectedEntities() const;
    // Remove multiple entities
    void removeSelectedEntities();
    // Get all profile names
    QStringList getAllProfileNames() const;
    // Filter hierarchy based on profile and search text
    void filterHierarchy(const QString& profileFilter = "", const QString& searchText = "");
    QList<QVariantMap> copiedItems;
    void setLibraryFileName(const QString& fileName);
    void selectMultipleEntitiesInTree(const QList<QString>& entityIds);
    QMap<QString, QTreeWidgetItem*> Items;
    void updateEntityActiveState(const QString& entityID, bool active);
    void setEntityActiveState(const QString& entityId, bool active);
    QTreeWidget *tree;
    QLineEdit *searchBar;
    QComboBox *profileFilterCombo;

signals:
    void copyItemsRequested(QList<QVariantMap> data);
    void pasteItemsRequested(QVariantMap targetData, QList<QVariantMap> itemsToPaste);
    void removeEntitiesRequested(QList<QPair<QString, QString>> entityInfoList);
    // Signal for profile filter change
    void profileFilterChanged(const QString& profileName);
    // Signal for search filter change
    void searchFilterChanged(const QString& searchText);
    // Signal item selection
    void itemSelected(QVariantMap data);
    // Signal multiple items selected
    void itemsSelected(QList<QVariantMap> data);
    // Signal copy item request
    void copyItemRequested(QVariantMap data);
    // Signal copy multiple items request
    // void copyItemsRequested(QList<QVariantMap> data);
    // Signal paste item request
    void pasteItemRequested(QVariantMap targetData);
    // Signal paste multiple items request
    // void pasteItemsRequested(QVariantMap targetData, QList<QVariantMap> itemsToPaste);
    // Signal remove component request
    void removeComponentRequested(QString entityID, QString componentName);
    // Signal remove multiple entities request
    // void removeEntitiesRequested(QList<QPair<QString, QString>> entityInfoList);
    // Signal item drop event
    void itemDropped(QVariantMap sourceData, QVariantMap targetData);
    // Signal entity selection
    void entitySelected(Entity* entity, QVariantMap data);
    //formation
    void addFormationRequested(QList<QVariantMap> selectedEntities);
    void libraryFileNameChanged(const QString& fileName);
    void setEntitiesActiveRequested(QList<QVariantMap> entities, bool active);
    void addWeaponToEntitiesRequested(QList<QVariantMap> entities);
    void addSensorToEntitiesRequested(QList<QVariantMap> entities);
    void addTeamToEntitiesRequested(QList<QVariantMap> entities, QString team);
    void addIFFToEntitiesRequested(QList<QVariantMap> entities);
    void addRadioToEntitiesRequested(QList<QVariantMap> entities);
    void setCategoryToEntitiesRequested(QList<QVariantMap> entities, QString category);

protected:
    // %%% Event Handlers %%%
    // Show context menu
    void showContextMenu(const QPoint &pos);
    // Handle context menu event
    void contextMenuEvent(QContextMenuEvent *event) override;
    // Handle key press event for multi-select operations
    void keyPressEvent(QKeyEvent *event) override;
    // Handle drag enter event
    void dragEnterEvent(QDragEnterEvent *event) override;
    // Handle drag move event
    void dragMoveEvent(QDragMoveEvent *event) override;
    // Handle drop event
    void dropEvent(QDropEvent *event) override;

private slots:
    // Handle search text change
    void onSearchTextChanged(const QString& text);
    // Handle profile filter change
    void onProfileFilterChanged(int index);
    // Update profile dropdown with all available profiles
    void updateProfileDropdown();

private:
    ContextMenu *contextMenu;
    // %%% Layouts %%%
    QVBoxLayout *mainLayout;
    QHBoxLayout *filterLayout;
    // %%% Data Storage %%%
    QMap<QString, QString> profileMap;
    // Original items backup for filtering
    QMap<QString, QTreeWidgetItem*> originalItems;
    static inline QVariantMap dragsourceData;
    // %%% Helper Methods %%%
    // Recursively show/hide items based on filters
    void applyFilters(QTreeWidgetItem* item, const QString& profileFilter, const QString& searchText);
    // Check if item matches search criteria
    bool itemMatchesSearch(QTreeWidgetItem* item, const QString& searchText);
    // Check if item belongs to profile
    bool itemBelongsToProfile(QTreeWidgetItem* item, const QString& profileName);
};

#endif // HIERARCHYTREE_H
