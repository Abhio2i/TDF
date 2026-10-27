#include "shapeeventhandler.h"
#include "../Shape/shapes_feature.h"
#include "GUI/Tacticaldisplay/Layer/layerpanel.h"
#include "../../../core/Debug/console.h"
#include "../../../core/Hierarchy/Struct/vector.h"
#include "../../../core/Hierarchy/Components/mesh.h"
#include "../../../core/Hierarchy/Components/collider.h"
#include "../../../core/Hierarchy/Components/trajectory.h"
#include "../Gis/gislib.h"
#include <QMenu>
#include <QAction>
#include <QPainterPath>
#include <QToolTip>
#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QColorDialog>
#include <QMessageBox>
#include <QAction>
#include <cmath>
#include <QVector2D>
#include <QLabel>
#include <QRegularExpression>
#include <QLayoutItem>
#include <QRectF>
#include <QFontMetrics>
#include <QInputDialog>
#include <QApplication>
#include "GUI/Tacticaldisplay/Layer/layerpanel-styles.h"


// ============================================================================
// Constructor / Destructor
// ============================================================================

ShapeEventHandler::ShapeEventHandler(IShapeHost* host)
    : m_host(host)
    , m_shapesFeature(nullptr)
    , m_layerPanel(nullptr)
{
    if (m_host) {
        m_shapesFeature = m_host->getShapesFeature();
        m_layerPanel = m_host->getLayerPanel();

        if (m_shapesFeature) {
            m_shapesFeature->setExternalPolygonVertices(&m_tempPolygonVertices);
            m_shapesFeature->setExternalLineVertices(&m_tempLineVertices);
            m_shapesFeature->setExternalPolygonCanvasPoints(&m_tempPolygonCanvasPoints);
            m_shapesFeature->setExternalLineCanvasPoints(&m_tempLineCanvasPoints);
        }
    }
}

ShapeEventHandler::~ShapeEventHandler()
{
    // Clean up temporary meshes
    for (MeshEntry& entry : m_tempMeshes) {
        // CHAITANYA JAIN CHANGE:
        // Fixed MeshEntry ownership lifecycle double-free crash.
        // Reason: MeshEntry destructor automatically deletes position, rotation, size, velocity.
        // Only delete unowned fields like mesh and collider here.
        if (entry.mesh) {
            for (Vector* v : entry.mesh->polygen) delete v;
            delete entry.mesh->color;
            delete entry.mesh;
        }
        delete entry.collider;
        delete entry.trajectory;
    }
    m_tempMeshes.clear();

    // Clean up temporary vertices
    for (Vector* v : m_tempPolygonVertices) delete v;
    m_tempPolygonVertices.clear();
    for (Vector* v : m_tempLineVertices) delete v;
    m_tempLineVertices.clear();

    if (m_copiedShape) {
        // CHAITANYA JAIN CHANGE:
        // Fixed MeshEntry ownership lifecycle double-free crash.
        // Reason: MeshEntry destructor automatically deletes position, rotation, size, velocity.
        // Only delete unowned fields like mesh and collider here.
        if (m_copiedShape->mesh) {
            for (Vector* v : m_copiedShape->mesh->polygen) delete v;
            delete m_copiedShape->mesh->color;
            delete m_copiedShape->mesh;
        }
        delete m_copiedShape;
        m_copiedShape = nullptr;
    }

    if (m_shapePropertiesDialog) {
        delete m_shapePropertiesDialog;
        m_shapePropertiesDialog = nullptr;
    }

    if (m_textPropertiesDialog) {
        delete m_textPropertiesDialog;
        m_textPropertiesDialog = nullptr;
    }
}

void ShapeEventHandler::setLayerPanel(LayerPanel* panel) {
    m_layerPanel = panel;
}

void ShapeEventHandler::clearStates() {
    m_selectedShapeIds.clear();
    m_highlightedShapeId.clear();
    m_draggingShapeId.clear();
    m_editingShapeId.clear();
    m_activeRotateId.clear();
    m_rotatingBitmapId.clear();
    m_isDraggingShape = false;
    m_isResizingShape = false;
    m_isRotatingBitmap = false;
    m_isPlacingBitmap = false;
    m_isDraggingBitmap = false;
    if (m_copiedShape) {
        if (m_copiedShape->mesh) {
            for (Vector* v : m_copiedShape->mesh->polygen) delete v;
            delete m_copiedShape->mesh->color;
            delete m_copiedShape->mesh;
        }
        delete m_copiedShape;
        m_copiedShape = nullptr;
    }
    cancelShapeDrawing();
    cancelActiveShapeOperation();
}

//=========================================================================
// by chaitanya jain --- Shapes operation handling
//=========================================================================
void ShapeEventHandler::cancelActiveShapeOperation()
{
    if (!m_host) return;

    // Stop whole-shape dragging
    m_isDraggingShape = false;
    m_draggingShapeId.clear();

    // Stop EDIT mode completely
    m_editingShapeId.clear();
    m_isResizingShape = false;
    m_selectedHandleIndex = -1;
    m_resizeHandles.clear();

    // Stop ROTATE mode completely
    m_activeRotateId.clear();

    // Stop bitmap rotation
    m_isRotatingBitmap = false;
    m_rotatingBitmapId.clear();

    // Stop TEXT edit
    m_isEditingText = false;
    m_editingTextId.clear();
    for (auto& textEntry : m_tempMeshes) {
        if (textEntry.name.startsWith("Text") || textEntry.name.startsWith("TempText")) {
            textEntry.isTextSelected = false;
        }
    }

    // Reset interaction state
    m_host->setCurrentMode(TransformMode::Translate);
    m_host->setCursor(Qt::ArrowCursor);

    m_host->refreshView();
}


// void ShapeEventHandler::startShapeEdit(const QString& shapeId)
// {
//     if (!m_host || shapeId.isEmpty()) return;

//     // IMPORTANT:
//     // Close Rotate / old Edit / Drag before starting new Edit.
//     cancelActiveShapeOperation();

//     m_host->setCurrentMode(TransformMode::EditShape);

//     m_editingShapeId = shapeId;
//     m_isResizingShape = false;
//     m_selectedHandleIndex = -1;

//     updateResizeHandlesForRotation(shapeId);

//     m_host->setCursor(Qt::SizeAllCursor);
//     m_host->refreshView();
//     m_host->forceRedraw();
// }
// ============================================================================
// Start Shape Edit
// ============================================================================

void ShapeEventHandler::startShapeEdit(const QString& shapeId)
{
    if (!m_host || shapeId.isEmpty())
        return;

    // ------------------------------------------------------------------------
    // IMPORTANT:
    // Close any previous Edit / Rotate / Drag operation first.
    // ------------------------------------------------------------------------
    cancelActiveShapeOperation();

    // Enter Edit Mode
    m_host->setCurrentMode(TransformMode::EditShape);

    // Set current editing shape
    m_editingShapeId = shapeId;
    m_isResizingShape = false;
    m_selectedHandleIndex = -1;

    // Create resize handles
    updateResizeHandlesForRotation(shapeId);

    // Update cursor & repaint
    m_host->setCursor(Qt::SizeAllCursor);
    m_host->refreshView();
    m_host->forceRedraw();
}

// ============================================================================
// Start Shape Rotate
// ============================================================================

void ShapeEventHandler::startShapeRotate(const QString& shapeId)
{
    if (!m_host || shapeId.isEmpty())
        return;

    // ------------------------------------------------------------------------
    // IMPORTANT:
    // Close any previous Edit / Rotate / Drag operation first.
    // ------------------------------------------------------------------------
    cancelActiveShapeOperation();

    // Activate rotation for selected shape
    m_activeRotateId = shapeId;

    // Keep Translate mode.
    // Rotation starts only when rotation handle is dragged.
    m_host->setCurrentMode(TransformMode::Translate);



    // Update cursor & repaint
    m_host->setCursor(Qt::ArrowCursor);
    m_host->refreshView();
    m_host->forceRedraw();
}

// ============================================================================
// Mouse Double Click Event
// by Chaitanya Jain
// ============================================================================

void ShapeEventHandler::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_host || !m_host->getGISlib() || !event) {
        return;
    }

    if (m_host->getCurrentMode() != TransformMode::DrawShape) {
        return;
    }

    const QString shapeType = m_host->getSelectedShape();

    if (shapeType != "Line" && shapeType != "Polygon") {
        return;
    }

    QPointF geoPos = m_host->getGISlib()->canvasToGeo(event->pos());

    if (m_shapesFeature) {
        m_shapesFeature->handleShapeDrawing(shapeType, geoPos, true);
    }

    m_host->refreshView();
    m_host->forceRedraw();
}
// ============================================================================
// Mouse Press Event
// ============================================================================

void ShapeEventHandler::mousePressEvent(QMouseEvent *event) {
    if (!m_host || !m_host->getGISlib()) return;

    // CHAITANYA JAIN CHANGE:
    // What was fixed: Box Selection State Migration & Multi-Selection Fix
    // Why the bug happened: Box selection state was duplicated across CanvasWidget and ShapeEventHandler, leading to compile errors, broken state synchronization, and a lack of proper shape targeting.
    // Why this change is safe: Centralized box selection entirely in ShapeEventHandler, delegating all mouse events here to decouple it from CanvasWidget.
    if (m_isBoxSelectionMode) {
        m_isBoxSelecting = true;
        m_boxStartPos = event->pos();
        m_boxCurrentPos = event->pos();
        clearSelectedShapeIds();
        m_host->refreshView();
        return;
    }

    // CHAITANYA JAIN CHANGE:
    // Removed m_isPastePending block. Paste happens immediately in pasteShape() now.
    /*
    if (m_isPastePending && event->button() == Qt::LeftButton) {
        QPointF geoPos = m_host->getGISlib()->canvasToGeo(event->pos());
        placePastedShape(geoPos);
        m_isPastePending = false;
        m_host->setCursor(Qt::ArrowCursor);
        return;
    }
    */

    // CHAITANYA JAIN CHANGE:
    // Track last left click for smart pasting
    if (event->button() == Qt::LeftButton && m_host && m_host->getGISlib()) {
        m_lastLeftClickGeoPos = m_host->getGISlib()->canvasToGeo(event->pos());
        m_clickedSinceCopy = true;
    }

    if (m_host->getCurrentMode() == TransformMode::DrawShape ||
        m_host->getCurrentMode() == TransformMode::PlaceBitmap) {
        if (m_layerPanel &&
            m_layerPanel->layerItems.isEmpty() &&
            m_layerPanel->rasterLayers.isEmpty()) {

            QWidget* parentWidget = dynamic_cast<QWidget*>(m_host);
            if (!parentWidget && m_layerPanel) {
                parentWidget = dynamic_cast<QWidget*>(m_layerPanel->parent());
            }

            QMessageBox msgBox(parentWidget);
            msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
            msgBox.setWindowTitle("No Layer Found");
            msgBox.setText("No layers exist!\n\n"
                           "Please create a layer first to draw shapes.");
            msgBox.setIcon(QMessageBox::Warning);

            QPushButton* createBtn = msgBox.addButton("Create New Layer", QMessageBox::AcceptRole);
            msgBox.addButton("Cancel", QMessageBox::RejectRole);
            msgBox.exec();

            if (msgBox.clickedButton() == createBtn) {
                m_layerPanel->addLayer();
                // If they still didn't create a layer, cancel
                if (m_layerPanel->layerItems.isEmpty() && m_layerPanel->rasterLayers.isEmpty()) {
                    m_host->setCurrentMode(TransformMode::Translate);
                    return;
                }
                // Otherwise, fall through and let shape creation continue
            } else {
                m_host->setCurrentMode(TransformMode::Translate);
                return;
            }
        }
    }
    // -----------------------------------------------------------------
    // Cancel active edit/rotate/resize when right-clicking outside
    // drawing mode.
    // -----------------------------------------------------------------
    // if (event->button() == Qt::RightButton)
    // {
    //     if (m_host->getCurrentMode() != TransformMode::DrawShape)
    //     {
    //         cancelActiveShapeOperation();
    //     }
    // }
    if (event->button() == Qt::RightButton &&
        m_host->getCurrentMode() == TransformMode::DrawShape &&
        (m_host->getSelectedShape() == "Line" || m_host->getSelectedShape() == "Polygon")) {

        bool canFinalize = false;
        if (m_host->getSelectedShape() == "Line" && m_tempLineVertices.size() >= 2) {
            canFinalize = true;
        } else if (m_host->getSelectedShape() == "Polygon" && m_tempPolygonVertices.size() >= 3) {
            canFinalize = true;
        }

        if (canFinalize) {
            QPointF lastGeoPos;
            if (m_host->getSelectedShape() == "Line" && !m_tempLineVertices.empty()) {
                Vector* lastVertex = m_tempLineVertices.back();
                lastGeoPos = QPointF(lastVertex->x, lastVertex->y);
            } else if (m_host->getSelectedShape() == "Polygon" && !m_tempPolygonVertices.empty()) {
                Vector* lastVertex = m_tempPolygonVertices.back();
                lastGeoPos = QPointF(lastVertex->x, lastVertex->y);
            }

            if (m_shapesFeature) {
                m_shapesFeature->handleShapeDrawing(m_host->getSelectedShape(), lastGeoPos, true);
            }
            m_host->refreshView();
            m_host->forceRedraw();
            return;
        } else {
            QString minVertices = (m_host->getSelectedShape() == "Line") ? "2" : "3";
            m_host->logWarning("Need at least " + minVertices +
                               " vertices to finalize " + m_host->getSelectedShape());
            return;
        }
    }

    if (m_host->getCurrentMode() == TransformMode::DrawShape &&
        (m_host->getSelectedShape() == "Circle" || m_host->getSelectedShape() == "Rectangle")) {
        QPointF geoPos = m_host->getGISlib()->canvasToGeo(event->pos());
        if (m_shapesFeature) {
            m_shapesFeature->startDragShape(m_host->getSelectedShape(), geoPos);
            m_host->refreshView();
            m_host->forceRedraw();
        }
        return;
    }

    if (m_host->getCurrentMode() == TransformMode::DrawShape &&
        (m_host->getSelectedShape() == "Line" || m_host->getSelectedShape() == "Polygon")) {
        QPointF geoPos = m_host->getGISlib()->canvasToGeo(event->pos());
        bool finalize = (event->type() == QEvent::MouseButtonDblClick);

        if (m_shapesFeature) {
            m_shapesFeature->handleShapeDrawing(m_host->getSelectedShape(), geoPos, finalize);
        }

        if (finalize) {
            m_host->logMessage("Finalized " + m_host->getSelectedShape() + " with DOUBLE-CLICK");
        }
        return;
    }

    if (m_host->getCurrentMode() == TransformMode::DrawShape &&
        (m_host->getSelectedShape() == "Point" || m_host->getSelectedShape() == "Points")) {
        QPointF geoPos = m_host->getGISlib()->canvasToGeo(event->pos());
        if (m_shapesFeature) {
            m_shapesFeature->handleShapeDrawing("Points", geoPos, false);
            m_host->refreshView();
            m_host->forceRedraw();
        }
        return;
    }

    if (handleTextMousePress(event)) return;
    if (handleBitmapsMousePress(event)) return;

    if (m_host->getCurrentMode() == TransformMode::Translate)
    {
        if (handleShapeSelection(event))
            return;

        cancelActiveShapeOperation();

        return;
    }
    switch(m_host->getCurrentMode())
    {
    case TransformMode::EditShape:

        if(!handleResizeHandleSelection(event))
            cancelActiveShapeOperation();

        return;

    case TransformMode::Rotate:

        cancelActiveShapeOperation();
        return;

    case TransformMode::Scale:

        cancelActiveShapeOperation();
        return;

    default:
        break;
    }
}

// ============================================================================
// Mouse Move Event
// ============================================================================

void ShapeEventHandler::mouseMoveEvent(QMouseEvent *event) {
    if (!m_host || !m_host->getGISlib()) return;

    if (m_isBoxSelecting) {
        // CHAITANYA JAIN CHANGE:
        // What was fixed: Box Selection Stuck Drag
        // Why the bug happened: If a mouse release event was lost, the selection would stay stuck to the cursor.
        // Why this change is safe: Cancels the drag state if the left mouse button is no longer held, restoring the cursor.
        if (!(event->buttons() & Qt::LeftButton)) {
            m_isBoxSelecting = false;
            m_host->setCursor(m_isBoxSelectionMode ? Qt::CrossCursor : Qt::ArrowCursor);
        } else {
            m_boxCurrentPos = event->pos();
            m_host->refreshView();
            return;
        }
    }

    if (m_isDraggingShape && !m_draggingShapeId.isEmpty() &&
        m_host->getCurrentMode() == TransformMode::Translate) {
        handleShapeDragging(event);
        return;
    }

    if (m_shapesFeature && m_shapesFeature->isDraggingShape()) {
        QPointF currentGeoPos = m_host->getGISlib()->canvasToGeo(event->pos());
        m_shapesFeature->updateDragShape(currentGeoPos);
        return;
    }

    if (m_host->getCurrentMode() == TransformMode::EditShape && m_isResizingShape &&
        m_selectedHandleIndex >= 0 && !m_editingShapeId.isEmpty()) {

        for (auto& entry : m_tempMeshes) {
            if (entry.name == m_editingShapeId) {
                static bool resizeSaved = false;
                if (!resizeSaved) {
                    if (m_shapesFeature) {
                        m_shapesFeature->saveShapeState(m_editingShapeId, &entry);
                    }
                    resizeSaved = true;
                }

                if (!entry.position || !entry.size) return;

                QPointF newPos = event->pos();
                QPointF centerGeo(entry.position->x(), entry.position->y());
                QPointF centerCanvas = m_host->getGISlib()->geoToCanvas(centerGeo.y(), centerGeo.x());
                float rotationRad = entry.rotation->z();

                if (entry.name.startsWith("TempPolygon") || entry.name.startsWith("TempPolyline")) {
                    handlePolygonVertexEditing(event, entry);
                } else if (entry.name.startsWith("TempRectangle")) {
                    handleRectangleResizing(event, entry);
                } else if (entry.name.startsWith("TempCircle")) {
                    handleCircleResizing(event, entry);
                } else if (!entry.bitmapPath.isEmpty()) {
                    handleBitmapResizing(event, entry);
                }

                if (m_layerPanel && !entry.bitmapPath.isEmpty()) {
                    m_layerPanel->updateRasterLayerFromShape(entry.name);
                }

                m_host->refreshView();
                return;
            }
        }
        return;
    }

    handleTextMouseMove(event);
    handleBitmapsMouseMove(event);
}

// ============================================================================
// Mouse Release Event
// ============================================================================

void ShapeEventHandler::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (m_isBoxSelecting) {
            m_isBoxSelecting = false;
            // CHAITANYA JAIN CHANGE:
            // What was fixed: One-Shot Tool Behavior and Zooming
            // Why the bug happened: The box selection was not turning itself off automatically upon mouse release, causing a stuck state. Also, it was missing the expected zooming behavior.
            // Why this change is safe: Turns off the Box Selection mode, resets the cursor safely, and applies the exact same zooming bounds logic that was historically tied to the zooming rectangle.
            // m_isBoxSelectionMode = false;
            m_host->setCursor(Qt::ArrowCursor);

            QRectF selRect = QRectF(m_boxStartPos, m_boxCurrentPos).normalized();
            std::vector<QString> shapeIds;
            for (auto& entry : m_tempMeshes) {
                bool isSelected = false;
                if (entry.name.startsWith("Text") && !entry.text.isEmpty()) {
                    QPointF textPos = m_host->getGISlib()->geoToCanvas(entry.position->y(), entry.position->x());
                    QFontMetrics fm(entry.textFont);
                    QRect textRect = fm.boundingRect(entry.text);
                    textRect.moveTo(textPos.x(), textPos.y() - fm.ascent());
                    QRectF expandedRect = textRect.adjusted(-10, -10, 10, 10);
                    if (selRect.intersects(expandedRect))
                        isSelected = true;
                } else {
                    QPolygonF poly = getRotatedShapePolygon(entry);
                    if (!poly.isEmpty()) {
                        for (const QPointF& pt : poly) {
                            if (selRect.contains(pt)) {
                                isSelected = true;
                                break;
                            }
                        }
                    }
                }
                if (isSelected) {
                    shapeIds.push_back(entry.name);
                }
            }
            m_selectedShapeIds = shapeIds;
            if (!m_selectedShapeIds.empty()) {
                m_highlightedShapeId = m_selectedShapeIds[0];
            }

            if (m_host) {
                m_host->selectEntitiesInBox(selRect);
            }

            m_host->refreshView();
            return;
        }

        if (m_shapesFeature && m_shapesFeature->isDraggingShape()) {
            m_shapesFeature->finalizeDragShape();
            return;
        }

        if (m_isDraggingShape) {
            stopShapeDragging();
        }

        m_isResizingShape = false;
        m_selectedHandleIndex = -1;
    }

    handleTextMouseRelease(event);
    handleBitmapsMouseRelease(event);
}

// ============================================================================
// Key Press Event
// ============================================================================

void ShapeEventHandler::keyPressEvent(QKeyEvent *event) {
    if (!m_host) return;

    if (event->key() == Qt::Key_Escape) {
        // CHAITANYA JAIN CHANGE:
        // Removed m_isPastePending reset on Escape. Paste is now immediate.
        /*
        if (m_isPastePending) {
            m_isPastePending = false;
            m_host->setCursor(Qt::ArrowCursor);
            return;
        }
        */

        if (m_host->getCurrentMode() == TransformMode::EditShape) {
            m_host->setCurrentMode(TransformMode::Translate);
            m_editingShapeId = "";
            m_selectedHandleIndex = -1;
            m_isResizingShape = false;
            m_resizeHandles.clear();
            m_host->setCursor(Qt::ArrowCursor);
            m_host->refreshView();
            return;
        }

        if (m_shapesFeature && m_shapesFeature->isDraggingShape()) {
            m_shapesFeature->cancelDragShape();
            m_host->setShapeDrawingMode(false, "");
            return;
        }

        if (m_host->getCurrentMode() == TransformMode::DrawShape) {
            if (m_host->getSelectedShape() == "Polygon" && m_tempPolygonVertices.size() >= 3) {
                finalizePolygon();
                m_host->setShapeDrawingMode(false, "");
                m_host->refreshView();
                m_host->forceRedraw();
                return;
            } else if (m_host->getSelectedShape() == "Line" && m_tempLineVertices.size() >= 2) {
                finalizeLine();
                m_host->setShapeDrawingMode(false, "");
                m_host->refreshView();
                m_host->forceRedraw();
                return;
            } else {
                cancelShapeDrawing();
                m_host->setShapeDrawingMode(false, "");
                m_host->refreshView();
                return;
            }
        }

        if (m_isDraggingBitmap) {
            stopBitmapDragging();
            return;
        }

        if (!m_activeRotateId.isEmpty()) {
            m_activeRotateId.clear();
            m_host->refreshView();
            return;
        }

        if (m_host->getCurrentMode() == TransformMode::PlaceBitmap) {
            m_isPlacingBitmap = false;
            m_selectedBitmapType = "";
            m_host->setCurrentMode(TransformMode::Translate);
            return;
        }
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_host->getCurrentMode() == TransformMode::DrawShape) {
            if (m_host->getSelectedShape() == "Polygon" && m_tempPolygonVertices.size() >= 3) {
                finalizePolygon();
                m_host->setShapeDrawingMode(false, "");
                m_host->refreshView();
                m_host->forceRedraw();
                m_host->logMessage("Polygon finalized with ENTER key");
                return;
            } else if (m_host->getSelectedShape() == "Line" && m_tempLineVertices.size() >= 2) {
                finalizeLine();
                m_host->setShapeDrawingMode(false, "");
                m_host->refreshView();
                m_host->forceRedraw();
                m_host->logMessage("Line finalized with ENTER key");
                return;
            } else {
                QString minV = (m_host->getSelectedShape() == "Line") ? "2" : "3";
                m_host->logWarning("Need at least " + minV +
                                   " vertices to finalize with ENTER");
                return;
            }
        }
    }

    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_C) {
            copySelectedShape();
            return;
        }
        if (event->key() == Qt::Key_V) {
            pasteShape();
            return;
        }
    }

    if (event->key() == Qt::Key_Delete) {
        if (!m_highlightedShapeId.isEmpty()) {
            deleteShape(m_highlightedShapeId);
            return;
        }
        if (!m_editingShapeId.isEmpty()) {
            deleteShape(m_editingShapeId);
            return;
        }
        if (!m_draggingShapeId.isEmpty()) {
            deleteShape(m_draggingShapeId);
            return;
        }
    }

    handleBitmapsKeyPress(event);
}

// ============================================================================
// Paint Event
// ============================================================================

void ShapeEventHandler::paintEvent(QPainter& painter) {
    if (!m_host || !m_host->getGISlib()) return;

    drawTempShapesPreview(painter);
    drawTempMeshes(painter);

    if (m_isBoxSelecting) {
        painter.setPen(QPen(Qt::green, 1, Qt::DashLine));
        painter.setBrush(QColor(0, 255, 0, 30));
        painter.drawRect(QRect(m_boxStartPos, m_boxCurrentPos).normalized());
    }

    drawSelectionOutline(painter);
    drawResizeHandles(painter);


    if (!m_activeRotateId.isEmpty()) {
        for (const auto& entry : m_tempMeshes) {
            if (entry.name == m_activeRotateId) {
                drawRotationHandle(painter, entry);
                break;
            }
        }
    }

    if (m_shapesFeature && m_shapesFeature->isDraggingShape()) {
        m_shapesFeature->drawDragPreview(painter, m_host->getGISlib());
    }

    handleBitmapsPaint(painter);
}

// ============================================================================
// Shape Selection
// ============================================================================

bool ShapeEventHandler::handleShapeSelection(QMouseEvent *event)
{
    if (!m_host || !m_host->getGISlib())
        return false;

    // ==============================================================
    // Do not select existing shapes while drawing Line / Polygon
    // ==============================================================
    if (m_host->getCurrentMode() == TransformMode::DrawShape &&
        (m_host->getSelectedShape() == "Line" ||
         m_host->getSelectedShape() == "Polygon"))
    {
        return false;
    }

    // Do not start normal dragging while editing a shape
    if (m_host->getCurrentMode() == TransformMode::EditShape)
        return false;


    // ==============================================================
    // IMPORTANT:
    // Instead of immediately selecting the FIRST shape that reports
    // a hit, find the BEST / CLOSEST hit.
    //
    // This fixes imported/overlapping layers selecting an unrelated
    // shape depending on m_tempMeshes iteration order.
    // ==============================================================
    MeshEntry* bestEntry = nullptr;
    qreal bestDistance = std::numeric_limits<qreal>::max();


    for (auto& entry : m_tempMeshes)
    {
        // ----------------------------------------------------------
        // Hidden layer shapes must not be interactive
        // ----------------------------------------------------------
        if (!shouldDrawShape(entry.name))
            continue;


        // ----------------------------------------------------------
        // Normal geometric shapes only.
        // Text / Bitmap have their own handlers.
        // ----------------------------------------------------------
        if (!entry.name.startsWith("Temp") ||
            entry.name.startsWith("TempText") ||
            entry.name.startsWith("TempBitmap") ||
            !entry.bitmapPath.isEmpty())
        {
            continue;
        }


        if (!entry.position)
            continue;


        bool isHit = false;
        qreal hitDistance = std::numeric_limits<qreal>::max();


        // ==========================================================
        // POINT
        // ==========================================================
        if (entry.name.startsWith("TempPoint"))
        {
            QPointF centerCanvas =
                m_host->getGISlib()->geoToCanvas(
                    entry.position->y(),
                    entry.position->x()
                    );

            hitDistance =
                QVector2D(
                    QPointF(event->pos()) - centerCanvas
                    ).length();

            isHit = (hitDistance <= 10.0);
        }


        // ==========================================================
        // ALL OTHER SHAPES NEED ROTATED POLYGON
        // ==========================================================
        else
        {
            QPolygonF rotatedPoly =
                getRotatedShapePolygon(entry);

            if (rotatedPoly.isEmpty())
                continue;


            // ======================================================
            // SAFETY FIX FOR IMPORTED SHAPES
            //
            // Reject invalid coordinates before hit testing.
            // ======================================================
            bool validPolygon = true;

            for (const QPointF& p : rotatedPoly)
            {
                if (!qIsFinite(p.x()) ||
                    !qIsFinite(p.y()))
                {
                    validPolygon = false;
                    break;
                }
            }

            if (!validPolygon)
                continue;


            // ======================================================
            // LINE / POLYLINE
            // ======================================================
            if (entry.name.startsWith("TempPolyline"))
            {
                const qreal lineTolerance = 8.0;

                qreal closestLineDistance =
                    std::numeric_limits<qreal>::max();


                for (int i = 0;
                     i < rotatedPoly.size() - 1;
                     ++i)
                {
                    const QPointF a = rotatedPoly[i];
                    const QPointF b = rotatedPoly[i + 1];
                    const QPointF p = QPointF(event->pos());

                    QPointF ab = b - a;
                    QPointF ap = p - a;

                    qreal lengthSquared =
                        ab.x() * ab.x() +
                        ab.y() * ab.y();

                    if (lengthSquared <= 0.0001)
                        continue;


                    qreal t =
                        (ap.x() * ab.x() +
                         ap.y() * ab.y()) /
                        lengthSquared;

                    t = qBound(
                        static_cast<qreal>(0.0),
                        t,
                        static_cast<qreal>(1.0)
                        );


                    QPointF closestPoint =
                        a + ab * t;


                    qreal distance =
                        QVector2D(
                            p - closestPoint
                            ).length();


                    if (distance < closestLineDistance)
                    {
                        closestLineDistance = distance;
                    }
                }


                if (closestLineDistance <= lineTolerance)
                {
                    isHit = true;
                    hitDistance = closestLineDistance;
                }
            }


            // ======================================================
            // CIRCLE / RECTANGLE / POLYGON
            // ======================================================
            else
            {
                // --------------------------------------------------
                // Calculate visual center
                // --------------------------------------------------
                QPointF polyCenter(0.0, 0.0);

                for (const QPointF& p : rotatedPoly)
                {
                    polyCenter += p;
                }

                polyCenter /= rotatedPoly.size();


                // --------------------------------------------------
                // Distance is used to choose the closest shape when
                // multiple shapes overlap.
                // --------------------------------------------------
                hitDistance =
                    QVector2D(
                        QPointF(event->pos()) -
                        polyCenter
                        ).length();


                // --------------------------------------------------
                // Use original polygon for actual hit detection.
                //
                // Your old code used 0.8 scaled polygon.
                // That made only inner 80% clickable.
                // --------------------------------------------------
                isHit =
                    rotatedPoly.containsPoint(
                        QPointF(event->pos()),
                        Qt::OddEvenFill
                        );
            }
        }


        // ==========================================================
        // KEEP BEST HIT
        // ==========================================================
        if (isHit &&
            hitDistance < bestDistance)
        {
            bestDistance = hitDistance;
            bestEntry = &entry;
        }
    }


    // ==============================================================
    // NOTHING CLICKED
    // ==============================================================
    if (!bestEntry)
        return false;


    // ==============================================================
    // SELECT / DRAG THE ACTUAL BEST MATCH
    // ==============================================================
    MeshEntry& entry = *bestEntry;


    m_isDraggingShape = true;
    m_draggingShapeId = entry.name;


    // --------------------------------------------------------------
    // Save history before dragging
    // --------------------------------------------------------------
    if (m_shapesFeature)
    {
        m_shapesFeature->saveShapeState(
            m_draggingShapeId,
            &entry
            );
    }


    m_shapeDragStartPos =
        event->pos();


    m_host->setCursor(
        Qt::ClosedHandCursor
        );


    // ==============================================================
    // UPDATE CANVAS SELECTION
    //
    // Preserve existing multi-selection if clicked shape is already
    // part of selected group.
    //
    // Otherwise switch to clicked shape.
    // ==============================================================
    auto selectedIt =
        std::find(
            m_selectedShapeIds.begin(),
            m_selectedShapeIds.end(),
            entry.name
            );


    if (m_selectedShapeIds.empty())
    {
        m_selectedShapeIds.push_back(
            entry.name
            );

        m_highlightedShapeId =
            entry.name;
    }
    else if (selectedIt ==
             m_selectedShapeIds.end())
    {
        m_selectedShapeIds.clear();

        m_selectedShapeIds.push_back(
            entry.name
            );

        m_highlightedShapeId =
            entry.name;
    }
    else
    {
        // Shape already belongs to current selection.
        // Do not destroy multi-selection.
        m_highlightedShapeId =
            entry.name;
    }


    // ==============================================================
    // CANVAS -> LAYER PANEL SYNCHRONIZATION
    // ==============================================================
    if (m_layerPanel)
    {
        m_layerPanel->selectShapeInPanel(
            entry.name
            );
    }


    m_host->refreshView();

    return true;
}

void ShapeEventHandler::handleShapeRightClick(QMouseEvent *event) {
    if (!m_host || !m_host->getGISlib()) return;

    QString closestShapeId;
    qreal minDistance = std::numeric_limits<qreal>::max();
    MeshEntry* closestEntry = nullptr;

    for (auto& entry : m_tempMeshes) {
        if (!entry.position || (!entry.mesh && entry.text.isEmpty() &&
                                !entry.name.startsWith("TempPoint") && entry.bitmapPath.isEmpty())) {
            continue;
        }

        bool isHit = false;
        qreal distance = std::numeric_limits<qreal>::max();

        if (entry.name.startsWith("Text") && !entry.text.isEmpty()) {
            QPointF entityPos = m_host->getGISlib()->geoToCanvas(entry.position->y(), entry.position->x());
            QFontMetrics fm(entry.textFont);
            QRect textRect = fm.boundingRect(entry.text);
            textRect.moveTo(entityPos.x(), entityPos.y() - fm.ascent());
            QRect innerRect = textRect.adjusted(
                textRect.width() * 0.125f, textRect.height() * 0.125f,
                -textRect.width() * 0.125f, -textRect.height() * 0.125f
                );
            isHit = innerRect.contains(event->pos());
            distance = QVector2D(event->pos() - entityPos).length();
        } else {
            QPolygonF rotatedPoly = getRotatedShapePolygon(entry);
            if (rotatedPoly.isEmpty()) continue;
            QPointF centerCanvas = m_host->getGISlib()->geoToCanvas(entry.position->y(), entry.position->x());

            if (entry.name.startsWith("TempPoint")) {
                distance = QVector2D(event->pos() - centerCanvas).length();
                isHit = (distance <= 6.0f);
            } else if (entry.name.startsWith("TempPolyline")) {
                qreal lineTolerance = 6.0f;
                for (int i = 0; i < rotatedPoly.size() - 1; ++i) {
                    if (m_shapesFeature &&
                        m_shapesFeature->isPointNearLineSegment(event->pos(), rotatedPoly[i], rotatedPoly[i+1], lineTolerance)) {
                        isHit = true;
                        distance = QVector2D(event->pos() - centerCanvas).length();
                        break;
                    }
                }
            } else {
                QPointF polyCenter(0, 0);
                for (const QPointF& p : rotatedPoly) {
                    polyCenter += p;
                }
                polyCenter /= rotatedPoly.size();
                QPolygonF scaledPoly;
                for (const QPointF& p : rotatedPoly) {
                    QPointF offset = p - polyCenter;
                    scaledPoly << (polyCenter + offset * 1.0f);
                }
                isHit = scaledPoly.containsPoint(event->pos(), Qt::OddEvenFill);
                distance = QVector2D(event->pos() - centerCanvas).length();
            }
        }

        if (isHit && distance < minDistance) {
            minDistance = distance;
            closestShapeId = entry.name;
            closestEntry = &entry;
            break;
        }
    }

    if (!closestShapeId.isEmpty() && closestEntry) {
        QWidget* parentWidget = dynamic_cast<QWidget*>(m_host);
        if (!parentWidget) return;

        QMenu contextMenu(parentWidget);
        contextMenu.setWindowFlags(contextMenu.windowFlags() | Qt::Popup);
        contextMenu.setStyleSheet(
            "QMenu { background-color: white; color: black; border: 1px solid #cccccc; }"
            "QMenu::item { background-color: white; color: black; padding: 5px 20px; }"
            "QMenu::item:selected { background-color: #0078d7; color: white; }"
            );

        QAction* copyAction = contextMenu.addAction("Copy Shape");
        QObject::connect(copyAction, &QAction::triggered, [this, closestShapeId]() {
            m_highlightedShapeId = closestShapeId;
            copySelectedShape();
        });

        if (closestShapeId.startsWith("TempText") ||
            closestShapeId.startsWith("Text") ||
            closestEntry->shapeType.compare("Text", Qt::CaseInsensitive) == 0) {

            QAction* editTextAction = contextMenu.addAction("Edit Text Properties");
            QObject::connect(editTextAction, &QAction::triggered, [=]() {
                showTextPropertiesDialog(closestShapeId);
            });
            QAction* deleteAction = contextMenu.addAction("Delete Text");
            QObject::connect(deleteAction, &QAction::triggered, [=]() {
                deleteText(closestShapeId);
            });
        } else if (closestShapeId.startsWith("TempPolyline") ||
                   closestShapeId.startsWith("TempCircle") ||
                   closestShapeId.startsWith("TempRectangle") ||
                   closestShapeId.startsWith("TempPolygon") ||
                   closestShapeId.startsWith("TempPoint")) {

            QAction* editAction = contextMenu.addAction("Edit");

            QObject::connect(editAction, &QAction::triggered, [=]() {

                // ============================================================
                // FIX: Completely close any active Text editing/dragging state
                // before opening Shape Edit mode.
                // ============================================================

                m_isEditingText = false;
                m_editingTextId.clear();

                for (auto& textEntry : m_tempMeshes)
                {
                    if (!textEntry.text.isEmpty() ||
                        textEntry.name.startsWith("Text") ||
                        textEntry.name.startsWith("TempText"))
                    {
                        textEntry.isTextSelected = false;
                    }
                }

                // Remove stale Text highlight
                if (m_highlightedShapeId.startsWith("Text") ||
                    m_highlightedShapeId.startsWith("TempText"))
                {
                    m_highlightedShapeId.clear();
                }

                // ============================================================
                // Now activate ONLY this Shape for editing
                // ============================================================

                m_editingShapeId = closestShapeId;

                m_host->setCurrentMode(TransformMode::EditShape);

                m_host->setCursor(Qt::SizeAllCursor);

                updateResizeHandlesForRotation(closestShapeId);

                m_host->refreshView();
            });

            QAction* propertiesAction = contextMenu.addAction("Shape Properties");
            QObject::connect(propertiesAction, &QAction::triggered, [=]() {
                showShapePropertiesDialog(closestShapeId);
            });

            if (m_shapesFeature && m_shapesFeature->hasHistory(closestShapeId)) {
                QAction* historyAction = contextMenu.addAction("History Preview");
                QObject::connect(historyAction, &QAction::triggered, [=]() {
                    m_shapesFeature->showHistoryPreview(closestShapeId);
                });

                QAction* applyHistoryAction = contextMenu.addAction("History Restore");
                QObject::connect(applyHistoryAction, &QAction::triggered, [=]() {
                    if (m_shapesFeature && m_shapesFeature->restorePreviousState(closestShapeId, closestEntry)) {
                        m_shapesFeature->hideHistoryPreview();
                        m_host->refreshView();
                    }
                });
            }

            if (m_shapesFeature && m_shapesFeature->isShowingPreview() &&
                m_shapesFeature->getPreviewShapeId() == closestShapeId) {
                QAction* hidePreviewAction = contextMenu.addAction("Hide Preview");
                QObject::connect(hidePreviewAction, &QAction::triggered, [=]() {
                    if (m_shapesFeature) {
                        m_shapesFeature->hideHistoryPreview();
                    }
                });
            }

            QAction* rotateAction = contextMenu.addAction("Rotate");
            QObject::connect(rotateAction, &QAction::triggered, [=]() {
                startShapeRotate(closestShapeId);
            });

            QAction* deleteAction = contextMenu.addAction("Delete Shape");
            QObject::connect(deleteAction, &QAction::triggered, [=]() {
                deleteShape(closestShapeId);
            });
        } else if (!closestEntry->bitmapPath.isEmpty()) {
            QAction* editAction = contextMenu.addAction("Edit");
            QObject::connect(editAction, &QAction::triggered, [=]() {
                m_host->setCurrentMode(TransformMode::EditShape);
                m_editingShapeId = closestShapeId;
                m_host->setCursor(Qt::SizeAllCursor);
                updateResizeHandlesForRotation(closestShapeId);
                m_host->refreshView();
            });

            if (m_shapesFeature && m_shapesFeature->hasHistory(closestShapeId)) {
                QAction* historyAction = contextMenu.addAction("Undo Last Change");
                QObject::connect(historyAction, &QAction::triggered, [=]() {
                    if (m_shapesFeature && m_shapesFeature->restorePreviousState(closestShapeId, closestEntry)) {
                        m_host->refreshView();
                    }
                });
            }

            QAction* rotateAction = contextMenu.addAction("Rotate");
            QObject::connect(rotateAction, &QAction::triggered, [=]() {
                startShapeRotate(closestShapeId);
            });

            QString deleteText;
            if (closestShapeId.startsWith("TempBitmap")) {
                deleteText = "Delete Bitmap";
            } else if (closestShapeId.startsWith("UserImage_")) {
                deleteText = "Delete Image";
            } else {
                deleteText = "Delete";
            }

            QAction* deleteAction = contextMenu.addAction(deleteText);
            QObject::connect(deleteAction, &QAction::triggered, [=]() {
                deleteShape(closestShapeId);
            });
        }

        contextMenu.exec(event->globalPos());
    }
}

// ============================================================================
// Shape Operations
// ============================================================================

void ShapeEventHandler::copySelectedShape() {
    if (m_copiedShape) {
        // CHAITANYA JAIN CHANGE:
        // Fixed MeshEntry ownership lifecycle double-free crash.
        // Reason: MeshEntry destructor automatically deletes position, rotation, size, velocity.
        // Only delete unowned fields like mesh and collider here.
        if (m_copiedShape->mesh) {
            for (Vector* v : m_copiedShape->mesh->polygen) delete v;
            delete m_copiedShape->mesh->color;
            delete m_copiedShape->mesh;
        }
        delete m_copiedShape;
        m_copiedShape = nullptr;
    }

    MeshEntry* src = nullptr;
    for (auto& entry : m_tempMeshes) {
        if (entry.name == m_draggingShapeId || entry.name == m_highlightedShapeId) {
            src = &entry;
            break;
        }
    }

    if (!src) {
        m_host->logWarning("No shape selected to copy.");
        return;
    }

    m_copiedShape = new MeshEntry();
    m_copiedShape->name       = src->name;
    m_copiedShape->text       = src->text;
    m_copiedShape->bitmapPath = src->bitmapPath;
    m_copiedShape->textColor  = src->textColor;
    m_copiedShape->textFont   = src->textFont;
    m_copiedShape->textSize   = src->textSize;
    m_copiedShape->isTextSelected = false;

    m_copiedShape->position = src->position
                                  ? new QVector3D(*src->position) : new QVector3D();
    m_copiedShape->rotation = src->rotation
                                  ? new QQuaternion(*src->rotation) : new QQuaternion();
    m_copiedShape->size = src->size
                              ? new QVector3D(*src->size) : new QVector3D(1,1,1);
    m_copiedShape->velocity = src->velocity
                                  ? new QVector3D(*src->velocity) : new QVector3D();

    m_copiedShape->trajectory = nullptr;
    m_copiedShape->collider   = nullptr;

    if (src->mesh) {
        m_copiedShape->mesh = new Mesh();
        m_copiedShape->mesh->lineWidth = src->mesh->lineWidth;
        m_copiedShape->mesh->closePath = src->mesh->closePath;
        m_copiedShape->mesh->color = src->mesh->color
                                         ? new QColor(*src->mesh->color) : new QColor(Qt::red);
        for (Vector* v : src->mesh->polygen)
            m_copiedShape->mesh->polygen.push_back(new Vector(v->x, v->y, v->z));
    } else {
        m_copiedShape->mesh = nullptr;
    }

    // CHAITANYA JAIN CHANGE: Reset click tracking when a shape is copied
    m_clickedSinceCopy = false;

    m_host->logMessage("Shape copied: " + src->name);
}

void ShapeEventHandler::pasteShape() {
    if (!m_copiedShape) {
        m_host->logWarning("Nothing to paste.");
        return;
    }

    // CHAITANYA JAIN CHANGE:
    // Fixed double-creation bug. Instead of entering m_isPastePending state,
    // immediately calculate an offset and place the shape.
    /* OLD CODE:
    m_isPastePending = true;
    m_host->setCursor(Qt::CrossCursor);
    m_host->logMessage("Click on canvas to paste shape at desired location.");
    */

    // Immediately calculate offset position and place it
    if (m_host && m_host->getGISlib() && m_copiedShape->position) {
        QPointF pasteGeoPos;

        if (m_clickedSinceCopy) {
            // CHAITANYA JAIN CHANGE: Paste at the last clicked position if the user clicked after copying
            pasteGeoPos = m_lastLeftClickGeoPos;
            // Reset so a rapid second paste goes exactly on top unless they click again
            m_clickedSinceCopy = false;
        } else {
            // CHAITANYA JAIN CHANGE: Paste exactly on top of the original if no click happened
            // canvasToGeo gives (lon, lat). m_copiedShape->position is (lon, lat).
            pasteGeoPos = QPointF(m_copiedShape->position->x(), m_copiedShape->position->y());
        }

        placePastedShape(pasteGeoPos);
        m_host->refreshView();
    }
}

void ShapeEventHandler::placePastedShape(const QPointF& geoPos) {
    if (!m_copiedShape) return;

    static int pasteCounter = 0;
    QString baseName = m_copiedShape->name;
    static const QRegularExpression reSuffix("(_copy_\\d+|_\\d+)$");
    baseName.remove(reSuffix);

    QString newName;
    do {
        newName = QString("%1_copy_%2").arg(baseName).arg(pasteCounter++);
    } while (std::any_of(m_tempMeshes.begin(), m_tempMeshes.end(),
                         [&](const MeshEntry& e){ return e.name == newName; }));

    MeshEntry newEntry;
    newEntry.name           = newName;
    newEntry.text           = m_copiedShape->text;
    newEntry.bitmapPath     = m_copiedShape->bitmapPath;
    newEntry.textColor      = m_copiedShape->textColor;
    newEntry.textFont       = m_copiedShape->textFont;
    newEntry.textSize       = m_copiedShape->textSize;
    newEntry.isTextSelected = false;
    newEntry.trajectory     = nullptr;
    newEntry.collider       = nullptr;

    newEntry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
    newEntry.rotation = new QQuaternion(*m_copiedShape->rotation);
    newEntry.size     = new QVector3D(*m_copiedShape->size);
    newEntry.velocity = new QVector3D(*m_copiedShape->velocity);

    if (m_copiedShape->mesh) {
        newEntry.mesh            = new Mesh();
        newEntry.mesh->lineWidth = m_copiedShape->mesh->lineWidth;
        newEntry.mesh->closePath = m_copiedShape->mesh->closePath;
        newEntry.mesh->color     = m_copiedShape->mesh->color
                                   ? new QColor(*m_copiedShape->mesh->color)
                                   : new QColor(Qt::red);
        for (Vector* v : m_copiedShape->mesh->polygen)
            newEntry.mesh->polygen.push_back(new Vector(v->x, v->y, v->z));
    } else {
        newEntry.mesh = nullptr;
    }

    m_tempMeshes.push_back(newEntry);

    if (m_layerPanel) {
        QString shapeType;
        if (newName.contains("Circle", Qt::CaseInsensitive)) shapeType = "Circle";
        else if (newName.contains("Rectangle", Qt::CaseInsensitive)) shapeType = "Rectangle";
        else if (newName.contains("Polygon", Qt::CaseInsensitive)) shapeType = "Polygon";
        else if (newName.contains("Polyline", Qt::CaseInsensitive)) shapeType = "Line";
        else if (newName.contains("Point", Qt::CaseInsensitive)) shapeType = "Point";
        else if (newName.contains("Text", Qt::CaseInsensitive)) shapeType = "Text";
        else if (newName.contains("Bitmap", Qt::CaseInsensitive)) shapeType = "Bitmap";
        else if (newName.contains("UserImage", Qt::CaseInsensitive)) shapeType = "Image";
        else shapeType = "Shape";

        m_layerPanel->addShapeToLayer(newName, shapeType);
    }

    m_highlightedShapeId = newName;
    m_host->logMessage(QString("Pasted shape: %1 at (lon: %2, lat: %3)")
                           .arg(newName)
                           .arg(geoPos.x())
                           .arg(geoPos.y()));

    m_host->refreshView();
}

void ShapeEventHandler::centerOnShape(const QString& shapeId) {
    if (!m_host || !m_host->getGISlib()) return;

    MeshEntry* target = nullptr;
    for (auto& entry : m_tempMeshes) {
        if (entry.name == shapeId) {
            target = &entry;
            break;
        }
    }
    if (!target || !target->position) return;

    double lon = target->position->x();
    double lat = target->position->y();
    m_host->getGISlib()->setCenter(lat, lon);
    m_highlightedShapeId = shapeId;
    m_host->refreshView();
}

void ShapeEventHandler::updateShapeProperties(const QString& shapeId, const QColor& color, int borderThickness) {
    for (auto& entry : m_tempMeshes) {
        if (entry.name == shapeId && entry.mesh && entry.mesh->color) {
            *entry.mesh->color = color;
            entry.mesh->lineWidth = borderThickness;
            m_host->refreshView();
            break;
        }
    }
}

void ShapeEventHandler::showShapePropertiesDialog(const QString& shapeId) {
    MeshEntry* shapeEntry = findMeshEntry(shapeId);
    if (!shapeEntry) {
        m_host->logError("Invalid entry for properties dialog: " + shapeId);
        return;
    }

    QWidget* parentWidget = dynamic_cast<QWidget*>(m_host);
    if (!parentWidget && m_layerPanel) {
        parentWidget = dynamic_cast<QWidget*>(m_layerPanel->parent());
    }

    bool isBitmap = !shapeEntry->bitmapPath.isEmpty();

    if (!m_shapePropertiesDialog) {
        m_shapePropertiesDialog = new QDialog(parentWidget);
        m_shapePropertiesDialog->setWindowTitle(isBitmap ? "Bitmap Properties" : "Shape Properties");
        m_shapePropertiesDialog->setFixedSize(300, 200);
        m_shapePropertiesDialog->setStyleSheet(
            "QDialog { background-color: #2b2b2b; color: white; }"
            "QLabel { color: white; font-weight: bold; }"
            "QSpinBox { background-color: #3c3c3c; color: white; border: 1px solid #555; padding: 5px; }"
            "QDoubleSpinBox { background-color: #3c3c3c; color: white; border: 1px solid #555; padding: 5px; }"
            "QPushButton { background-color: #404040; color: white; border: 1px solid #555; padding: 8px; font-weight: bold; }"
            "QPushButton:hover { background-color: #505050; }"
            "QPushButton:pressed { background-color: #606060; }"
            );
        m_shapePropertiesDialog->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(m_shapePropertiesDialog, &QDialog::finished, [this]() {
            m_shapePropertiesDialog = nullptr;
        });
    }

    if (m_shapePropertiesDialog->isVisible()) {
        m_shapePropertiesDialog->raise();
        m_shapePropertiesDialog->activateWindow();
        return;
    }

    if (m_shapePropertiesDialog->layout()) {
        QLayoutItem* item;
        while ((item = m_shapePropertiesDialog->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete m_shapePropertiesDialog->layout();
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(m_shapePropertiesDialog);

    QLabel *colorLabel = new QLabel(isBitmap ? "Background Color:" : "Shape Color:", m_shapePropertiesDialog);
    QPushButton *colorButton = new QPushButton(m_shapePropertiesDialog);
    colorButton->setText("Select Color");

    QColor initialColor = isBitmap ? Qt::white : (shapeEntry->mesh && shapeEntry->mesh->color ? *shapeEntry->mesh->color : Qt::red);
    colorButton->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold; border: 2px solid white;").arg(initialColor.name()));
    colorButton->setProperty("selectedColor", initialColor);

    mainLayout->addWidget(colorLabel);
    mainLayout->addWidget(colorButton);

    QSpinBox *thicknessSpinBox = nullptr;
    if (!isBitmap) {
        QLabel *thicknessLabel = new QLabel("Border Thickness:", m_shapePropertiesDialog);
        thicknessSpinBox = new QSpinBox(m_shapePropertiesDialog);
        thicknessSpinBox->setRange(1, 10);
        thicknessSpinBox->setValue(shapeEntry->mesh ? shapeEntry->mesh->lineWidth : 2);
        mainLayout->addWidget(thicknessLabel);
        mainLayout->addWidget(thicknessSpinBox);
    }

    QDoubleSpinBox *widthSpinBox = nullptr;
    QDoubleSpinBox *heightSpinBox = nullptr;
    if (isBitmap) {
        QLabel *sizeLabel = new QLabel("Size:", m_shapePropertiesDialog);
        QHBoxLayout *sizeLayout = new QHBoxLayout();
        widthSpinBox = new QDoubleSpinBox(m_shapePropertiesDialog);
        heightSpinBox = new QDoubleSpinBox(m_shapePropertiesDialog);
        widthSpinBox->setRange(0.1, 50.0);
        heightSpinBox->setRange(0.1, 50.0);
        widthSpinBox->setValue(shapeEntry->size->x());
        heightSpinBox->setValue(shapeEntry->size->y());
        widthSpinBox->setSuffix(" units");
        heightSpinBox->setSuffix(" units");
        sizeLayout->addWidget(new QLabel("W:", m_shapePropertiesDialog));
        sizeLayout->addWidget(widthSpinBox);
        sizeLayout->addWidget(new QLabel("H:", m_shapePropertiesDialog));
        sizeLayout->addWidget(heightSpinBox);
        mainLayout->addWidget(sizeLabel);
        mainLayout->addLayout(sizeLayout);
    }

    mainLayout->addStretch();

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *applyButton = new QPushButton("Apply", m_shapePropertiesDialog);
    QPushButton *cancelButton = new QPushButton("Cancel", m_shapePropertiesDialog);
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    QString currentShapeId = shapeId;

    QObject::connect(colorButton, &QPushButton::clicked, [=]() {
        QColor currentColor = colorButton->property("selectedColor").value<QColor>();
        QColor newColor = QColorDialog::getColor(currentColor, m_shapePropertiesDialog,
                                                 "Select Color", QColorDialog::DontUseNativeDialog);
        if (newColor.isValid()) {
            colorButton->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold; border: 2px solid white;").arg(newColor.name()));
            colorButton->setProperty("selectedColor", newColor);
        }
    });

    QObject::connect(applyButton, &QPushButton::clicked, [=]() {
        QColor selectedColor = colorButton->property("selectedColor").value<QColor>();
        if (isBitmap) {
            if (widthSpinBox && heightSpinBox) {
                shapeEntry->size->setX(widthSpinBox->value());
                shapeEntry->size->setY(heightSpinBox->value());
            }
        } else {
            if (thicknessSpinBox) {
                if (selectedColor.isValid() && shapeEntry->mesh && shapeEntry->mesh->color) {
                    *shapeEntry->mesh->color = selectedColor;
                }
                if (shapeEntry->mesh) {
                    shapeEntry->mesh->lineWidth = thicknessSpinBox->value();
                }
            }
        }
        m_host->refreshView();
        m_shapePropertiesDialog->accept();
    });

    QObject::connect(cancelButton, &QPushButton::clicked, m_shapePropertiesDialog, &QDialog::reject);
    m_shapePropertiesDialog->show();
    m_shapePropertiesDialog->raise();
    m_shapePropertiesDialog->activateWindow();
}

// ============================================================================
// Shape Drawing
// ============================================================================

void ShapeEventHandler::setShapeDrawingMode(bool enabled, const QString& shapeType) {
    m_host->setSelectedShape(shapeType);

    if (enabled) {
        if (m_layerPanel) {
            if (m_layerPanel->layerItems.isEmpty() && m_layerPanel->rasterLayers.isEmpty()) {
                QWidget* parentWidget = dynamic_cast<QWidget*>(m_host);
                if (!parentWidget && m_layerPanel) {
                    parentWidget = dynamic_cast<QWidget*>(m_layerPanel->parent());
                }

                QMessageBox msgBox(parentWidget);
                msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
                msgBox.setWindowTitle("No Layer Found");
                msgBox.setText("No layers exist!\n\nPlease create a layer first to draw shapes.");
                msgBox.setIcon(QMessageBox::Warning);

                QPushButton* createBtn = msgBox.addButton("Create New Layer", QMessageBox::AcceptRole);
                msgBox.addButton("Cancel", QMessageBox::RejectRole);
                msgBox.exec();

                if (msgBox.clickedButton() == createBtn) {
                    m_layerPanel->addLayer();
                    if (m_layerPanel->layerItems.isEmpty() && m_layerPanel->rasterLayers.isEmpty()) {
                        return;
                    }
                } else {
                    return;
                }
            }
        }

        if (m_host->getCurrentMode() != TransformMode::EditShape) {
            m_host->setCurrentMode(TransformMode::DrawShape);
        }
        m_host->setCursor(Qt::CrossCursor);

        if (shapeType == "Polygon") {
            for (Vector* v : m_tempPolygonVertices) delete v;
            m_tempPolygonVertices.clear();
            m_tempPolygonCanvasPoints.clear();
        } else if (shapeType == "Line") {
            for (Vector* v : m_tempLineVertices) delete v;
            m_tempLineVertices.clear();
            m_tempLineCanvasPoints.clear();
        }
    } else {
        if (m_host->getCurrentMode() == TransformMode::DrawShape) {
            m_host->setCurrentMode(TransformMode::Translate);
        }
        m_host->setSelectedShape("");
        m_host->setCursor(Qt::ArrowCursor);

        for (Vector* v : m_tempPolygonVertices) delete v;
        m_tempPolygonVertices.clear();
        m_tempPolygonCanvasPoints.clear();

        for (Vector* v : m_tempLineVertices) delete v;
        m_tempLineVertices.clear();
        m_tempLineCanvasPoints.clear();
    }

    m_host->refreshView();
}

void ShapeEventHandler::finalizePolygon() {
    if (!m_host || !m_host->getGISlib() || m_tempPolygonVertices.size() < 3) return;
    if (!m_shapesFeature) return;

    MeshEntry entry;
    entry.name = QString("TempPolygon_%1").arg(m_polygonCounter++);

    float avgX = 0, avgY = 0;
    for (const Vector* v : m_tempPolygonVertices) {
        avgX += v->x;
        avgY += v->y;
    }
    avgX /= m_tempPolygonVertices.size();
    avgY /= m_tempPolygonVertices.size();

    entry.position->setX(avgX);
    entry.position->setY(avgY);
    entry.position->setZ(0);
    entry.size->setX(1.0f);
    entry.size->setY(1.0f);
    entry.size->setZ(1.0f);

    entry.mesh = new Mesh();
    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 2;
    entry.mesh->closePath = true;

    for (Vector* v : m_tempPolygonVertices) {
        entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
    }

    m_tempMeshes.push_back(entry);
    m_shapesFeature->addShapeToActiveLayer(entry.name, "Polygon");

    for (Vector* v : m_tempPolygonVertices) {
        delete v;
    }
    m_tempPolygonVertices.clear();
    m_tempPolygonCanvasPoints.clear();

    m_host->refreshView();
    m_host->forceRedraw();
}

void ShapeEventHandler::finalizeLine() {
    if (!m_host || !m_host->getGISlib() || m_tempLineVertices.size() < 2) return;
    if (!m_shapesFeature) return;

    MeshEntry entry;
    entry.name = QString("TempPolyline_%1").arg(m_polylineCounter++);

    float avgX = 0, avgY = 0;
    for (const Vector* v : m_tempLineVertices) {
        avgX += v->x;
        avgY += v->y;
    }
    avgX /= m_tempLineVertices.size();
    avgY /= m_tempLineVertices.size();

    entry.position->setX(avgX);
    entry.position->setY(avgY);
    entry.position->setZ(0);
    entry.size->setX(1.0f);
    entry.size->setY(1.0f);
    entry.size->setZ(1.0f);

    entry.mesh = new Mesh();
    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 2;
    entry.mesh->closePath = false;

    for (Vector* v : m_tempLineVertices) {
        entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
    }

    m_tempMeshes.push_back(entry);
    m_shapesFeature->addShapeToActiveLayer(entry.name, "Line");

    for (Vector* v : m_tempLineVertices) {
        delete v;
    }
    m_tempLineVertices.clear();
    m_tempLineCanvasPoints.clear();

    m_host->refreshView();
    m_host->forceRedraw();
}

void ShapeEventHandler::cancelShapeDrawing() {
    for (Vector* v : m_tempPolygonVertices) delete v;
    m_tempPolygonVertices.clear();
    m_tempPolygonCanvasPoints.clear();

    for (Vector* v : m_tempLineVertices) delete v;
    m_tempLineVertices.clear();
    m_tempLineCanvasPoints.clear();
}

// ============================================================================
// Shape Queries
// ============================================================================

bool ShapeEventHandler::isShape(const QString& shapeId) const {
    return shapeId.startsWith("TempCircle") ||
           shapeId.startsWith("TempRectangle") ||
           shapeId.startsWith("TempPolygon") ||
           shapeId.startsWith("TempPolyline") ||
           shapeId.startsWith("TempPoint") ||
           shapeId.startsWith("TempBitmap") ||
           shapeId.startsWith("UserImage_");
}

bool ShapeEventHandler::shouldDrawShape(const QString& shapeId) const {
    if (!m_layerPanel) {
        return true;
    }
    QString layerName = m_layerPanel->getLayerForShape(shapeId);
    if (layerName.isEmpty()) {
        return true;
    }
    return m_layerPanel->isLayerVisible(layerName);
}

MeshEntry* ShapeEventHandler::findMeshEntry(const QString& shapeId) {
    for (auto& entry : m_tempMeshes) {
        if (entry.name == shapeId) {
            return &entry;
        }
    }
    return nullptr;
}

const MeshEntry* ShapeEventHandler::findMeshEntry(const QString& shapeId) const {
    for (const auto& entry : m_tempMeshes) {
        if (entry.name == shapeId) {
            return &entry;
        }
    }
    return nullptr;
}

QPolygonF ShapeEventHandler::getRotatedShapePolygon(const MeshEntry& entry) const {
    QPolygonF polygon;
    if (!m_host || !m_host->getGISlib()) return polygon;

    QPointF centerGeo(entry.position->x(), entry.position->y());
    QPointF centerCanvas = m_host->getGISlib()->geoToCanvas(centerGeo.y(), centerGeo.x());
    float rotationRad = entry.rotation->z();
    float cosFwd = std::cos(rotationRad);
    float sinFwd = std::sin(rotationRad);

    if (entry.name.startsWith("TempCircle")) {
        double radiusAbsLon = static_cast<double>(entry.position->x()) + static_cast<double>(entry.size->x());
        double radiusAbsLat = static_cast<double>(entry.position->y());
        QPointF radiusPointCanvas = m_host->getGISlib()->geoToCanvas(radiusAbsLat, radiusAbsLon);
        float radius = QVector2D(radiusPointCanvas - centerCanvas).length();

        for (int i = 0; i < 32; ++i) {
            float angle = (i * 2.0f * M_PI) / 32.0f;
            QPointF p = centerCanvas + QPointF(radius * std::cos(angle), radius * std::sin(angle));
            polygon << p;
        }
        return polygon;
    }

    if (entry.name.startsWith("TempText") && !entry.text.isEmpty()) {
        QPointF textPos = m_host->getGISlib()->geoToCanvas(entry.position->y(), entry.position->x());
        QFontMetrics fm(entry.textFont);
        QRect textRect = fm.boundingRect(entry.text);
        textRect.moveTo(textPos.x(), textPos.y() - fm.ascent());

        QRectF expanded = textRect.adjusted(-5, -5, 5, 5);
        polygon << expanded.topLeft() << expanded.topRight()
                << expanded.bottomRight() << expanded.bottomLeft();
        return polygon;
    }

    if (entry.mesh && !entry.mesh->polygen.empty()) {
        for (Vector* v : entry.mesh->polygen) {
            double localX = static_cast<double>(v->x);
            double localY = static_cast<double>(v->y);
            double worldX = localX * cosFwd - localY * sinFwd;
            double worldY = localX * sinFwd + localY * cosFwd;
            double absLon = static_cast<double>(centerGeo.x()) + worldX;
            double absLat = static_cast<double>(centerGeo.y()) + worldY;
            QPointF vCanvas = m_host->getGISlib()->geoToCanvas(absLat, absLon);
            polygon << vCanvas;
        }
        return polygon;
    }

    if (!entry.bitmapPath.isEmpty()) {
        float halfW = entry.size->x() / 2.0f;
        float halfH = entry.size->y() / 2.0f;

        QVector<QPointF> localCorners = {
            QPointF(-halfW, -halfH),
            QPointF(halfW, -halfH),
            QPointF(halfW, halfH),
            QPointF(-halfW, halfH)
        };

        for (const QPointF& corner : localCorners) {
            double worldX = static_cast<double>(corner.x()) * cosFwd - static_cast<double>(corner.y()) * sinFwd;
            double worldY = static_cast<double>(corner.x()) * sinFwd + static_cast<double>(corner.y()) * cosFwd;
            double absLon = static_cast<double>(centerGeo.x()) + worldX;
            double absLat = static_cast<double>(centerGeo.y()) + worldY;
            QPointF cCanvas = m_host->getGISlib()->geoToCanvas(absLat, absLon);
            polygon << cCanvas;
        }
        return polygon;
    }

    if (entry.name.startsWith("TempPoint")) {
        for (int i = 0; i < 8; ++i) {
            float angle = (i * 2.0f * M_PI) / 8.0f;
            QPointF p = centerCanvas + QPointF(6 * std::cos(angle), 6 * std::sin(angle));
            polygon << p;
        }
    }
    return polygon;
}

// ============================================================================
// Shape Dragging
// ============================================================================

void ShapeEventHandler::handleShapeDragging(QMouseEvent *event) {
    if (!m_isDraggingShape || m_draggingShapeId.isEmpty() || !m_host || !m_host->getGISlib()) return;

    for (auto& entry : m_tempMeshes) {
        if (entry.name == m_draggingShapeId) {
            static QString lastDraggedShape = "";
            if (lastDraggedShape != m_draggingShapeId) {
                if (m_shapesFeature) {
                    m_shapesFeature->saveShapeState(m_draggingShapeId, &entry);
                }
                lastDraggedShape = m_draggingShapeId;
            }

            QPointF currentGeoPos = m_host->getGISlib()->canvasToGeo(m_shapeDragStartPos);
            QPointF newGeoPos = m_host->getGISlib()->canvasToGeo(event->pos());
            double deltaLon = newGeoPos.x() - currentGeoPos.x();
            double deltaLat = newGeoPos.y() - currentGeoPos.y();

            entry.position->setX(entry.position->x() + deltaLon);
            entry.position->setY(entry.position->y() + deltaLat);
            m_shapeDragStartPos = event->pos();

            m_host->refreshView();
            return;
        }
    }
    stopShapeDragging();
}

void ShapeEventHandler::stopShapeDragging() {
    if (m_isDraggingShape) {
        m_isDraggingShape = false;
        m_draggingShapeId = "";
        m_host->setCursor(Qt::ArrowCursor);
        m_host->refreshView();
    }
}

bool ShapeEventHandler::moveShapeByName(const std::string& shapeName, const QPointF& geoPos) {
    if (!m_shapesFeature) return false;

    for (auto& entry : m_tempMeshes) {
        if (entry.name.toStdString() == shapeName) {
            m_shapesFeature->moveShapeByScript(&entry, geoPos);
            return true;
        }
    }
    return false;
}

bool ShapeEventHandler::deleteObjectById(const QString& id) {
    for (auto it = m_tempMeshes.begin(); it != m_tempMeshes.end(); ++it) {
        if (it->name == id) {
            if (m_shapesFeature) {
                m_shapesFeature->clearHistory(id);
            }
            // Let MeshEntry destructor handle memory cleanup for position, rotation, etc.
            // when erase() is called.
            m_tempMeshes.erase(it);

            if (m_layerPanel) {
                m_layerPanel->removeShapeFromLayer(id);
            }
            m_host->refreshView();
            return true;
        }
    }
    return false;
}

// CHAITANYA JAIN CHANGE:
// What was fixed: Delete Confirmation UX
// Why the bug happened: Pressing the Delete key on the keyboard or clicking Delete in the context menu erased shapes instantly without any safety prompt, causing accidental data loss.
// Why this change is safe: Injected QMessageBox::question before deletion memory logic, strictly executing only if the user presses "Yes".
void ShapeEventHandler::deleteShape(const QString& shapeId) {
    QWidget* parentWidget = dynamic_cast<QWidget*>(m_host);
    if (!parentWidget && m_layerPanel) {
        parentWidget = dynamic_cast<QWidget*>(m_layerPanel->parent());
    }

    QMessageBox msgBox(parentWidget);
    msgBox.setWindowTitle("Confirm Deletion");
    msgBox.setText(QString("Are you sure you want to delete this shape?"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() != QMessageBox::Yes) {
        return;
    }

    if (m_shapesFeature) {
        m_shapesFeature->clearHistory(shapeId);
    }

    for (auto it = m_tempMeshes.begin(); it != m_tempMeshes.end(); ++it) {
        if (it->name == shapeId) {
            // Let MeshEntry destructor handle memory cleanup for position, rotation, etc.
            // when erase() is called.
            m_tempMeshes.erase(it);

            if (m_layerPanel) {
                m_layerPanel->removeShapeFromLayer(shapeId);
            }

            if (m_highlightedShapeId == shapeId) {
                m_highlightedShapeId = "";
            }
            if (m_draggingShapeId == shapeId) {
                m_draggingShapeId = "";
                m_isDraggingShape = false;
            }
            if (m_editingShapeId == shapeId) {
                m_editingShapeId = "";
                m_isResizingShape = false;
                m_resizeHandles.clear();
                m_host->setCurrentMode(TransformMode::Translate);
                m_host->setCursor(Qt::ArrowCursor);
            }

            m_host->refreshView();
            return;
        }
    }
}

// ============================================================================
// Counters
// ============================================================================

void ShapeEventHandler::syncCountersFromLoadedShapes(const std::vector<MeshEntry>& meshes) {
    QRegularExpression re("_(\\d+)$");
    for (const MeshEntry& entry : meshes) {
        QRegularExpressionMatch m = re.match(entry.name);
        if (!m.hasMatch()) continue;
        int idx = m.captured(1).toInt();

        if (entry.name.startsWith("TempBitmap_")) {
            m_bitmapCounter = qMax(m_bitmapCounter, idx + 1);
        } else if (entry.name.startsWith("TempText_")) {
            m_textCounter = qMax(m_textCounter, idx + 1);
        } else if (entry.name.startsWith("UserImage_")) {
            m_userImageCounter = qMax(m_userImageCounter, idx + 1);
        } else if (entry.name.startsWith("TempPolyline_")) {
            m_polylineCounter = qMax(m_polylineCounter, idx + 1);
        } else if (entry.name.startsWith("TempPolygon_")) {
            m_polygonCounter = qMax(m_polygonCounter, idx + 1);
        }
    }
}

void ShapeEventHandler::resetCounters() {
    m_polygonCounter = 0;
    m_polylineCounter = 0;
    m_bitmapCounter = 0;
    m_textCounter = 0;
    m_userImageCounter = 0;
}

// ============================================================================
// Drawing Methods
// ============================================================================

void ShapeEventHandler::drawTempShapesPreview(QPainter& painter) {
    if (!m_host || !m_host->getGISlib()) return;

    if (!m_tempPolygonVertices.empty()) {
        drawPolygonPreview(painter);
    }

    if (!m_tempLineVertices.empty()) {
        drawLinePreview(painter);
    }
}

void ShapeEventHandler::drawPolygonPreview(QPainter& painter) {
    if (!m_host || !m_host->getGISlib()) return;

    QPolygonF previewPolygon;
    for (const Vector* vertex : m_tempPolygonVertices) {
        QPointF canvasPoint = m_host->getGISlib()->geoToCanvas(vertex->y, vertex->x);
        previewPolygon << canvasPoint;
    }

    if (m_host->getCurrentMode() == TransformMode::DrawShape &&
        m_host->getSelectedShape() == "Polygon") {
        QPoint mousePos = m_host->getMousePosition();
        previewPolygon << mousePos;
    }

    painter.save();

    painter.setPen(QPen(Qt::red, 3, Qt::SolidLine));
    painter.setBrush(QColor(255, 0, 0, 50));

    if (previewPolygon.size() == 1) {
        painter.setBrush(Qt::red);
        painter.drawEllipse(previewPolygon[0], 6, 6);
        painter.setPen(Qt::white);
        painter.drawText(previewPolygon[0] + QPointF(8, -8), "1");
    } else if (previewPolygon.size() == 2) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Qt::red, 3, Qt::SolidLine));
        painter.drawLine(previewPolygon[0], previewPolygon[1]);
        painter.setBrush(Qt::red);
        painter.drawEllipse(previewPolygon[0], 6, 6);
        painter.drawEllipse(previewPolygon[1], 6, 6);
        painter.setPen(Qt::white);
        painter.drawText(previewPolygon[0] + QPointF(8, -8), "1");
        painter.drawText(previewPolygon[1] + QPointF(8, -8), "2");
    } else if (previewPolygon.size() > 2) {
        painter.setPen(QPen(Qt::red, 3, Qt::SolidLine));
        painter.setBrush(QColor(255, 0, 0, 30));
        painter.drawPolygon(previewPolygon);

        painter.setBrush(Qt::red);
        for (int i = 0; i < previewPolygon.size(); ++i) {
            painter.drawEllipse(previewPolygon[i], 5, 5);
            painter.setPen(Qt::white);
            painter.drawText(previewPolygon[i] + QPointF(8, -8), QString::number(i + 1));
            painter.setPen(QPen(Qt::red, 2));
        }

        if (m_host->getSelectedShape() == "Polygon") {
            painter.setPen(QPen(Qt::red, 2, Qt::DotLine));
            painter.drawLine(previewPolygon.last(), previewPolygon.first());
        }
    }

    painter.restore();
}

void ShapeEventHandler::drawLinePreview(QPainter& painter) {
    if (!m_host || !m_host->getGISlib()) return;

    QPolygonF previewLine;
    for (const Vector* vertex : m_tempLineVertices) {
        QPointF canvasPoint = m_host->getGISlib()->geoToCanvas(vertex->y, vertex->x);
        previewLine << canvasPoint;
    }

    if (m_host->getCurrentMode() == TransformMode::DrawShape &&
        m_host->getSelectedShape() == "Line") {
        QPoint mousePos = m_host->getMousePosition();
        previewLine << mousePos;
    }

    painter.save();

    painter.setPen(QPen(Qt::red, 3, Qt::SolidLine));
    painter.setBrush(Qt::NoBrush);

    if (previewLine.size() == 1) {
        painter.setBrush(Qt::red);
        painter.drawEllipse(previewLine[0], 6, 6);
        painter.setPen(Qt::white);
        painter.drawText(previewLine[0] + QPointF(8, -8), "1");
    } else if (previewLine.size() >= 2) {
        painter.setPen(QPen(Qt::red, 3, Qt::SolidLine));
        painter.drawPolyline(previewLine);

        painter.setBrush(Qt::red);
        for (int i = 0; i < previewLine.size(); ++i) {
            painter.drawEllipse(previewLine[i], 5, 5);
            painter.setPen(Qt::white);
            painter.drawText(previewLine[i] + QPointF(8, -8), QString::number(i + 1));
            painter.setPen(QPen(Qt::red, 2));
        }
    }

    painter.restore();
}

void ShapeEventHandler::drawTempMeshes(QPainter& painter) {
    if (!m_host || !m_host->getGISlib()) return;

    for (const auto& entry : m_tempMeshes) {
        if (!shouldDrawShape(entry.name)) continue;

        if (entry.name.startsWith("Text") && !entry.text.isEmpty()) {
            drawText(painter, entry);
        } else if (!entry.bitmapPath.isEmpty()) {
            drawBitmap(painter, entry);
        } else if (entry.name.startsWith("TempPoint")) {
            drawPoint(painter, entry);
        } else if (entry.name.startsWith("TempCircle")) {
            drawCircle(painter, entry);
        } else {
            drawPolygonShape(painter, entry);
        }
    }

    if (m_shapesFeature && m_shapesFeature->isShowingPreview()) {
        m_shapesFeature->drawHistoryPreview(painter, m_host->getGISlib());
    }
}
void ShapeEventHandler::drawText(QPainter& painter, const MeshEntry& entry)
{
    if (!m_host || !m_host->getGISlib() || !entry.position)
        return;

    QPointF canvasPos =
        m_host->getGISlib()->geoToCanvas(
            entry.position->y(),
            entry.position->x()
            );

    painter.save();

    // ---------------------------------------------------------
    // Use the exact same font for drawing AND bounding rectangle
    // ---------------------------------------------------------
    QFont font = entry.textFont;

    if (entry.textSize > 0)
        font.setPointSize(entry.textSize);

    painter.setFont(font);
    painter.setPen(entry.textColor);

    // ---------------------------------------------------------
    // Draw actual text
    // ---------------------------------------------------------
    painter.drawText(canvasPos, entry.text);

    // =========================================================
    // FIX:
    // Show blue Text selection/edit handles ONLY in EditShape mode.
    //
    // Normal canvas selection:
    //      NO blue handles
    //
    // Layer Panel selection:
    //      NO blue handles
    //
    // Edit mode:
    //      Blue handles visible
    // =========================================================
    if (entry.isTextSelected &&
        m_host->getCurrentMode() == TransformMode::EditShape)
    {
        drawTextSelectionHandles(painter, entry);
    }

    // ---------------------------------------------------------
    // Layer Panel / normal selection highlight for Text
    //
    // Yellow dashed rectangle is allowed during normal selection.
    // This is separate from Edit handles.
    // ---------------------------------------------------------
    if (entry.name == m_highlightedShapeId)
    {
        QFontMetricsF fm(font);

        QRectF textRect =
            fm.boundingRect(entry.text);

        // drawText(QPointF) uses canvasPos as baseline.
        textRect.moveTopLeft(
            QPointF(
                canvasPos.x(),
                canvasPos.y() - fm.ascent()
                )
            );

        // Small padding around text
        textRect.adjust(-5, -4, 5, 4);

        painter.setPen(
            QPen(
                Qt::yellow,
                3,
                Qt::DashLine
                )
            );

        painter.setBrush(Qt::NoBrush);

        painter.drawRect(textRect);
    }

    painter.restore();
}
void ShapeEventHandler::drawTextSelectionHandles(
    QPainter& painter,
    const MeshEntry& entry)
{
    bool isSelected = entry.isTextSelected ||
                      (entry.name == m_highlightedShapeId) ||
                      (entry.name == m_editingTextId) ||
                      (entry.name == m_editingShapeId);

    if (!m_host ||
        !m_host->getGISlib() ||
        !entry.position ||
        !isSelected)
    {
        return;
    }

    // ============================================================
    // 1. GET TEXT CANVAS POSITION
    // ============================================================
    QPointF canvasPos =
        m_host->getGISlib()->geoToCanvas(
            entry.position->y(),
            entry.position->x()
            );


    // ============================================================
    // 2. USE EXACT SAME FONT AS drawText()
    // ============================================================
    QFont font = entry.textFont;

    if (entry.textSize > 0)
    {
        font.setPointSize(entry.textSize);
    }

    QFontMetricsF fm(font);


    // ============================================================
    // 3. CALCULATE TEXT BOUNDING RECT
    //
    // drawText(QPointF, text) uses canvasPos as BASELINE.
    // ============================================================
    QRectF textRect =
        fm.boundingRect(entry.text);

    textRect.moveTopLeft(
        QPointF(
            canvasPos.x(),
            canvasPos.y() - fm.ascent()
            )
        );


    // ============================================================
    // 4. CUSTOM SELECTION BOX
    //
    // This will ALWAYS be visible when Text is selected.
    // ============================================================
    QRectF selectionRect =
        textRect.adjusted(
            -5.0,
            -4.0,
            5.0,
            4.0
            );


    painter.save();

    painter.setBrush(Qt::NoBrush);

    painter.setPen(
        QPen(
            Qt::blue,
            2,
            Qt::DashLine
            )
        );

    // Draw custom selection box
    painter.drawRect(selectionRect);


    // ============================================================
    // 5. RESIZE / EDIT HANDLES
    //
    // IMPORTANT:
    // 4 blue dots ONLY appear in EditShape mode.
    //
    // Normal selection:
    //     Custom Box = YES
    //     Blue Handles = NO
    //
    // EditShape:
    //     Custom Box = YES
    //     Blue Handles = YES
    // ============================================================
    if (m_host->getCurrentMode() == TransformMode::EditShape)
    {
        painter.setPen(
            QPen(
                Qt::blue,
                1
                )
            );

        painter.setBrush(
            Qt::blue
            );


        QVector<QPointF> handles =
            {
                selectionRect.topLeft(),
                selectionRect.topRight(),
                selectionRect.bottomRight(),
                selectionRect.bottomLeft()
            };


        for (const QPointF& handle : handles)
        {
            painter.drawRect(
                QRectF(
                    handle.x() - 3.0,
                    handle.y() - 3.0,
                    6.0,
                    6.0
                    )
                );
        }
    }


    painter.restore();
}
void ShapeEventHandler::drawBitmap(QPainter& painter, const MeshEntry& entry) {
    if (!m_host || !m_host->getGISlib()) return;

    QPixmap img(entry.bitmapPath);
    if (img.isNull()) return;

    QPointF canvasPos = m_host->getGISlib()->geoToCanvas(entry.position->y(), entry.position->x());
    double sizeAbsLon = static_cast<double>(entry.position->x()) + static_cast<double>(entry.size->x());
    double sizeAbsLat = static_cast<double>(entry.position->y()) + static_cast<double>(entry.size->y());
    QPointF sizePointCanvas = m_host->getGISlib()->geoToCanvas(sizeAbsLat, sizeAbsLon);
    float canvasWidth = qAbs(sizePointCanvas.x() - canvasPos.x()) * 2;
    float canvasHeight = qAbs(sizePointCanvas.y() - canvasPos.y()) * 2;

    // CHAITANYA JAIN CHANGE:
    // Prevented full-raster extreme-zoom allocation and added visible-region rendering.
    // Reason: large zoom levels caused excessive processing/memory use, UI freeze,
    // and possible application crash. We let QPainter hardware-scale instead of QPixmap::scaled.
    /* OLD CODE:
    QPixmap scaled = img.scaled(canvasWidth, canvasHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    painter.save();
    painter.translate(canvasPos);
    painter.rotate(entry.rotation->z() * (180.0 / M_PI));
    painter.drawPixmap(QPointF(-scaled.width() / 2.0f, -scaled.height() / 2.0f), scaled);
    painter.restore();
    */

    painter.save();
    painter.translate(canvasPos);
    painter.rotate(entry.rotation->z() * (180.0 / M_PI));

    // Set destination rectangle centered on the translated origin
    QRectF destRect(-canvasWidth / 2.0f, -canvasHeight / 2.0f, canvasWidth, canvasHeight);

    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawPixmap(destRect, img, img.rect());

    painter.restore();

    if (entry.name == m_highlightedShapeId) {
        painter.save();
        painter.setPen(QPen(Qt::yellow, 3, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        QPolygonF outline = getRotatedShapePolygon(entry);
        if (!outline.isEmpty()) {
            painter.drawPolygon(outline);
        }
        painter.restore();
    }
}

void ShapeEventHandler::drawPoint(QPainter& painter, const MeshEntry& entry) {
    if (!m_host || !m_host->getGISlib() || !entry.mesh) return;

    QPointF canvasPoint = m_host->getGISlib()->geoToCanvas(entry.position->y(), entry.position->x());
    painter.setPen(QPen(*entry.mesh->color, entry.mesh->lineWidth, Qt::SolidLine));
    painter.setBrush(*entry.mesh->color);
    painter.drawEllipse(canvasPoint, 3, 3);

    if (entry.name == m_highlightedShapeId) {
        painter.save();
        painter.setPen(QPen(Qt::yellow, 3, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(canvasPoint, 8, 8);
        painter.restore();
    }
}

void ShapeEventHandler::drawCircle(QPainter& painter, const MeshEntry& entry) {
    if (!m_host || !m_host->getGISlib() || !entry.mesh || !entry.mesh->color) return;

    QPointF canvasCenter = m_host->getGISlib()->geoToCanvas(entry.position->y(), entry.position->x());
    double radiusAbsLon = static_cast<double>(entry.position->x()) + static_cast<double>(entry.size->x());
    double radiusAbsLat = static_cast<double>(entry.position->y());
    QPointF radiusPointCanvas = m_host->getGISlib()->geoToCanvas(radiusAbsLat, radiusAbsLon);
    float canvasRadius = sqrt(pow(radiusPointCanvas.x() - canvasCenter.x(), 2) +
                              pow(radiusPointCanvas.y() - canvasCenter.y(), 2));

    if (canvasRadius > 0 && canvasRadius < 10000) {
        painter.setPen(QPen(*entry.mesh->color, entry.mesh->lineWidth, Qt::SolidLine));
        QColor fillColor = *entry.mesh->color;
        fillColor.setAlpha(50);
        painter.setBrush(fillColor);
        painter.drawEllipse(canvasCenter, canvasRadius, canvasRadius);
    }

    if (entry.name == m_highlightedShapeId) {
        painter.save();
        painter.setPen(QPen(Qt::yellow, 3, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        QPolygonF outline = getRotatedShapePolygon(entry);
        if (!outline.isEmpty()) {
            painter.drawPolygon(outline);
        }
        painter.restore();
    }
}

void ShapeEventHandler::drawPolygonShape(QPainter& painter, const MeshEntry& entry) {
    if (!m_host || !m_host->getGISlib() || !entry.mesh || !entry.mesh->color || entry.mesh->polygen.empty()) return;

    QPolygonF polygon;
    QPointF centerGeo(entry.position->x(), entry.position->y());
    float rotationRad = entry.rotation->z();
    float cosA = std::cos(rotationRad);
    float sinA = std::sin(rotationRad);

    for (Vector* point : entry.mesh->polygen) {
        if (!point) continue;
        double localX = static_cast<double>(point->x);
        double localY = static_cast<double>(point->y);
        double rotatedX = localX * cosA - localY * sinA;
        double rotatedY = localX * sinA + localY * cosA;
        double worldGeoX = static_cast<double>(centerGeo.x()) + rotatedX;
        double worldGeoY = static_cast<double>(centerGeo.y()) + rotatedY;
        QPointF canvasPoint = m_host->getGISlib()->geoToCanvas(worldGeoY, worldGeoX);
        polygon << canvasPoint;
    }

    if (!polygon.isEmpty()) {
        painter.setPen(QPen(*entry.mesh->color, entry.mesh->lineWidth, Qt::SolidLine));
        if (entry.name.startsWith("TempPolygon") || entry.name.startsWith("TempRectangle")) {
            QColor fillColor = *entry.mesh->color;
            fillColor.setAlpha(50);
            painter.setBrush(fillColor);
            painter.drawPolygon(polygon);
        } else {
            painter.setBrush(Qt::NoBrush);
            painter.drawPolyline(polygon);
        }
    }

    if (entry.name == m_highlightedShapeId) {
        painter.save();
        painter.setPen(QPen(Qt::yellow, 3, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        QPolygonF outline = getRotatedShapePolygon(entry);
        if (!outline.isEmpty()) {
            painter.drawPolygon(outline);
        }
        painter.restore();
    }
}

void ShapeEventHandler::drawSelectionOutline(QPainter& painter) {
    // Note: showOutline should be managed by the host
    // For now, we'll always draw it if there are selected shapes

    if (!m_selectedShapeIds.empty()) {
        QPen shapePen(Qt::yellow);
        shapePen.setWidth(2);
        shapePen.setStyle(Qt::DashLine);
        painter.setPen(shapePen);
        painter.setBrush(Qt::NoBrush);

        for (const QString& name : m_selectedShapeIds) {
            for (const auto& entry : m_tempMeshes) {
                if (entry.name != name) continue;
                if (!shouldDrawShape(entry.name)) continue;

                QPolygonF poly = getRotatedShapePolygon(entry);
                if (poly.isEmpty()) continue;

                painter.save();
                painter.setPen(shapePen);
                painter.setBrush(Qt::NoBrush);

                // CHAITANYA JAIN CHANGE:
                // Clip outlines to perfectly prevent Qt 16-bit integer overflow scattering when selected
                if (entry.name.startsWith("TempPolyline")) {
                    painter.drawPolyline(poly);
                } else if (entry.name.startsWith("TempPoint")) {
                    QPointF center = m_host->getGISlib()->geoToCanvas(entry.position->y(), entry.position->x());
                    painter.drawEllipse(center, 8, 8);
                } else {
                    painter.drawPolygon(poly);
                }
                painter.restore();
                break;
            }
        }
    }

    // if (m_isDraggingShape && !m_draggingShapeId.isEmpty()) {
    // Individual drag outline should only be drawn when the dragged
    // shape is NOT already part of multi-selection.
    bool draggingShapeIsMultiSelected =
        std::find(
            m_selectedShapeIds.begin(),
            m_selectedShapeIds.end(),
            m_draggingShapeId
            ) != m_selectedShapeIds.end();

    if (m_isDraggingShape &&
        !m_draggingShapeId.isEmpty() &&
        !draggingShapeIsMultiSelected)
    {
        if (shouldDrawShape(m_draggingShapeId)) {
            for (const auto& entry : m_tempMeshes) {
                if (entry.name == m_draggingShapeId) {
                    QPolygonF rotatedPoly = getRotatedShapePolygon(entry);
                    if (!rotatedPoly.isEmpty()) {
                        painter.save();
                        QPen pen(Qt::yellow, 2, Qt::DashLine);
                        painter.setPen(pen);
                        painter.setBrush(Qt::NoBrush);
                        // CHAITANYA JAIN CHANGE:
                        // Clip outlines to perfectly prevent Qt 16-bit integer overflow scattering when dragging
                        if (entry.name.startsWith("TempPolyline")) {
                            painter.drawPolyline(rotatedPoly);
                        } else {
                            painter.drawPolygon(rotatedPoly);
                        }
                        painter.restore();
                    }
                    break;
                }
            }
        }
    }
}
void ShapeEventHandler::clearShapeSelectionState()
{
    // Clear multi shape selection
    m_selectedShapeIds.clear();

    // Clear highlighted individual shape
    m_highlightedShapeId.clear();

    // CRITICAL FIX:
    // Prevent stale Line/Polyline drag outline
    m_isDraggingShape = false;
    m_draggingShapeId.clear();

    // Reset individual edit state
    m_isResizingShape = false;
    m_selectedHandleIndex = -1;
    m_editingShapeId.clear();

    // Reset rotate state
    m_activeRotateId.clear();

    // Reset bitmap drag state
    m_isDraggingBitmap = false;
    m_draggingBitmapId.clear();

    // Reset user image drag state
    m_isDraggingUserImage = false;
    m_draggingUserImageId.clear();

    if (m_host)
    {
        m_host->setCursor(Qt::ArrowCursor);
        m_host->refreshView();
    }
}

void ShapeEventHandler::drawResizeHandles(QPainter& painter) {
    if (m_host->getCurrentMode() != TransformMode::EditShape || m_editingShapeId.isEmpty()) return;
    if (!shouldDrawShape(m_editingShapeId)) return;

    painter.save();
    for (size_t i = 0; i < m_resizeHandles.size(); ++i) {
        painter.setPen(QPen(Qt::blue, 2));
        painter.setBrush(Qt::blue);
        painter.drawRect(QRectF(m_resizeHandles[i] - QPointF(4, 4), QSizeF(8, 8)));

        if (i == static_cast<size_t>(m_selectedHandleIndex)) {
            painter.setPen(QPen(Qt::yellow, 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(QRectF(m_resizeHandles[i] - QPointF(6, 6), QSizeF(12, 12)));
        }
    }
    painter.restore();
}

void ShapeEventHandler::drawRotationHandle(QPainter& painter, const MeshEntry& entry) {
    if (m_activeRotateId != entry.name)
        return;
    if (!entry.position || !entry.size) return;

    bool isRotatable = !entry.bitmapPath.isEmpty() ||
                       entry.name.startsWith("TempRectangle") ||
                       entry.name.startsWith("TempCircle") ||
                       entry.name.startsWith("TempPolygon") ||
                       entry.name.startsWith("TempPolyline");

    if (!isRotatable) return;

    QPointF center = m_host->getGISlib()->geoToCanvas(entry.position->y(), entry.position->x());
    QRectF boundingBox;

    if (entry.name.startsWith("TempCircle")) {
        double radiusAbsLon = static_cast<double>(entry.position->x()) + static_cast<double>(entry.size->x());
        double radiusAbsLat = static_cast<double>(entry.position->y());
        QPointF radiusPointCanvas = m_host->getGISlib()->geoToCanvas(radiusAbsLat, radiusAbsLon);
        float canvasRadius = QVector2D(radiusPointCanvas - center).length();
        boundingBox = QRectF(center.x() - canvasRadius, center.y() - canvasRadius,
                             canvasRadius * 2, canvasRadius * 2);
    } else if (entry.name.startsWith("TempRectangle") || !entry.bitmapPath.isEmpty()) {
        double sizeAbsLon = static_cast<double>(entry.position->x()) + static_cast<double>(entry.size->x());
        double sizeAbsLat = static_cast<double>(entry.position->y()) + static_cast<double>(entry.size->y());
        QPointF sizePointCanvas = m_host->getGISlib()->geoToCanvas(sizeAbsLat, sizeAbsLon);
        float w = qAbs(sizePointCanvas.x() - center.x()) * 2;
        float h = qAbs(sizePointCanvas.y() - center.y()) * 2;
        boundingBox = QRectF(center.x() - w/2, center.y() - h/2, w, h);
    } else if (entry.name.startsWith("TempPolygon") || entry.name.startsWith("TempPolyline")) {
        QPolygonF poly;
        if (entry.mesh && !entry.mesh->polygen.empty()) {
            float rotationRad = entry.rotation->z();
            float cosFwd = std::cos(rotationRad);
            float sinFwd = std::sin(rotationRad);
            for (const Vector* v : entry.mesh->polygen) {
                double localX = static_cast<double>(v->x);
                double localY = static_cast<double>(v->y);
                double worldX = localX * cosFwd - localY * sinFwd;
                double worldY = localX * sinFwd + localY * cosFwd;
                double absLon = static_cast<double>(entry.position->x()) + worldX;
                double absLat = static_cast<double>(entry.position->y()) + worldY;
                poly << m_host->getGISlib()->geoToCanvas(absLat, absLon);
            }
        }
        if (poly.isEmpty()) return;
        boundingBox = poly.boundingRect();
    } else {
        return;
    }

    QPointF handleOffset(30, 30);
    QPointF handle = boundingBox.bottomRight() + handleOffset;

    painter.save();
    painter.setPen(QPen(QColor(100, 200, 255), 1, Qt::DashLine));
    painter.drawLine(center, handle);

    painter.setPen(QPen(Qt::blue, 2));
    painter.setBrush(QColor(100, 200, 255));
    painter.drawEllipse(handle, 10, 10);

    painter.setPen(QPen(Qt::white, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(handle, 6, 6);

    painter.setPen(QPen(Qt::white, 2));
    QRectF arcRect(handle.x() - 4, handle.y() - 4, 8, 8);
    painter.drawArc(arcRect, 30 * 16, 300 * 16);

    float currentAngle = entry.rotation->z() * (180.0 / M_PI);
    QString angleText = QString("%1°").arg(currentAngle, 0, 'f', 1);
    painter.setFont(QFont("Arial", 9, QFont::Bold));
    painter.setPen(Qt::blue);
    painter.drawText(handle + QPointF(15, 5), angleText);
    painter.restore();
}

// ============================================================================
// Resize Handles
// ============================================================================

void ShapeEventHandler::updateResizeHandlesForRotation(const QString& shapeId) {
    if (!m_host || !m_host->getGISlib()) return;

    for (auto& entry : m_tempMeshes) {
        if (entry.name == shapeId) {
            if (!entry.position || !entry.size) return;

            QPointF centerGeo(entry.position->x(), entry.position->y());
            float rotationRad = entry.rotation->z();
            m_resizeHandles.clear();

            if (entry.name.startsWith("TempRectangle") || !entry.bitmapPath.isEmpty()) {
                float halfW = entry.size->x() / 2.0f;
                float halfH = entry.size->y() / 2.0f;
                QVector<QPointF> localCorners = {
                    QPointF(-halfW, halfH),
                    QPointF(halfW, halfH),
                    QPointF(halfW, -halfH),
                    QPointF(-halfW, -halfH)
                };
                float cosFwd = std::cos(rotationRad);
                float sinFwd = std::sin(rotationRad);
                for (const QPointF& corner : localCorners) {
                    float worldX = corner.x() * cosFwd - corner.y() * sinFwd;
                    float worldY = corner.x() * sinFwd + corner.y() * cosFwd;
                    QPointF cornerGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);
                    m_resizeHandles.push_back(m_host->getGISlib()->geoToCanvas(cornerGeo.y(), cornerGeo.x()));
                }
            } else if (entry.name.startsWith("TempPolygon") || entry.name.startsWith("TempPolyline")) {
                if (!entry.mesh || entry.mesh->polygen.empty()) return;
                float cosFwd = std::cos(rotationRad);
                float sinFwd = std::sin(rotationRad);
                for (Vector* v : entry.mesh->polygen) {
                    float worldX = v->x * cosFwd - v->y * sinFwd;
                    float worldY = v->x * sinFwd + v->y * cosFwd;
                    QPointF vGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);
                    m_resizeHandles.push_back(m_host->getGISlib()->geoToCanvas(vGeo.y(), vGeo.x()));
                }
            } else if (entry.name.startsWith("TempCircle")) {
                QPointF radiusPointGeo(entry.position->x() + entry.size->x(), entry.position->y());
                m_resizeHandles = { m_host->getGISlib()->geoToCanvas(radiusPointGeo.y(), radiusPointGeo.x()) };
            }
            break;
        }
    }
}

bool ShapeEventHandler::handleResizeHandleSelection(QMouseEvent *event)
{
    if (!shouldDrawShape(m_editingShapeId))
        return false;

    const qreal handleTolerance = 10.0f;

    m_selectedHandleIndex = -1;

    for (size_t i = 0; i < m_resizeHandles.size(); ++i)
    {
        if (QVector2D(event->pos() - m_resizeHandles[i]).length() < handleTolerance)
        {
            m_selectedHandleIndex = i;
            m_isResizingShape = true;
            m_host->refreshView();

            return true;      // Handle mila
        }
    }

    m_isResizingShape=false;
    return false;           // Handle nahi mila
}
// ============================================================================
// Resize Handlers
// ============================================================================

void ShapeEventHandler::handlePolygonVertexEditing(QMouseEvent *event, MeshEntry& entry) {
    if (!m_host || !m_host->getGISlib() || !entry.mesh) return;

    if (m_selectedHandleIndex < static_cast<int>(entry.mesh->polygen.size())) {
        Vector* vertex = entry.mesh->polygen[m_selectedHandleIndex];
        if (!vertex) return;

        QPointF centerGeo(entry.position->x(), entry.position->y());
        QPointF mouseGeo = m_host->getGISlib()->canvasToGeo(event->pos());
        float rotationRad = entry.rotation->z();

        float deltaX = mouseGeo.x() - centerGeo.x();
        float deltaY = mouseGeo.y() - centerGeo.y();

        float cosInv = std::cos(-rotationRad);
        float sinInv = std::sin(-rotationRad);

        float localX = deltaX * cosInv - deltaY * sinInv;
        float localY = deltaX * sinInv + deltaY * cosInv;

        vertex->x = localX;
        vertex->y = localY;

        updateResizeHandlesForRotation(entry.name);
    }
}

void ShapeEventHandler::handleRectangleResizing(QMouseEvent *event, MeshEntry& entry) {
    if (!m_host || !m_host->getGISlib() || !entry.position || !entry.size || !entry.mesh) return;

    QPointF newPos = event->pos();
    QPointF centerGeo(entry.position->x(), entry.position->y());
    QPointF centerCanvas = m_host->getGISlib()->geoToCanvas(centerGeo.y(), centerGeo.x());
    float rotationRad = entry.rotation->z();

    QPointF mouseDelta = newPos - centerCanvas;
    float cosInv = std::cos(-rotationRad);
    float sinInv = std::sin(-rotationRad);
    float localX = mouseDelta.x() * cosInv - mouseDelta.y() * sinInv;
    float localY = mouseDelta.x() * sinInv + mouseDelta.y() * cosInv;

    float currentHalfW = entry.size->x() / 2.0f;
    float currentHalfH = entry.size->y() / 2.0f;

    QPointF refPointGeo(centerGeo.x() + currentHalfW, centerGeo.y() + currentHalfH);
    QPointF refPointCanvas = m_host->getGISlib()->geoToCanvas(refPointGeo.y(), refPointGeo.x());
    float canvasScaleX = std::abs(refPointCanvas.x() - centerCanvas.x()) / currentHalfW;
    float canvasScaleY = std::abs(refPointCanvas.y() - centerCanvas.y()) / currentHalfH;

    float newHalfW = std::abs(localX) / canvasScaleX;
    float newHalfH = std::abs(localY) / canvasScaleY;

    const float minGeoSize = 0.0001f;
    newHalfW = std::max(newHalfW, minGeoSize);
    newHalfH = std::max(newHalfH, minGeoSize);

    entry.size->setX(newHalfW * 2.0f);
    entry.size->setY(newHalfH * 2.0f);

    for (Vector* v : entry.mesh->polygen) delete v;
    entry.mesh->polygen.clear();
    entry.mesh->polygen = {
        new Vector(-newHalfW, newHalfH, 0),
        new Vector(newHalfW, newHalfH, 0),
        new Vector(newHalfW, -newHalfH, 0),
        new Vector(-newHalfW, -newHalfH, 0)
    };

    updateResizeHandlesForRotation(entry.name);
}

void ShapeEventHandler::handleCircleResizing(QMouseEvent *event, MeshEntry& entry) {
    if (!m_host || !m_host->getGISlib() || !entry.position || !entry.size) return;

    QPointF newPos = event->pos();
    QPointF centerGeo(entry.position->x(), entry.position->y());
    QPointF centerCanvas = m_host->getGISlib()->geoToCanvas(centerGeo.y(), centerGeo.x());

    float newCanvasRadius = QVector2D(newPos - centerCanvas).length();
    newCanvasRadius = std::max(10.0f, newCanvasRadius);
    QPointF radiusPointCanvas = centerCanvas + QPointF(newCanvasRadius, 0);
    QPointF radiusPointGeo = m_host->getGISlib()->canvasToGeo(radiusPointCanvas);
    QPointF centerGeoCalc = m_host->getGISlib()->canvasToGeo(centerCanvas);
    float newGeoRadius = qAbs(radiusPointGeo.x() - centerGeoCalc.x());
    entry.size->setX(newGeoRadius);
    entry.size->setY(newGeoRadius);
    m_resizeHandles = { radiusPointCanvas };
}

void ShapeEventHandler::handleBitmapResizing(QMouseEvent *event, MeshEntry& entry) {
    if (!m_host || !m_host->getGISlib() || !entry.position || !entry.size) return;

    QPointF newPos = event->pos();
    QPointF centerGeo(entry.position->x(), entry.position->y());
    QPointF centerCanvas = m_host->getGISlib()->geoToCanvas(centerGeo.y(), centerGeo.x());
    float rotationRad = entry.rotation->z();

    QPointF mouseDelta = newPos - centerCanvas;
    float cosInv = std::cos(-rotationRad);
    float sinInv = std::sin(-rotationRad);
    float localX = mouseDelta.x() * cosInv - mouseDelta.y() * sinInv;
    float localY = mouseDelta.x() * sinInv + mouseDelta.y() * cosInv;

    float currentHalfW = entry.size->x() / 2.0f;
    float currentHalfH = entry.size->y() / 2.0f;

    QPointF refPointGeo(centerGeo.x() + currentHalfW, centerGeo.y() + currentHalfH);
    QPointF refPointCanvas = m_host->getGISlib()->geoToCanvas(refPointGeo.y(), refPointGeo.x());
    float canvasScaleX = std::abs(refPointCanvas.x() - centerCanvas.x()) / currentHalfW;
    float canvasScaleY = std::abs(refPointCanvas.y() - centerCanvas.y()) / currentHalfH;

    float newHalfW = std::abs(localX) / canvasScaleX;
    float newHalfH = std::abs(localY) / canvasScaleY;

    const float minGeoSize = 0.0001f;
    newHalfW = std::max(newHalfW, minGeoSize);
    newHalfH = std::max(newHalfH, minGeoSize);

    entry.size->setX(newHalfW * 2.0f);
    entry.size->setY(newHalfH * 2.0f);

    updateResizeHandlesForRotation(entry.name);
}

// ============================================================================
// Text Handling
// ============================================================================
void ShapeEventHandler::addImportedText(
    const QString& layerName,
    const QString& featureName,
    const QVector3D& position,
    const QString& text)
{
    MeshEntry entry;

    // =====================================================================
    // CHAITANYA JAIN CHANGE:
    // FIX: Imported Text must have a globally unique INTERNAL ID.
    //
    // Problem:
    // Layer 1 -> Text_0
    // Layer 2 -> Text_0
    //
    // If entry.name is the same, selection / dragging / visibility lookup
    // can affect the wrong Text from another imported layer.
    //
    // Solution:
    // displayName = original user-visible/imported feature name
    // entry.name  = globally unique internal ID
    // =====================================================================

    QString displayName = featureName.trimmed();

    if (displayName.isEmpty())
    {
        displayName = QString("Text_%1").arg(m_textCounter++);
    }

    // ---------------------------------------------------------------------
    // Generate unique internal ID.
    //
    // Example:
    // Layer 1, Text_0 -> Text_0_import_0
    // Layer 2, Text_0 -> Text_0_import_1
    //
    // Even if both layers contain exactly the same feature name,
    // entry.name will never collide.
    // ---------------------------------------------------------------------
    QString safeBaseName = displayName;

    // Avoid problematic separators in internal IDs.
    safeBaseName.replace(" ", "_");

    QString uniqueName;
    int uniqueIndex = 0;

    do
    {
        uniqueName =
            QString("%1_import_%2")
                .arg(safeBaseName)
                .arg(uniqueIndex++);

        bool alreadyExists = false;

        for (const auto& existingEntry : m_tempMeshes)
        {
            if (existingEntry.name == uniqueName)
            {
                alreadyExists = true;
                break;
            }
        }

        if (!alreadyExists)
            break;

    } while (true);

    // Internal unique identifier
    entry.name = uniqueName;

    // Internal object type
    entry.shapeType = "Text";


    // =====================================================================
    // Position / Transform
    // =====================================================================

    entry.position =
        new QVector3D(position);

    entry.rotation =
        new QQuaternion();

    entry.size =
        new QVector3D(
            1.0f,
            1.0f,
            1.0f
            );

    entry.velocity =
        new QVector3D(
            0,
            0,
            0
            );

    entry.trajectory = nullptr;
    entry.collider = nullptr;


    // =====================================================================
    // Text Data
    // =====================================================================

    entry.bitmapPath = "";

    entry.text = text;

    entry.textColor =
        Qt::black;

    entry.textFont =
        QFont(
            "Arial",
            12
            );

    entry.textSize =
        12;

    entry.isTextSelected =
        false;


    // =====================================================================
    // Mesh
    // =====================================================================

    entry.mesh =
        new Mesh();

    if (!entry.mesh)
    {
        m_host->logError(
            "Failed to allocate Mesh for imported text"
            );

        // Clean allocated members before returning.
        delete entry.position;
        delete entry.rotation;
        delete entry.size;
        delete entry.velocity;

        return;
    }

    entry.mesh->color =
        new QColor(
            entry.textColor
            );

    entry.mesh->lineWidth =
        1;

    entry.mesh->closePath =
        false;


    // =====================================================================
    // Add to Canvas
    // =====================================================================

    m_tempMeshes.push_back(
        entry
        );


    // =====================================================================
    // Add to Layer Panel
    //
    // IMPORTANT:
    //
    // First argument  = UNIQUE INTERNAL ID
    // Second argument = USER-VISIBLE DISPLAY NAME
    // Third argument  = LAYER NAME
    //
    // Therefore:
    //
    // Canvas internally:
    //     Text_0_import_0
    //
    // Layer Panel displays:
    //     Text_0
    //
    // This prevents duplicate IDs between imported layers while keeping
    // clean names visible to the user.
    // =====================================================================

    if (m_layerPanel)
    {
        m_layerPanel->addShapeToLayer(
            entry.name,      // Unique internal ID
            displayName,     // Original visible name
            layerName
            );
    }


    // =====================================================================
    // Debug
    // =====================================================================

    qDebug()
        << "Imported Text Registered:"
        << "Internal ID =" << entry.name
        << "Display Name =" << displayName
        << "Layer =" << layerName
        << "Type =" << entry.shapeType
        << "Text =" << entry.text
        << "Position =" << *entry.position;


    // =====================================================================
    // Refresh
    // =====================================================================

    m_host->refreshView();
}
void ShapeEventHandler::addTextAtGeo(
    const QString& text,
    const QPointF& geoPos)
{
    // =====================================================================
    // Check if a layer exists
    // =====================================================================

    if (m_layerPanel &&
        m_layerPanel->layerItems.isEmpty() &&
        m_layerPanel->rasterLayers.isEmpty())
    {
        QWidget* parentWidget =
            dynamic_cast<QWidget*>(m_host);

        if (!parentWidget &&
            m_layerPanel)
        {
            parentWidget =
                dynamic_cast<QWidget*>(
                    m_layerPanel->parent()
                    );
        }

        QMessageBox msgBox(
            parentWidget
            );

        msgBox.setStyleSheet(
            LayerPanelStyles::MessageBox
            );

        msgBox.setWindowTitle(
            "No Layer Found"
            );

        msgBox.setText(
            "No layers exist!\n\n"
            "Please create a layer first to add text."
            );

        msgBox.setIcon(
            QMessageBox::Warning
            );

        QPushButton* createBtn =
            msgBox.addButton(
                "Create New Layer",
                QMessageBox::AcceptRole
                );

        msgBox.addButton(
            "Cancel",
            QMessageBox::RejectRole
            );

        msgBox.exec();

        if (msgBox.clickedButton() ==
            createBtn)
        {
            m_layerPanel->addLayer();

            if (m_layerPanel->layerItems.isEmpty() &&
                m_layerPanel->rasterLayers.isEmpty())
            {
                return;
            }
        }
        else
        {
            return;
        }
    }

    // =====================================================================
    // Create Text MeshEntry
    // =====================================================================

    MeshEntry entry;

    // No TempText prefix.
    // Name and type are separate.

    entry.name =
        QString("Text_%1")
            .arg(m_textCounter++);

    entry.shapeType =
        "Text";

    // =====================================================================
    // Position / Transform
    // =====================================================================

    entry.position =
        new QVector3D(
            geoPos.x(),
            geoPos.y(),
            0
            );

    entry.rotation =
        new QQuaternion();

    entry.size =
        new QVector3D(
            1.0f,
            1.0f,
            1.0f
            );

    entry.velocity =
        new QVector3D(
            0,
            0,
            0
            );

    entry.trajectory =
        nullptr;

    entry.collider =
        nullptr;

    // =====================================================================
    // Text Data
    // =====================================================================

    entry.bitmapPath =
        "";

    entry.text =
        text;

    entry.textColor =
        Qt::black;

    entry.textFont =
        QFont(
            "Arial",
            12
            );

    entry.textSize =
        12;

    entry.isTextSelected =
        false;

    // =====================================================================
    // Create Mesh
    // =====================================================================

    entry.mesh =
        new Mesh();

    if (!entry.mesh)
    {
        m_host->logError(
            "Failed to allocate Mesh for text"
            );

        return;
    }

    entry.mesh->color =
        new QColor(
            entry.textColor
            );

    entry.mesh->lineWidth =
        1;

    entry.mesh->closePath =
        false;

    // =====================================================================
    // Add Text to Canvas
    // =====================================================================

    m_tempMeshes.push_back(
        entry
        );

    // =====================================================================
    // Add Text to Layer Panel
    // =====================================================================

    if (m_layerPanel)
    {
        m_layerPanel->addShapeToLayer(
            entry.name,
            entry.name
            );
    }

    // =====================================================================
    // Refresh Canvas
    // =====================================================================

    m_host->refreshView();
}

void ShapeEventHandler::showTextPropertiesDialog(const QString& textId) {
    MeshEntry* textEntry = findMeshEntry(textId);
    if (!textEntry) return;

    QWidget* parentWidget = dynamic_cast<QWidget*>(m_host);
    if (!parentWidget && m_layerPanel) {
        parentWidget = dynamic_cast<QWidget*>(m_layerPanel->parent());
    }

    if (!m_textPropertiesDialog) {
        m_textPropertiesDialog = new QDialog(parentWidget);
        m_textPropertiesDialog->setWindowTitle("Text Properties");
        m_textPropertiesDialog->setFixedSize(300, 300);
        m_textPropertiesDialog->setStyleSheet(
            "QDialog { background-color: #2b2b2b; color: white; }"
            "QLabel { color: white; font-weight: bold; }"
            "QLineEdit { background-color: #3c3c3c; color: white; border: 1px solid #555; padding: 5px; }"
            "QSpinBox { background-color: #3c3c3c; color: white; border: 1px solid #555; padding: 5px; }"
            "QPushButton { background-color: #404040; color: white; border: 1px solid #555; padding: 8px; font-weight: bold; }"
            "QPushButton:hover { background-color: #505050; }"
            "QPushButton:pressed { background-color: #606060; }"
            );
        m_textPropertiesDialog->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(m_textPropertiesDialog, &QDialog::finished, [this]() {
            m_textPropertiesDialog = nullptr;
            stopTextDragging();
        });
    }

    if (m_textPropertiesDialog->isVisible()) {
        m_textPropertiesDialog->raise();
        m_textPropertiesDialog->activateWindow();
        return;
    }

    if (m_textPropertiesDialog->layout()) {
        QLayoutItem* item;
        while ((item = m_textPropertiesDialog->layout()->takeAt(0)) != nullptr) {
            if (item->widget()) delete item->widget();
            delete item;
        }
        delete m_textPropertiesDialog->layout();
    }

    QVBoxLayout* layout = new QVBoxLayout(m_textPropertiesDialog);

    QLabel* contentLabel = new QLabel("Text Content:", m_textPropertiesDialog);
    QLineEdit* contentEdit = new QLineEdit(textEntry->text, m_textPropertiesDialog);

    QLabel* colorLabel = new QLabel("Text Color:", m_textPropertiesDialog);
    QPushButton* colorButton = new QPushButton(m_textPropertiesDialog);
    colorButton->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold; border: 2px solid white;").arg(textEntry->textColor.name()));
    colorButton->setText("Select Color");
    QObject::connect(colorButton, &QPushButton::clicked, [=]() {
        QColor newColor = QColorDialog::getColor(textEntry->textColor, m_textPropertiesDialog,
                                                 "Select Text Color", QColorDialog::DontUseNativeDialog);
        if (newColor.isValid()) {
            textEntry->textColor = newColor;
            colorButton->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold; border: 2px solid white;").arg(newColor.name()));
            m_host->refreshView();
        }
    });

    QLabel* sizeLabel = new QLabel("Font Size:", m_textPropertiesDialog);
    QSpinBox* sizeSpin = new QSpinBox(m_textPropertiesDialog);
    sizeSpin->setRange(8, 72);
    sizeSpin->setValue(textEntry->textSize);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* applyButton = new QPushButton("Apply", m_textPropertiesDialog);
    QPushButton* deleteButton = new QPushButton("Delete Text", m_textPropertiesDialog);
    QPushButton* closeButton = new QPushButton("Close", m_textPropertiesDialog);
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addWidget(closeButton);

    layout->addWidget(contentLabel);
    layout->addWidget(contentEdit);
    layout->addWidget(colorLabel);
    layout->addWidget(colorButton);
    layout->addWidget(sizeLabel);
    layout->addWidget(sizeSpin);
    layout->addStretch();
    layout->addLayout(buttonLayout);

    QObject::connect(applyButton, &QPushButton::clicked, [=]() {
        textEntry->text = contentEdit->text();
        textEntry->textSize = sizeSpin->value();
        textEntry->textFont.setPointSize(textEntry->textSize);
        if (textEntry->mesh && textEntry->mesh->color) {
            *textEntry->mesh->color = textEntry->textColor;
        }
        m_host->refreshView();
        if (m_textPropertiesDialog) {
            m_textPropertiesDialog->close();
        }
    });

    QObject::connect(deleteButton, &QPushButton::clicked, [=]() {
        QMessageBox::StandardButton reply = QMessageBox::question(m_textPropertiesDialog, "Confirm Delete",
                                                                  "Are you sure you want to delete this text?",
                                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            deleteText(textId);
            if (m_textPropertiesDialog) {
                m_textPropertiesDialog->close();
            }
        }
    });

    QObject::connect(closeButton, &QPushButton::clicked, m_textPropertiesDialog, &QDialog::close);
    m_textPropertiesDialog->show();
    m_textPropertiesDialog->raise();
    m_textPropertiesDialog->activateWindow();
}

// CHAITANYA JAIN CHANGE:
// What was fixed: Delete Confirmation UX
// Why the bug happened: Pressing the Delete key on the keyboard or clicking Delete in the context menu erased shapes instantly without any safety prompt, causing accidental data loss.
// Why this change is safe: Injected QMessageBox::question before deletion memory logic, strictly executing only if the user presses "Yes".
void ShapeEventHandler::deleteText(const QString& textId) {
    QWidget* parentWidget = dynamic_cast<QWidget*>(m_host);
    if (!parentWidget && m_layerPanel) {
        parentWidget = dynamic_cast<QWidget*>(m_layerPanel->parent());
    }

    QMessageBox msgBox(parentWidget);
    msgBox.setWindowTitle("Confirm Deletion");
    msgBox.setText(QString("Are you sure you want to delete this text?"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() != QMessageBox::Yes) {
        return;
    }

    for (auto it = m_tempMeshes.begin(); it != m_tempMeshes.end(); ++it) {
        if (it->name == textId) {
            // CHAITANYA JAIN CHANGE:
            // Why: MeshEntry destructor automatically deletes position, rotation, size, velocity.
            // Only delete unowned fields like mesh and collider here to prevent double-free crash.
            if (it->mesh) {
                delete it->mesh->color;
                delete it->mesh;
            }
            delete it->collider;
            delete it->trajectory;
            m_tempMeshes.erase(it);
            if (m_layerPanel) {
                m_layerPanel->removeShapeFromLayer(textId);
            }
            m_host->refreshView();
            break;
        }
    }
}

bool ShapeEventHandler::handleTextSelection(QMouseEvent *event)
{
    if (!m_host || !m_host->getGISlib())
        return false;

    // Only handle left mouse button press
    if (event->type() != QEvent::MouseButtonPress ||
        event->button() != Qt::LeftButton) {
        return false;
    }

    // =====================================================================
    // 1. Check if any text item was clicked
    // =====================================================================
    for (auto& entry : m_tempMeshes) {

        // CHAITANYA JAIN CHANGE:
        // Prevent interacting with text belonging to hidden layers
        if (!shouldDrawShape(entry.name))
            continue;

        if (entry.name.startsWith("Text") &&
            !entry.text.isEmpty()) {

            // Convert text geo position to canvas position
            QPointF textPos =
                m_host->getGISlib()->geoToCanvas(
                    entry.position->y(),
                    entry.position->x()
                    );

            // Calculate text bounding rectangle
            QFontMetrics fm(entry.textFont);
            QRect textRect = fm.boundingRect(entry.text);

            textRect.moveTo(
                textPos.x(),
                textPos.y() - fm.ascent()
                );

            // Slightly increase clickable area
            QRectF expandedRect =
                textRect.adjusted(-5, -5, 5, 5);

            // =================================================================
            // TEXT CLICKED ON CANVAS
            // =================================================================
            if (expandedRect.contains(event->pos())) {

                // -------------------------------------------------------------
                // Deselect all previously selected text items
                // -------------------------------------------------------------
                for (auto& otherEntry : m_tempMeshes) {
                    if (otherEntry.name.startsWith("Text")) {
                        otherEntry.isTextSelected = false;
                    }
                }

                // -------------------------------------------------------------
                // Close any active shape/bitmap edit mode before editing text
                // -------------------------------------------------------------
                cancelActiveShapeOperation();

                // -------------------------------------------------------------
                // Select clicked text on canvas
                // -------------------------------------------------------------
                entry.isTextSelected = true;

                m_editingTextId = entry.name;
                m_isEditingText = true;

                m_shapeDragStartPos = event->pos();

                m_host->setCursor(Qt::SizeAllCursor);

                // =============================================================
                // CHAITANYA JAIN CHANGE:
                // FIX: Canvas Text -> Layer Panel selection synchronization
                //
                // When text is clicked directly on the canvas,
                // select/highlight the corresponding Text item in Layer Panel.
                // =============================================================
                if (m_layerPanel) {
                    m_layerPanel->selectShapeInPanel(entry.name);
                }

                // Refresh canvas to show selection
                m_host->refreshView();

                return true;
            }
        }
    }

    // =====================================================================
    // 2. Click was outside all text items
    //    Deselect currently selected text
    // =====================================================================
    for (auto& entry : m_tempMeshes) {
        if (entry.name.startsWith("TempText")) {
            entry.isTextSelected = false;
        }
    }

    m_editingTextId = "";
    m_isEditingText = false;

    m_host->refreshView();

    return false;
}

bool ShapeEventHandler::handleTextMousePress(QMouseEvent *event)
{
    if (!event ||
        !m_host ||
        event->button() != Qt::LeftButton ||
        m_host->getCurrentMode() == TransformMode::DrawShape)
    {
        return false;
    }

    // =====================================================================
    // CHAITANYA JAIN CHANGE:
    // FIX: Only one Text should remain selected during normal single click.
    //
    // IMPORTANT:
    // CanvasWidget handles genuine multi-selection/group drag BEFORE this
    // function is called. Therefore, if execution reaches here, this is an
    // individual text-selection operation.
    // =====================================================================

    // First let the existing hit-test determine whether a Text was clicked.
    bool textSelected = handleTextSelection(event);

    if (!textSelected)
    {
        return false;
    }

    // =====================================================================
    // Find the Text that handleTextSelection() has just selected.
    //
    // Normally m_editingTextId is set by handleTextSelection().
    // We preserve that selected Text while clearing all other Text selections.
    // =====================================================================
    QString clickedTextId = m_editingTextId;

    for (auto& entry : m_tempMeshes)
    {
        if (entry.text.isEmpty())
            continue;

        // Keep ONLY the currently clicked Text selected.
        if (!clickedTextId.isEmpty() &&
            entry.name == clickedTextId)
        {
            entry.isTextSelected = true;
        }
        else
        {
            entry.isTextSelected = false;
        }
    }

    // =====================================================================
    // Normal individual Text click must clear previous Shape selections.
    //
    // This prevents:
    // Circle selected -> click Text -> Circle still showing selection outline.
    //
    // Genuine multi-selection/group drag is safe because CanvasWidget's
    // multi-drag block returns BEFORE handleTextMousePress() is called.
    // =====================================================================
    m_selectedShapeIds.clear();

    // Keep highlighted ID synchronized with clicked Text.
    if (!clickedTextId.isEmpty())
    {
        m_highlightedShapeId = clickedTextId;
    }
    else
    {
        m_highlightedShapeId.clear();
    }

    m_host->refreshView();

    return true;
}
void ShapeEventHandler::handleTextMouseMove(QMouseEvent *event)
{
    if (m_isEditingText &&
        !m_editingTextId.isEmpty() &&
        (event->buttons() & Qt::LeftButton))
    {
        handleTextDragging(event);
    }
}

void ShapeEventHandler::handleTextMouseRelease(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (m_isEditingText && !m_editingTextId.isEmpty()) {
            stopTextDragging();
        }
    }
}

void ShapeEventHandler::handleTextDragging(QMouseEvent *event)
{
    if (!m_isEditingText ||
        m_editingTextId.isEmpty() ||
        !m_host ||
        !m_host->getGISlib())
        return;

    // Drag only while left mouse button is held
    if (!(event->buttons() & Qt::LeftButton))
        return;

    for (auto& entry : m_tempMeshes)
    {
        if (entry.name != m_editingTextId)
            continue;

        if (!entry.position)
            return;

        // Directly convert current mouse position to geo coordinates.
        // This prevents accumulated drift between text and cursor.
        QPointF newGeoPos =
            m_host->getGISlib()->canvasToGeo(event->pos());

        entry.position->setX(newGeoPos.x());
        entry.position->setY(newGeoPos.y());

        m_host->refreshView();
        return;
    }

    stopTextDragging();
}
bool ShapeEventHandler::handleTextRightClick(QMouseEvent *event) {
    if (!m_host || !m_host->getGISlib()) return false;
    for (auto& entry : m_tempMeshes) {
        // CHAITANYA JAIN CHANGE:
        // Why: Prevent interacting with text in hidden layers
        if (!shouldDrawShape(entry.name)) continue;

        if (entry.name.startsWith("Text") && !entry.text.isEmpty()) {
            QPointF textPos = m_host->getGISlib()->geoToCanvas(entry.position->y(), entry.position->x());
            QFontMetrics fm(entry.textFont);
            QRect textRect = fm.boundingRect(entry.text);
            textRect.moveTo(textPos.x(), textPos.y() - fm.ascent());
            QRectF expandedRect = textRect.adjusted(-5, -5, 5, 5);

            if (expandedRect.contains(event->pos())) {
                QWidget* parentWidget = dynamic_cast<QWidget*>(m_host);
                if (!parentWidget) return false;

                QMenu contextMenu(parentWidget);
                contextMenu.setStyleSheet(
                    "QMenu { background-color: white; color: black; border: 1px solid #cccccc; }"
                    "QMenu::item { background-color: white; color: black; padding: 5px 20px; }"
                    "QMenu::item:selected { background-color: #0078d7; color: white; }"
                    );

                QAction* editTextAction = contextMenu.addAction("Edit Text Properties");
                QObject::connect(editTextAction, &QAction::triggered, [=]() {
                    showTextPropertiesDialog(entry.name);
                });

                QAction* deleteAction = contextMenu.addAction("Delete Text");
                QObject::connect(deleteAction, &QAction::triggered, [=]() {
                    deleteText(entry.name);
                });

                contextMenu.exec(event->globalPos());
                return true;
            }
        }
    }
    return false;
}

void ShapeEventHandler::stopTextDragging() {
    if (m_isEditingText) {
        m_isEditingText = false;
        m_editingTextId = "";
        m_host->setCursor(Qt::ArrowCursor);
        m_host->refreshView();
    }
}

// ============================================================================
// Bitmap Handling
// ============================================================================
void ShapeEventHandler::addImportedBitmap(
    const QString& layerName,
    const QString& featureName,
    const QVector3D& position,
    const QString& bitmapTypeOrPath,
    const QVector3D& size,
    float rotation)
{
    // ============================================================
    // 1. Resolve bitmap resource path
    //
    // GeoJSON may provide:
    //   "Hospital"
    //   "School"
    //   "Forest Area"
    //
    // Or an existing Qt resource path:
    //   ":/icons/images/hospital.png"
    // ============================================================

    QString resolvedBitmapPath =
        getBitmapImagePath(bitmapTypeOrPath);

    // If it is not a known bitmap type, assume it may already
    // be a valid Qt resource/file path.
    if (resolvedBitmapPath.isEmpty()) {
        resolvedBitmapPath = bitmapTypeOrPath;
    }

    if (resolvedBitmapPath.isEmpty()) {
        qWarning() << "addImportedBitmap: Empty bitmap path/type";
        return;
    }


    // ============================================================
    // 2. Determine logical bitmap type
    // ============================================================

    QString bitmapType = bitmapTypeOrPath;

    // If resource path was passed instead of logical type,
    // recover the logical bitmap name.
    if (bitmapTypeOrPath == ":/icons/images/hospital.png") {
        bitmapType = "Hospital";
    }
    else if (bitmapTypeOrPath == ":/icons/images/school.png") {
        bitmapType = "School";
    }
    else if (bitmapTypeOrPath == ":/icons/images/forest-area.png") {
        bitmapType = "Forest Area";
    }


    // ============================================================
    // 3. Create MeshEntry
    // ============================================================

    MeshEntry entry;

    entry.name = QString("TempBitmap_import_%1_%2")
                     .arg(layerName)
                     .arg(m_bitmapCounter++);

    entry.position = new QVector3D(position);
    entry.rotation = new QQuaternion(
        QQuaternion::fromEulerAngles(
            0.0f,
            0.0f,
            rotation
            )
        );


    entry.size = new QVector3D(size);

    entry.velocity = new QVector3D(0, 0, 0);

    entry.trajectory = nullptr;
    entry.collider = nullptr;


    // ============================================================
    // 4. Store resolved resource path
    // ============================================================

    entry.bitmapPath = resolvedBitmapPath;

    entry.text = "";


    // ============================================================
    // 5. Create Mesh
    // ============================================================

    entry.mesh = new Mesh();

    entry.mesh->color = new QColor(Qt::white);
    entry.mesh->lineWidth = 1;
    entry.mesh->closePath = false;


    // ============================================================
    // 6. Add bitmap to ShapeEventHandler's mesh collection
    //
    // No CanvasWidget dependency.
    // ============================================================

    if (m_host)
    {
        m_host->addImportedMesh(entry);
    }
    else
    {
        qWarning()
        << "addImportedBitmap: ILayerHost is null";

        delete entry.position;
        delete entry.rotation;
        delete entry.size;
        delete entry.velocity;
        delete entry.mesh;

        return;
    }

    // ============================================================
    // 7. Register bitmap inside LayerPanel
    // ============================================================

    if (m_layerPanel) {

        QString displayName;

        // Prefer original GeoJSON feature name if available.
        if (!featureName.isEmpty()) {
            displayName = featureName;
        }
        else {
            displayName = bitmapType;
        }

        m_layerPanel->addShapeToLayer(
            entry.name,
            displayName,
            layerName
            );
    }


    // ============================================================
    // 8. Refresh canvas through ILayerHost
    //
    // No direct CanvasWidget dependency.
    // ============================================================

    if (m_host) {
        m_host->refreshView();
    }


    qDebug()
        << "Imported bitmap added:"
        << "Layer =" << layerName
        << "Feature =" << featureName
        << "Bitmap Type =" << bitmapType
        << "Bitmap Path =" << resolvedBitmapPath
        << "Position =" << position
        << "Size =" << size
        << "Rotation =" << rotation;
}


// ============================================================================
// Resolve predefined bitmap type to Qt Resource path
// ============================================================================

QString ShapeEventHandler::getBitmapImagePath(
    const QString& bitmapType) const
{
    static const QMap<QString, QString> bitmapMap = {
        {
            "Hospital",
            ":/icons/images/hospital.png"
        },
        {
            "School",
            ":/icons/images/school.png"
        },
        {
            "Forest Area",
            ":/icons/images/forest-area.png"
        }
    };

    // Exact match
    if (bitmapMap.contains(bitmapType))
    {
        return bitmapMap.value(bitmapType);
    }

    // Case-insensitive match
    for (auto it = bitmapMap.constBegin();
         it != bitmapMap.constEnd();
         ++it)
    {
        if (it.key().compare(
                bitmapType,
                Qt::CaseInsensitive) == 0)
        {
            return it.value();
        }
    }

    return QString();
}

void ShapeEventHandler::placeBitmapAtGeo(const QPointF& geoPos)
{
    // Basic safety checks
    if (!m_host || !m_host->getGISlib()) {
        return;
    }

    // No bitmap type selected
    if (m_selectedBitmapType.isEmpty()) {
        m_host->logError("No bitmap type selected.");
        return;
    }

    // Check if a layer exists
    if (m_layerPanel &&
        m_layerPanel->layerItems.isEmpty() &&
        m_layerPanel->rasterLayers.isEmpty()) {

        QWidget* parentWidget = dynamic_cast<QWidget*>(m_host);

        if (!parentWidget) {
            parentWidget = dynamic_cast<QWidget*>(m_layerPanel->parent());
        }

        QMessageBox msgBox(parentWidget);
        msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
        msgBox.setWindowTitle("No Layer Found");
        msgBox.setText(
            "No layers exist!\n\n"
            "Please create a layer first to add bitmaps."
            );
        msgBox.setIcon(QMessageBox::Warning);

        QPushButton* createBtn =
            msgBox.addButton(
                "Create New Layer",
                QMessageBox::AcceptRole
                );

        msgBox.addButton(
            "Cancel",
            QMessageBox::RejectRole
            );

        msgBox.exec();

        // User cancelled
        if (msgBox.clickedButton() != createBtn) {
            return;
        }

        // Create layer
        m_layerPanel->addLayer();

        // Make sure layer was actually created
        if (m_layerPanel->layerItems.isEmpty() &&
            m_layerPanel->rasterLayers.isEmpty()) {

            m_host->logError(
                "Failed to create layer for bitmap."
                );
            return;
        }
    }

    // Get bitmap path from selected bitmap type
    QString bitmapPath =
        getBitmapImagePath(m_selectedBitmapType);

    if (bitmapPath.isEmpty()) {
        m_host->logError(
            "No bitmap path found for type: " +
            m_selectedBitmapType
            );
        return;
    }

    // Create bitmap MeshEntry
    MeshEntry entry;

    entry.name =
        QString("TempBitmap_%1")
            .arg(m_bitmapCounter++);

    // geoPos:
    // x = Longitude
    // y = Latitude
    entry.position =
        new QVector3D(
            geoPos.x(),
            geoPos.y(),
            0
            );

    entry.rotation = new QQuaternion();

    // Keep same default size as manual bitmap placement
    entry.size =
        new QVector3D(
            0.2,
            0.2,
            1
            );

    entry.velocity =
        new QVector3D(
            0,
            0,
            0
            );

    entry.trajectory = nullptr;
    entry.collider = nullptr;

    // Actual bitmap image path
    entry.bitmapPath = bitmapPath;

    entry.text = "";

    // Create mesh
    entry.mesh = new Mesh();

    if (!entry.mesh) {
        m_host->logError(
            "Failed to create Mesh for bitmap."
            );
        return;
    }

    entry.mesh->color =
        new QColor(Qt::white);

    entry.mesh->lineWidth = 1;
    entry.mesh->closePath = false;

    // Add bitmap to temporary meshes
    m_tempMeshes.push_back(entry);

    // Add bitmap to Layer Panel
    if (m_layerPanel) {
        m_layerPanel->addShapeToLayer(
            entry.name,
            "Bitmap"
            );
    }

    // Refresh canvas so bitmap becomes visible
    m_host->refreshView();
}
void ShapeEventHandler::onBitmapImageSelected(const QString& filePath) {
    if (m_layerPanel &&
        m_layerPanel->layerItems.isEmpty() &&
        m_layerPanel->rasterLayers.isEmpty()) {

        QWidget* parentWidget = dynamic_cast<QWidget*>(m_host);
        if (!parentWidget && m_layerPanel) {
            parentWidget = dynamic_cast<QWidget*>(m_layerPanel->parent());
        }

        QMessageBox msgBox(parentWidget);
        msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
        msgBox.setWindowTitle("No Layer Found");
        msgBox.setText("No layers exist!\n\nPlease create a layer first to add user images.");
        msgBox.setIcon(QMessageBox::Warning);

        QPushButton* createBtn = msgBox.addButton("Create New Layer", QMessageBox::AcceptRole);
        msgBox.addButton("Cancel", QMessageBox::RejectRole);
        msgBox.exec();

        if (msgBox.clickedButton() == createBtn) {
            m_layerPanel->addLayer();
            if (m_layerPanel->layerItems.isEmpty() && m_layerPanel->rasterLayers.isEmpty()) {
                return;
            }
        } else {
            return;
        }
    }

    MeshEntry entry;
    entry.name = QString("UserImage_%1").arg(m_userImageCounter++);

    QWidget* parentWidget = dynamic_cast<QWidget*>(m_host);
    QPointF centerCanvas(parentWidget ? parentWidget->width() / 2.0f : 400.0f,
                         parentWidget ? parentWidget->height() / 2.0f : 300.0f);
    QPointF geoPos = m_host->getGISlib()->canvasToGeo(centerCanvas);

    entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
    entry.rotation = new QQuaternion();
    entry.size = new QVector3D(0.2, 0.2, 1);
    entry.velocity = new QVector3D(0, 0, 0);
    entry.trajectory = nullptr;
    entry.collider = nullptr;
    entry.bitmapPath = filePath;
    entry.mesh = new Mesh();
    if (!entry.mesh) return;
    entry.mesh->color = new QColor(Qt::white);
    entry.mesh->lineWidth = 1;
    entry.mesh->closePath = false;
    m_tempMeshes.push_back(entry);

    if (m_layerPanel) {
        m_layerPanel->addShapeToLayer(entry.name, "Image");
    }

    m_host->refreshView();
}

void ShapeEventHandler::onBitmapSelected(const QString& bitmapType) {
    m_selectedBitmapType = bitmapType;
    m_isPlacingBitmap = true;
    m_host->setCurrentMode(TransformMode::PlaceBitmap);
    m_host->setCursor(Qt::CrossCursor);
    m_host->refreshView();
}

void ShapeEventHandler::onBitmapImageSelectedAtGeo(const QString& path, const QPointF& geo) {
    onBitmapImageSelected(path);
    placeBitmapAtGeo(geo);
    m_isPlacingBitmap = false;
    m_host->setCurrentMode(TransformMode::PlaceBitmap);
    m_host->refreshView();
}

void ShapeEventHandler::onBitmapSelectedAtGeo(const QString& bitmapType, const QPointF& geo) {
    m_selectedBitmapType = bitmapType;
    if (!geo.isNull()) {
        placeBitmapAtGeo(geo);
        m_isPlacingBitmap = false;
        m_host->setCurrentMode(TransformMode::Translate);
        m_host->setCursor(Qt::ArrowCursor);
        m_host->refreshView();
        return;
    }
    m_isPlacingBitmap = true;
    m_host->setCurrentMode(TransformMode::PlaceBitmap);
    m_host->setCursor(Qt::CrossCursor);
    m_host->refreshView();
}

bool ShapeEventHandler::handleBitmapSelection(QMouseEvent *event) {
    if (!m_host || !m_host->getGISlib()) return false;

    for (auto& entry : m_tempMeshes) {
        if (entry.bitmapPath.isEmpty()) continue;

        QPolygonF rotatedPoly = getRotatedShapePolygon(entry);
        if (rotatedPoly.isEmpty()) continue;

        QPointF center(0, 0);
        for (const QPointF& p : rotatedPoly) center += p;
        center /= rotatedPoly.size();

        QPolygonF scaledPoly;
        for (const QPointF& p : rotatedPoly) {
            QPointF offset = p - center;
            scaledPoly << (center + offset * 1.0f);
        }

        if (scaledPoly.containsPoint(event->pos(), Qt::OddEvenFill)) {
            m_isDraggingBitmap = true;
            m_draggingBitmapId = entry.name;
            m_bitmapDragStartPos = event->pos();
            m_host->setCursor(Qt::ClosedHandCursor);
            m_isDraggingUserImage = false;
            m_isDraggingShape = false;
            m_draggingShapeId = "";
            m_highlightedShapeId = entry.name;
            if (m_layerPanel) {
                m_layerPanel->selectShapeInPanel(entry.name);
            }
            m_host->refreshView();
            return true;
        }
    }
    return false;
}

void ShapeEventHandler::handleBitmapDragging(QMouseEvent *event) {
    if (!m_isDraggingBitmap || m_draggingBitmapId.isEmpty() || !m_host || !m_host->getGISlib()) return;

    for (auto& entry : m_tempMeshes) {
        if (entry.name == m_draggingBitmapId && !entry.bitmapPath.isEmpty()) {
            QPointF currentGeoPos(entry.position->x(), entry.position->y());
            QPointF currentCanvasPos = m_host->getGISlib()->geoToCanvas(currentGeoPos.y(), currentGeoPos.x());
            QPointF canvasDelta = event->pos() - m_bitmapDragStartPos;
            QPointF newCanvasPos = currentCanvasPos + canvasDelta;
            QPointF newGeoPos = m_host->getGISlib()->canvasToGeo(newCanvasPos);

            entry.position->setX(newGeoPos.x());
            entry.position->setY(newGeoPos.y());
            m_bitmapDragStartPos = event->pos();
            m_host->refreshView();
            return;
        }
    }
    stopBitmapDragging();
}

void ShapeEventHandler::stopBitmapDragging() {
    if (m_isDraggingBitmap) {
        m_isDraggingBitmap = false;
        m_draggingBitmapId = "";
        m_highlightedShapeId = "";
        m_host->setCursor(Qt::ArrowCursor);
        m_host->refreshView();
    }
}

bool ShapeEventHandler::handleUserImageSelection(QMouseEvent *event) {
    if (!m_host || !m_host->getGISlib()) return false;

    for (auto& entry : m_tempMeshes) {
        if (entry.name.startsWith("UserImage_") && !entry.bitmapPath.isEmpty()) {
            QPointF imageCenterCanvas = m_host->getGISlib()->geoToCanvas(entry.position->y(), entry.position->x());
            QPointF sizePointGeo(entry.position->x() + entry.size->x(), entry.position->y() + entry.size->y());
            QPointF sizePointCanvas = m_host->getGISlib()->geoToCanvas(sizePointGeo.y(), sizePointGeo.x());
            float canvasWidth = qAbs(sizePointCanvas.x() - imageCenterCanvas.x()) * 2;
            float canvasHeight = qAbs(sizePointCanvas.y() - imageCenterCanvas.y()) * 2;
            float selectionW = canvasWidth * 1.1f;
            float selectionH = canvasHeight * 1.1f;
            QRectF imageRect(imageCenterCanvas.x() - selectionW/2, imageCenterCanvas.y() - selectionH/2,
                             selectionW, selectionH);
            if (imageRect.contains(event->pos())) {
                m_isDraggingUserImage = true;
                m_draggingUserImageId = entry.name;
                m_userImageDragStartPos = event->pos();
                m_host->setCursor(Qt::ClosedHandCursor);
                m_isDraggingBitmap = false;
                m_host->refreshView();
                return true;
            }
        }
    }
    return false;
}

void ShapeEventHandler::handleUserImageDragging(QMouseEvent *event) {
    if (!m_isDraggingUserImage || m_draggingUserImageId.isEmpty() || !m_host || !m_host->getGISlib()) return;
    for (auto& entry : m_tempMeshes) {
        if (entry.name == m_draggingUserImageId && entry.name.startsWith("UserImage_")) {
            QPointF currentGeoPos(entry.position->x(), entry.position->y());
            QPointF currentCanvasPos = m_host->getGISlib()->geoToCanvas(currentGeoPos.y(), currentGeoPos.x());
            QPointF canvasDelta = event->pos() - m_userImageDragStartPos;
            QPointF newCanvasPos = currentCanvasPos + canvasDelta;
            QPointF newGeoPos = m_host->getGISlib()->canvasToGeo(newCanvasPos);

            entry.position->setX(newGeoPos.x());
            entry.position->setY(newGeoPos.y());
            m_userImageDragStartPos = event->pos();
            m_host->refreshView();
            return;
        }
    }
    stopUserImageDragging();
}

void ShapeEventHandler::stopUserImageDragging() {
    if (m_isDraggingUserImage) {
        m_isDraggingUserImage = false;
        m_draggingUserImageId = "";
        m_host->setCursor(Qt::ArrowCursor);
        m_host->refreshView();
    }
}

bool ShapeEventHandler::handleBitmapsMousePress(QMouseEvent *event)
{
    if (!m_host || !m_host->getGISlib())
        return false;

    // ============================================================
    // CHAITANYA JAIN CHANGE:
    // FIX: Do not consume bitmap/image body clicks while
    // EditShape mode is active.
    //
    // In EditShape mode, mousePressEvent() must get the event so
    // handleResizeHandleSelection() can detect blue resize handles.
    // ============================================================
    if (m_host->getCurrentMode() != TransformMode::EditShape) {

        if (handleUserImageSelection(event)) {
            return true;
        }

        if (handleBitmapSelection(event)) {
            return true;
        }
    }

    // ============================================================
    // ROTATION HANDLE LOGIC
    // ============================================================
    if (m_host->getCurrentMode() == TransformMode::Translate &&
        !m_activeRotateId.isEmpty()) {

        const qreal tolerance = 18.0;

        for (auto& entry : m_tempMeshes) {

            if (entry.name != m_activeRotateId)
                continue;

            bool isRotatable =
                !entry.bitmapPath.isEmpty() ||
                entry.name.startsWith("TempRectangle") ||
                entry.name.startsWith("TempCircle") ||
                entry.name.startsWith("TempPolygon") ||
                entry.name.startsWith("TempPolyline");

            if (!isRotatable)
                continue;

            if (!entry.position || !entry.size)
                continue;

            QPointF center =
                m_host->getGISlib()->geoToCanvas(
                    entry.position->y(),
                    entry.position->x()
                    );

            QRectF bbox;

            // ----------------------------------------------------
            // CIRCLE
            // ----------------------------------------------------
            if (entry.name.startsWith("TempCircle")) {

                QPointF radiusPointGeo(
                    entry.position->x() + entry.size->x(),
                    entry.position->y()
                    );

                QPointF radiusPointCanvas =
                    m_host->getGISlib()->geoToCanvas(
                        radiusPointGeo.y(),
                        radiusPointGeo.x()
                        );

                float canvasRadius =
                    QVector2D(radiusPointCanvas - center).length();

                bbox = QRectF(
                    center.x() - canvasRadius,
                    center.y() - canvasRadius,
                    canvasRadius * 2,
                    canvasRadius * 2
                    );
            }

            // ----------------------------------------------------
            // POLYGON / POLYLINE
            // ----------------------------------------------------
            else if (entry.name.startsWith("TempPolygon") ||
                     entry.name.startsWith("TempPolyline")) {

                QPolygonF poly;

                if (entry.mesh && !entry.mesh->polygen.empty()) {

                    float rotationRad = entry.rotation->z();
                    float cosFwd = std::cos(rotationRad);
                    float sinFwd = std::sin(rotationRad);

                    for (const Vector* v : entry.mesh->polygen) {

                        float worldX =
                            v->x * cosFwd -
                            v->y * sinFwd;

                        float worldY =
                            v->x * sinFwd +
                            v->y * cosFwd;

                        QPointF vGeo(
                            entry.position->x() + worldX,
                            entry.position->y() + worldY
                            );

                        poly <<
                            m_host->getGISlib()->geoToCanvas(
                                vGeo.y(),
                                vGeo.x()
                                );
                    }
                }

                if (poly.isEmpty())
                    continue;

                bbox = poly.boundingRect();
            }

            // ----------------------------------------------------
            // BITMAP / RECTANGLE / OTHER ROTATABLE OBJECTS
            // ----------------------------------------------------
            else {

                QPointF sizePoint =
                    m_host->getGISlib()->geoToCanvas(
                        entry.position->y() + entry.size->y(),
                        entry.position->x() + entry.size->x()
                        );

                float w =
                    qAbs(sizePoint.x() - center.x()) * 2;

                float h =
                    qAbs(sizePoint.y() - center.y()) * 2;

                bbox = QRectF(
                    center.x() - w / 2,
                    center.y() - h / 2,
                    w,
                    h
                    );
            }

            // ----------------------------------------------------
            // CHECK ROTATION HANDLE CLICK
            // ----------------------------------------------------
            QPointF handle =
                bbox.bottomRight() + QPointF(35, 35);

            if (QVector2D(event->pos() - handle).length()
                < tolerance) {

                m_isRotatingBitmap = true;
                m_rotatingBitmapId = entry.name;

                m_rotateHandleCenter = center;
                m_rotateHandleStartPos = event->pos();

                m_initialBitmapAngle =
                    std::atan2(
                        event->pos().y() - center.y(),
                        event->pos().x() - center.x()
                        );

                m_host->setCursor(Qt::PointingHandCursor);
                m_host->refreshView();

                return true;
            }
        }
    }

    // ============================================================
    // PLACE BITMAP
    //
    // IMPORTANT FIX:
    // Do NOT create MeshEntry here again.
    // placeBitmapAtGeo() is now the single place responsible for:
    //
    // 1. Checking/creating layer
    // 2. Resolving bitmap path
    // 3. Creating MeshEntry
    // 4. Adding it to m_tempMeshes
    // 5. Adding bitmap to LayerPanel
    // 6. Refreshing canvas
    // ============================================================
    if (m_host->getCurrentMode() == TransformMode::PlaceBitmap &&
        !m_selectedBitmapType.isEmpty()) {

        // Convert clicked canvas position to geographic coordinates
        QPointF geoPos =
            m_host->getGISlib()->canvasToGeo(event->pos());

        // Use common bitmap placement function
        placeBitmapAtGeo(geoPos);

        // Finish bitmap placement mode
        m_isPlacingBitmap = false;
        m_selectedBitmapType.clear();

        // Return to normal Translate mode
        m_host->setCurrentMode(TransformMode::Translate);
        m_host->setCursor(Qt::ArrowCursor);

        m_host->refreshView();

        return true;
    }

    return false;
}

void ShapeEventHandler::handleBitmapsMouseMove(QMouseEvent *event) {
    if (m_isRotatingBitmap && !m_rotatingBitmapId.isEmpty()) {
        for (auto& entry : m_tempMeshes) {
            if (entry.name == m_rotatingBitmapId) {
                qreal dx = event->pos().x() - m_rotateHandleCenter.x();
                qreal dy = event->pos().y() - m_rotateHandleCenter.y();
                qreal currentAngle = std::atan2(dy, dx);
                qreal deltaAngle = currentAngle - m_initialBitmapAngle;
                entry.rotation->setZ(entry.rotation->z() + deltaAngle);
                m_initialBitmapAngle = currentAngle;
                m_host->refreshView();
                return;
            }
        }
        m_isRotatingBitmap = false;
        m_rotatingBitmapId.clear();
        m_host->setCursor(Qt::ArrowCursor);
    }
    if (m_isDraggingUserImage && !m_draggingUserImageId.isEmpty()) {
        handleUserImageDragging(event);
        return;
    }
    if (m_isDraggingBitmap && !m_draggingBitmapId.isEmpty()) {
        handleBitmapDragging(event);
        return;
    }
}

void ShapeEventHandler::handleBitmapsMouseRelease(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (m_isRotatingBitmap) {
            m_isRotatingBitmap = false;
            m_rotatingBitmapId.clear();
            m_activeRotateId.clear();
            m_host->setCursor(Qt::ArrowCursor);
            m_host->refreshView();
            return;
        }
        if (m_isDraggingUserImage) {
            stopUserImageDragging();
        }
        if (m_isDraggingBitmap) {
            stopBitmapDragging();
        }
    }
}

void ShapeEventHandler::handleBitmapsKeyPress(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape && m_isDraggingBitmap) {
        stopBitmapDragging();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        if (!m_activeRotateId.isEmpty()) {
            m_activeRotateId.clear();
            m_host->refreshView();
            return;
        }
        if (m_host->getCurrentMode() == TransformMode::PlaceBitmap) {
            m_isPlacingBitmap = false;
            m_selectedBitmapType = "";
            m_host->setCurrentMode(TransformMode::Translate);
            return;
        }
    }
}

void ShapeEventHandler::handleBitmapsPaint(QPainter& painter) {
    if (!m_host || !m_host->getGISlib()) return;

    for (const auto& entry : m_tempMeshes) {
        if (entry.name == m_activeRotateId) {
            drawRotationHandle(painter, entry);
        }
    }

    if (m_isDraggingBitmap && !m_draggingBitmapId.isEmpty()) {
        for (const auto& entry : m_tempMeshes) {
            if (entry.name == m_draggingBitmapId && !entry.bitmapPath.isEmpty()) {
                QPointF centerGeo(entry.position->x(), entry.position->y());
                QPointF centerCanvas = m_host->getGISlib()->geoToCanvas(centerGeo.y(), centerGeo.x());
                float rotationRad = entry.rotation->z();

                float halfW = entry.size->x() / 2.0f;
                float halfH = entry.size->y() / 2.0f;

                QVector<QPointF> localCorners = {
                    QPointF(-halfW, -halfH),
                    QPointF(halfW, -halfH),
                    QPointF(halfW, halfH),
                    QPointF(-halfW, halfH)
                };

                QPolygonF rotatedRect;
                float cosFwd = std::cos(rotationRad);
                float sinFwd = std::sin(rotationRad);

                for (const QPointF& corner : localCorners) {
                    float worldX = corner.x() * cosFwd - corner.y() * sinFwd;
                    float worldY = corner.x() * sinFwd + corner.y() * cosFwd;
                    QPointF cGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);
                    QPointF cCanvas = m_host->getGISlib()->geoToCanvas(cGeo.y(), cGeo.x());
                    rotatedRect << cCanvas;
                }
                painter.save();
                QPen pen(Qt::yellow, 2, Qt::DashLine);
                painter.setPen(pen);
                painter.setBrush(Qt::NoBrush);
                painter.drawPolygon(rotatedRect);
                painter.restore();
                break;
            }
        }
    }

    if (m_isDraggingUserImage && !m_draggingUserImageId.isEmpty()) {
        for (const auto& entry : m_tempMeshes) {
            if (entry.name == m_draggingUserImageId && !entry.bitmapPath.isEmpty()) {
                QPointF centerGeo(entry.position->x(), entry.position->y());
                float rotationRad = entry.rotation->z();
                float halfW = entry.size->x() / 2.0f;
                float halfH = entry.size->y() / 2.0f;
                QVector<QPointF> localCorners = {
                    QPointF(-halfW, -halfH),
                    QPointF(halfW, -halfH),
                    QPointF(halfW, halfH),
                    QPointF(-halfW, halfH)
                };
                QPolygonF rotatedRect;
                float cosFwd = std::cos(rotationRad);
                float sinFwd = std::sin(rotationRad);
                for (const QPointF& corner : localCorners) {
                    float worldX = corner.x() * cosFwd - corner.y() * sinFwd;
                    float worldY = corner.x() * sinFwd + corner.y() * cosFwd;
                    QPointF cGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);
                    QPointF cCanvas = m_host->getGISlib()->geoToCanvas(cGeo.y(), cGeo.x());
                    rotatedRect << cCanvas;
                }
                painter.save();
                QPen pen(Qt::magenta, 2, Qt::DashLine);
                painter.setPen(pen);
                painter.setBrush(Qt::NoBrush);
                painter.drawPolygon(rotatedRect);
                painter.restore();
                break;
            }
        }
    }
}

// ============================================================================
// Helper: isPointInPolygon
// ============================================================================

bool ShapeEventHandler::isPointInPolygon(const QPointF& point, const std::vector<Vector*>& vertices,
                                         const QPointF& centroidGeo, GISlib* gislib) const {
    if (!gislib) return false;

    int i, j, n = vertices.size();
    bool inside = false;
    for (i = 0, j = n - 1; i < n; j = i++) {
        QPointF vi = gislib->geoToCanvas(vertices[i]->y + centroidGeo.y(),
                                         vertices[i]->x + centroidGeo.x());
        QPointF vj = gislib->geoToCanvas(vertices[j]->y + centroidGeo.y(),
                                         vertices[j]->x + centroidGeo.x());
        if (((vi.y() > point.y()) != (vj.y() > point.y())) &&
            (point.x() < (vj.x() - vi.x()) * (point.y() - vi.y()) / (vj.y() - vi.y()) + vi.x())) {
            inside = !inside;
        }
    }
    return inside;
}