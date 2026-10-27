#ifndef HIERARCHYTREE_H
#define HIERARCHYTREE_H

#include <QWidget>
#include <QTreeWidget>
#include <QMap>
#include <QVariant>
#include <QComboBox>
#include <QLineEdit>
#include <QHBoxLayout>
#include "core/Hierarchy/entity.h"
#include "qwidgetaction.h"
#include <QLabel>

class ContextMenu;

namespace HierarchyItemType {
inline const QString ProfileItem      = "profile";
inline const QString FolderItem       = "folder";
inline const QString EntityItem       = "entity";
inline const QString ComponentItem    = "component";
inline const QString SubComponentItem = "subcomponent";
inline const QString OptionTag        = "option";
}

class CompactMenuAction : public QWidgetAction {
public:
    CompactMenuAction(const QIcon& icon, const QString& text, QObject* parent = nullptr)
        : QWidgetAction(parent) {
        QWidget* widget = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(0);
        QLabel* iconLabel = new QLabel();
        if (!icon.isNull()) {
            iconLabel->setPixmap(icon.pixmap(16, 16));
        }
        iconLabel->setFixedSize(16, 16);
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

class HierarchyTree : public QWidget
{
    Q_OBJECT

public:
    explicit HierarchyTree(QWidget *parent = nullptr);
    ~HierarchyTree();
    bool islib = false;
    QTreeWidget* getTreeWidget();
    void profileAdded(QString ID, QString profileName);
    void folderAdded(QString parentID, QString ID, QString folderName);
    void entityAdded(QString parentID, QString ID, QString entityName);
    void componentAdded(QString parentID, QString ID, QString componentName);
    void subComponentAdded(QString parentID, QString ID, QString subComponentName);
    void profileRemoved(QString ID);
    void folderRemoved(QString ID);
    void entityRemoved(QString ID);
    void componentRemoved(QString entityID, QString componentName);
    void subComponentRemoved(QString CompID, QString subCompID, QString componentName);
    void subComponentRenamed(QString componentId, QString subCompId, QString newName);
    void profileRenamed(QString ID, QString name);
    void folderRenamed(QString ID, QString name);
    void entityRenamed(QString ID, QString name);
    ContextMenu* getContextMenu() const;
    void selectEntityById(const QString& entityId);
    void renameEntityById(const QString& entityId, const QString& newName);
    void removeEntityById(const QString& entityId);
    QList<QVariantMap> getSelectedEntities() const;
    void removeSelectedEntities();
    QStringList getAllProfileNames() const;
    void filterHierarchy(const QString& profileFilter = "", const QString& searchText = "");
    QList<QVariantMap> copiedItems;
    void setLibraryFileName(const QString& fileName);
    void selectMultipleEntitiesInTree(const QList<QString>& entityIds);
    QMap<QString, QTreeWidgetItem*> Items;
    void setEntityActiveState(const QString& entityId, bool active);
    QTreeWidget *tree;
    QLineEdit *searchBar;
    QComboBox *profileFilterCombo;
    QString libraryFilePath() const { return m_libraryFilePath; }

signals:
    void copyItemsRequested(QList<QVariantMap> data);
    void pasteItemsRequested(QVariantMap targetData, QList<QVariantMap> itemsToPaste);
    void removeEntitiesRequested(QList<QPair<QString, QString>> entityInfoList);
    void profileFilterChanged(const QString& profileName);
    void searchFilterChanged(const QString& searchText);
    void itemSelected(QVariantMap data);
    void itemsSelected(QList<QVariantMap> data);
    void copyItemRequested(QVariantMap data);
    void pasteItemRequested(QVariantMap targetData);
    void removeComponentRequested(QString entityID, QString componentName);
    void itemDropped(QVariantMap sourceData, QVariantMap targetData);
    void entitySelected(Entity* entity, QVariantMap data);
    void addFormationRequested(QList<QVariantMap> selectedEntities);
    void libraryFileNameChanged(const QString& fileName);
    void setEntitiesActiveRequested(QList<QVariantMap> entities, bool active);
    void addWeaponToEntitiesRequested(QList<QVariantMap> entities);
    void addSensorToEntitiesRequested(QList<QVariantMap> entities);
    void addTeamToEntitiesRequested(QList<QVariantMap> entities, QString team);
    void addIFFToEntitiesRequested(QList<QVariantMap> entities);
    void addRadioToEntitiesRequested(QList<QVariantMap> entities);
    void setCategoryToEntitiesRequested(QList<QVariantMap> entities, QString category, QString subCategory);
    void selectFormationWithEntityRequested(const QString& formationId);
    void formationSelectWithEntity(const QString& formationId);
    void entityRemovedNotify(const QString& entityId);
    void editMultipleSubComponentsRequested(QList<QVariantMap> subComponents);

protected:
    void showContextMenu(const QPoint &pos);
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onSearchTextChanged(const QString& text);
    void onProfileFilterChanged(int index);
    void updateProfileDropdown();

private:
    ContextMenu *contextMenu;
    QVBoxLayout *mainLayout;
    QHBoxLayout *filterLayout;
    QMap<QString, QString> profileMap;
    QMap<QString, QTreeWidgetItem*> originalItems;
    static inline QVariantMap dragsourceData;

    //fix by yogesh
    void removeItemFromMapRecursively(QTreeWidgetItem* item);
    void applyFilters(QTreeWidgetItem* item, const QString& profileFilter, const QString& searchText);
    bool itemMatchesSearch(QTreeWidgetItem* item, const QString& searchText);
    bool itemBelongsToProfile(QTreeWidgetItem* item, const QString& profileName);
    void removeDescendantsFromMap(QTreeWidgetItem* item);
    bool isItemInTree(QTreeWidgetItem* item) const;
    QString getCurrentEditorContext() const;
    QString multiSelectMenuStyleSheet() const;
    QString m_libraryFilePath;
};

#endif // HIERARCHYTREE_H
