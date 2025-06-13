/* Header guard to prevent multiple inclusions */
#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

/* Include necessary Qt classes for menu and tree widget functionality */
#include <QMenu>
#include <QTreeWidgetItem>
#include <QVariantMap>

/* Declaration of ContextMenu class, inheriting from QMenu */
class ContextMenu : public QMenu
{
    /* Macro to declare Qt meta-object compiler features (signals/slots) */
    Q_OBJECT
public:
    /* Constructor with optional parent widget */
    explicit ContextMenu(QWidget *parent = nullptr);

    /* Method to set up the context menu based on the provided tree widget item */
    void setupMenu(QTreeWidgetItem *item);

signals:
    /* Signals for various context menu actions */
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

private:
    /* Private methods to set up specific menu types based on item data */
    void setupProfileMenu(const QVariantMap &data);
    void setupFolderMenu(const QVariantMap &data);
    void setupEntityMenu(const QVariantMap &data);
};

/* End of header guard */
#endif // CONTEXTMENU_H
