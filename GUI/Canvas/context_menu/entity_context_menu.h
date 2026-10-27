#ifndef ENTITY_CONTEXT_MENU_H
#define ENTITY_CONTEXT_MENU_H

#include <QObject>
#include <QMenu>
#include <QAction>
#include <QMap>
#include <QList>
#include <QString>
#include <QPoint>

class EntityContextMenu : public QObject
{
    Q_OBJECT
public:
    explicit EntityContextMenu(QWidget* parent = nullptr);
    void exec(const QPoint& globalPos, const QString& entityId,
              bool showAdvancedActions = true, bool trajectoryVisible = true, bool canLeaveFormation = false);
    QAction* registerPluginAction(const QString& pluginId, QAction* action, int position = -1);
    void unregisterPlugin(const QString& pluginId);

signals:
    void renameRequested(const QString& entityId);
    void removeRequested(const QString& entityId);
    void editTrajectoryRequested(const QString& entityId);
    void changeTrajectoryColorRequested(const QString& entityId);
    void changeEntitySizeRequested(const QString& entityId);
    void toggleTrajectoryVisibilityRequested(const QString& entityId);
    void leaveFormationRequested(const QString& entityId);

private:
    void buildCoreActions();
    QMenu*   m_menu = nullptr;
    QAction* m_renameAction = nullptr;
    QAction* m_removeAction = nullptr;
    QAction* m_editTrajectoryAction = nullptr;
    QAction* m_changeTrajectoryColorAction = nullptr;
    QAction* m_toggleTrajectoryVisibilityAction = nullptr;
    QString m_currentEntityId;
    QMap<QString, QList<QAction*>> m_pluginOwnedActions;
    QAction* m_changeSizeAction = nullptr;
    QAction* m_leaveFormationAction = nullptr;
};

#endif // ENTITY_CONTEXT_MENU_H
