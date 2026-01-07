
#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include <QMenu>
#include <QTreeWidgetItem>
#include <QVariantMap>
#include "GUI/Hierarchytree/additemdialog.h"

class Hierarchy;
class AddItemDialog;

class ContextMenu : public QMenu
{
    Q_OBJECT

public:
    explicit ContextMenu(QWidget *parent = nullptr);
    void setupMenu(QTreeWidgetItem *item);

    void setHierarchy(Hierarchy* hierarchy) { m_hierarchy = hierarchy; }
    Hierarchy* m_hierarchy = nullptr;

signals:
    void addFolderRequested(QString parentID, QString folderName, bool isProfileParent, const QVariantMap& components = QVariantMap());
    // void addEntityRequested(QString parentID, QString entityName, bool isProfileParent, QVariantMap components = QVariantMap(), AddItemDialog* dialog = nullptr);
    void addEntityRequested(QString parentID, QString entityName,
                            bool isProfileParent,
                            QVariantMap components = QVariantMap(),
                            AddItemDialog* dialog = nullptr,
                            QString sensorType = "Generic");
    void addProfileRequested(QString profileName);
    void removeProfileRequested(QString ID);
    void removeFolderRequested(QString parentID, QString ID, bool isProfileParent);
    void removeEntityRequested(QString parentID, QString ID, bool isProfileParent);
    void pasteItemRequested(QVariantMap data);
    void renameItemRequested(QVariantMap data);
    void copyItemRequested(QVariantMap data);
    void removeComponentRequested(QString entityID, QString componentName);
    void addComponentRequested(QString entityID, QString componentType, QString componentName,
                               QString sensorType = "Generic", QString profileId = "");
    void attchedEntity(QString parentID, QString name);
    void removeSubComponentRequested(QString parentComponentID, QString subComponentID, QString subComponentName);

private:
    void setupProfileMenu(const QVariantMap &data);
    void setupFolderMenu(const QVariantMap &data);
    void setupEntityMenu(const QVariantMap &data);
    void setupComponentMenu(const QVariantMap &data);
    void setupSubComponentMenu(const QVariantMap &data);
};

#endif // CONTEXTMENU_H
