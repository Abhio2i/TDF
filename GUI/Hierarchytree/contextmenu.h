/* ========================================================================= */
/* File: contextmenu.h                                                      */
/* Purpose: Defines context menu for hierarchy tree interactions             */
/* ========================================================================= */

#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include <QMenu>                                  // For menu widget
#include <QTreeWidgetItem>                        // For tree widget item
#include <QVariantMap>                            // For key-value data mapping

// %%% Class Definition %%%
/* Context menu for hierarchy tree */
class ContextMenu : public QMenu
{
    Q_OBJECT

public:
    // Initialize context menu
    explicit ContextMenu(QWidget *parent = nullptr);
    // Setup menu for tree item
    void setupMenu(QTreeWidgetItem *item);

signals:
    // Signal add folder request
    void addFolderRequested(QString parentID, QString folderName, bool isProfileParent, const QVariantMap& components = QVariantMap());
    // Signal add entity request
    void addEntityRequested(QString parentID, QString entityName, bool isProfileParent, QVariantMap components = QVariantMap());
    // Signal add profile request
    void addProfileRequested(QString profileName);
    // Signal remove profile request
    void removeProfileRequested(QString ID);
    // Signal remove folder request
    void removeFolderRequested(QString parentID, QString ID, bool isProfileParent);
    // Signal remove entity request
    void removeEntityRequested(QString parentID, QString ID, bool isProfileParent);
    // Signal paste item request
    void pasteItemRequested(QVariantMap data);
    // Signal rename item request
    void renameItemRequested(QVariantMap data);
    // Signal copy item request
    void copyItemRequested(QVariantMap data);
    // Signal remove component request
    void removeComponentRequested(QString entityID, QString componentName);
    // Signal add component request
    void addComponentRequested(QString entityID, QString componentType, QString componentName);
    // Signal attach entity request
    void attchedEntity(QString parentID, QString name);

private:
    // %%% Menu Setup Methods %%%
    // Setup profile menu
    void setupProfileMenu(const QVariantMap &data);
    // Setup folder menu
    void setupFolderMenu(const QVariantMap &data);
    // Setup entity menu
    void setupEntityMenu(const QVariantMap &data);
};

#endif // CONTEXTMENU_H
