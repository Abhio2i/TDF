#include "contextmenu_handler.h"
#include "GUI/Tacticaldisplay/Shape/IShapeHost.h"
#include "GUI/Tacticaldisplay/Shape/mesh_entry.h"
#include "GUI/Tacticaldisplay/Gis/gislib.h"
#include "GUI/Tacticaldisplay/Shape/shapes_feature.h"      // ADDED - For ShapesFeature
#include "core/Hierarchy/Struct/vector.h"                 // ADDED - For Vector
#include "core/Hierarchy/EntityProfiles/platform.h"
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QWidget>
#include <QApplication>
#include <QProgressDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTimer>

// ============================================================================
// Constructor / Destructor
// ============================================================================

ContextMenuHandler::ContextMenuHandler(IShapeHost* host)
    : m_host(host)
{
    if (m_host) {
        m_layerPanel = m_host->getLayerPanel();
        m_gislib = m_host->getGISlib();
    }
}

ContextMenuHandler::~ContextMenuHandler()
{
}

// ============================================================================
// Main Context Menu Entry Point
// ============================================================================

void ContextMenuHandler::showContextMenu(
    const QPoint& globalPos,
    const QPoint& mousePos,
    bool isShapeHit,
    const QString& shapeId,
    bool isEntityHit,
    const QString& entityId,
    bool isTrajectoryHit,
    int waypointIndex,
    bool /*isOnEmptyCanvas*/,
    TransformMode currentMode,
    const QString& selectedShape)
{
    QWidget* parent = getParentWidget();
    if (!parent) return;

    // Check if we need to finalize shape drawing
    if (currentMode == TransformMode::DrawShape &&
        (selectedShape == "Line" || selectedShape == "Polygon")) {
        bool canFinalize = false;
        if (selectedShape == "Line" && m_host) {
            canFinalize = m_host->getTempLineVertices().size() >= 2;
        } else if (selectedShape == "Polygon" && m_host) {
            canFinalize = m_host->getTempPolygonVertices().size() >= 3;
        }

        if (canFinalize && m_shapeFinalizeCallback) {
            QPointF lastGeoPos;
            if (selectedShape == "Line" && m_host) {
                auto& verts = m_host->getTempLineVertices();
                if (!verts.empty()) {
                    Vector* v = verts.back();
                    lastGeoPos = QPointF(v->x, v->y);
                }
            } else if (selectedShape == "Polygon" && m_host) {
                auto& verts = m_host->getTempPolygonVertices();
                if (!verts.empty()) {
                    Vector* v = verts.back();
                    lastGeoPos = QPointF(v->x, v->y);
                }
            }
            m_shapeFinalizeCallback(selectedShape, lastGeoPos);
            return;
        }
    }

    // Shape context menu
    if (isShapeHit && !shapeId.isEmpty()) {
        MeshEntry* entry = nullptr;
        if (m_host) {
            for (auto& e : m_host->getTempMeshes()) {
                if (e.name == shapeId) {
                    entry = &e;
                    break;
                }
            }
        }
        showShapeContextMenu(globalPos, shapeId, entry);
        return;
    }

    // Entity context menu
    if (isEntityHit && !entityId.isEmpty()) {

        MeshEntry* entry = m_host ? m_host->getEntityMeshEntry(entityId) : nullptr;
        showEntityContextMenu(globalPos, entityId, entry);
        return;
    }

    // Trajectory context menu
    if (isTrajectoryHit) {

        int totalWaypoints = m_host ? m_host->getTrajectoryWaypointCount() : 0;
        showTrajectoryContextMenu(globalPos, waypointIndex, totalWaypoints);
        return;
    }

    // General context menu
    showGeneralContextMenu(globalPos, mousePos, isScenarioEditor(), isRuntimeEditor());
}

// ============================================================================
// Shape Context Menu
// ============================================================================

void ContextMenuHandler::showShapeContextMenu(
    const QPoint& globalPos,
    const QString& shapeId,
    MeshEntry* shapeEntry)
{
    QWidget* parent = getParentWidget();
    if (!parent) return;

    QMenu menu(parent);
    setupMenuStyle(&menu);

    QAction* copyAction = menu.addAction("Copy Shape");
    QObject::connect(copyAction, &QAction::triggered, [this]() {
        if (!m_host)
            return;

        m_host->cancelActiveShapeOperation();

        m_host->logMessage("Copy shape requested");
    });

    menu.addSeparator();
    if (shapeId.startsWith("TempText")) {
        QAction* editTextAction = menu.addAction("Edit Text Properties");
        QObject::connect(editTextAction, &QAction::triggered, [this, shapeId]() {
            if (m_host) {
                m_host->logMessage("Edit text: " + shapeId);
            }
        });

        QAction* deleteAction = menu.addAction("Delete Text");
        QObject::connect(deleteAction, &QAction::triggered, [this, shapeId]() {
            if (!m_host)
                return;

            m_host->cancelActiveShapeOperation();

            m_host->logMessage("Delete text: " + shapeId);
        });
    }
    else if (shapeId.startsWith("TempPolyline") ||
             shapeId.startsWith("TempCircle") ||
             shapeId.startsWith("TempRectangle") ||
             shapeId.startsWith("TempPolygon") ||
             shapeId.startsWith("TempPoint")) {

        QAction* editAction = menu.addAction("Edit");
        QObject::connect(editAction, &QAction::triggered, [this, shapeId]() {
            if (!m_host)
                return;

            m_host->startShapeEdit(shapeId);
            m_host->logMessage("Edit shape: " + shapeId);
        });
        QAction* propertiesAction = menu.addAction("Shape Properties");
        QObject::connect(propertiesAction, &QAction::triggered, [this, shapeId]() {
            if (!m_host)
                return;

            m_host->cancelActiveShapeOperation();

            ShapesFeature* sf = m_host->getShapesFeature();
            if (sf) {
                m_host->logMessage("Properties for: " + shapeId);
            }
        });
        if (shapeEntry && m_host) {
            ShapesFeature* sf = m_host->getShapesFeature();
            if (sf) {
                if (sf->hasHistory(shapeId)) {
                    QAction* historyAction = menu.addAction("History Preview");
                    QObject::connect(historyAction, &QAction::triggered, [sf, shapeId]() {
                        sf->showHistoryPreview(shapeId);
                    });

                    QAction* applyHistoryAction = menu.addAction("History Restore");
                    QObject::connect(applyHistoryAction, &QAction::triggered, [sf, shapeId, shapeEntry]() {
                        if (sf->restorePreviousState(shapeId, shapeEntry)) {
                            sf->hideHistoryPreview();
                        }
                    });
                }

                if (sf->isShowingPreview() && sf->getPreviewShapeId() == shapeId) {
                    QAction* hidePreviewAction = menu.addAction("Hide Preview");
                    QObject::connect(hidePreviewAction, &QAction::triggered, [sf]() {
                        sf->hideHistoryPreview();
                    });
                }
            }
        }

        QAction* rotateAction = menu.addAction("Rotate");
        QObject::connect(rotateAction, &QAction::triggered, [this, shapeId]() {
            if (!m_host)
                return;

            m_host->startShapeRotate(shapeId);

        });
        QAction* deleteAction = menu.addAction("Delete Shape");
        QObject::connect(deleteAction, &QAction::triggered, [this, shapeId]() {
            if (!m_host)
                return;

            m_host->cancelActiveShapeOperation();

            m_host->logMessage("Delete shape: " + shapeId);
        });
    }
    else if (shapeEntry && !shapeEntry->bitmapPath.isEmpty()) {
        QAction* editAction = menu.addAction("Edit");
        QObject::connect(editAction, &QAction::triggered, [this, shapeId]() {
            if (!m_host)
                return;

            m_host->startShapeEdit(shapeId);
            m_host->logMessage("Edit image: " + shapeId);
        });

        QAction* rotateAction = menu.addAction("Rotate");
        QObject::connect(rotateAction, &QAction::triggered, [this, shapeId]() {
            if (!m_host)
                return;

            m_host->startShapeRotate(shapeId);
            m_host->logMessage("Rotate image: " + shapeId);
        });

        QString deleteText = shapeId.startsWith("TempBitmap") ? "Delete Bitmap" : "Delete Image";
        QAction* deleteAction = menu.addAction(deleteText);
        QObject::connect(deleteAction, &QAction::triggered, [this, shapeId]() {
            if (!m_host)
                return;

            m_host->cancelActiveShapeOperation();

            m_host->logMessage("Delete Image: " + shapeId);
        });
    }

    menu.exec(globalPos);
}

// ============================================================================
// Entity Context Menu
// ============================================================================

void ContextMenuHandler::showEntityContextMenu(
    const QPoint& globalPos,
    const QString& entityId,
    MeshEntry* entry)
{
    QWidget* parent = getParentWidget();
    if (!parent) return;

    QMenu menu(parent);
    setupMenuStyle(&menu);

    bool isScenario = isScenarioEditor();
    bool isRuntime = isRuntimeEditor();

    // Common actions
    QAction* removeAction = menu.addAction("Remove");
    QObject::connect(removeAction, &QAction::triggered, [this, entityId]() {
        if (m_entityCallback) m_entityCallback(entityId);
    });

    QAction* renameAction = menu.addAction("Rename");
    QObject::connect(renameAction, &QAction::triggered, [this, entityId]() {
        if (m_renameCallback) m_renameCallback(entityId);
    });

    // Platform-specific actions
    if (entry && entry->platform) {
        // Edit Trajectory
        QAction* editTrajectoryAction = menu.addAction("Edit Trajectory");
        QObject::connect(editTrajectoryAction, &QAction::triggered, [this]() {
            if (m_trajectoryCallback) m_trajectoryCallback(true);
        });

        // Change Size
        QAction* setSizeAction = menu.addAction("Change Entity Size");
        QObject::connect(setSizeAction, &QAction::triggered, [this, entityId]() {
            if (m_resizeCallback) m_resizeCallback(entityId);
        });

        // Change Trajectory Color
        QAction* changeTrajectoryColorAction = menu.addAction("Change Trajectory Color");
        QObject::connect(changeTrajectoryColorAction, &QAction::triggered, [this, entityId]() {
            if (m_trajectoryColorCallback) m_trajectoryColorCallback(entityId);
        });

        // === SEND MESSAGE (Only in Runtime + has radios) ===
        if (isRuntime /*&& entry->platform->radios*/ && !entry->platform->radios.radios.empty()) {
            QAction* sendMessageAction = menu.addAction("Send Message");
            QObject::connect(sendMessageAction, &QAction::triggered, [this, entityId]() {
                if (m_sendMessageCallback) m_sendMessageCallback(entityId);
            });
        }
    }
    menu.exec(globalPos);
}

// ============================================================================
// Trajectory Context Menu
// ============================================================================

void ContextMenuHandler::showTrajectoryContextMenu(
    const QPoint& globalPos,
    int waypointIndex,
    int totalWaypoints)
{
    QWidget* parent = getParentWidget();
    if (!parent) return;

    QMenu menu(parent);
    setupMenuStyle(&menu);

    if (waypointIndex >= 0) {

        QAction* editWaypointAction = menu.addAction("Edit Waypoint");
        QObject::connect(editWaypointAction, &QAction::triggered, [this, waypointIndex]() {
            if (m_editWaypointCallback) {
                m_editWaypointCallback(waypointIndex);
            } else if (m_host) {
                m_host->logMessage("Edit waypoint: " + QString::number(waypointIndex));
            }
        });

        QAction* deleteWaypointAction = menu.addAction("Delete Waypoint");
        QObject::connect(deleteWaypointAction, &QAction::triggered, [this, waypointIndex]() {
            if (m_deleteWaypointCallback) {
                m_deleteWaypointCallback(waypointIndex);
            } else if (m_host) {
                m_host->logMessage("Delete waypoint: " + QString::number(waypointIndex));
            }
        });

        menu.addSeparator();
    }

    if (totalWaypoints > 0) {
        QAction* deleteAllAction = menu.addAction("Delete All Waypoints");
        QObject::connect(deleteAllAction, &QAction::triggered, [this]() {
            if (m_deleteAllWaypointsCallback) {
                m_deleteAllWaypointsCallback();
            } else if (m_host) {
                m_host->logMessage("Delete all waypoints");
            }
        });

        menu.addSeparator();
    }

    QAction* saveAction = menu.addAction("Save Trajectory");
    QObject::connect(saveAction, &QAction::triggered, [this]() {
        if (m_trajectoryCallback) {
            m_trajectoryCallback(false);
        }
    });

    QAction* cancelAction = menu.addAction("Cancel");
    QObject::connect(cancelAction, &QAction::triggered, [this]() {
        if (m_cancelTrajectoryCallback) {
            m_cancelTrajectoryCallback();
        } else if (m_trajectoryCallback) {
            m_trajectoryCallback(false);
        }
    });

    menu.exec(globalPos);
}

// ============================================================================
// General Context Menu
// ============================================================================

void ContextMenuHandler::showGeneralContextMenu(
    const QPoint& globalPos,
    const QPoint& mousePos,
    bool isScenarioEditor,
    bool isRuntimeEditor)
{
    QWidget* parent = getParentWidget();
    if (!parent) return;
    QMenu menu(parent);
    setupMenuStyle(&menu);
    bool isBoxMode = m_isBoxSelectionModeCallback ? m_isBoxSelectionModeCallback() : false;
    QAction* boxSelectAction = menu.addAction(
        isBoxMode ? "Disable Box Selection Mode" : "Enable Box Selection Mode"
        );
    boxSelectAction->setCheckable(true);
    boxSelectAction->setChecked(isBoxMode);
    QObject::connect(boxSelectAction, &QAction::triggered, [this]() {
        if (m_boxSelectionCallback) {
            m_boxSelectionCallback();
        }
        bool active = m_isBoxSelectionModeCallback ? m_isBoxSelectionModeCallback() : false;
        if (m_host) {
            m_host->setCursor(active ? Qt::CrossCursor : Qt::ArrowCursor);
            m_host->logMessage(active ? "Box Selection Mode ENABLED" : "Box Selection Mode DISABLED");
        }
    });
    menu.addSeparator();
    if (isScenarioEditor) {
        QAction* addEntityAction = menu.addAction("Add Entity");
        QObject::connect(addEntityAction, &QAction::triggered, [this, mousePos]() {
            if (m_addEntityCallback) {
                m_addEntityCallback(mousePos);
            }
        });
        menu.addSeparator();
    }
    if (isRuntimeEditor) {
        QAction* addEntityAction = menu.addAction("Add Entity");
        QObject::connect(addEntityAction, &QAction::triggered, [this, mousePos]() {
            if (m_addEntityCallback) {
                m_addEntityCallback(mousePos);
            }
        });
        menu.addSeparator();
    }

    QAction* addLineAction = menu.addAction("Add Line");
    QObject::connect(addLineAction, &QAction::triggered, [this]() {
        if (m_addShapeCallback) {
            m_addShapeCallback("Line");
        }
    });

    QAction* addCircleAction = menu.addAction("Add Circle");
    QObject::connect(addCircleAction, &QAction::triggered, [this]() {
        if (m_addShapeCallback) {
            m_addShapeCallback("Circle");
        }
    });

    QAction* addRectangleAction = menu.addAction("Add Rectangle");
    QObject::connect(addRectangleAction, &QAction::triggered, [this]() {
        if (m_addShapeCallback) {
            m_addShapeCallback("Rectangle");
        }
    });

    QAction* addPolygonAction = menu.addAction("Add Polygon");
    QObject::connect(addPolygonAction, &QAction::triggered, [this]() {
        if (m_addShapeCallback) {
            m_addShapeCallback("Polygon");
        }
    });

    QAction* addPointAction = menu.addAction("Add Point");
    QObject::connect(addPointAction, &QAction::triggered, [this]() {
        if (m_addShapeCallback) {
            m_addShapeCallback("Points");
        }
    });

    menu.addSeparator();
    QAction* addTextAction = menu.addAction("Add Text");
    QObject::connect(addTextAction, &QAction::triggered, [this, mousePos]() {
        if (m_addTextCallback) {
            m_addTextCallback(mousePos);
        }
    });

    if (m_pasteShapeCallback) {
        menu.addSeparator();
        QAction* pasteAction = menu.addAction("Paste Shape");
        QObject::connect(pasteAction, &QAction::triggered, [this]() {
            if (m_pasteShapeCallback) {
                m_pasteShapeCallback();
            }
        });
    }

    menu.exec(globalPos);
}

// ============================================================================
// Helper Methods
// ============================================================================

void ContextMenuHandler::setupMenuStyle(QMenu* menu)
{
    menu->setStyleSheet(
        "QMenu { background-color: white; color: black; border: 1px solid #cccccc; }"
        "QMenu::item { background-color: white; color: black; padding: 5px 20px; }"
        "QMenu::item:selected { background-color: #e6e6e6; color: black; }"
        "QMenu::item:hover { background-color: #f0f0f0; color: black; }"
        );
}

QWidget* ContextMenuHandler::getParentWidget() const
{
    return dynamic_cast<QWidget*>(m_host);
}

bool ContextMenuHandler::isScenarioEditor() const
{
    QWidget* parent = getParentWidget();
    if (!parent) return false;

    QWidget* w = parent;
    while (w) {
        if (QString(w->metaObject()->className()).contains("ScenarioEditor")) {
            return true;
        }
        w = w->parentWidget();
    }
    return false;
}

bool ContextMenuHandler::isRuntimeEditor() const
{
    QWidget* parent = getParentWidget();
    if (!parent) return false;
    QWidget* w = parent;
    while (w) {
        if (QString(w->metaObject()->className()).contains("RuntimeEditor")) {
            return true;
        }
        w = w->parentWidget();
    }
    return false;
}
