
#ifndef HIERARCHYTREE_H
#define HIERARCHYTREE_H

#include <QWidget>
#include <QTreeWidget>
#include <QMap>
#include <QVariant>
#include "core/Hierarchy/entity.h"

class ContextMenu;

class HierarchyTree : public QWidget
{
    Q_OBJECT

public:
    explicit HierarchyTree(QWidget *parent = nullptr);
    ~HierarchyTree();

    QTreeWidget* getTreeWidget();

    void profileAdded(QString ID, QString profileName);
    void folderAdded(QString parentID, QString ID, QString folderName);
    void entityAdded(QString parentID, QString ID, QString entityName);
    void componentAdded(QString parentID, QString componentName);

    void profileRemoved(QString ID);
    void folderRemoved(QString ID);
    void entityRemoved(QString ID);
    void componentRemoved(QString entityID, QString componentName);

    void profileRenamed(QString ID, QString name);
    void folderRenamed(QString ID, QString name);
    void entityRenamed(QString ID, QString name);

    ContextMenu* getContextMenu() const { return contextMenu; }
    void selectEntityById(const QString& entityId);

signals:
    void itemSelected(QVariantMap data);
    void copyItemRequested(QVariantMap data);
    void pasteItemRequested(QVariantMap targetData);
    void removeComponentRequested(QString entityID, QString componentName);
    void itemDropped(QVariantMap sourceData, QVariantMap targetData);
    void entitySelected(Entity* entity, QVariantMap data);

protected:
    void showContextMenu(const QPoint &pos);
    void contextMenuEvent(QContextMenuEvent *event);
    // bool eventFilter(QObject *watched, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QTreeWidget *tree;
    ContextMenu *contextMenu;
    QMap<QString, QTreeWidgetItem*> Items;
};

#endif // HIERARCHYTREE_H
