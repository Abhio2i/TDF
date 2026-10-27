/* =============================================================================
 * FILE:         contextmenu.h
 * MODULE:       Context-Sensitive Menu for Hierarchy Tree
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the ContextMenu class which provides a context‑sensitive
 *               menu for hierarchy tree operations. The menu adapts to different
 *               item types (profile, folder, entity, component, sub‑component)
 *               and emits signals for add/remove/copy/paste/rename/activate and
 *               team/category assignment. Supports multi‑select copy/paste and
 *               integration with AddItemDialog for creating new entities/folders.
 *
 * REQUIREMENTS: REQ-CONTEXT-010  Context menu for hierarchy tree
 *               REQ-CONTEXT-011  Profile item menu (add/remove profile)
 *               REQ-CONTEXT-012  Folder item menu (add/remove folder)
 *               REQ-CONTEXT-013  Entity item menu (add/remove entity, components,
 *                                 copy/paste, rename, set active, team, category)
 *               REQ-CONTEXT-014  Component item menu (radio, sensor, IFF, weapon)
 *               REQ-CONTEXT-015  Sub‑component item menu
 *               REQ-CONTEXT-016  Copy/paste (single and multiple items)
 *               REQ-CONTEXT-017  Add component to entity (sensor, IFF, radio, weapon)
 *               REQ-CONTEXT-018  Set entity active/inactive
 *               REQ-CONTEXT-019  Assign team (Blue/Red) to entity
 *               REQ-CONTEXT-020  Assign category to entity
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-CONTEXT-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include <QMenu>                              // For menu base class
#include <QTreeWidgetItem>                    // For tree item handling
#include <QVariantMap>                        // For data storage and transfer
#include <QList>                              // For list of items
#include "GUI/Hierarchytree/additemdialog.h"  // For add item dialog integration

// Forward declarations
class Hierarchy;
class AddItemDialog;
class AddWeaponDialog;
class Weapon;

// %%% Class Definition %%%
/* Context-sensitive menu that adapts to different hierarchy item types */
class ContextMenu : public QMenu
{
    Q_OBJECT
public:
    // %%% Constructor %%%
    /* Initialize context menu with optional parent widget */
    explicit ContextMenu(QWidget *parent = nullptr);

    // %%% Menu Setup Method %%%
    /* Configure menu options based on selected tree item */
    void setupMenu(QTreeWidgetItem *item);

    // %%% Hierarchy Accessor %%%
    /* Set hierarchy reference for profile and data access */
    void setHierarchy(Hierarchy* hierarchy) { m_hierarchy = hierarchy; }

    // %%% Hierarchy Reference %%%
    Hierarchy* m_hierarchy = nullptr;

    // %%% Multi-Select Support %%%
    QList<QVariantMap> m_copiedItems;

signals:
    // %%% Folder Operations Signals %%%
    /* Request to add new folder */
    void addFolderRequested(QString parentID, QString folderName, bool isProfileParent,
                            const QVariantMap& components = QVariantMap());
    /* Request to remove existing folder */
    void removeFolderRequested(QString parentID, QString ID, bool isProfileParent);

    // %%% Entity Operations Signals %%%
    /* Request to add new entity with components */
    void addEntityRequested(QString parentID, QString entityName,
                            bool isProfileParent,
                            QVariantMap components = QVariantMap(),
                            AddItemDialog* dialog = nullptr,
                            QString sensorType = "Generic",
                            double initLat = 20000,
                            double initlon = 20000,
                            float heading = 20000);
    /* Request to remove existing entity */
    void removeEntityRequested(QString parentID, QString ID, bool isProfileParent);

    // %%% Profile Operations Signals %%%
    /* Request to add new profile */
    void addProfileRequested(QString profileName);
    /* Request to remove existing profile */
    void removeProfileRequested(QString ID);

    // %%% Component Operations Signals %%%
    /* Request to add component to entity (Radio / Sensor / IFF) */
    void addComponentRequested(QString entityID, QString componentType, QString componentName,
                               QString sensorType = "Generic", QString profileId = "");
    /* Request to remove component from entity */
    void removeComponentRequested(QString entityID, QString componentName);
    /* Request to remove sub-component from parent component */
    void removeSubComponentRequested(QString parentComponentID, QString subComponentID,
                                     QString subComponentName);

    // %%% Clipboard Operations Signals %%%
    /* Request to copy item to clipboard */
    void copyItemRequested(QVariantMap data);
    /* Request to paste item from clipboard */
    void pasteItemRequested(QVariantMap data);
    /* Request to paste multiple items from clipboard */
    void pasteItemsRequested(QVariantMap targetData, QList<QVariantMap> itemsToPaste);
    /* Request to rename existing item */
    void addIFFToEntityRequested(QVariantMap entityData);
    void addRadioToEntityRequested(QVariantMap entityData);
    void renameItemRequested(QVariantMap data);
    void setEntityActiveRequested(QString entityID, bool active);
    void addWeaponToEntityRequested(QVariantMap entityData);
    void addSensorToEntityRequested(QVariantMap entityData);
    void addTeamToEntityRequested(QVariantMap entityData, QString team);
    void setCategoryToEntityRequested(QVariantMap data, QString category, QString subCategory);
    void selectFormationWithEntityRequested(const QString& formationId);
    void changeMothershipRequested(const QString& formationId);
    void editSubComponentRequested(QString parentID, QString subCompID, QString subCompName, QString subCompType);
    void editMultipleSubComponentsRequested(QList<QVariantMap> subComponents);
    void editEntityRequested(QVariantMap entityData);
    void editMultipleEntitiesRequested(QList<QVariantMap> entities);
    void removeEntityFromFormationRequested(const QString& formationId,
                                            const QString& entityId);
    void addEntityToFormationRequested(const QString& formationId);
    void breakFormationRequested(const QString& formationId);
    void resumeFormationRequested(const QString& formationId);

private:
    // %%% Menu Setup Methods %%%
    /* Configure menu options for profile items */
    void setupProfileMenu(const QVariantMap &data);
    /* Configure menu options for folder items */
    void setupFolderMenu(const QVariantMap &data);
    /* Configure menu options for entity items */
    void setupEntityMenu(const QVariantMap &data);
    /* Configure menu options for component items (Radio, Sensor, IFF, Weapon) */
    void setupComponentMenu(const QVariantMap &data);
    /* Configure menu options for sub-component items */
    void setupSubComponentMenu(const QVariantMap &data);

    /* Returns "database" / "scenario" / "runtime" based on the currently
     * active editor in MainWindow, or "" if undetermined. Single source of
     * truth — used instead of repeating the MainWindow-inspection logic
     * inline in every setup*Menu() function. */
    QString getCurrentEditorContext() const;

    /* Resolves the correct Hierarchy instance (database / scenario library /
     * runtime library, or the weapon-specific scenario/runtime hierarchy)
     * for the given editor context. Falls back to the database hierarchy
     * if no context-specific one is available. */
    Hierarchy* resolveHierarchyForContext(const QString& editorContext, bool isWeaponProfile) const;

    /* Walks the hierarchy tree to find which top-level profile type
     * (e.g. "Platform", "Weapon", "Sensor") the given parentID ultimately
     * belongs to, whether parentID is a profile node itself or a folder
     * nested under one. Returns "" if it cannot be determined. */
    QString findParentProfileType(const QString& parentID) const;

    /* Builds a fully-configured Weapon object from an AddWeaponDialog,
     * either from manually entered config (fromDatabaseSelection = false)
     * or from an existing database weapon's data while preserving this
     * weapon's own ID/Name/parentID (fromDatabaseSelection = true).
     * Returns nullptr on invalid/incomplete dialog input. Caller owns the
     * returned pointer and is responsible for inserting it into the
     * hierarchy or deleting it on failure. */
    Weapon* buildWeaponFromDialog(const AddWeaponDialog& dlg,
                                  Hierarchy* correctHierarchy,
                                  Hierarchy* dbHierarchy,
                                  const QString& parentID,
                                  bool fromDatabaseSelection) const;
};

#endif // CONTEXTMENU_H
