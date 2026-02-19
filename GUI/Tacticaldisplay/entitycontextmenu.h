// entitycontextmenu.h
#ifndef ENTITYCONTEXTMENU_H
#define ENTITYCONTEXTMENU_H

#include <QMenu>
#include <QObject>
#include <QWidget>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QString>
#include <functional>

// Forward declaration
class CanvasWidget;
struct MeshEntry;

class EntityContextMenu : public QMenu
{
    Q_OBJECT

public:
    explicit EntityContextMenu(CanvasWidget* parent = nullptr);

    void setupMenu(const QString& entityId, MeshEntry& entry);

signals:
    void removeEntityRequested(const QString& entityId);
    void renameEntityRequested(const QString& entityId, const QString& newName);
    void editTrajectoryRequested(const QString& entityId);
    void showPropertiesRequested(const QString& entityId);

private slots:
    void onRemoveTriggered();
    void onRenameTriggered();
    void onEditTrajectoryTriggered();
    void onPropertiesTriggered();

private:
    CanvasWidget* m_parent;
    QString m_currentEntityId;
    MeshEntry* m_currentEntry;

    QAction* m_removeAction;
    QAction* m_renameAction;
    QAction* m_editTrajectoryAction;
    QAction* m_propertiesAction;
};

#endif // ENTITYCONTEXTMENU_H
