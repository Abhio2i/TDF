#ifndef TRAJECTORY_CONTEXT_MENU_H
#define TRAJECTORY_CONTEXT_MENU_H

#include <QObject>
#include <QMenu>
#include <QAction>
#include <QPoint>
#include <QString>

class TrajectoryContextMenu : public QObject
{
    Q_OBJECT
public:
    explicit TrajectoryContextMenu(QWidget* parent = nullptr);
    void execOnWaypoint(const QPoint& globalPos, int waypointIndex);
    void execOnEmpty(const QPoint& globalPos);

signals:
    void editWaypointRequested(int index);
    void deleteWaypointRequested(int index);
    void deleteAllWaypointsRequested();
    void finishAndSaveRequested();
    void cancelEditRequested();

private:
    QMenu* m_menu = nullptr;
    QAction* m_editAction = nullptr;
    QAction* m_deleteAction = nullptr;
    QAction* m_deleteAllAction = nullptr;
    QAction* m_saveAction = nullptr;
    QAction* m_cancelAction = nullptr;
    int m_currentIndex = -1;
};

#endif // TRAJECTORY_CONTEXT_MENU_H
