#include "entity_context_menu.h"

EntityContextMenu::EntityContextMenu(QWidget* parent)
    : QObject(parent)
{
    m_menu = new QMenu(parent);
    buildCoreActions();
}

void EntityContextMenu::buildCoreActions()
{
    m_renameAction = m_menu->addAction("Rename");
    connect(m_renameAction, &QAction::triggered, this, [this]() {
        emit renameRequested(m_currentEntityId);
    });
    m_removeAction = m_menu->addAction("Remove");
    connect(m_removeAction, &QAction::triggered, this, [this]() {
        emit removeRequested(m_currentEntityId);
    });
    m_leaveFormationAction = m_menu->addAction("Leave Formation");
    connect(m_leaveFormationAction, &QAction::triggered, this, [this]() {
        emit leaveFormationRequested(m_currentEntityId);
    });
    m_menu->addSeparator();
    m_editTrajectoryAction = m_menu->addAction("Edit Trajectory");
    connect(m_editTrajectoryAction, &QAction::triggered, this, [this]() {
        emit editTrajectoryRequested(m_currentEntityId);
    });
    m_changeTrajectoryColorAction = m_menu->addAction("Change Trajectory Color");
    connect(m_changeTrajectoryColorAction, &QAction::triggered, this, [this]() {
        emit changeTrajectoryColorRequested(m_currentEntityId);
    });
    m_toggleTrajectoryVisibilityAction = m_menu->addAction("Hide Trajectory");
    connect(m_toggleTrajectoryVisibilityAction, &QAction::triggered, this, [this]() {
        emit toggleTrajectoryVisibilityRequested(m_currentEntityId);
    });
    m_changeSizeAction = m_menu->addAction("Change Entity Size");
    connect(m_changeSizeAction, &QAction::triggered, this, [this]() {
        emit changeEntitySizeRequested(m_currentEntityId);
    });
}

void EntityContextMenu::exec(const QPoint& globalPos, const QString& entityId,
                             bool showAdvancedActions,
                             bool trajectoryVisible,
                             bool canLeaveFormation)
{
    m_currentEntityId = entityId;

    if (m_leaveFormationAction)
        m_leaveFormationAction->setVisible(canLeaveFormation);

    if (m_editTrajectoryAction)
        m_editTrajectoryAction->setVisible(showAdvancedActions);
    if (m_changeTrajectoryColorAction)
        m_changeTrajectoryColorAction->setVisible(showAdvancedActions);
    if (m_toggleTrajectoryVisibilityAction) {
        m_toggleTrajectoryVisibilityAction->setVisible(showAdvancedActions);
        m_toggleTrajectoryVisibilityAction->setText(
            trajectoryVisible ? QStringLiteral("Hide Trajectory")
                              : QStringLiteral("Unhide Trajectory"));
    }
    if (m_changeSizeAction)
        m_changeSizeAction->setVisible(showAdvancedActions);
    const auto actions = m_menu->actions();
    for (QAction* a : actions) {
        if (a->isSeparator()) {
            a->setVisible(showAdvancedActions || canLeaveFormation);
            break;
        }
    }
    m_menu->exec(globalPos);
}

QAction* EntityContextMenu::registerPluginAction(const QString& pluginId, QAction* action, int position)
{
    if (!action)
        return nullptr;
    if (position >= 0 && position < m_menu->actions().size())
        m_menu->insertAction(m_menu->actions().at(position), action);
    else
        m_menu->addAction(action);
    m_pluginOwnedActions[pluginId].append(action);
    return action;
}

void EntityContextMenu::unregisterPlugin(const QString& pluginId)
{
    if (!m_pluginOwnedActions.contains(pluginId))
        return;
    for (QAction* action : m_pluginOwnedActions[pluginId]) {
        m_menu->removeAction(action);
        action->deleteLater();
    }
    m_pluginOwnedActions.remove(pluginId);
}
