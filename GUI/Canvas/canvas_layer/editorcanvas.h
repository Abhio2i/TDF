#ifndef EDITORCANVAS_H
#define EDITORCANVAS_H

#include "Canvas/overlay_canvas.h"
#include "GUI/Tacticaldisplay/Gis/transformmode.h"
#include "core/Hierarchy/ECS/Struct/transform_component.h"
#include "core/Hierarchy/ECS/Struct/mesh_component.h"
#include "GUI/Canvas/context_menu/entity_context_menu.h"
#include <QPointF>
#include <QHash>
#include <QSet>
#include <QVector>
#include <QJsonArray>
#include <QColor>
#include <QKeyEvent>
#include <QSet>
#include <QTimer>
#include <QToolTip>
#include <vector>

class Hierarchy;
class TrajectoryContextMenu;


//fix by yogesh
class Hierarchy;
class TrajectoryContextMenu;
class Waypoints;

class EditorCanvas : public OverlayCanvas
{
    Q_OBJECT
public:
    explicit EditorCanvas(MapWidget* map, QWidget* parent = nullptr);

    void drawContent(QPainter& painter) override;
    bool handleMousePress(QMouseEvent* event) override;
    bool handleMouseMove(QMouseEvent* event) override;
    bool handleMouseRelease(QMouseEvent* event) override;
    void setTransformMode(TransformMode mode);
    TransformMode transformMode() const { return m_mode; }

    bool hasSelection() const { return m_hasSelection; }
    ECS::Entity selectedEntity() const { return m_selectedEntity; }
    void clearSelection();

    void setHierarchy(Hierarchy* hierarchy);
    void setSimulationRunning(bool running) { m_simulationRunning = running; }
    bool isSimulationRunning() const { return m_simulationRunning; }
    void selectEntityById(const QString& hierarchyId);
    EntityContextMenu* entityContextMenu() const { return m_entityContextMenu; }
    void selectEntitiesById(const QStringList& hierarchyIds);
    void setIncludeGisInSelection(bool include);
    bool m_selectionModeActive = false;

    // Trajectory edit
    void startTrajectoryEdit(const QString& hierarchyId);
    void stopTrajectoryEdit(bool save = true);
    bool isEditingTrajectory() const { return m_isEditingTrajectory; }
    QString editingTrajectoryEntityId() const { return m_editingTrajectoryEntityId; }

    struct EditableWaypoint {
        double lat        = 0.0;
        double lon        = 0.0;
        double alt        = 0.0;
        float  speed      = 0.0f;
        bool   dropWeapon = false;
        int    dropCount  = 0;
    };
    const QVector<EditableWaypoint>& editWaypoints() const { return m_editWaypoints; }
    int selectedWaypointIndex() const { return m_selectedWaypointIndex; }
    void setTrajectoryColor(const QString& hierarchyId, const QColor& color);
    QColor trajectoryColor(const QString& hierarchyId) const;
    void setTrajectoriesVisible(bool visible);
    bool trajectoriesVisible() const { return m_showTrajectories; }
    void setEntitiesVisible(bool visible);
    bool entitiesVisible() const { return m_showEntities; }
    void setTooltipEnabled(bool enabled);
    bool tooltipEnabled() const { return m_showTooltip; }
    void setTooltipOptions(const QSet<QString>& options);
    bool getEntityGeo(const QString& hierarchyId, double& outLat, double& outLon) const;
    void onWaypointSelectedFromInspector(const QString& entityId, int waypointIndex);
    QSet<ECS::Entity> selectedEntities() const;

signals:
    void entitySelectedById(const QString& hierarchyId);
    void entityRenameRequested(const QString& hierarchyId, const QString& newName);
    void entityRemoveRequested(const QString& hierarchyId);
    void trajectoryUpdated(const QString& entityId, const QJsonArray& waypoints);
    void trajectoryEditStarted(const QString& hierarchyId);
    void trajectoryEditFinished(const QString& hierarchyId, bool saved);
    //added by yogesh
    void trajectoryUpdatedforLogger(QString entityId, std::vector<Waypoints *> Trajectories);
    void selectionModeCleared();
    //void selectionModeCleared();
    void entityAddRequested(const QString& parentId, const QString& baseName,
                            bool isProfileParent, const QVariantMap& components,
                            double initLat, double initLon);
    void entityDoubleClicked(const QString& hierarchyId);
    void entityActiveChanged(const QString& hierarchyId, bool active);
    void selectionChanged();
    void entityLeaveFormationRequested(const QString& hierarchyId);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    bool findEntityAt(const QPointF& canvasPos, ECS::Entity& outEntity, float& outIconRadius);
    bool placeEntityAtGeo(const QString& hierarchyId, double lat, double lon);
    QString findSelectedShapeAt(const QPointF& canvasPos) const;
    void selectEntity(ECS::Entity entity);
    void beginDrag(const QPointF& canvasPos);
    void updateMove(const QPointF& canvasPos);
    void updateRotate(const QPointF& canvasPos);
    void endDrag();
    void drawSelectionCircle(QPainter& painter, const QPointF& canvasPos, float radius) const;
    void drawMoveGizmo(QPainter& painter, const QPointF& canvasPos) const;
    void drawRotateGizmo(QPainter& painter, const QPointF& canvasPos, float headingDeg) const;

    void rebuildEntityIdCache();
    void onHierarchyEntityAdded(QString parentID, QString ID, QString entityName);
    void onHierarchyEntityRemoved(QString ID);

    void onEntityRenameRequested(const QString& hierarchyId);
    void onEntityRemoveRequested(const QString& hierarchyId);
    void onChangeTrajectoryColorRequested(const QString& hierarchyId);
    void onToggleTrajectoryVisibilityRequested(const QString& hierarchyId);
    bool isEntityTrajectoryVisible(const QString& hierarchyId) const;

    // Trajectory edit
    void saveEditedTrajectory();
    int  findNearestEditWaypoint(const QPointF& canvasPos, float maxDistPx = 14.0f) const;
    void addEditWaypointAt(const QPointF& canvasPos);
    void removeSelectedEditWaypoint();
    void removeAllEditWaypoints();
    void editWaypointDialog(int index);
    void drawTrajectoryEditOverlay(QPainter& painter);

      TransformMode m_mode = Translate;
    bool        m_hasSelection   = false;
    ECS::Entity m_selectedEntity = 0;

    bool    m_isDragging       = false;
    QPointF m_dragStartCanvas;
    double  m_dragStartLat     = 0.0;
    double  m_dragStartLon     = 0.0;
    float   m_dragStartHeading = 0.0f;

    Hierarchy* m_hierarchy = nullptr;
    bool m_simulationRunning = false;
    QHash<QString, ECS::Entity> m_idToEntity;
    QHash<ECS::Entity, QString> m_entityToId;

    EntityContextMenu* m_entityContextMenu = nullptr;
    TrajectoryContextMenu* m_trajectoryContextMenu = nullptr;

    static constexpr float kDefaultIconRadius = 15.0f;
    static constexpr float kMinHitRadius      = 12.0f;
    QSet<ECS::Entity> m_multiSelected;
    bool m_includeGis = false;
    bool m_isRubberBanding = false;
    QPoint m_rubberStart;
    QRect  m_rubberRect;
    QStringList m_selectedShapeIds;

    QHash<ECS::Entity, QPointF> m_dragStartPositions;
    QHash<QString, QPointF>     m_dragStartShapePositions;
    QHash<ECS::Entity, QVector<QPointF>> m_dragStartTrajectoryPositions;

    void startRubberBand(const QPoint& pos);
    void updateRubberBand(const QPoint& pos);
    void finishRubberBand();
    void moveSelectedGisShapes(double deltaLon, double deltaLat);

    // Trajectory edit
    bool m_isEditingTrajectory = false;
    QString m_editingTrajectoryEntityId;
    ECS::Entity m_editingTrajectoryEntity = 0;
    int m_selectedWaypointIndex = -1;
    bool m_isDraggingWaypoint = false;
    QPointF m_waypointDragStartCanvas;
    QVector<EditableWaypoint> m_editWaypoints;
    // ==================== (distance/angle preview) START ====================
    QPointF m_currentEditMousePos;
    bool    m_hasEditMousePos = false;
    // ==================== (distance/angle preview) END ======================

    QHash<QString, QColor> m_trajectoryColors;
    void drawSelectedEntityTrajectory(QPainter& painter);
    void onChangeEntitySizeRequested(const QString& hierarchyId);
    bool m_showTrajectories = true;
    bool m_showEntities = true;
    bool m_showTooltip = false;
    QSet<QString> m_activeTooltipOptions;
    QString m_hoveredEntityId;
    QTimer m_tooltipTimer;
    void checkEntityHover(const QPoint& mousePos);
    void updateHoverTooltip();
    int m_highlightedWaypointIndex = -1;
    QString m_highlightedWaypointEntityId;
    void drawRadioMessages(QPainter& painter);
        QSet<ECS::Entity> m_lastSelectionSnapshot;
};

#endif // EDITORCANVAS_H
