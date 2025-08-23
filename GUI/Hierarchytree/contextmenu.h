
#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H
#include <QMenu>
#include <QTreeWidgetItem>
#include <QVariantMap>
class ContextMenu : public QMenu
{
    Q_OBJECT
public:
    explicit ContextMenu(QWidget *parent = nullptr);
    void setupMenu(QTreeWidgetItem *item);

signals:
    void addFolderRequested(QString parentID, QString folderName, bool isProfileParent, const QVariantMap& components = QVariantMap());
    void addEntityRequested(QString parentID, QString entityName, bool isProfileParent, QVariantMap components = QVariantMap());
    void addProfileRequested(QString profileName);
    void removeProfileRequested(QString ID);
    void removeFolderRequested(QString parentID, QString ID, bool isProfileParent);
    void removeEntityRequested(QString parentID, QString ID, bool isProfileParent);
    void pasteItemRequested(QVariantMap data);
    void renameItemRequested(QVariantMap data);
    void copyItemRequested(QVariantMap data);
    void removeComponentRequested(QString entityID, QString componentName);
    void addComponentRequested(QString entityID, QString componentType, QString componentName);
    void attchedEntity(QString parentID, QString name);
private:
    void setupProfileMenu(const QVariantMap &data);
    void setupFolderMenu(const QVariantMap &data);
    void setupEntityMenu(const QVariantMap &data);
};

#endif // CONTEXTMENU_H
