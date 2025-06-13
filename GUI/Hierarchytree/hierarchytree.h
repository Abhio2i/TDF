/* Header guard section */
#ifndef HIERARCHYTREE_H
#define HIERARCHYTREE_H

/* Includes section */
#include <QWidget>
#include <QTreeWidget>
#include <QMap>
#include <QVariant>

/* Forward declaration section */
class ContextMenu;

/* Class declaration section */
class HierarchyTree : public QWidget
{
    /* Qt meta-object section */
    Q_OBJECT

public:
    /* Constructor and destructor section */
    explicit HierarchyTree(QWidget *parent = nullptr);
    ~HierarchyTree();

    /* Tree widget accessor section */
    QTreeWidget* getTreeWidget();

    /* Item management methods section */
    void profileAdded(QString ID, QString profileName);
    void folderAdded(QString parentID, QString ID, QString folderName);
    void entityAdded(QString parentID, QString ID, QString entityName);
    void componentAdded(QString parentID, QString componentName);

    /* Item removal methods section */
    void profileRemoved(QString ID);
    void folderRemoved(QString ID);
    void entityRemoved(QString ID);
    void componentRemoved(QString entityID, QString componentName);

    /* Renaming methods section */
    void profileRenamed(QString ID, QString name);
    void folderRenamed(QString ID, QString name);
    void entityRenamed(QString ID, QString name);

    /* Context menu accessor section */
    ContextMenu* getContextMenu() const { return contextMenu; }

signals:
    /* Signals section */
    void itemSelected(QVariantMap data);
    void copyItemRequested(QVariantMap data);
    void pasteItemRequested(QVariantMap targetData);
    void removeComponentRequested(QString entityID, QString componentName);

protected:
    /* Event handling section */
    void showContextMenu(const QPoint &pos);
    void contextMenuEvent(QContextMenuEvent *event);
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    /* Private members section */
    QTreeWidget *tree;
    ContextMenu *contextMenu;
    QMap<QString, QTreeWidgetItem*> Items;

};

/* End of header guard section */
#endif // HIERARCHYTREE_H
