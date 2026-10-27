#include "trajectory_context_menu.h"

TrajectoryContextMenu::TrajectoryContextMenu(QWidget* parent)
    : QObject(parent)
{
    m_menu = new QMenu(parent);

    m_editAction = m_menu->addAction("Edit Waypoint");
    connect(m_editAction, &QAction::triggered, this, [this]() {
        emit editWaypointRequested(m_currentIndex);
    });
    m_deleteAction = m_menu->addAction("Delete Waypoint");
    connect(m_deleteAction, &QAction::triggered, this, [this]() {
        emit deleteWaypointRequested(m_currentIndex);
    });
    m_deleteAllAction = m_menu->addAction("Delete All Waypoints");
    connect(m_deleteAllAction, &QAction::triggered, this, [this]() {
        emit deleteAllWaypointsRequested();
    });
    m_menu->addSeparator();
    m_saveAction = m_menu->addAction("Finish & Save");
    connect(m_saveAction, &QAction::triggered, this, [this]() {
        emit finishAndSaveRequested();
    });
    m_cancelAction = m_menu->addAction("Cancel Edit");
    connect(m_cancelAction, &QAction::triggered, this, [this]() {
        emit cancelEditRequested();
    });
    m_menu->setStyleSheet(R"(
        QMenu {
            background-color: #1A3652;
            color: white;
            border: 1px solid #27446d;
            border-radius: 3px;
            padding: 4px 0px;
            font-size: 13px;
        }
        QMenu::item {
            padding: 8px 28px 8px 20px;
            margin: 2px 4px;
            border-radius: 2px;
        }
        QMenu::item:selected {
            background-color: #0078D4;
        }
        QMenu::separator {
            height: 1px;
            background-color: #27446d;
            margin: 4px 10px;
        }
    )");
}

void TrajectoryContextMenu::execOnWaypoint(const QPoint& globalPos, int waypointIndex)
{
    m_currentIndex = waypointIndex;
    m_editAction->setVisible(true);
    m_deleteAction->setVisible(true);
    m_deleteAllAction->setVisible(true);
    m_menu->exec(globalPos);
}

void TrajectoryContextMenu::execOnEmpty(const QPoint& globalPos)
{
    m_currentIndex = -1;
    m_editAction->setVisible(false);
    m_deleteAction->setVisible(false);
    m_deleteAllAction->setVisible(true);
    m_menu->exec(globalPos);
}
