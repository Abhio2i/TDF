/* ========================================================================= */
/* File: hierarchytree.h                                                    */
/* Purpose: Defines widget for displaying hierarchy tree                     */
/* ========================================================================= */

#ifndef HIERARCHYTREE_H
#define HIERARCHYTREE_H

#include <QWidget>                                // For widget base class
#include <QTreeWidget>                            // For tree widget
#include <QMap>                                   // For key-value mapping
#include <QVariant>                               // For variant data type
#include "core/Hierarchy/entity.h"                // For entity data structure

// %%% Forward Declarations %%%
class ContextMenu;

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
    // Get tree widget
    QTreeWidget* getTreeWidget();
    // Add profile to tree
    void profileAdded(QString ID, QString profileName);
    // Add folder to tree
    void folderAdded(QString parentID, QString ID, QString folderName);
    // Add entity to tree
    void entityAdded(QString parentID, QString ID, QString entityName);
    // Add component to entity
    void componentAdded(QString parentID, QString componentName);
    // Remove profile from tree
    void profileRemoved(QString ID);
    // Remove folder from tree
    void folderRemoved(QString ID);
    // Remove entity from tree
    void entityRemoved(QString ID);
    // Remove component from entity
    void componentRemoved(QString entityID, QString componentName);
    // Rename profile in tree
    void profileRenamed(QString ID, QString name);
    // Rename folder in tree
    void folderRenamed(QString ID, QString name);
    // Rename entity in tree
    void entityRenamed(QString ID, QString name);
    // Get context menu
    ContextMenu* getContextMenu() const { return contextMenu; }
    // Select entity by ID
    void selectEntityById(const QString& entityId);

signals:
    // Signal item selection
    void itemSelected(QVariantMap data);
    // Signal copy item request
    void copyItemRequested(QVariantMap data);
    // Signal paste item request
    void pasteItemRequested(QVariantMap targetData);
    // Signal remove component request
    void removeComponentRequested(QString entityID, QString componentName);
    // Signal item drop event
    void itemDropped(QVariantMap sourceData, QVariantMap targetData);
    // Signal entity selection
    void entitySelected(Entity* entity, QVariantMap data);

protected:
    // %%% Event Handlers %%%
    // Show context menu
    void showContextMenu(const QPoint &pos);
    // Handle context menu event
    void contextMenuEvent(QContextMenuEvent *event);
    // Handle event filtering (commented)
    // bool eventFilter(QObject *watched, QEvent *event) override;
    // Handle drag enter event
    void dragEnterEvent(QDragEnterEvent *event) override;
    // Handle drag move event
    void dragMoveEvent(QDragMoveEvent *event) override;
    // Handle drop event
    void dropEvent(QDropEvent *event) override;

private:
    // %%% UI Components %%%
    // Tree widget
    QTreeWidget *tree;
    // Context menu
    ContextMenu *contextMenu;
    // Map of items by ID
    QMap<QString, QTreeWidgetItem*> Items;
};

#endif // HIERARCHYTREE_H
