#ifndef CONTEXT_MENU_HANDLER_H
#define CONTEXT_MENU_HANDLER_H

#include <QMenu>
#include <QPoint>
#include <QString>
#include <functional>

// ========================================================================
// ADD THESE INCLUDES
// ========================================================================
#include "GUI/Tacticaldisplay/Gis/transformmode.h"
#include "GUI/Tacticaldisplay/Shape/IShapeHost.h"
#include "GUI/Tacticaldisplay/Shape/mesh_entry.h"

class CanvasWidget;
class GISlib;
class LayerPanel;
class ShapesFeature;

/**
 * @brief Handles all right-click context menus for CanvasWidget
 *        Completely decoupled - uses IShapeHost interface
 */
class ContextMenuHandler
{
public:
    explicit ContextMenuHandler(IShapeHost* host);
    ~ContextMenuHandler();

    void showContextMenu(
        const QPoint& globalPos,
        const QPoint& mousePos,
        bool isShapeHit,
        const QString& shapeId,
        bool isEntityHit,
        const QString& entityId,
        bool isTrajectoryHit,
        int waypointIndex,
        bool isOnEmptyCanvas,
        TransformMode currentMode,
        const QString& selectedShape
        );

    void showTrajectoryContextMenu(
        const QPoint& globalPos,
        int waypointIndex,
        int totalWaypoints
        );

    void showShapeContextMenu(
        const QPoint& globalPos,
        const QString& shapeId,
        MeshEntry* shapeEntry
        );

    void showEntityContextMenu(
        const QPoint& globalPos,
        const QString& entityId,
        MeshEntry* entry
        );

    void showGeneralContextMenu(
        const QPoint& globalPos,
        const QPoint& mousePos,
        bool isScenarioEditor,
        bool isRuntimeEditor
        );

    // Callbacks
    using ShapeFinalizeCallback = std::function<void(const QString&, const QPointF&)>;
    using ShapeDrawingCallback = std::function<void(bool, const QString&)>;
    using TrajectoryCallback = std::function<void(bool)>;
    using EntityCallback = std::function<void(const QString&)>;
    using AddEntityCallback = std::function<void(const QPoint&)>;
    using AddShapeCallback = std::function<void(const QString&)>;
    using AddTextCallback = std::function<void(const QPoint&)>;
    using PasteShapeCallback = std::function<void()>;
    using BoxSelectionCallback = std::function<void()>;
    using IsBoxSelectionModeCallback = std::function<bool()>;
    using EditWaypointCallback = std::function<void(int)>;
    using DeleteWaypointCallback = std::function<void(int)>;
    using DeleteAllWaypointsCallback = std::function<void()>;
    using CancelTrajectoryCallback = std::function<void()>;
    using RenameCallback = std::function<void(const QString&)>;
    using ResizeCallback = std::function<void(const QString&)>;
    using TrajectoryColorCallback = std::function<void(const QString&)>;
    using SendMessageCallback = std::function<void(const QString&)>;


    void setShapeFinalizeCallback(ShapeFinalizeCallback callback) { m_shapeFinalizeCallback = callback; }
    void setShapeDrawingCallback(ShapeDrawingCallback callback) { m_shapeDrawingCallback = callback; }
    void setTrajectoryCallback(TrajectoryCallback callback) { m_trajectoryCallback = callback; }
    void setEntityCallback(EntityCallback callback) { m_entityCallback = callback; }
    void setAddEntityCallback(AddEntityCallback callback) { m_addEntityCallback = callback; }
    void setAddShapeCallback(AddShapeCallback callback) { m_addShapeCallback = callback; }
    void setAddTextCallback(AddTextCallback callback) { m_addTextCallback = callback; }
    void setPasteShapeCallback(PasteShapeCallback callback) { m_pasteShapeCallback = callback; }
    void setBoxSelectionCallback(BoxSelectionCallback callback) { m_boxSelectionCallback = callback; }
    void setIsBoxSelectionModeCallback(IsBoxSelectionModeCallback callback) { m_isBoxSelectionModeCallback = callback; }

    void setRenameCallback(RenameCallback callback) { m_renameCallback = callback; }
    void setResizeCallback(ResizeCallback callback) { m_resizeCallback = callback; }
    void setTrajectoryColorCallback(TrajectoryColorCallback callback) { m_trajectoryColorCallback = callback; }
    void setSendMessageCallback(SendMessageCallback callback) { m_sendMessageCallback = callback; }
    void setEditWaypointCallback(EditWaypointCallback callback) { m_editWaypointCallback = callback; }
    void setDeleteWaypointCallback(DeleteWaypointCallback callback) { m_deleteWaypointCallback = callback; }
    void setDeleteAllWaypointsCallback(DeleteAllWaypointsCallback callback) { m_deleteAllWaypointsCallback = callback; }
    void setCancelTrajectoryCallback(CancelTrajectoryCallback callback) { m_cancelTrajectoryCallback = callback; }

private:

    QMenu* createShapeMenu(QMenu* parent, const QString& shapeId, MeshEntry* shapeEntry);
    QMenu* createEntityMenu(QMenu* parent, const QString& entityId, MeshEntry* entry);
    QMenu* createTrajectoryMenu(QMenu* parent, int waypointIndex, int totalWaypoints);
    QMenu* createGeneralMenu(QMenu* parent, const QPoint& mousePos);

    void setupMenuStyle(QMenu* menu);
    QWidget* getParentWidget() const;
    bool isScenarioEditor() const;
    bool isRuntimeEditor() const;

    IShapeHost* m_host = nullptr;
    LayerPanel* m_layerPanel = nullptr;
    GISlib* m_gislib = nullptr;

    ShapeFinalizeCallback m_shapeFinalizeCallback;
    ShapeDrawingCallback m_shapeDrawingCallback;
    TrajectoryCallback m_trajectoryCallback;
    EntityCallback m_entityCallback;
    AddEntityCallback m_addEntityCallback;
    AddShapeCallback m_addShapeCallback;
    AddTextCallback m_addTextCallback;
    PasteShapeCallback m_pasteShapeCallback;
    BoxSelectionCallback m_boxSelectionCallback;
    IsBoxSelectionModeCallback m_isBoxSelectionModeCallback;

    RenameCallback m_renameCallback;
    ResizeCallback m_resizeCallback;
    TrajectoryColorCallback m_trajectoryColorCallback;
    SendMessageCallback m_sendMessageCallback;
    EditWaypointCallback m_editWaypointCallback;
    DeleteWaypointCallback m_deleteWaypointCallback;
    DeleteAllWaypointsCallback m_deleteAllWaypointsCallback;
    CancelTrajectoryCallback m_cancelTrajectoryCallback;


};

#endif // CONTEXT_MENU_HANDLER_H
