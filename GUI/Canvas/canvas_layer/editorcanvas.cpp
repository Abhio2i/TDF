#include "editorcanvas.h"
#include "GUI/Canvas/context_menu/trajectory_context_menu.h"
#include "GUI/Canvas/context_menu/trajectory_color_store.h"
#include "map_widget.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/Components/trajectory.h"
#include "core/Hierarchy/Struct/waypoints.h"
#include "core/Hierarchy/ECS/Struct/trajectory_component.h"
#include "GUI/Tacticaldisplay/waypointeditdialog.h"
#include "backend/api/shape_api.h"
#include "backend/managers/shape_manager.h"
#include "GUI/Canvas/context_menu/entity_image_size_store.h"
#include "core/Hierarchy/ECS/Struct/Meta_Data_Component.h"
#include "core/Hierarchy/EntityProfiles/formation.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "GUI/Tacticaldisplay/tooltiphelper.h"
#include "GUI/dialogwindowutils.h"
#include <QLineF>
#include <QtMath>
#include <QInputDialog>
#include <QColorDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>
#include <limits>
#include <QHash>
#include <QStringList>
#include <QRect>
#include <QPoint>
#include <QDebug>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDataStream>
#include <QIODevice>
#include <QMap>
#include <QVariant>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QCursor>
#include <QToolTip>
#include <QMessageBox>

static double haversineKm(double lat1, double lon1, double lat2, double lon2);
static double bearingDeg(double lat1, double lon1, double lat2, double lon2);
static QPointF destinationPoint(double lat1, double lon1, double bearingDegVal, double distKm);
static bool entityCanLeaveFormation(Hierarchy* h, const QString& hierarchyId)
{
    if (!h || hierarchyId.isEmpty())
        return false;

    auto eit = h->Entities.find(hierarchyId.toStdString());
    if (eit == h->Entities.end() || !eit->second)
        return false;

    Platform* pl = dynamic_cast<Platform*>(eit->second);
    Formation* form = pl ? pl->dynamicModel.formation : nullptr;

    if (!form) {
        for (const auto& pair : h->Entities) {
            Formation* f = dynamic_cast<Formation*>(pair.second);
            if (f && f->isAllyEntity(hierarchyId)) {
                form = f;
                break;
            }
        }
    }
    if (!form)
        return false;

    return form->isAllyEntity(hierarchyId) &&
           !form->isMothershipEntity(hierarchyId);
}
EditorCanvas::EditorCanvas(MapWidget *map, QWidget *parent)
    : OverlayCanvas(map, parent)
{
    setAcceptDrops(true);
    m_entityContextMenu = new EntityContextMenu(this);
    connect(m_entityContextMenu, &EntityContextMenu::renameRequested,
            this, &EditorCanvas::onEntityRenameRequested);
    connect(m_entityContextMenu, &EntityContextMenu::removeRequested,
            this, &EditorCanvas::onEntityRemoveRequested);
    connect(m_entityContextMenu, &EntityContextMenu::editTrajectoryRequested,
            this, &EditorCanvas::startTrajectoryEdit);
    connect(m_entityContextMenu, &EntityContextMenu::changeTrajectoryColorRequested,
            this, &EditorCanvas::onChangeTrajectoryColorRequested);
    connect(m_entityContextMenu, &EntityContextMenu::changeEntitySizeRequested,
            this, &EditorCanvas::onChangeEntitySizeRequested);
    connect(m_entityContextMenu, &EntityContextMenu::toggleTrajectoryVisibilityRequested,
            this, &EditorCanvas::onToggleTrajectoryVisibilityRequested);
    m_trajectoryContextMenu = new TrajectoryContextMenu(this);
    m_trajectoryContextMenu = new TrajectoryContextMenu(this);
    connect(m_trajectoryContextMenu, &TrajectoryContextMenu::editWaypointRequested,
            this, &EditorCanvas::editWaypointDialog);
    connect(m_trajectoryContextMenu, &TrajectoryContextMenu::deleteWaypointRequested,
            this, [this](int /*index*/) {
                removeSelectedEditWaypoint();
            });
    connect(m_trajectoryContextMenu, &TrajectoryContextMenu::deleteAllWaypointsRequested,
            this, [this]() {
                removeAllEditWaypoints();
            });
    connect(m_trajectoryContextMenu, &TrajectoryContextMenu::finishAndSaveRequested,
            this, [this]() { stopTrajectoryEdit(true); });
    connect(m_trajectoryContextMenu, &TrajectoryContextMenu::cancelEditRequested,
            this, [this]() { stopTrajectoryEdit(false); });
    m_tooltipTimer.setSingleShot(true);
    connect(&m_tooltipTimer, &QTimer::timeout, this, &EditorCanvas::updateHoverTooltip);
    connect(m_entityContextMenu, &EntityContextMenu::leaveFormationRequested,
            this, [this](const QString& entityId) {
                emit entityLeaveFormationRequested(entityId);
            });
}

// ---------------------------------------------------------------------
// Mode / Selection helpers
// ---------------------------------------------------------------------

void EditorCanvas::setTransformMode(TransformMode mode)
{
    if (m_mode == mode)
        return;
    endDrag();
    m_mode = mode;
    update();
}

void EditorCanvas::setIncludeGisInSelection(bool include)
{
    m_includeGis = include;
    m_selectionModeActive = true;
}
QSet<ECS::Entity> EditorCanvas::selectedEntities() const
{
    if (!m_multiSelected.isEmpty())
        return m_multiSelected;
    if (m_hasSelection)
        return QSet<ECS::Entity>{ m_selectedEntity };
    return {};
}
void EditorCanvas::clearSelection()
{
    endDrag();
    m_hasSelection = false;
    m_multiSelected.clear();
    m_selectedShapeIds.clear();
    update();
}

void EditorCanvas::selectEntity(ECS::Entity entity)
{
    m_multiSelected.clear();
    m_selectedShapeIds.clear();
    m_hasSelection   = true;
    m_selectedEntity = entity;
    update();
}

// ---------------------------------------------------------------------
// Hierarchy ID cache
// ---------------------------------------------------------------------

void EditorCanvas::rebuildEntityIdCache()
{
    m_idToEntity.clear();
    m_entityToId.clear();
    if (!m_hierarchy)
        return;
    for (const auto& pair : m_hierarchy->Entities) {
        if (!pair.second)
            continue;
        const QString id = QString::fromStdString(pair.first);
        m_idToEntity.insert(id, pair.second->ecsID);
        m_entityToId.insert(pair.second->ecsID, id);
    }
}

void EditorCanvas::onHierarchyEntityAdded(QString parentID, QString ID, QString entityName)
{
    Q_UNUSED(parentID)
    Q_UNUSED(entityName)

    if (!m_hierarchy)
        return;

    auto it = m_hierarchy->Entities.find(ID.toStdString());
    if (it == m_hierarchy->Entities.end() || !it->second)
        return;

    m_idToEntity.insert(ID, it->second->ecsID);
    m_entityToId.insert(it->second->ecsID, ID);
}

void EditorCanvas::onHierarchyEntityRemoved(QString ID)
{
    auto it = m_idToEntity.find(ID);
    if (it == m_idToEntity.end())
        return;
    const ECS::Entity removedEntity = it.value();
    m_entityToId.remove(removedEntity);
    m_idToEntity.erase(it);
    m_multiSelected.remove(removedEntity);
    if (m_hasSelection && m_entityToId.value(m_selectedEntity).isEmpty())
        clearSelection();
    if (m_isEditingTrajectory && m_editingTrajectoryEntityId == ID)
        stopTrajectoryEdit(false);
}

void EditorCanvas::selectEntitiesById(const QStringList& hierarchyIds)
{
    endDrag();
    m_hasSelection = false;
    m_multiSelected.clear();
    m_selectedShapeIds.clear();
    for (const QString& id : hierarchyIds) {
        auto it = m_idToEntity.find(id);
        if (it != m_idToEntity.end())
            m_multiSelected.insert(it.value());
    }
    m_hasSelection = !m_multiSelected.isEmpty();
    update();
}

void EditorCanvas::setHierarchy(Hierarchy* hierarchy)
{
    if (m_hierarchy == hierarchy)
        return;
    m_hierarchy = hierarchy;
    rebuildEntityIdCache();
    if (m_hierarchy) {
        connect(m_hierarchy, &Hierarchy::entityAdded,
                this, &EditorCanvas::onHierarchyEntityAdded);
        connect(m_hierarchy, &Hierarchy::entityRemoved,
                this, &EditorCanvas::onHierarchyEntityRemoved);
    }
}

void EditorCanvas::selectEntityById(const QString& hierarchyId)
{
    auto it = m_idToEntity.find(hierarchyId);
    if (it == m_idToEntity.end())
        return;
    selectEntity(it.value());
}

void EditorCanvas::onEntityRenameRequested(const QString& hierarchyId)
{
    QInputDialog dialog(this);
    dialog.setWindowTitle("Rename Entity");
    dialog.setLabelText("Enter New Name:");
    dialog.setTextValue(QString());
    dialog.setInputMode(QInputDialog::TextInput);
    makeDialogResizable(&dialog);
    dialog.setWindowModality(Qt::WindowModal);
    if (dialog.exec() != QDialog::Accepted)
        return;
    QString newName = dialog.textValue();
    if (newName.trimmed().isEmpty())
        return;
    emit entityRenameRequested(hierarchyId, newName);
}

void EditorCanvas::onEntityRemoveRequested(const QString& hierarchyId)
{
    emit entityRemoveRequested(hierarchyId);
}

void EditorCanvas::onChangeTrajectoryColorRequested(const QString& hierarchyId)
{
    if (hierarchyId.isEmpty())
        return;
    QColor current = m_trajectoryColors.value(hierarchyId, QColor(0, 100, 255));
    QColor newColor = QColorDialog::getColor(
        current, this, "Select Trajectory Color",
        QColorDialog::DontUseNativeDialog | QColorDialog::ShowAlphaChannel);
    if (!newColor.isValid())
        return;
    setTrajectoryColor(hierarchyId, newColor);
}

void EditorCanvas::setTrajectoryColor(const QString& hierarchyId, const QColor& color)
{
    if (hierarchyId.isEmpty() || !color.isValid())
        return;

    m_trajectoryColors[hierarchyId] = color;
    TrajectoryColorStore::instance().setByHierarchyId(hierarchyId, color);

    auto it = m_idToEntity.find(hierarchyId);
    if (it != m_idToEntity.end()) {
        TrajectoryColorStore::instance().setByEntity(it.value(), color);
    }
    if (getMap())
        getMap()->update();
    update();
}

QColor EditorCanvas::trajectoryColor(const QString& hierarchyId) const
{
    return m_trajectoryColors.value(hierarchyId, QColor(0, 100, 255));
}

// ---------------------------------------------------------------------
// Picking
// ---------------------------------------------------------------------

bool EditorCanvas::findEntityAt(const QPointF& canvasPos, ECS::Entity& outEntity, float& outIconRadius)
{
    ECS::World* world = getWorld();
    MapWidget*  map   = getMap();
    if (!world || !map)
        return false;

    bool        found      = false;
    float       bestDist   = std::numeric_limits<float>::max();
    ECS::Entity bestEntity = 0;
    float       bestRadius = kDefaultIconRadius;

    for (ECS::Entity e : world->Query<ECS::GeocordComponent>()) {
        ECS::GeocordComponent* geo = world->GetComponent<ECS::GeocordComponent>(e);
        if (!geo)
            continue;

        if (ECS::Entity_Data* data = world->GetComponent<ECS::Entity_Data>(e)) {
            if (!data->active || data->destroy)
                continue;
        } else {
            const QString hierarchyId = m_entityToId.value(e, QString());
            if (!hierarchyId.isEmpty() && m_hierarchy) {
                auto eit = m_hierarchy->Entities.find(hierarchyId.toStdString());
                if (eit != m_hierarchy->Entities.end() && eit->second && !eit->second->Active)
                    continue;
            }
        }

        const QPointF entityCanvasPos = map->geoToCanvas(geo->lat, geo->lon);
        float radius = kDefaultIconRadius;
        if (ECS::SpriteComponent* sprite = world->GetComponent<ECS::SpriteComponent>(e))
            radius = qMax(sprite->scale / 2.0f, kMinHitRadius);
        const float dist = static_cast<float>(QLineF(entityCanvasPos, canvasPos).length());
        if (dist <= radius && dist < bestDist) {
            found      = true;
            bestDist   = dist;
            bestEntity = e;
            bestRadius = radius;
        }
    }
    if (found) {
        outEntity     = bestEntity;
        outIconRadius = bestRadius;
    }
    return found;
}

// ---------------------------------------------------------------------
// Rubber-band selection
// ---------------------------------------------------------------------

void EditorCanvas::startRubberBand(const QPoint& pos)
{
    m_isRubberBanding = true;
    m_rubberStart = pos;
    m_rubberRect = QRect(pos, QSize());
    m_multiSelected.clear();
    m_selectedShapeIds.clear();
    m_hasSelection = false;
    m_selectedEntity = 0;
    update();
}

void EditorCanvas::updateRubberBand(const QPoint& pos)
{
    if (!m_isRubberBanding)
        return;
    m_rubberRect = QRect(m_rubberStart, pos).normalized();
    update();
}

void EditorCanvas::finishRubberBand()
{
    if (!m_isRubberBanding)
        return;
    m_isRubberBanding = false;

    MapWidget* map = getMap();
    ECS::World* world = getWorld();
    if (!map || !world) {
        update();
        return;
    }

    for (ECS::Entity e : world->Query<ECS::GeocordComponent>()) {
        ECS::GeocordComponent* geo = world->GetComponent<ECS::GeocordComponent>(e);
        if (!geo)
            continue;

        if (ECS::Entity_Data* data = world->GetComponent<ECS::Entity_Data>(e)) {
            if (!data->active || data->destroy)
                continue;
        } else {
            const QString hierarchyId = m_entityToId.value(e, QString());
            if (!hierarchyId.isEmpty() && m_hierarchy) {
                auto eit = m_hierarchy->Entities.find(hierarchyId.toStdString());
                if (eit != m_hierarchy->Entities.end() && eit->second && !eit->second->Active)
                    continue;
            }
        }

        const QPointF canvasPos = map->geoToCanvas(geo->lat, geo->lon);
        if (m_rubberRect.contains(canvasPos.toPoint())) {
            m_multiSelected.insert(e);
        }
    }

    if (m_includeGis) {
        QPointF tl = map->canvasToGeo(m_rubberRect.topLeft());
        QPointF br = map->canvasToGeo(m_rubberRect.bottomRight());

        const double minLon = qMin(tl.x(), br.x());
        const double maxLon = qMax(tl.x(), br.x());
        const double minLat = qMin(tl.y(), br.y());
        const double maxLat = qMax(tl.y(), br.y());

        QRectF geoRect(QPointF(minLon, minLat), QPointF(maxLon, maxLat));
        QVector<QString> shapeIds = ShapeManager::instance()->findShapesInRect(geoRect);
        m_selectedShapeIds = shapeIds.toList();
    }

    m_hasSelection = !m_multiSelected.isEmpty() || !m_selectedShapeIds.isEmpty();
    if (m_multiSelected.size() == 1 && m_selectedShapeIds.isEmpty()) {
        m_selectedEntity = *m_multiSelected.begin();
        const QString hierarchyId = m_entityToId.value(m_selectedEntity, QString());
        if (!hierarchyId.isEmpty())
            emit entitySelectedById(hierarchyId);
    }
    update();
}

// ---------------------------------------------------------------------
// Drag / Move / Rotate
// ---------------------------------------------------------------------

void EditorCanvas::beginDrag(const QPointF& canvasPos)
{
    if (m_simulationRunning)
        return;
    ECS::World* world = getWorld();
    if (!world)
        return;
    m_isDragging      = true;
    m_dragStartCanvas = canvasPos;
    m_dragStartPositions.clear();
    m_dragStartTrajectoryPositions.clear();

    for (ECS::Entity e : m_multiSelected) {
        ECS::GeocordComponent* geo = world->GetComponent<ECS::GeocordComponent>(e);
        if (geo)
            m_dragStartPositions.insert(e, QPointF(geo->lon, geo->lat));
        if (m_selectionModeActive) {
            ECS::trajectoryPathComponent* pathComp = world->GetComponent<ECS::trajectoryPathComponent>(e);
            if (pathComp && pathComp->activeCount > 0) {
                QVector<QPointF> pts;
                pts.reserve(pathComp->activeCount);
                for (int i = 0; i < pathComp->activeCount; ++i)
                    pts.append(QPointF(pathComp->path[i].lon, pathComp->path[i].lat));
                m_dragStartTrajectoryPositions.insert(e, pts);
            }
        }
    }
    if (m_hasSelection && m_multiSelected.isEmpty() && m_selectedShapeIds.isEmpty()) {
        ECS::GeocordComponent* geo = world->GetComponent<ECS::GeocordComponent>(m_selectedEntity);
        if (geo) {
            m_dragStartLat     = geo->lat;
            m_dragStartLon     = geo->lon;
            m_dragStartHeading = geo->heading;
            m_dragStartPositions.insert(m_selectedEntity, QPointF(geo->lon, geo->lat));
        }

        if (m_selectionModeActive) {
            ECS::trajectoryPathComponent* pathComp = world->GetComponent<ECS::trajectoryPathComponent>(m_selectedEntity);
            if (pathComp && pathComp->activeCount > 0) {
                QVector<QPointF> pts;
                pts.reserve(pathComp->activeCount);
                for (int i = 0; i < pathComp->activeCount; ++i)
                    pts.append(QPointF(pathComp->path[i].lon, pathComp->path[i].lat));
                m_dragStartTrajectoryPositions.insert(m_selectedEntity, pts);
            }
        }
    }

    m_dragStartShapePositions.clear();
    for (const QString& sid : m_selectedShapeIds) {
        QJsonObject props = ShapeApi::instance()->getShapeProperties(sid);
        if (!props.isEmpty()) {
            m_dragStartShapePositions.insert(sid,
                                             QPointF(props["longitude"].toDouble(), props["latitude"].toDouble()));
        }
    }
}

void EditorCanvas::updateMove(const QPointF& canvasPos)
{
    ECS::World* world = getWorld();
    MapWidget*  map   = getMap();
    if (!world || !map || !m_isDragging)
        return;
    if (m_simulationRunning)
        return;
    const QPointF startGeo = map->canvasToGeo(m_dragStartCanvas);
    const QPointF nowGeo   = map->canvasToGeo(canvasPos);
    const double dLon = nowGeo.x() - startGeo.x();
    const double dLat = nowGeo.y() - startGeo.y();

    for (auto it = m_dragStartPositions.constBegin(); it != m_dragStartPositions.constEnd(); ++it) {
        ECS::Entity e = it.key();
        ECS::GeocordComponent* geo = world->GetComponent<ECS::GeocordComponent>(e);
        if (!geo)
            continue;
        const double newLon = it.value().x() + dLon;
        const double newLat = it.value().y() + dLat;
        geo->lon = newLon;
        geo->lat = newLat;
        const QString hierarchyId = m_entityToId.value(e, QString());
        if (!hierarchyId.isEmpty() && m_hierarchy) {
            auto pit = m_hierarchy->Platforms.find(hierarchyId.toStdString());
            if (pit != m_hierarchy->Platforms.end() && pit->second) {
                pit->second->transform.setGeoCord(newLat, newLon);
            } else {
                auto eit = m_hierarchy->Entities.find(hierarchyId.toStdString());
                if (eit != m_hierarchy->Entities.end() && eit->second) {
                    if (Platform* platform = dynamic_cast<Platform*>(eit->second))
                        platform->transform.setGeoCord(newLat, newLon);
                    else
                        if (Specialzone* spzone = dynamic_cast<Specialzone*>(eit->second))
                            spzone->transform.setGeoCord(newLat, newLon);
                        else
                            if (FixedPoints* fxd = dynamic_cast<FixedPoints*>(eit->second))
                                fxd->transform.setGeoCord(newLat, newLon);
                }
            }
        }
    }

    if (m_selectionModeActive) {
        for (auto it = m_dragStartTrajectoryPositions.constBegin(); it != m_dragStartTrajectoryPositions.constEnd(); ++it) {
            ECS::Entity e = it.key();
            ECS::trajectoryPathComponent* pathComp = world->GetComponent<ECS::trajectoryPathComponent>(e);
            if (!pathComp)
                continue;
            const QVector<QPointF>& startPts = it.value();
            const int count = qMin(static_cast<int>(pathComp->activeCount), static_cast<int>(startPts.size()));
            if (count <= 0)
                continue;


            QVector<QPointF> newPts(count);
            newPts[0] = QPointF(startPts[0].x() + dLon, startPts[0].y() + dLat);

            for (int i = 1; i < count; ++i) {
                const double segDistKm = haversineKm(startPts[i - 1].y(), startPts[i - 1].x(),
                                                     startPts[i].y(),     startPts[i].x());
                const double segBearing = bearingDeg(startPts[i - 1].y(), startPts[i - 1].x(),
                                                     startPts[i].y(),     startPts[i].x());
                newPts[i] = destinationPoint(newPts[i - 1].y(), newPts[i - 1].x(),
                                             segBearing, segDistKm);
            }

            for (int i = 0; i < count; ++i) {
                pathComp->path[i].lon = newPts[i].x();
                pathComp->path[i].lat = newPts[i].y();
            }
            pathComp->delta = true;

            const QString hierarchyId = m_entityToId.value(e, QString());
            if (!hierarchyId.isEmpty() && m_hierarchy) {
                auto pit = m_hierarchy->Platforms.find(hierarchyId.toStdString());
                if (pit != m_hierarchy->Platforms.end() && pit->second) {
                    Trajectory& traj = pit->second->trajectory;
                    const int wpCount = qMin(static_cast<int>(traj.Trajectories.size()), count);
                    for (int i = 0; i < wpCount; ++i) {
                        Waypoints* wp = traj.Trajectories[i];
                        if (!wp || !wp->position)
                            continue;
                        wp->position->x = static_cast<float>(newPts[i].y());
                        wp->position->z = static_cast<float>(newPts[i].x());
                    }
                }
            }
        }
    }

    if (m_includeGis && !m_selectedShapeIds.isEmpty()) {
        for (auto it = m_dragStartShapePositions.constBegin(); it != m_dragStartShapePositions.constEnd(); ++it) {
            const QString& sid = it.key();
            const QPointF& start = it.value();
            ShapeApi::instance()->moveShape(sid, start.x() + dLon, start.y() + dLat);
        }
        map->update();
    }
    update();
}

void EditorCanvas::updateRotate(const QPointF& canvasPos)
{
    ECS::World* world = getWorld();
    MapWidget*  map   = getMap();
    if (!world || !map || !m_isDragging)
        return;

    ECS::Entity target = m_selectedEntity;
    if (m_multiSelected.size() == 1)
        target = *m_multiSelected.begin();

    ECS::GeocordComponent* geo = world->GetComponent<ECS::GeocordComponent>(target);
    if (!geo)
        return;

    const QPointF entityCanvasPos = map->geoToCanvas(geo->lat, geo->lon);
    const double  dx = canvasPos.x() - entityCanvasPos.x();
    const double  dy = canvasPos.y() - entityCanvasPos.y();

    double heading = qRadiansToDegrees(std::atan2(dx, -dy));
    if (heading < 0.0)
        heading += 360.0;

    geo->heading = static_cast<float>(heading);
    const QString hierarchyId = m_entityToId.value(target, QString());
    if (!hierarchyId.isEmpty() && m_hierarchy) {
        auto pit = m_hierarchy->Platforms.find(hierarchyId.toStdString());
        if (pit != m_hierarchy->Platforms.end() && pit->second) {
            pit->second->transform.setHeading(geo->heading);
        } else {
            auto eit = m_hierarchy->Entities.find(hierarchyId.toStdString());
            if (eit != m_hierarchy->Entities.end() && eit->second) {
                if (Platform* platform = dynamic_cast<Platform*>(eit->second)) {
                    platform->transform.setHeading(geo->heading);
                }
            }
        }
    }
    // ============================================
    update();
}

void EditorCanvas::endDrag()
{
    m_isDragging = false;
    m_dragStartPositions.clear();
    m_dragStartShapePositions.clear();
    m_dragStartTrajectoryPositions.clear();
}

void EditorCanvas::moveSelectedGisShapes(double deltaLon, double deltaLat)
{
    for (const QString& id : m_selectedShapeIds) {
        ShapeApi::instance()->moveShapeBy(id, deltaLon, deltaLat);
    }
    if (getMap())
        getMap()->update();
}

// ---------------------------------------------------------------------
// Mouse handling
// ---------------------------------------------------------------------

bool EditorCanvas::handleMousePress(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return false;

    if (m_isEditingTrajectory) {
        int idx = findNearestEditWaypoint(event->pos());
        if (idx >= 0) {
            m_selectedWaypointIndex = idx;
            m_isDraggingWaypoint = true;
            m_waypointDragStartCanvas = event->pos();
            update();
            return true;
        }
        addEditWaypointAt(event->pos());
        return true;
    }

    const bool inSelectionMode = (m_mode == Translate && m_selectionModeActive);

    if (inSelectionMode) {
        ECS::Entity hitEntity;
        float hitRadius = 0.f;

        if (findEntityAt(event->pos(), hitEntity, hitRadius)) {
            if (m_multiSelected.contains(hitEntity) ||
                (m_hasSelection && hitEntity == m_selectedEntity)) {
                beginDrag(event->pos());
                return true;
            }

            selectEntity(hitEntity);
            const QString hierarchyId = m_entityToId.value(hitEntity, QString());
            if (!hierarchyId.isEmpty())
                emit entitySelectedById(hierarchyId);
            beginDrag(event->pos());
            return true;
        }

        if (m_includeGis && !m_selectedShapeIds.isEmpty()) {
            const QString hitShapeId = findSelectedShapeAt(event->pos());
            if (!hitShapeId.isEmpty()) {
                beginDrag(event->pos());
                return true;
            }
        }

        if (m_hasSelection || !m_multiSelected.isEmpty() || !m_selectedShapeIds.isEmpty()) {
            clearSelection();
            m_selectionModeActive = false;
            emit selectionModeCleared();
            return true;
        }

        startRubberBand(event->pos());
        return true;
    }

    //modes (View / Move / Rotate)
    ECS::Entity hitEntity;
    float hitRadius = 0.f;
    if (findEntityAt(event->pos(), hitEntity, hitRadius)) {
        selectEntity(hitEntity);
        const QString hierarchyId = m_entityToId.value(hitEntity, QString());
        if (!hierarchyId.isEmpty())
            emit entitySelectedById(hierarchyId);

        if (m_mode == Translate || m_mode == Rotate)
            beginDrag(event->pos());
        return true;
    }

    clearSelection();
    return false;
}

bool EditorCanvas::handleMouseMove(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        if (m_isDragging)
            endDrag();
        if (m_isDraggingWaypoint)
            m_isDraggingWaypoint = false;
        if (m_isRubberBanding) {
            m_isRubberBanding = false;
            m_rubberRect = QRect();
            update();
        }
    }
    if (m_isEditingTrajectory && m_isDraggingWaypoint &&
        m_selectedWaypointIndex >= 0 &&
        m_selectedWaypointIndex < m_editWaypoints.size())
    {
        MapWidget* map = getMap();
        if (!map) return true;
        QPointF geo = map->canvasToGeo(event->pos());
        m_editWaypoints[m_selectedWaypointIndex].lat = geo.y();
        m_editWaypoints[m_selectedWaypointIndex].lon = geo.x();
        update();
        return true;
    }
    // ====================(distance/angle preview) START ====================
    if (m_isEditingTrajectory && !m_isDraggingWaypoint) {
        m_currentEditMousePos = event->pos();
        m_hasEditMousePos     = true;
        update();
        return true;
    }
    // ====================(distance/angle preview) END ======================

    if (m_isRubberBanding) {
        if (!(event->buttons() & Qt::LeftButton))
            return true;
        updateRubberBand(event->pos());
        return true;
    }

    if (!m_isDragging) {
        checkEntityHover(event->pos());
        return false;
    }

    if (!(event->buttons() & Qt::LeftButton))
        return false;

    if (m_mode == Translate) {
        updateMove(event->pos());
        return true;
    }

    if (m_mode == Rotate) {
        updateRotate(event->pos());
        return true;
    }
    return false;
}

bool EditorCanvas::handleMouseRelease(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        if (m_isDragging)
            endDrag();
        if (m_isDraggingWaypoint)
            m_isDraggingWaypoint = false;
        if (m_isRubberBanding) {
            m_isRubberBanding = false;
            m_rubberRect = QRect();
            update();
        }
        return false;
    }

    if (event->button() != Qt::LeftButton)
        return false;
    if (m_isDraggingWaypoint) {
        m_isDraggingWaypoint = false;
        return true;
    }

    if (m_isRubberBanding) {
        finishRubberBand();
        return true;
    }

    if (!m_isDragging)
        return false;

    endDrag();
    return true;
}

void EditorCanvas::contextMenuEvent(QContextMenuEvent* event)
{
    if (m_isDragging)
        endDrag();
    if (m_isDraggingWaypoint)
        m_isDraggingWaypoint = false;
    if (m_isRubberBanding) {
        m_isRubberBanding = false;
        m_rubberRect = QRect();
        update();
    }

    MapWidget* map = getMap();
    if (map && map->isGisInteractionEnabled()) {
        event->ignore();
        return;
    }
    if (m_isEditingTrajectory) {
        int idx = findNearestEditWaypoint(event->pos());
        if (idx >= 0) {
            m_selectedWaypointIndex = idx;
            m_trajectoryContextMenu->execOnWaypoint(event->globalPos(), idx);
        } else {
            m_trajectoryContextMenu->execOnEmpty(event->globalPos());
        }
        event->accept();
        return;
    }
    ECS::Entity hitEntity;
    float hitRadius = 0.f;
    if (findEntityAt(event->pos(), hitEntity, hitRadius)) {
        selectEntity(hitEntity);
        const QString hierarchyId = m_entityToId.value(hitEntity, QString());
        if (!hierarchyId.isEmpty()) {
            emit entitySelectedById(hierarchyId);

            bool showAdvanced = true;
            if (m_hierarchy) {
                auto it = m_hierarchy->Entities.find(hierarchyId.toStdString());
                if (it != m_hierarchy->Entities.end() && it->second) {
                    Platform* platform = dynamic_cast<Platform*>(it->second);
                    showAdvanced = (platform != nullptr);
                }
            }
            const bool trajVisible = isEntityTrajectoryVisible(hierarchyId);
            if (findEntityAt(event->pos(), hitEntity, hitRadius)) {
                selectEntity(hitEntity);
                const QString hierarchyId = m_entityToId.value(hitEntity, QString());
                if (!hierarchyId.isEmpty()) {
                    emit entitySelectedById(hierarchyId);

                    bool showAdvanced = true;
                    if (m_hierarchy) {
                        auto it = m_hierarchy->Entities.find(hierarchyId.toStdString());
                        if (it != m_hierarchy->Entities.end() && it->second) {
                            Platform* platform = dynamic_cast<Platform*>(it->second);
                            showAdvanced = (platform != nullptr);
                        }
                    }
                    const bool trajVisible = isEntityTrajectoryVisible(hierarchyId);
                    const bool canLeave    = entityCanLeaveFormation(m_hierarchy, hierarchyId);

                    m_entityContextMenu->exec(
                        event->globalPos(),
                        hierarchyId,
                        showAdvanced,
                        trajVisible,
                        canLeave);
                }
                event->accept();
                return;
            }
        }
        event->accept();
        return;
    }
    OverlayCanvas::contextMenuEvent(event);
}

void EditorCanvas::keyPressEvent(QKeyEvent* event)
{
    if (m_isEditingTrajectory) {
        if (event->key() == Qt::Key_Escape) {
            stopTrajectoryEdit(false);
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            stopTrajectoryEdit(true);
            return;
        }
        if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
            removeSelectedEditWaypoint();
            return;
        }
    }

    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        if (!m_selectedShapeIds.isEmpty()) {
            const int count = m_selectedShapeIds.size();
            const QString msg = (count == 1)
                                    ? QStringLiteral("Are you sure you want to delete this shape?")
                                    : QStringLiteral("Are you sure you want to delete %1 shapes?").arg(count);
            if (QMessageBox::question(this, QStringLiteral("Delete Shape"),
                                      msg,
                                      QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                for (const QString& sid : m_selectedShapeIds)
                    ShapeApi::instance()->deleteShape(sid);
                m_selectedShapeIds.clear();
                if (getMap())
                    getMap()->update();
                update();
            }
            event->accept();
            return;
        }

        QStringList entityIds;
        if (!m_multiSelected.isEmpty()) {
            for (ECS::Entity e : m_multiSelected) {
                const QString id = m_entityToId.value(e, QString());
                if (!id.isEmpty())
                    entityIds.append(id);
            }
        } else if (m_hasSelection) {
            const QString id = m_entityToId.value(m_selectedEntity, QString());
            if (!id.isEmpty())
                entityIds.append(id);
        }

        if (!entityIds.isEmpty()) {
            for (const QString& id : entityIds)
                emit entityRemoveRequested(id);
            event->accept();
            return;
        }
    }

    if (event->key() == Qt::Key_M) {
        ECS::Entity targetEntity = 0;
        bool haveTarget = false;

        if (!m_multiSelected.isEmpty()) {
            targetEntity = *m_multiSelected.begin();
            haveTarget = true;
        } else if (m_hasSelection) {
            targetEntity = m_selectedEntity;
            haveTarget = true;
        }

        if (!haveTarget || !m_hierarchy) {
            event->accept();
            return;
        }

        const QString hierarchyId = m_entityToId.value(targetEntity, QString());
        if (hierarchyId.isEmpty()) {
            event->accept();
            return;
        }

        Platform* platform = nullptr;
        auto pit = m_hierarchy->Platforms.find(hierarchyId.toStdString());
        if (pit != m_hierarchy->Platforms.end() && pit->second) {
            platform = pit->second;
        } else {
            auto eit = m_hierarchy->Entities.find(hierarchyId.toStdString());
            if (eit != m_hierarchy->Entities.end() && eit->second)
                platform = dynamic_cast<Platform*>(eit->second);
        }

        if (!platform || !platform->Active) {
            event->accept();
            return;
        }

        QInputDialog msgDialog(this);
        msgDialog.setWindowTitle(tr("Send Message"));
        msgDialog.setLabelText(tr("Enter message:"));
        msgDialog.setTextValue(QString());
        msgDialog.setInputMode(QInputDialog::TextInput);
        makeDialogResizable(&msgDialog);
        msgDialog.setWindowModality(Qt::WindowModal);

        bool ok = (msgDialog.exec() == QDialog::Accepted);
        QString text = msgDialog.textValue();

        if (ok && !text.isEmpty()) {
            for (auto const& pair : platform->radios.radios) {
                if (pair.second)
                    pair.second->sendMsg(text.toStdString());
            }

            if (getMap())
                getMap()->update();
            update();
            QPointer<EditorCanvas> safeThis(this);
            QTimer::singleShot(5000, this, [safeThis]() {
                if (!safeThis || !safeThis->m_hierarchy)
                    return;
                for (const auto& pPair : safeThis->m_hierarchy->Platforms) {
                    Platform* p = pPair.second;
                    if (!p)
                        continue;
                    for (auto const& rPair : p->radios.radios) {
                        if (rPair.second)
                            rPair.second->msg.clear();
                    }
                }
                if (safeThis->getMap())
                    safeThis->getMap()->update();
                safeThis->update();
            });
        }
        event->accept();
        return;
    }

    OverlayCanvas::keyPressEvent(event);
}

// ---------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------

void EditorCanvas::drawContent(QPainter& painter)
{
    ECS::World* world = getWorld();
    MapWidget*  map   = getMap();
    if (!world || !map)
        return;


    if (m_isRubberBanding && !m_rubberRect.isNull()) {
        painter.save();
        painter.setPen(QPen(QColor(0, 180, 255), 1.5, Qt::DashLine));
        painter.setBrush(QColor(0, 180, 255, 40));
        painter.drawRect(m_rubberRect);
        painter.restore();
    }

    if (!m_multiSelected.isEmpty()) {
        for (ECS::Entity e : m_multiSelected) {
            ECS::GeocordComponent* geo = world->GetComponent<ECS::GeocordComponent>(e);
            if (!geo)
                continue;
            if (ECS::Entity_Data* data = world->GetComponent<ECS::Entity_Data>(e)) {
                if (!data->active || data->destroy)
                    continue;
            }

            const QPointF canvasPos = map->geoToCanvas(geo->lat, geo->lon);
            float radius = kDefaultIconRadius;
            if (ECS::SpriteComponent* sprite = world->GetComponent<ECS::SpriteComponent>(e))
                radius = qMax(sprite->scale / 2.0f, kMinHitRadius);

            drawSelectionCircle(painter, canvasPos, radius);
        }
    }
    else if (m_hasSelection) {
        bool selectedActive = true;
        if (ECS::Entity_Data* data = world->GetComponent<ECS::Entity_Data>(m_selectedEntity)) {
            if (!data->active || data->destroy)
                selectedActive = false;
        }
        if (selectedActive) {
            ECS::GeocordComponent* geo = world->GetComponent<ECS::GeocordComponent>(m_selectedEntity);
            if (geo) {
                const QPointF canvasPos = map->geoToCanvas(geo->lat, geo->lon);

                float radius = kDefaultIconRadius;
                if (ECS::SpriteComponent* sprite = world->GetComponent<ECS::SpriteComponent>(m_selectedEntity))
                    radius = qMax(sprite->scale / 2.0f, kMinHitRadius);

                switch (m_mode) {
                case Translate:
                    drawMoveGizmo(painter, canvasPos);
                    break;
                case Rotate:
                    drawRotateGizmo(painter, canvasPos, geo->heading);
                    break;
                default:
                    drawSelectionCircle(painter, canvasPos, radius);
                    break;
                }
            }
        }
    }
    if (m_includeGis && !m_selectedShapeIds.isEmpty()) {
        painter.save();
        painter.setPen(QPen(QColor(0, 220, 120), 2.0, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        for (const QString& sid : m_selectedShapeIds) {
            QJsonObject props = ShapeApi::instance()->getShapeProperties(sid);
            if (props.isEmpty())
                continue;
            double lon = props["longitude"].toDouble();
            double lat = props["latitude"].toDouble();
            QPointF c = map->geoToCanvas(lat, lon);
            painter.drawEllipse(c, 18, 18);
        }
        painter.restore();
    }
    drawSelectedEntityTrajectory(painter);
    if (m_isEditingTrajectory)
        drawTrajectoryEditOverlay(painter);
    drawRadioMessages(painter);

}

void EditorCanvas::drawSelectionCircle(QPainter& painter, const QPointF& canvasPos, float radius) const
{
    painter.save();
    QPen pen(QColor(255, 0, 0));
    pen.setWidth(2);
    pen.setStyle(Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(canvasPos, radius, radius);
    painter.restore();
}

void EditorCanvas::drawMoveGizmo(QPainter& painter, const QPointF& canvasPos) const
{
    constexpr int kArmLength = 40;
    painter.save();
    QPen vPen(Qt::blue);
    vPen.setWidth(2);
    painter.setPen(vPen);
    painter.drawLine(canvasPos, canvasPos - QPointF(0, kArmLength));
    QPen hPen(Qt::red);
    hPen.setWidth(2);
    painter.setPen(hPen);
    painter.drawLine(canvasPos, canvasPos + QPointF(kArmLength, 0));
    painter.restore();
}

void EditorCanvas::drawRotateGizmo(QPainter& painter, const QPointF& canvasPos, float headingDeg) const
{
    constexpr int kRadius = 30;
    painter.save();
    QPen ringPen(QColor(255, 215, 0));
    ringPen.setWidth(2);
    ringPen.setStyle(Qt::DashLine);
    painter.setPen(ringPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(canvasPos, kRadius, kRadius);
    const double  rad = qDegreesToRadians(static_cast<double>(headingDeg));
    const QPointF tip = canvasPos + QPointF(std::sin(rad), -std::cos(rad)) * kRadius;
    QPen tipPen(Qt::red);
    tipPen.setWidth(2);
    painter.setPen(tipPen);
    painter.drawLine(canvasPos, tip);
    painter.restore();
}

// ---------------------------------------------------------------------
// Trajectory Edit Mode
// ---------------------------------------------------------------------

void EditorCanvas::startTrajectoryEdit(const QString& hierarchyId)
{
    if (!m_hierarchy || hierarchyId.isEmpty())
        return;
    auto itEnt = m_hierarchy->Entities.find(hierarchyId.toStdString());
    if (itEnt == m_hierarchy->Entities.end() || !itEnt->second)
        return;
    Entity* entity = itEnt->second;
    Trajectory* traj = nullptr;
    auto pit = m_hierarchy->Platforms.find(hierarchyId.toStdString());
    if (pit != m_hierarchy->Platforms.end() && pit->second) {
        traj = &pit->second->trajectory;
    }
    m_editWaypoints.clear();
    if (traj) {
        for (const Waypoints* wp : traj->Trajectories) {
            if (!wp || !wp->position) continue;
            EditableWaypoint e;
            e.lat   = wp->position->x;
            e.alt   = wp->position->y;
            e.lon   = wp->position->z;
            e.speed = wp->speed;
            m_editWaypoints.append(e);
        }
    }
    m_isEditingTrajectory       = true;
    m_editingTrajectoryEntityId = hierarchyId;
    m_editingTrajectoryEntity   = entity->ecsID;
    m_selectedWaypointIndex     = -1;
    m_isDraggingWaypoint        = false;
    m_hasEditMousePos           = false;
    selectEntity(entity->ecsID);
    setCursor(Qt::CrossCursor);
    emit trajectoryEditStarted(hierarchyId);
    update();
}

void EditorCanvas::stopTrajectoryEdit(bool save)
{
    if (!m_isEditingTrajectory)
        return;
    if (save)
        saveEditedTrajectory();
    const QString id = m_editingTrajectoryEntityId;
    m_isEditingTrajectory = false;
    m_editingTrajectoryEntityId.clear();
    m_editingTrajectoryEntity = 0;
    m_editWaypoints.clear();
    m_selectedWaypointIndex = -1;
    m_isDraggingWaypoint = false;
    m_hasEditMousePos = false;
    setCursor(Qt::ArrowCursor);
    emit trajectoryEditFinished(id, save);
    update();
}



//fix by yogesh
void EditorCanvas::saveEditedTrajectory()
{
    if (!m_hierarchy || m_editingTrajectoryEntityId.isEmpty())
        return;

    const std::string idStd = m_editingTrajectoryEntityId.toStdString();
    Trajectory* traj = nullptr;

    auto pit = m_hierarchy->Platforms.find(idStd);
    if (pit != m_hierarchy->Platforms.end() && pit->second) {
        traj = &pit->second->trajectory;
    }
    if (!traj) {
        qWarning() << "[EditorCanvas] No Platform/Trajectory for"
                   << m_editingTrajectoryEntityId;
        return;
    }

    for (Waypoints* wp : traj->Trajectories) {
        if (wp) {
            if (wp->position) {
                delete wp->position;
                wp->position = nullptr;
            }
            if (wp->geocord) {
                delete wp->geocord;
                wp->geocord = nullptr;
            }
            delete wp;
        }
    }
    traj->Trajectories.clear();

    QJsonArray waypointsArray;
    for (const auto& e : m_editWaypoints) {
        Waypoints* newWp = new Waypoints();
        newWp->position = new Vector(
            static_cast<float>(e.lat),
            static_cast<float>(e.alt),
            static_cast<float>(e.lon));
        newWp->geocord = new Geocords();
        newWp->geocord->latitude  = e.lat;
        newWp->geocord->longitude = e.lon;
        newWp->geocord->altitude  = e.alt;
        newWp->geocord->Heading   = 0.0;
        newWp->speed = e.speed;
        newWp->dropWeapon = e.dropWeapon;
        newWp->dropcount  = static_cast<float>(e.dropCount);
        traj->addTrajectory(newWp);
        waypointsArray.append(newWp->toJson());
    }
    traj->Active = !traj->Trajectories.empty();

    // ── ECS path (StaticCanvas live draw) ──
    auto itEnt = m_hierarchy->Entities.find(idStd);
    if (itEnt != m_hierarchy->Entities.end() && itEnt->second) {
        ECS::World* world = getWorld();
        if (world) {
            ECS::trajectoryPathComponent* pathComp =
                world->GetComponent<ECS::trajectoryPathComponent>(itEnt->second->ecsID);
            if (pathComp) {
                pathComp->activeCount = 0;
                const int n = static_cast<int>(traj->Trajectories.size());
                for (int i = 0; i < n; ++i) {
                    Waypoints* wp = traj->Trajectories[i];
                    if (!wp || !wp->position)
                        continue;
                    pathComp->path[i].lat = wp->position->x;
                    pathComp->path[i].lon = wp->position->z;
                    pathComp->activeCount++;
                }
                pathComp->delta = true;
            }
        }
    }

    // // ── NEW: logger / recording ──
    // // Is signal se RuntimeEditor → Recording::trajectoryCRUD(UPDATE)
    // // bina iske replay mein updated path nahi aata
    // emit trajectoryUpdatedForLogger(
    //     m_editingTrajectoryEntityId,
    //     traj->Trajectories);

    emit trajectoryUpdatedforLogger(m_editingTrajectoryEntityId, traj->Trajectories);
    if (getMap())
        getMap()->update();
    update();
}
int EditorCanvas::findNearestEditWaypoint(const QPointF& canvasPos, float maxDistPx) const
{
    MapWidget* map = const_cast<EditorCanvas*>(this)->getMap();
    if (!map || m_editWaypoints.isEmpty())
        return -1;

    int best = -1;
    float bestDist = maxDistPx;

    for (int i = 0; i < m_editWaypoints.size(); ++i) {
        QPointF p = map->geoToCanvas(m_editWaypoints[i].lat, m_editWaypoints[i].lon);
        float d = static_cast<float>(QLineF(p, canvasPos).length());
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

void EditorCanvas::addEditWaypointAt(const QPointF& canvasPos)
{
    MapWidget* map = getMap();
    if (!map)
        return;

    QPointF geo = map->canvasToGeo(canvasPos);
    EditableWaypoint e;
    e.lat   = geo.y();
    e.lon   = geo.x();
    e.alt   = 0.0;
    e.speed = 0.0f;
    m_editWaypoints.append(e);
    m_selectedWaypointIndex = m_editWaypoints.size() - 1;
    update();
}

void EditorCanvas::removeSelectedEditWaypoint()
{
    if (m_selectedWaypointIndex < 0 || m_selectedWaypointIndex >= m_editWaypoints.size())
        return;
    m_editWaypoints.removeAt(m_selectedWaypointIndex);
    if (m_selectedWaypointIndex >= m_editWaypoints.size())
        m_selectedWaypointIndex = m_editWaypoints.size() - 1;
    update();
}

void EditorCanvas::editWaypointDialog(int index)
{
    if (index < 0 || index >= m_editWaypoints.size())
        return;

    EditableWaypoint& e = m_editWaypoints[index];
    double latitude   = e.lat;
    double longitude  = e.lon;
    double altitude   = e.alt;
    double speed      = static_cast<double>(e.speed);
    bool   dropWeapon = e.dropWeapon;
    int    dropCount  = e.dropCount;

    if (WaypointEditDialog::editWaypoint(this->window(), latitude, longitude, altitude, speed, dropWeapon, dropCount)) {
        e.lat        = latitude;
        e.lon        = longitude;
        e.alt        = altitude;
        e.speed      = static_cast<float>(speed);
        e.dropWeapon = dropWeapon;
        e.dropCount  = dropCount;
        m_selectedWaypointIndex = index;
        update();
    }
}
static double haversineKm(double lat1, double lon1, double lat2, double lon2);
static double bearingDeg(double lat1, double lon1, double lat2, double lon2);

void EditorCanvas::drawTrajectoryEditOverlay(QPainter& painter)
{
    MapWidget* map = getMap();
    if (!map || m_editWaypoints.isEmpty())
        return;
    painter.save();
    QPen pathPen(QColor(255, 20, 147), 2, Qt::SolidLine);
    painter.setPen(pathPen);
    painter.setBrush(Qt::NoBrush);
    QPolygonF poly;
    poly.reserve(m_editWaypoints.size());
    for (const auto& wp : m_editWaypoints)
        poly << map->geoToCanvas(wp.lat, wp.lon);
    if (poly.size() >= 2)
        painter.drawPolyline(poly);

    for (int i = 0; i < poly.size(); ++i) {
        const bool selected = (i == m_selectedWaypointIndex);

        if (selected) {
            painter.setPen(QPen(Qt::yellow, 2));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(poly[i], 8, 8);
            painter.setPen(QPen(QColor(255, 20, 147), 5));
            painter.drawPoint(poly[i]);
        } else {
            painter.setPen(QPen(QColor(255, 20, 147), 5));
            painter.drawPoint(poly[i]);
        }

        painter.setPen(selected ? QColor(255, 80, 180) : QColor(255, 20, 147));
        painter.setFont(QFont("Arial", 8, QFont::Bold));
        painter.drawText(poly[i] + QPointF(8, -6), QString::number(i + 1));
    }
    // ====================  (distance/angle preview) START ====================
    if (m_hasEditMousePos && !m_isDraggingWaypoint && !m_editWaypoints.isEmpty()) {
        const EditableWaypoint& last = m_editWaypoints.last();
        const QPointF mouseGeo = map->canvasToGeo(m_currentEditMousePos);
        const double distKm = haversineKm(last.lat, last.lon, mouseGeo.y(), mouseGeo.x());
        const double brg    = bearingDeg(last.lat, last.lon, mouseGeo.y(), mouseGeo.x());
        const QPointF lastPt = poly.last();
        painter.setPen(QPen(QColor(255, 20, 147, 160), 1, Qt::DashLine));
        painter.drawLine(lastPt, m_currentEditMousePos);
        const QString label = (distKm < 1.0)
                                  ? QString("%1 m, %2°").arg(distKm * 1000.0, 0, 'f', 0).arg(brg, 0, 'f', 0)
                                  : QString("%1 km, %2°").arg(distKm, 0, 'f', 2).arg(brg, 0, 'f', 0);
        const QFont labelFont("Arial", 9, QFont::Bold);
        const QFontMetrics fm(labelFont);
        const QRectF textRect = fm.boundingRect(label);
        const QPointF textPos = m_currentEditMousePos + QPointF(14, 18);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 160));
        painter.drawRoundedRect(
            QRectF(textPos.x() - 4, textPos.y() - textRect.height(),
                   textRect.width() + 8, textRect.height() + 6),
            4, 4);

        painter.setFont(labelFont);
        painter.setPen(Qt::white);
        painter.drawText(textPos, label);
    }
    // ====================  (distance/angle preview) END ====================

    painter.restore();
}

static double haversineKm(double lat1, double lon1, double lat2, double lon2)
{
    constexpr double R = 6371.0;
    const double dLat = qDegreesToRadians(lat2 - lat1);
    const double dLon = qDegreesToRadians(lon2 - lon1);
    const double a =
        qSin(dLat / 2) * qSin(dLat / 2) +
        qCos(qDegreesToRadians(lat1)) * qCos(qDegreesToRadians(lat2)) *
            qSin(dLon / 2) * qSin(dLon / 2);
    return R * 2.0 * qAtan2(qSqrt(a), qSqrt(1.0 - a));
}
void EditorCanvas::drawSelectedEntityTrajectory(QPainter& painter)
{
    if (m_isEditingTrajectory)
        return;
    // if (!m_showTrajectories)
    //     return;

    ECS::World* world = getWorld();
    MapWidget*  map   = getMap();
    if (!world || !map)
        return;
    {
        const QSet<ECS::Entity> sel = selectedEntities();
        if (sel != m_lastSelectionSnapshot) {
            m_lastSelectionSnapshot = sel;
            emit selectionChanged();
        }
    }

    QSet<ECS::Entity> targets;
    if (!m_multiSelected.isEmpty())
        targets = m_multiSelected;
    else if (m_hasSelection)
        targets.insert(m_selectedEntity);

    if (targets.isEmpty())
        return;

    painter.save();

    for (ECS::Entity e : targets) {
        if (ECS::Entity_Data* data = world->GetComponent<ECS::Entity_Data>(e)) {
            if (!data->active || data->destroy)
                continue;
        } else {
            const QString hierarchyId = m_entityToId.value(e, QString());
            if (!hierarchyId.isEmpty() && m_hierarchy) {
                auto eit = m_hierarchy->Entities.find(hierarchyId.toStdString());
                if (eit != m_hierarchy->Entities.end() && eit->second && !eit->second->Active)
                    continue;
            }
        }

        ECS::trajectoryPathComponent* pathComp =
            world->GetComponent<ECS::trajectoryPathComponent>(e);
        if (!pathComp || pathComp->activeCount < 1 || !pathComp->active)
            continue;

        const QString hierarchyId = m_entityToId.value(e, QString());
        QColor trajectoryColor = m_trajectoryColors.value(
            hierarchyId,
            QColor(0, 128, 128)
            );

        QPolygonF poly;
        poly.reserve(pathComp->activeCount);
        for (int i = 0; i < pathComp->activeCount; ++i) {
            poly << map->geoToCanvas(
                pathComp->path[i].lat,
                pathComp->path[i].lon);
        }

        if (poly.size() >= 2) {
            painter.setPen(QPen(trajectoryColor, 2, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            painter.drawPolyline(poly);
        }

        const bool highlightThisEntity =
            (m_highlightedWaypointIndex >= 0) &&
            (m_highlightedWaypointEntityId.isEmpty() ||
             m_highlightedWaypointEntityId == hierarchyId);

        for (int i = 0; i < poly.size(); ++i) {
            const bool selected = highlightThisEntity && (i == m_highlightedWaypointIndex);
            if (selected) {
                painter.setPen(QPen(Qt::yellow, 2));
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(poly[i], 8, 8);
                painter.setPen(QPen(QColor(255, 220, 0), 6));
                painter.drawPoint(poly[i]);
            } else {
                painter.setPen(QPen(trajectoryColor, 5));
                painter.drawPoint(poly[i]);
            }
        }

        painter.setPen(Qt::black);
        painter.setFont(QFont("Arial", 8, QFont::Bold));

        double distance = 0.0;
        for (int i = 0; i < pathComp->activeCount; ++i) {
            const double lat = pathComp->path[i].lat;
            const double lon = pathComp->path[i].lon;
            QPointF point = map->geoToCanvas(lat, lon);

            if (i > 0) {
                distance += haversineKm(
                    pathComp->path[i - 1].lat, pathComp->path[i - 1].lon,
                    lat, lon);
            }

            painter.drawText(
                point + QPointF(7, -7),
                QString("WP%1 %2km")
                    .arg(i + 1)
                    .arg(QString::number(distance, 'f', 0))
                );
            painter.drawText(
                point + QPointF(7, -20),
                QString("%1 ,%2")
                    .arg(QString::number(lat, 'f', 3))
                    .arg(QString::number(lon, 'f', 3))
                );
        }
    }
    painter.restore();
}
QString EditorCanvas::findSelectedShapeAt(const QPointF& canvasPos) const
{
    MapWidget* map = const_cast<EditorCanvas*>(this)->getMap();
    if (!map)
        return QString();

    for (const QString& sid : m_selectedShapeIds) {
        QJsonObject props = ShapeApi::instance()->getShapeProperties(sid);
        if (props.isEmpty())
            continue;

        const double lon = props["longitude"].toDouble();
        const double lat = props["latitude"].toDouble();
        const QPointF center = map->geoToCanvas(lat, lon);

        if (QLineF(center, canvasPos).length() <= 18.0)
            return sid;
    }
    return QString();
}
void EditorCanvas::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    MapWidget* map = getMap();
    if (map && map->isGisInteractionEnabled()) {
        event->ignore();
        return;
    }

    if (m_isEditingTrajectory) {
        event->accept();
        return;
    }

    ECS::Entity hitEntity = 0;
    float hitRadius = 0.f;
    if (!findEntityAt(event->pos(), hitEntity, hitRadius)) {
        event->ignore();
        return;
    }

    selectEntity(hitEntity);
    const QString hierarchyId = m_entityToId.value(hitEntity, QString());
    if (hierarchyId.isEmpty()) {
        event->accept();
        return;
    }

    emit entitySelectedById(hierarchyId);
    emit entityDoubleClicked(hierarchyId);
    event->accept();
}
// ---------------------------------------------------------------------
// Hierarchy tree>canvas drag & drop
// ---------------------------------------------------------------------

void EditorCanvas::dragEnterEvent(QDragEnterEvent* event)
{
    if (!event || !event->mimeData()) {
        event->ignore();
        return;
    }
    if (event->mimeData()->hasFormat(QStringLiteral("application/x-qabstractitemmodeldatalist")) ||
        event->mimeData()->hasFormat(QStringLiteral("application/x-entity"))) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void EditorCanvas::dragMoveEvent(QDragMoveEvent* event)
{
    if (!event || !event->mimeData()) {
        event->ignore();
        return;
    }
    if (event->mimeData()->hasFormat(QStringLiteral("application/x-qabstractitemmodeldatalist")) ||
        event->mimeData()->hasFormat(QStringLiteral("application/x-entity"))) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

bool EditorCanvas::placeEntityAtGeo(const QString& hierarchyId, double lat, double lon)
{
    if (hierarchyId.isEmpty())
        return false;
    if (m_simulationRunning)
        return false;
    bool placed = false;
    auto it = m_idToEntity.find(hierarchyId);
    if (it != m_idToEntity.end()) {
        ECS::World* world = getWorld();
        if (world) {
            ECS::GeocordComponent* geo = world->GetComponent<ECS::GeocordComponent>(it.value());
            if (geo) {
                geo->lat = lat;
                geo->lon = lon;
                placed = true;
            }
        }
    }

    if (m_hierarchy) {
        auto pit = m_hierarchy->Platforms.find(hierarchyId.toStdString());
        if (pit != m_hierarchy->Platforms.end() && pit->second) {
            pit->second->transform.setGeoCord(lat, lon);
            pit->second->Active = true;
            auto* data = pit->second->root->world.GetComponent<ECS::Entity_Data>(pit->second->ecsID);
            if (data) data->active = true;
            placed = true;

            QJsonObject delta;
            delta["active"] = true;
            m_hierarchy->UpdateComponent(hierarchyId, "_self", delta);
            emit entityActiveChanged(hierarchyId, true);
        } else {
            auto eit = m_hierarchy->Entities.find(hierarchyId.toStdString());
            if (eit != m_hierarchy->Entities.end() && eit->second) {
                if (Platform* platform = dynamic_cast<Platform*>(eit->second)) {
                    platform->transform.setGeoCord(lat, lon);
                    platform->Active = true;
                    auto* data = platform->root->world.GetComponent<ECS::Entity_Data>(platform->ecsID);
                    if (data) data->active = true;
                    placed = true;

                    QJsonObject delta;
                    delta["active"] = true;
                    m_hierarchy->UpdateComponent(hierarchyId, "_self", delta);
                    emit entityActiveChanged(hierarchyId, true);
                }
                else if (Specialzone* spzone = dynamic_cast<Specialzone*>(eit->second)) {
                    spzone->transform.setGeoCord(lat, lon);
                    spzone->Active = true;
                    auto* data = spzone->root->world.GetComponent<ECS::Entity_Data>(spzone->ecsID);
                    if (data) data->active = true;
                    placed = true;

                    QJsonObject delta;
                    delta["active"] = true;
                    m_hierarchy->UpdateComponent(hierarchyId, "_self", delta);
                    emit entityActiveChanged(hierarchyId, true);
                }
                else if (FixedPoints* fxd = dynamic_cast<FixedPoints*>(eit->second)) {
                    fxd->transform.setGeoCord(lat, lon);
                    fxd->Active = true;
                    auto* data = fxd->root->world.GetComponent<ECS::Entity_Data>(fxd->ecsID);
                    if (data) data->active = true;
                    placed = true;

                    QJsonObject delta;
                    delta["active"] = true;
                    m_hierarchy->UpdateComponent(hierarchyId, "_self", delta);
                    emit entityActiveChanged(hierarchyId, true);
                }
            }
        }
    }
    return placed;
}

void EditorCanvas::dropEvent(QDropEvent* event)
{
    MapWidget* map = getMap();
    if (!map || !event || !event->mimeData()) {
        if (event)
            event->ignore();
        return;
    }
    const QMimeData* mimeData = event->mimeData();
    const QPointF geoPos = map->canvasToGeo(event->pos());
    const double dropLat = geoPos.y();
    const double dropLon = geoPos.x();
    bool accepted = false;
    QStringList droppedIds;

    if (mimeData->hasFormat(QStringLiteral("application/x-qabstractitemmodeldatalist"))) {
        QByteArray encoded = mimeData->data(QStringLiteral("application/x-qabstractitemmodeldatalist"));
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        while (!stream.atEnd()) {
            int row = 0;
            int col = 0;
            QMap<int, QVariant> roleDataMap;
            stream >> row >> col >> roleDataMap;

            const QVariantMap customData = roleDataMap.value(Qt::UserRole).toMap();
            const QString type = customData.value(QStringLiteral("type")).toString();
            const QString entityId = customData.value(QStringLiteral("ID")).toString();
            const QString name = customData.value(QStringLiteral("name")).toString();

            if (type == QStringLiteral("entity") &&
                !entityId.isEmpty() &&
                !name.isEmpty()) {
                if (placeEntityAtGeo(entityId, dropLat, dropLon)) {
                    accepted = true;
                    droppedIds.append(entityId);
                }
            }
        }
    }

    if (!accepted && mimeData->hasFormat(QStringLiteral("application/x-entity"))) {
        QByteArray entityData = mimeData->data(QStringLiteral("application/x-entity"));
        QDataStream stream(&entityData, QIODevice::ReadOnly);
        QString entityId;
        stream >> entityId;
        if (!entityId.isEmpty() && placeEntityAtGeo(entityId, dropLat, dropLon)) {
            accepted = true;
            droppedIds.append(entityId);
        }
    }

    if (accepted) {
        event->acceptProposedAction();
        if (!droppedIds.isEmpty()) {
            if (droppedIds.size() == 1)
                selectEntityById(droppedIds.first());
            else
                selectEntitiesById(droppedIds);
            if (!droppedIds.first().isEmpty())
                emit entitySelectedById(droppedIds.first());
        }
        if (map)
            map->update();
        update();
    } else {
        event->ignore();
    }
}
void EditorCanvas::removeAllEditWaypoints()
{
    if (!m_isEditingTrajectory)
        return;
    m_editWaypoints.clear();
    m_selectedWaypointIndex = -1;
    m_isDraggingWaypoint = false;
    update();
}
void EditorCanvas::onChangeEntitySizeRequested(const QString& hierarchyId)
{
    if (hierarchyId.isEmpty() || !getWorld())
        return;

    auto it = m_idToEntity.find(hierarchyId);
    if (it == m_idToEntity.end())
        return;

    ECS::Entity e = it.value();
    ECS::World* world = getWorld();

    float currentScale = 1.0f;

    if (ECS::SpritePath* spritePath = world->GetComponent<ECS::SpritePath>(e)) {
        if (spritePath->scale > 0.01f)
            currentScale = spritePath->scale;
    }
    else if (ECS::SpriteComponent* sprite = world->GetComponent<ECS::SpriteComponent>(e)) {
        if (sprite->scale > 0.01f)
            currentScale = sprite->scale;
    }
    else {
        float stored = EntityImageSizeStore::instance().getByEntity(e, 1.0f);
        currentScale = qBound(0.1f, stored > 20.f ? stored / 50.f : stored, 20.0f);
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Change Entity Size");
    makeDialogResizable(&dialog);
    dialog.setWindowModality(Qt::WindowModal);
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QLabel* label = new QLabel("Set image scale:", &dialog);
    QDoubleSpinBox* spin = new QDoubleSpinBox(&dialog);
    spin->setRange(0.1, 20.0);
    spin->setSingleStep(0.1);
    spin->setDecimals(1);
    spin->setValue(currentScale);

    QDialogButtonBox* buttons = new QDialogButtonBox(
    QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);

    layout->addWidget(label);
    layout->addWidget(spin);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;
    const float newScale = static_cast<float>(spin->value());
    EntityImageSizeStore::instance().setByEntity(e, newScale);
    EntityImageSizeStore::instance().setByHierarchyId(hierarchyId, newScale);
    if (ECS::SpritePath* spritePath = world->GetComponent<ECS::SpritePath>(e)) {
        spritePath->scale = newScale;
    }
    if (ECS::SpriteComponent* sprite = world->GetComponent<ECS::SpriteComponent>(e)) {
        sprite->scale = newScale;
    }
    update();
    if (MapWidget* map = getMap()) {
        map->update();
        const auto children = map->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget* child : children)
            child->update();
    }
}

void EditorCanvas::setTrajectoriesVisible(bool visible)
{
    if (m_showTrajectories == visible)
        return;
    m_showTrajectories = visible;
    update();
    if (MapWidget* map = getMap())
        map->update();
}

void EditorCanvas::setEntitiesVisible(bool visible)
{
    if (m_showEntities == visible)
        return;
    m_showEntities = visible;
    update();
    if (MapWidget* map = getMap())
        map->update();
}

void EditorCanvas::setTooltipEnabled(bool enabled)
{
    if (m_showTooltip == enabled)
        return;
    m_showTooltip = enabled;
    if (!m_showTooltip) {
        m_tooltipTimer.stop();
        m_hoveredEntityId.clear();
        QToolTip::hideText();
    }
}

void EditorCanvas::setTooltipOptions(const QSet<QString>& options)
{
    m_activeTooltipOptions = options;
    if (!m_hoveredEntityId.isEmpty())
        updateHoverTooltip();
}

void EditorCanvas::checkEntityHover(const QPoint& mousePos)
{
    if (!m_showTooltip || !m_showEntities) {
        if (!m_hoveredEntityId.isEmpty()) {
            m_hoveredEntityId.clear();
            m_tooltipTimer.stop();
            QToolTip::hideText();
        }
        return;
    }

    ECS::Entity hitEntity = 0;
    float hitRadius = 0.f;
    QString newHoverId;
    if (findEntityAt(mousePos, hitEntity, hitRadius)) {
        newHoverId = m_entityToId.value(hitEntity, QString());
    }

    if (newHoverId != m_hoveredEntityId) {
        m_hoveredEntityId = newHoverId;
        if (m_hoveredEntityId.isEmpty()) {
            QToolTip::hideText();
            m_tooltipTimer.stop();
            setCursor(Qt::ArrowCursor);
        } else {
            m_tooltipTimer.start(700);
            setCursor(Qt::PointingHandCursor);
        }
    }
}

void EditorCanvas::updateHoverTooltip()
{
    if (m_hoveredEntityId.isEmpty() || !m_showTooltip || !isVisible())
        return;
    if (!m_hierarchy)
        return;
    auto it = m_hierarchy->Entities.find(m_hoveredEntityId.toStdString());
    if (it == m_hierarchy->Entities.end() || !it->second)
        return;
    Entity* entity = it->second;
    Platform* platform = dynamic_cast<Platform*>(entity);
    if (!platform)
        return;
    MeshEntry entry;
    entry.name          = QString::fromStdString(entity->Name);
    entry.platform      = platform;
    entry.coreTransform = &platform->transform;
    entry.trajectory    = &platform->trajectory;
    entry.entity        = entity;
    entry.velocity      = nullptr;
    QString tooltipText = TooltipHelper::generateEntityTooltip(entry, m_activeTooltipOptions);
    if (!tooltipText.isEmpty()) {
        TooltipHelper::showTooltip(tooltipText, this);
    }
}
bool EditorCanvas::getEntityGeo(const QString& hierarchyId, double& outLat, double& outLon) const
{
    if (hierarchyId.isEmpty())
        return false;
    auto it = m_idToEntity.constFind(hierarchyId);
    if (it == m_idToEntity.constEnd())
        return false;
    ECS::World* world = const_cast<EditorCanvas*>(this)->getWorld();
    if (!world)
        return false;
    ECS::GeocordComponent* geo = world->GetComponent<ECS::GeocordComponent>(it.value());
    if (!geo)
        return false;
    outLat = geo->lat;
    outLon = geo->lon;
    return true;
}
void EditorCanvas::onWaypointSelectedFromInspector(const QString& entityId, int waypointIndex)
{
    m_highlightedWaypointEntityId = entityId;
    m_highlightedWaypointIndex = waypointIndex;

    if (m_isEditingTrajectory &&
        (entityId.isEmpty() || entityId == m_editingTrajectoryEntityId) &&
        waypointIndex >= 0 &&
        waypointIndex < m_editWaypoints.size()) {
        m_selectedWaypointIndex = waypointIndex;
    }
    update();
}
void EditorCanvas::drawRadioMessages(QPainter& painter)
{
    if (!m_hierarchy)
        return;
    MapWidget* map = getMap();
    if (!map)
        return;
    painter.save();
    QFont font("Arial", 10, QFont::Bold);
    painter.setFont(font);
    painter.setPen(QColor(0, 255, 0, 255));

    for (const auto& pair : m_hierarchy->Platforms) {
        Platform* platform = pair.second;
        if (!platform || !platform->Active)
            continue;

        double lat = platform->transform.getLatitude();
        double lon = platform->transform.getLongitude();
        if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0)
            continue;

        QPointF canvasPos = map->geoToCanvas(lat, lon);
        for (const auto& radioPair : platform->radios.radios) {
            Radio* radio = radioPair.second;
            if (!radio)
                continue;
            QString msg = QString::fromStdString(radio->msg);
            if (msg.isEmpty())
                continue;
            painter.drawText(canvasPos + QPointF(12, -8), msg);
        }
    }
    painter.restore();
}

bool EditorCanvas::isEntityTrajectoryVisible(const QString& hierarchyId) const
{
    if (hierarchyId.isEmpty())
        return true;

    auto it = m_idToEntity.constFind(hierarchyId);
    if (it == m_idToEntity.constEnd())
        return true;

    ECS::World* world = const_cast<EditorCanvas*>(this)->getWorld();
    if (!world)
        return true;

    ECS::trajectoryPathComponent* pathComp =
        world->GetComponent<ECS::trajectoryPathComponent>(it.value());
    if (pathComp)
        return pathComp->active;

    if (m_hierarchy) {
        auto pit = m_hierarchy->Platforms.find(hierarchyId.toStdString());
        if (pit != m_hierarchy->Platforms.end() && pit->second)
            return pit->second->trajectory.Active;
    }

    return true;
}

void EditorCanvas::onToggleTrajectoryVisibilityRequested(const QString& hierarchyId)
{
    if (hierarchyId.isEmpty())
        return;
    auto it = m_idToEntity.find(hierarchyId);
    if (it == m_idToEntity.end())
        return;
    ECS::Entity e = it.value();
    ECS::World* world = getWorld();
    if (!world)
        return;
    bool newActive = true;
    if (ECS::trajectoryPathComponent* pathComp =
        world->GetComponent<ECS::trajectoryPathComponent>(e)) {
        pathComp->active = !pathComp->active;
        newActive = pathComp->active;
        pathComp->delta = true;
    }

    if (m_hierarchy) {
        auto pit = m_hierarchy->Platforms.find(hierarchyId.toStdString());
        if (pit != m_hierarchy->Platforms.end() && pit->second) {
            pit->second->trajectory.Active = newActive;
        } else {
            auto eit = m_hierarchy->Entities.find(hierarchyId.toStdString());
            if (eit != m_hierarchy->Entities.end() && eit->second) {
                if (Platform* platform = dynamic_cast<Platform*>(eit->second))
                    platform->trajectory.Active = newActive;
            }
        }
    }

    update();
    if (MapWidget* map = getMap())
        map->update();
}

static double bearingDeg(double lat1, double lon1, double lat2, double lon2)
{
    const double phi1 = qDegreesToRadians(lat1);
    const double phi2 = qDegreesToRadians(lat2);
    const double dLon = qDegreesToRadians(lon2 - lon1);

    const double y = qSin(dLon) * qCos(phi2);
    const double x = qCos(phi1) * qSin(phi2) -
                     qSin(phi1) * qCos(phi2) * qCos(dLon);

    double brng = qRadiansToDegrees(qAtan2(y, x));
    brng = std::fmod(brng + 360.0, 360.0);
    return brng;
}

static QPointF destinationPoint(double lat1, double lon1, double bearingDegVal, double distKm)
{
    constexpr double R = 6371.0;
    const double delta   = distKm / R;
    const double theta   = qDegreesToRadians(bearingDegVal);
    const double phi1    = qDegreesToRadians(lat1);
    const double lambda1 = qDegreesToRadians(lon1);
    const double sinPhi2 = qSin(phi1) * qCos(delta) + qCos(phi1) * qSin(delta) * qCos(theta);
    const double phi2    = qAsin(qBound(-1.0, sinPhi2, 1.0));
    const double y = qSin(theta) * qSin(delta) * qCos(phi1);
    const double x = qCos(delta) - qSin(phi1) * sinPhi2;
    const double lambda2 = lambda1 + qAtan2(y, x);
    double lonDeg = qRadiansToDegrees(lambda2);
    lonDeg = std::fmod(lonDeg + 540.0, 360.0) - 180.0;
    const double latDeg = qRadiansToDegrees(phi2);
    return QPointF(lonDeg, latDeg);
}
