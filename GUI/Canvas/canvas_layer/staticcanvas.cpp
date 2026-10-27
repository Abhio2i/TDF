#include "staticcanvas.h"
#include "core/Hierarchy/ECS/Struct/trajectory_component.h"
#include "GUI/Canvas/context_menu/trajectory_color_store.h"
#include "core/Hierarchy/ECS/Struct/Meta_Data_Component.h"
#include "core/Hierarchy/ECS/Struct/transform_component.h"
#include "map_widget.h"

StaticCanvas::StaticCanvas(MapWidget *map, QWidget *parent):
    OverlayCanvas(map, parent)
{

}

void StaticCanvas::drawContent(QPainter& painter)
{
    if (!getMap())
        return;
    if (!getWorld())
        return;
    ECS::World* world = getWorld();
    MapWidget* map = getMap();
    const auto& en_datas = world->GetComponentArray<ECS::Entity_Data>();
    if(m_showTrajectories){
        const auto& trajArray = world->GetComponentArray<ECS::trajectoryPathComponent>();
        const auto& activeEntities = trajArray->GetActiveEntities();
        const auto& trajectorys = trajArray->GetComponents();
        batchPath.clear();
        points.clear();
        for (int i = 0; i < trajectorys.size(); i++) {
            ECS::Entity e = activeEntities[i];
            const auto& traj = trajectorys[i];
            const auto& data = en_datas->GetData(e);
            if (traj.activeCount < 1 || !data->active || data->destroy || !traj.active) continue;

            QPolygonF pathArray;
            pathArray.reserve(traj.activeCount);
            for (int p = 0; p < traj.activeCount; ++p) {
                QPointF screen = getMap()->geoToCanvas(
                    traj.path[p].lat, traj.path[p].lon);
                pathArray.append(screen);
                points.append(screen);
            }
            batchPath.addPolygon(pathArray);
        }

        painter.setPen(QPen(Qt::blue, 2, Qt::DashLine));
        painter.drawPath(batchPath);
        painter.setPen(QPen(Qt::blue, 5));
        painter.drawPoints(points);
    }

    // const auto& paths   = getWorld()->GetComponentsArray<ECS::trajectoryPathComponent>();
    // const auto& en_data = getWorld()->GetComponentsArray<ECS::Entity_Data>();
    // const int size     = paths.size();
    // const int dataSize = static_cast<int>(en_data.size());

    // batchPath.clear();
    // points.clear();

    // for (int i = 0; i < size; ++i) {
    //     if (i < dataSize && (!en_data[i].active || en_data[i].destroy))
    //         continue;
    //     if (paths[i].activeCount < 1)
    //         continue;

    //     QPolygonF pathArray;
    //     pathArray.reserve(paths[i].activeCount);
    //     for (int p = 0; p < paths[i].activeCount; ++p) {
    //         QPointF screen = getMap()->geoToCanvas(
    //             paths[i].path[p].lat, paths[i].path[p].lon);
    //         pathArray.append(screen);
    //         points.append(screen);
    //     }
    //     batchPath.addPolygon(pathArray);
    // }

    // painter.setPen(QPen(Qt::blue, 2, Qt::DashLine));
    // painter.drawPath(batchPath);
    // painter.setPen(QPen(Qt::blue, 5));
    // painter.drawPoints(points);
}

void StaticCanvas::setTrajectoriesVisible(bool visible)
{
    if (m_showTrajectories == visible)
        return;
    m_showTrajectories = visible;
    update();
    if (MapWidget* map = getMap())
        map->update();
}
bool StaticCanvas::handleMousePress(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton/* && m_isDrawingPoint*/) {
        // addPointAt(event->pos());
        // m_isDrawingPoint = false;
        setCursor(Qt::ArrowCursor);
        return true;
    }
    return false;
}

bool StaticCanvas::handleMouseMove(QMouseEvent* event)
{
    Q_UNUSED(event)
    return false;
}

bool StaticCanvas::handleMouseRelease(QMouseEvent* event)
{
    Q_UNUSED(event)
    return false;
}
