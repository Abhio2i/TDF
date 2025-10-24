/*
 * CanvasWidget Implementation File
 * This file contains the implementation of the CanvasWidget class which provides
 * a interactive canvas for rendering entities, trajectories, shapes, and handling
 * various user interactions including drag-drop, drawing, and measurement.
 */

#include "canvaswidget.h"
#include "qjsondocument.h"
#include "qquaternion.h"
#include "qvector3d.h"
#include <QTimer>
#include <QDebug>
#include <QMouseEvent>
#include <cmath>
#include <core/Debug/console.h>
#include <QtMath>
#include <QUuid>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <algorithm>
#include <QMimeData>
#include <QInputDialog>
#include <string>
#include <QFile>
#include <QJsonDocument>

CanvasWidget::CanvasWidget(QWidget *parent) : QWidget(parent) {
    gislib = new GISlib();
    setMinimumSize(300, 300);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint);
    setFocusPolicy(Qt::StrongFocus);
    setUpdatesEnabled(true);
    setMouseTracking(true);
    simulate = false;
    frameCount = 0;
    fps = 0;
    zoomLevel = 1.0f;
    fpsTimer.start();
    selectedWaypointIndex = -1;
    isDraggingWaypoint = false;
    selectedHandleIndex = -1;
    isResizingShape = false;
    QTimer* fpsUpdateTimer = new QTimer(this);
    connect(fpsUpdateTimer, &QTimer::timeout, this, [this]() {
        fps = frameCount;
        frameCount = 0;
    });
    connect(gislib, &GISlib::distanceMeasured, this, &CanvasWidget::onDistanceMeasured);
    // NEW: Add GISlib event connections for geojson
    connect(gislib, &GISlib::keyPressed, this, &CanvasWidget::keyPressEvent);
    connect(gislib, &GISlib::mousePressed, this, &CanvasWidget::mousePressEvent);
    connect(gislib, &GISlib::mouseMoved, this, &CanvasWidget::mouseMoveEvent);
    connect(gislib, &GISlib::mouseReleased, this, &CanvasWidget::mouseReleaseEvent);
    connect(gislib, &GISlib::painted, this, &CanvasWidget::paintEvent);
    fpsUpdateTimer->start(100);
    update();
    // for preset layer
    airbasePixmap = QPixmap(airbaseIconPath);
    if (airbasePixmap.isNull()) {
        Console::error("Failed to load airbase icon: " + airbaseIconPath.toStdString());
    }
}

void CanvasWidget::selectWaypoint(int index) {
    if (index >= 0 && index < (int)currentTrajectory.size()) {
        selectedWaypointIndex = index;
        Console::log("Selected waypoint index: " + std::to_string(index));
        update();
    } else {
        deselectWaypoint();
    }
}

void CanvasWidget::Render(float /*deltatime*/) {
    angle += 2.0f;
    if (angle >= 360.0f) angle = 0;
    update();
}

void CanvasWidget::simulation() {
    simulate = true;
    Console::log("Simulation mode enabled");
}


void CanvasWidget::dragEnterEvents(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void CanvasWidget::dragMoveEvents(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-entity")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void CanvasWidget::dropEvents(QDropEvent *event)
{   qDebug()<<"i am working";
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasFormat("application/x-qabstractitemmodeldatalist")) {
        QByteArray encoded = mimeData->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded, QIODevice::ReadOnly);

        while (!stream.atEnd()) {
            int row, col;
            QMap<int, QVariant> roleDataMap;
            stream >> row >> col >> roleDataMap;
            QString text = roleDataMap.value(Qt::DisplayRole).toString();
            QVariantMap customData = roleDataMap.value(Qt::UserRole).toMap();
            QJsonObject json = QJsonObject::fromVariantMap(customData);
            if ("entity" || true) {
                if (customData["type"].toString() != "entity" ||
                    !customData.contains("name") || customData["name"].toString().isEmpty() ||
                    !customData.contains("ID") || customData["ID"].toString().isEmpty()) {
                    continue;
                }
                QPoint dropPos = event->pos();
                QPointF geoPos = gislib->canvasToGeo(dropPos);
                auto& entry = Meshes[customData["ID"].toString().toStdString()];
                entry.transform->setTranslation(QVector3D(geoPos.y(),0,geoPos.x()));
                entry.position->setZ(geoPos.x());
                entry.position->setX(geoPos.y());
                qDebug() << "Entity dropped at:" << geoPos;
                update(); // Canvas ko redraw karne ke liye

            }
        }
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void CanvasWidget::editor() {
    simulate = false;
    Console::log("Editor mode enabled");
}

void CanvasWidget::setTransformMode(TransformMode mode) {
    currentMode = mode;
    if (mode == MeasureDistance) {
        if (!measureDialog) {
            measureDialog = new MeasureDistanceDialog(parentWidget());  // Change parent to nullptr
            measureDialog->setAttribute(Qt::WA_DeleteOnClose, false);
            measureDialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint |
                                          Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
            connect(measureDialog, &MeasureDistanceDialog::newMeasurementRequested, this, [&]() {
                measurePoints.clear();
                update();
            });
            connect(measureDialog, &MeasureDistanceDialog::newMeasurementRequested, this, [&]() {
                measurePoints.clear();
                update();
            });
            connect(measureDialog, &MeasureDistanceDialog::measurementTypeChanged, this, &CanvasWidget::onMeasurementTypeChanged);
            connect(measureDialog, &QDialog::rejected, this, [&]() {
                setTransformMode(Translate);
            });
            connect(this, &CanvasWidget::pointsUpdated, measureDialog, [this](const QList<QPointF>& points) {
                if (measureDialog->isVisible()) {
                    measureDialog->clearMeasurements();
                    for (int i = 1; i < points.size(); ++i) {
                        double dist = gislib->calculateDistance(points[i-1], points[i]);
                        measureDialog->addMeasurement(points[i].x(), points[i].y(), dist);
                    }
                }
            });
            measureDialog->setModal(false);
        }
        if (!measureDialog->isVisible()) {
            measureDialog->show();
            measureDialog->raise(); // Ensure dialog is brought to the front
            measureDialog->clearMeasurements();
        }
        measurePoints.clear();
        setCursor(Qt::CrossCursor);
        Console::log("Multi-point distance measurement mode enabled");
    } else {
        if (measureDialog && measureDialog->isVisible()) {
            measureDialog->hide();
        }
        setCursor(Qt::ArrowCursor);
        Console::log("Transform mode set to: " + std::to_string(mode));
    }
    update();
}

void CanvasWidget::setTrajectoryDrawingMode(bool enabled) {
    isDrawingTrajectory = enabled;
    if (enabled) {


        if (!selectedEntityId.empty()) {

            auto it = Meshes.find(selectedEntityId);
            if (it != Meshes.end() && it->second.trajectory ==nullptr ) return;
            if (it->second.trajectory && it->second.trajectory->Active) {
                currentMode = DrawTrajectory;
                setCursor(Qt::CrossCursor);
                Console::log("Trajectory drawing mode enabled");
                for (Waypoints* wp : currentTrajectory) {
                    delete wp->position;
                    delete wp;
                }
                currentTrajectory.clear();
                for (const Waypoints* wp : it->second.trajectory->Trajectories) {
                    Waypoints* newWaypoint = new Waypoints();
                    newWaypoint->position = new Vector(wp->position->x, wp->position->y, wp->position->z);
                    currentTrajectory.push_back(newWaypoint);
                }
                Console::log("Loaded " + std::to_string(currentTrajectory.size()) + " waypoints for entity: " + selectedEntityId);
            } else {
                Console::log("No existing trajectory found for entity: " + selectedEntityId);
                currentTrajectory.clear();
            }
        } else {
            Console::error("No entity selected for trajectory editing");
            currentTrajectory.clear();
        }
    } else {
        currentMode = Translate;
        setCursor(Qt::ArrowCursor);
        deselectWaypoint();
        Console::log("Trajectory drawing mode disabled");
        if (!currentTrajectory.empty() && !selectedEntityId.empty()) {
            saveTrajectory();
        }
    }
    update();
}

void CanvasWidget::saveTrajectory() {
    Console::log("saveTrajectory called");
    if (!isDrawingTrajectory) {
        Console::error("Trajectory drawing mode is not enabled");
        return;
    }
    if (selectedEntityId.empty()) {
        Console::error("No entity selected for trajectory");
        return;
    }
    auto it = Meshes.find(selectedEntityId);
    if (it == Meshes.end()) {
        Console::error("Selected entity not found in Meshes: " + selectedEntityId);
        return;
    }
    MeshEntry& entry = it->second;
    if (!entry.trajectory) {
        entry.trajectory = new Trajectory();
        entry.trajectory->ID = selectedEntityId;
    }
    for (Waypoints* wp : entry.trajectory->Trajectories) {
        delete wp->position;
        delete wp;
    }
    entry.trajectory->Trajectories.clear();
    for (Waypoints* waypoint : currentTrajectory) {
        Waypoints* newWaypoint = new Waypoints();
        newWaypoint->position = new Vector(waypoint->position->x, waypoint->position->y, waypoint->position->z);
        entry.trajectory->addTrajectory(newWaypoint);
    }
    entry.trajectory->Active = !currentTrajectory.empty();

    Console::log("Saved trajectory with " + std::to_string(currentTrajectory.size()) + " waypoints for entity: " + selectedEntityId);

    // Prepare JSON data for emission
    QJsonArray waypointsArray;
    for (const Waypoints* wp : entry.trajectory->Trajectories) {
        QJsonObject wpObj;
        QJsonObject posObj;
        posObj["type"] = "vector";
        posObj["x"] = wp->position->x;
        posObj["y"] = wp->position->y;
        posObj["z"] = wp->position->z;
        wpObj["position"] = posObj;
        waypointsArray.append(wpObj);
    }
    emit trajectoryUpdated(QString::fromStdString(selectedEntityId), waypointsArray);
    Console::log("Emitted trajectoryUpdated signal for entity: " + selectedEntityId);
}

void CanvasWidget::updateWaypointsFromInspector(QString entityId, QJsonArray waypoints) {
    Console::log("updateWaypointsFromInspector called for entity: " + entityId.toStdString() +
                 " with " + std::to_string(waypoints.size()) + " waypoints");

    auto it = Meshes.find(entityId.toStdString());
    if (it == Meshes.end()) {
        Console::error("Entity not found in Meshes: " + entityId.toStdString());
        return;
    }

    MeshEntry& entry = it->second;
    if (!entry.trajectory) {
        entry.trajectory = new Trajectory();
        entry.trajectory->ID = entityId.toStdString();
    }

    // Clear existing trajectory waypoints
    for (Waypoints* wp : entry.trajectory->Trajectories) {
        delete wp->position;
        delete wp;
    }
    entry.trajectory->Trajectories.clear();

    // Update trajectory waypoints
    for (const QJsonValue& val : waypoints) {
        QJsonObject wpObj = val.toObject();
        if (!wpObj.contains("position")) {
            Console::error("Invalid waypoint data: missing position");
            continue;
        }
        QJsonObject posObj = wpObj["position"].toObject();
        Waypoints* newWaypoint = new Waypoints();
        newWaypoint->position = new Vector(
            posObj["x"].toDouble(),
            posObj["y"].toDouble(),
            posObj["z"].toDouble()
            );
        entry.trajectory->addTrajectory(newWaypoint);
    }
    entry.trajectory->Active = !waypoints.isEmpty();

    if (isDrawingTrajectory && selectedEntityId == entityId.toStdString()) {
        for (Waypoints* wp : currentTrajectory) {
            delete wp->position;
            delete wp;
        }
        currentTrajectory.clear();
        for (const Waypoints* wp : entry.trajectory->Trajectories) {
            Waypoints* newWaypoint = new Waypoints();
            newWaypoint->position = new Vector(wp->position->x, wp->position->y, wp->position->z);
            currentTrajectory.push_back(newWaypoint);
        }
        deselectWaypoint(); // Reset selection
        Console::log("Synced currentTrajectory with " + std::to_string(currentTrajectory.size()) + " waypoints");
    }

    update();
    Console::log("Updated trajectory for entity: " + entityId.toStdString() +
                 " with " + std::to_string(entry.trajectory->Trajectories.size()) + " waypoints");
}

void CanvasWidget::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0) {
        Console::log("Scroll up detected");
        zoomLevel +=1;
    } else {
        Console::log("Scroll down detected");
        zoomLevel -=1;
    }

    if(zoomLevel > 19){
        zoomLevel = 19;
    }else
        if(zoomLevel < 1){
            zoomLevel = 1;
        }
    update();
}

void CanvasWidget::handleMousePress(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        isPanning = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        Console::log("Panning started");
        return;
    }

    if (event->button() == Qt::LeftButton && !isDrawingTrajectory && currentMode != DrawShape) {
        // Pehle user image selection check karo
        if (handleUserImageSelection(event)) {
            return;
        }

        // Fir preset bitmap selection check karo
        if (handleBitmapSelection(event)) {
            return;
        }
    }

    if (currentMode == MeasureDistance && event->button() == Qt::LeftButton) {
        QPointF geo = gislib->canvasToGeo(event->pos());
        measurePoints.append(geo);
        if (measurePoints.size() >= 2) {
            QPointF prev = measurePoints[measurePoints.size() - 2];
            QPointF curr = measurePoints.last();
            double dist = gislib->calculateDistance(prev, curr);
            if (measureDialog) {
                measureDialog->addMeasurement(curr.x(), curr.y(), dist);
            }
        }
        update();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        // FIRST: If we're in EditShape mode, handle shape editing first
        if (currentMode == EditShape && !editingShapeId.isEmpty()) {
            const qreal handleTolerance = 10.0f;
            selectedHandleIndex = -1;

            // Find resize handles
            for (size_t i = 0; i < resizeHandles.size(); ++i) {
                if (QVector2D(event->pos() - resizeHandles[i]).length() < handleTolerance) {
                    selectedHandleIndex = i;
                    isResizingShape = true;
                    dragStartPos = event->pos();
                    Console::log("Selected resize handle index: " + std::to_string(i));
                    update();
                    return;
                }
            }

            // If no handle clicked, exit EditShape mode
            currentMode = Translate;
            editingShapeId = "";
            selectedHandleIndex = -1;
            isResizingShape = false;
            resizeHandles.clear();
            setCursor(Qt::ArrowCursor);
            Console::log("Exited EditShape mode: No handle clicked");
            update();
            return;
        }

        // SECOND: If we're in DrawShape mode for multi-point shapes, handle drawing
        if (currentMode == DrawShape && (selectedShape == "Line" || selectedShape == "Polygon")) {
            QPointF geoPos = gislib->canvasToGeo(event->pos());
            bool finalize = (event->type() == QEvent::MouseButtonDblClick);
            handleShapeDrawing(selectedShape, geoPos, finalize);
            return;
        }

        // THIRD: Handle shape dragging selection (should work in Translate mode)
        if (currentMode == Translate && !isDrawingTrajectory && currentMode != PlaceBitmap) {
            if (handleShapeSelection(event)) {
                return;
            }
        }

        if (event->type() == QEvent::MouseButtonDblClick) {
            // Detect double-click to show context menu for adding text
            QMenu contextMenu(this);
            QAction* addTextAction = contextMenu.addAction("Add Text");
            connect(addTextAction, &QAction::triggered, this, [=]() {
                // Prompt user for text input using QInputDialog
                bool ok;
                QString text = QInputDialog::getText(this, "Add Text", "Enter text:", QLineEdit::Normal, "", &ok);
                if (ok && !text.isEmpty()) {
                    // Create a new MeshEntry for the text
                    static int textCounter = 0;
                    MeshEntry entry;
                    entry.name = QString("TempText_%1").arg(textCounter++);
                    QPointF geoPos = gislib->canvasToGeo(event->pos());
                    entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
                    entry.rotation = new QQuaternion();
                    entry.size = new QVector3D(1.0f, 1.0f, 1.0f);
                    entry.velocity = new QVector3D(0, 0, 0);
                    entry.trajectory = nullptr;
                    entry.collider = nullptr;
                    entry.bitmapPath = "";
                    entry.text = text; // Store the user-provided text
                    entry.mesh = new Mesh();
                    if (!entry.mesh) {
                        Console::error("Failed to allocate Mesh for text");
                        return;
                    }
                    entry.mesh->color = new QColor(Qt::black); // Set default text color
                    entry.mesh->lineWidth = 1;
                    entry.mesh->closePath = false;
                    tempMeshes.push_back(entry); // Add text entry to tempMeshes
                    Console::log("Added text '" + text.toStdString() + "' at (lon: " + std::to_string(geoPos.x()) + ", lat: " + std::to_string(geoPos.y()) + ")");
                    update();
                }
            });
            contextMenu.exec(event->globalPos());
            return;
        }
        if (currentMode == EditShape && !editingShapeId.isEmpty()) {
            const qreal handleTolerance = 10.0f;
            selectedHandleIndex = -1;
            for (size_t i = 0; i < resizeHandles.size(); ++i) {
                if (QVector2D(event->pos() - resizeHandles[i]).length() < handleTolerance) {
                    selectedHandleIndex = i;
                    isResizingShape = true;
                    dragStartPos = event->pos();
                    Console::log("Selected resize handle index: " + std::to_string(i));
                    update();
                    return;
                }
            }
            currentMode = Translate;
            editingShapeId = "";
            selectedHandleIndex = -1;
            isResizingShape = false;
            resizeHandles.clear();
            setCursor(Qt::ArrowCursor);
            Console::log("Exited EditShape mode: No handle clicked");
            update();
            return;
        }
        if (currentMode == PlaceBitmap && !selectedBitmapType.isEmpty()) {
            QPointF geoPos = gislib->canvasToGeo(event->pos());
            QString bitmapPath = getBitmapImagePath(selectedBitmapType);
            if (bitmapPath.isEmpty()) {
                Console::error("No bitmap path found for type: " + selectedBitmapType.toStdString());
                return;
            }
            static int bitmapCounter = 0;
            MeshEntry entry;
            entry.name = QString("TempBitmap_%1").arg(bitmapCounter++);
            entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
            entry.rotation = new QQuaternion();
            entry.size = new QVector3D(4.0, 4.0, 1);
            entry.velocity = new QVector3D(0, 0, 0);
            entry.trajectory = nullptr;
            entry.collider = nullptr;
            entry.bitmapPath = bitmapPath;
            entry.text = "";
            entry.mesh = new Mesh();
            if (!entry.mesh) {
                Console::error("Error: Failed to allocate Mesh for bitmap");
                return;
            }
            entry.mesh->color = new QColor(Qt::white);
            entry.mesh->lineWidth = 1;
            entry.mesh->closePath = false;
            tempMeshes.push_back(entry);
            QString logMsg = QString("Placed bitmap %1 at (lon: %2, lat: %3)")
                                 .arg(entry.name)
                                 .arg(geoPos.x())
                                 .arg(geoPos.y());
            Console::log(logMsg.toStdString());
            isPlacingBitmap = false;
            selectedBitmapType = "";
            currentMode = Translate;
            setCursor(Qt::ArrowCursor);
            update();
            return;
        }
        if (currentMode == MeasureDistance) {
            return;
        }
        if (currentMode == DrawShape) {
            QPointF geoPos = gislib->canvasToGeo(event->pos());
            bool finalize = (event->type() == QEvent::MouseButtonDblClick);
            handleShapeDrawing(selectedShape, geoPos, finalize);
            return;
        }
        if (currentMode == DrawTrajectory && isDrawingTrajectory && !selectedEntityId.empty()) {
            QPointF geoPos = gislib->canvasToGeo(event->pos());
            int nearestIndex = findNearestWaypoint(event->pos());
            if (nearestIndex >= 0) {
                selectWaypoint(nearestIndex);
                isDraggingWaypoint = true;
                Console::log("Selected existing waypoint at index: " + std::to_string(nearestIndex));
            } else {
                Waypoints* waypoint = new Waypoints();
                waypoint->position = new Vector(geoPos.y(), 0 ,geoPos.x());
                currentTrajectory.push_back(waypoint);
                selectWaypoint(currentTrajectory.size() - 1);
                Console::log("Added new waypoint at (lon: " + std::to_string(waypoint->position->z) +
                             ", lat: " + std::to_string(waypoint->position->x) + ")");
                updateTrajectoryData();
            }
            update();
            return;

        }
        if (!selectedEntityId.empty()) {
            auto it = Meshes.find(selectedEntityId);
            if (it != Meshes.end()) {
                auto& entry = it->second;
                QPointF basePos = gislib->geoToCanvas(entry.transform->translation().x(), entry.transform->translation().z());
                const float handleTolerance = 15.0f;
                if (currentMode == Translate) {
                    QPointF xAxisHandle = basePos + QPointF(50, 0);
                    QPointF yAxisHandle = basePos + QPointF(0, 50);
                    if (QVector2D(event->pos() - xAxisHandle).length() < handleTolerance) {
                        activeDragAxis = "x";
                        dragStartPos = event->pos();
                        Console::log("Dragging X-axis of entity: " + selectedEntityId);
                        emit MoveEntity(QString::fromStdString(selectedEntityId));
                        update();
                        return;
                    }
                    if (QVector2D(event->pos() - yAxisHandle).length() < handleTolerance) {
                        activeDragAxis = "y";
                        dragStartPos = event->pos();
                        Console::log("Dragging Y-axis of entity: " + selectedEntityId);
                        emit MoveEntity(QString::fromStdString(selectedEntityId));
                        update();
                        return;
                    }
                } else if (currentMode == Rotate) {
                    float distFromCenter = QVector2D(event->pos() - basePos).length();
                    if (qAbs(distFromCenter - 40.0f) < handleTolerance) {
                        activeDragAxis = "rotate";
                        dragStartPos = event->pos();
                        Console::log("Gizmo selected for rotation.");
                        emit MoveEntity(QString::fromStdString(selectedEntityId));
                        update();
                        return;
                    }
                } else if (currentMode == Scale) {
                    QPointF xScaleHandle = basePos + QPointF(50, 0);
                    QPointF yScaleHandle = basePos + QPointF(0, 50);
                    if (QVector2D(event->pos() - xScaleHandle).length() < handleTolerance) {
                        activeDragAxis = "scale-x";
                        dragStartPos = event->pos();
                        Console::log("Gizmo X-axis selected for scaling entity: " + selectedEntityId);
                        emit MoveEntity(QString::fromStdString(selectedEntityId));
                        update();
                        return;
                    }
                    if (QVector2D(event->pos() - yScaleHandle).length() < handleTolerance) {
                        activeDragAxis = "scale-y";
                        dragStartPos = event->pos();
                        Console::log("Gizmo Y-axis selected for scaling entity: " + selectedEntityId);
                        emit MoveEntity(QString::fromStdString(selectedEntityId));
                        update();
                        return;
                    }
                }
            }
        }
        bool entityWasClicked = false;
        for (auto& [id, entry] : Meshes) {
            QPointF entityPos = gislib->geoToCanvas(entry.transform->translation().x(), entry.transform->translation().z());
            if (QVector2D(event->pos() - entityPos).length() < 20.0f) {
                if (selectedEntityId != id) {
                    selectedEntityId = id;
                    emit selectEntitybyCursor(QString::fromStdString(id));
                    Console::log("Selected entity: " + id);
                }
                dragStartPos = event->pos();
                activeDragAxis = "both";
                entityWasClicked = true;
                update();
                return;
            }
        }
        if (!entityWasClicked && currentMode != DrawTrajectory && currentMode != DrawShape && currentMode != PlaceBitmap) {
            selectedEntityId = "";
            activeDragAxis = "";
            Console::log("Deselected all entities.");
            update();
        }
    }
    if (event->button() == Qt::RightButton) {
        handleRightClick(event);
        return;
    }
    update();
}

//=============HandleShapeDrawing=======================

void CanvasWidget::handleShapeDrawing(const QString& shapeType, const QPointF& geoPos, bool finalize) {
    if (currentMode != DrawShape || shapeType.isEmpty()) return;

    selectedShape = shapeType;

    if (shapeType == "Circle") {
        drawCircle(geoPos);
        // Note: drawCircle now auto-exits DrawShape mode
    } else if (shapeType == "Rectangle") {
        drawRectangle(geoPos);
        // Note: drawRectangle now auto-exits DrawShape mode
    } else if (shapeType == "Line") {
        drawLine(geoPos, finalize);
        // Line doesn't auto-exit as it needs multiple points
    } else if (shapeType == "Polygon") {
        drawPolygon(geoPos, finalize);
        // Polygon doesn't auto-exit as it needs multiple points
    } else if (shapeType == "Points") {
        drawPoints(geoPos);
        // Note: drawPoints now auto-exits DrawShape mode
    }
}

void CanvasWidget::drawCircle(const QPointF& geoPos) {
    static int circleCounter = 0;
    MeshEntry entry;
    entry.name = QString("TempCircle_%1").arg(circleCounter++);
    entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0); // Center position
    entry.rotation = new QQuaternion();
    entry.size = new QVector3D(8, 8, 1); // Radius in geographic units
    entry.velocity = new QVector3D(0, 0, 0);
    entry.trajectory = nullptr;
    entry.collider = nullptr;
    entry.bitmapPath = "";
    entry.text = "";
    entry.mesh = new Mesh();
    if (!entry.mesh) {
        Console::error("Failed to allocate Mesh for circle");
        return;
    }
    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 2;
    entry.mesh->closePath = true;

    // Store center point (relative to position, which is same as absolute for circle)
    entry.mesh->polygen.push_back(new Vector(0, 0, 0)); // Relative to center

    tempMeshes.push_back(entry);

    // SWITCH TO TRANSLATE MODE after creating circle
    currentMode = Translate;
    setCursor(Qt::ArrowCursor);

    Console::log("Created temporary circle at (lon: " + std::to_string(geoPos.x()) + ", lat: " + std::to_string(geoPos.y()) + ")");
    update();
}

void CanvasWidget::drawRectangle(const QPointF& geoPos) {
    static int rectCounter = 0;
    MeshEntry entry;
    entry.name = QString("TempRectangle_%1").arg(rectCounter++);
    entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
    entry.rotation = new QQuaternion();
    entry.size = new QVector3D(7.0, 7.0, 1);
    entry.velocity = new QVector3D(0, 0, 0);
    entry.trajectory = nullptr;
    entry.collider = nullptr;
    entry.bitmapPath = "";
    entry.text = "";
    entry.mesh = new Mesh();
    if (!entry.mesh) {
        Console::error("Failed to allocate Mesh for rectangle");
        return;
    }
    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 2;
    entry.mesh->closePath = true;
    float halfWidth = 0.01f;
    float halfHeight = 0.005f;
    entry.mesh->polygen = {
        new Vector(-halfWidth, -halfHeight, 0),
        new Vector(halfWidth, -halfHeight, 0),
        new Vector(halfWidth, halfHeight, 0),
        new Vector(-halfWidth, halfHeight, 0)
    };
    tempMeshes.push_back(entry);

    // SWITCH TO TRANSLATE MODE after creating rectangle
    currentMode = Translate;
    setCursor(Qt::ArrowCursor);

    Console::log("Created temporary rectangle at (lon: " + std::to_string(geoPos.x()) + ", lat: " + std::to_string(geoPos.y()) + ")");
    update();
}

void CanvasWidget::drawLine(const QPointF& geoPos, bool finalize) {
    if (!finalize) {
        // Add vertex to temporary line
        tempLineVertices.push_back(new Vector(geoPos.x(), geoPos.y(), 0));
        tempLineCanvasPoints.push_back(gislib->geoToCanvas(geoPos.y(), geoPos.x()));
        Console::log("Added line vertex at (lon: " + std::to_string(geoPos.x()) + ", lat: " + std::to_string(geoPos.y()) + ")");
        update();
    } else if (tempLineVertices.size() >= 2) {
        // Finalize line on double-click or finalize signal
        static int polylineCounter = 0;
        MeshEntry entry;
        entry.name = QString("TempPolyline_%1").arg(polylineCounter++);

        // Calculate centroid
        float avgX = 0, avgY = 0;
        for (const Vector* v : tempLineVertices) {
            avgX += v->x;
            avgY += v->y;
        }
        avgX /= tempLineVertices.size();
        avgY /= tempLineVertices.size();

        entry.position = new QVector3D(avgX, avgY, 0); // Centroid
        entry.rotation = new QQuaternion();
        entry.size = new QVector3D(1.0f, 1.0f, 1.0f);
        entry.velocity = new QVector3D(0, 0, 0);
        entry.trajectory = nullptr;
        entry.collider = nullptr;
        entry.bitmapPath = "";
        entry.text = "";
        entry.mesh = new Mesh();

        if (!entry.mesh) {
            Console::error("Failed to allocate Mesh for line");
            return;
        }

        entry.mesh->color = new QColor(Qt::red);
        entry.mesh->lineWidth = 2;
        entry.mesh->closePath = false; // Important: line should not be closed

        // Store vertices as RELATIVE to centroid
        for (Vector* v : tempLineVertices) {
            entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
        }

        tempMeshes.push_back(entry);

        // Clear temporary data
        for (Vector* v : tempLineVertices) {
            delete v;
        }
        tempLineVertices.clear();
        tempLineCanvasPoints.clear();

        // SWITCH TO TRANSLATE MODE after creating line
        currentMode = Translate;
        setCursor(Qt::ArrowCursor);

        Console::log("Created temporary polyline with " + std::to_string(entry.mesh->polygen.size()) + " vertices");
        update();
    }
}

void CanvasWidget::drawPolygon(const QPointF& geoPos, bool finalize) {
    if (!finalize) {
        // Add vertex to temporary polygon
        tempPolygonVertices.push_back(new Vector(geoPos.x(), geoPos.y(), 0));
        tempPolygonCanvasPoints.push_back(gislib->geoToCanvas(geoPos.y(), geoPos.x()));
        Console::log("Added polygon vertex at (lon: " + std::to_string(geoPos.x()) + ", lat: " + std::to_string(geoPos.y()) + ")");
        update();
    } else if (tempPolygonVertices.size() >= 3) {
        // Finalize polygon on double-click or finalize signal
        static int polygonCounter = 0;
        MeshEntry entry;
        entry.name = QString("TempPolygon_%1").arg(polygonCounter++);

        // Calculate centroid
        float avgX = 0, avgY = 0;
        for (const Vector* v : tempPolygonVertices) {
            avgX += v->x;
            avgY += v->y;
        }
        avgX /= tempPolygonVertices.size();
        avgY /= tempPolygonVertices.size();

        entry.position = new QVector3D(avgX, avgY, 0); // Centroid
        entry.rotation = new QQuaternion();
        entry.size = new QVector3D(1.0f, 1.0f, 1.0f);
        entry.velocity = new QVector3D(0, 0, 0);
        entry.trajectory = nullptr;
        entry.collider = nullptr;
        entry.bitmapPath = "";
        entry.text = "";
        entry.mesh = new Mesh();

        if (!entry.mesh) {
            Console::error("Failed to allocate Mesh for polygon");
            return;
        }

        entry.mesh->color = new QColor(Qt::red);
        entry.mesh->lineWidth = 2;
        entry.mesh->closePath = true;

        // Store vertices as RELATIVE to centroid
        for (Vector* v : tempPolygonVertices) {
            entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
        }

        tempMeshes.push_back(entry);

        // Clear temporary data
        for (Vector* v : tempPolygonVertices) {
            delete v;
        }
        tempPolygonVertices.clear();
        tempPolygonCanvasPoints.clear();

        // SWITCH TO TRANSLATE MODE after creating polygon
        currentMode = Translate;
        setCursor(Qt::ArrowCursor);

        Console::log("Created temporary polygon with " + std::to_string(entry.mesh->polygen.size()) + " vertices");
        update();
    }
}

void CanvasWidget::drawPoints(const QPointF& geoPos) {
    static int pointCounter = 0;
    MeshEntry entry;
    entry.name = QString("TempPoint_%1").arg(pointCounter++);
    entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0); // Point position
    entry.rotation = new QQuaternion();
    entry.size = new QVector3D(0, 0, 1);
    entry.velocity = new QVector3D(0, 0, 0);
    entry.trajectory = nullptr;
    entry.collider = nullptr;
    entry.bitmapPath = "";
    entry.text = "";
    entry.mesh = new Mesh();
    if (!entry.mesh) {
        Console::error("Failed to allocate Mesh for point");
        return;
    }
    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 1;
    entry.mesh->closePath = true;

    // Store point position (relative to position, which is same as absolute for point)
    entry.mesh->polygen.push_back(new Vector(0, 0, 0)); // Relative to position

    tempMeshes.push_back(entry);

    // SWITCH TO TRANSLATE MODE after creating point
    currentMode = Translate;
    setCursor(Qt::ArrowCursor);

    Console::log("Created temporary point at (lon: " + std::to_string(geoPos.x()) + ", lat: " + std::to_string(geoPos.y()) + ")");
    update();
}

static bool isPointNearLineSegment(const QPointF& p, const QPointF& v1, const QPointF& v2, qreal tolerance) {
    QPointF vec = v2 - v1;
    QPointF vp = p - v1;
    qreal lenSquared = vec.x() * vec.x() + vec.y() * vec.y();
    if (lenSquared < 1e-6) {
        return QVector2D(p - v1).length() < tolerance;
    }
    qreal t = std::max<qreal>(0.0, std::min<qreal>(1.0, (vp.x() * vec.x() + vp.y() * vec.y()) / lenSquared));
    QPointF projection = v1 + t * vec;
    return QVector2D(p - projection).length() < tolerance;
}

void CanvasWidget::handleRightClick(QMouseEvent *event) {
    // First try handling trajectory right-click
    if (currentMode == DrawTrajectory && isDrawingTrajectory) {
        handleTrajectoryRightClick(event);
        return;
    }
    handleShapeRightClick(event);
    update();
}

void CanvasWidget::handleTrajectoryRightClick(QMouseEvent *event) {
    int nearestIndex = findNearestWaypoint(event->pos());
    if (nearestIndex >= 0) {
        selectWaypoint(nearestIndex);
        QMenu contextMenu(this);
        QAction* deleteAction = contextMenu.addAction("Delete Waypoint");
        connect(deleteAction, &QAction::triggered, this, [=]() {
            if (selectedWaypointIndex >= 0 && selectedWaypointIndex < (int)currentTrajectory.size()) {
                QMessageBox confirmationDialog(this);
                confirmationDialog.setWindowTitle("Confirm Delete");
                confirmationDialog.setText("Are you sure you want to delete this waypoint?");
                confirmationDialog.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                confirmationDialog.setDefaultButton(QMessageBox::No);
                QPalette palette = confirmationDialog.palette();
                palette.setColor(QPalette::ButtonText, Qt::white);
                palette.setColor(QPalette::Button, QColor("#333333"));
                palette.setColor(QPalette::Window, QColor("#222222"));
                palette.setColor(QPalette::WindowText, Qt::white);
                confirmationDialog.setPalette(palette);
                QList<QPushButton*> buttons = confirmationDialog.findChildren<QPushButton*>();
                for (QPushButton* button : buttons) {
                    button->setStyleSheet("color: white; background-color: #333333; border: 1px solid #555555; padding: 5px;");
                    Console::log("Applied stylesheet to button: " + button->text().toStdString());
                }
                QMessageBox::StandardButton reply = static_cast<QMessageBox::StandardButton>(confirmationDialog.exec());
                if (reply == QMessageBox::Yes) {
                    delete currentTrajectory[selectedWaypointIndex]->position;
                    delete currentTrajectory[selectedWaypointIndex];
                    currentTrajectory.erase(currentTrajectory.begin() + selectedWaypointIndex);
                    deselectWaypoint();
                    updateTrajectoryData();
                    Console::log("Deleted waypoint at index: " + std::to_string(nearestIndex));
                    update();
                }
            }
        });
        contextMenu.exec(event->globalPos());
    }
}

void CanvasWidget::handleShapeRightClick(QMouseEvent *event) {
    QPointF geoPos = gislib->canvasToGeo(event->pos());
    const qreal tolerance = 20.0;
    QString closestShapeId;
    qreal minDistance = std::numeric_limits<qreal>::max();
    MeshEntry* closestEntry = nullptr;
    auto closestIt = tempMeshes.end();  // Store the iterator here
    for (auto it = tempMeshes.begin(); it != tempMeshes.end(); ++it) {
        MeshEntry& entry = *it;
        if (!entry.position || (!entry.mesh && entry.text.isEmpty() && !entry.name.startsWith("TempPoint"))) {
            Console::error("Invalid MeshEntry data for " + entry.name.toStdString());
            continue;
        }
        QPointF entityPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
        bool isHit = false;
        qreal distance = std::numeric_limits<qreal>::max();
        // Check if the right-click is on a text entry
        if (entry.name.startsWith("TempText")) {
            QFont font("Arial", 12, QFont::Bold);
            QFontMetrics fm(font);
            QRect textRect = fm.boundingRect(entry.text);
            textRect.moveTo(entityPos.x(), entityPos.y());
            isHit = textRect.contains(event->pos());
            distance = QVector2D(event->pos() - entityPos).length();
            if (isHit) {
                Console::log(std::string("Right-click detected on text: ") + entry.text.toStdString() +
                             std::string(" at (x: ") + std::to_string(entityPos.x()) +
                             std::string(", y: ") + std::to_string(entityPos.y()) + ")");
            }
        }
        // Check if the right-click is on a point
        else if (entry.name.startsWith("TempPoint")) {
            distance = QVector2D(event->pos() - entityPos).length();
            isHit = (distance < tolerance);
            if (isHit) {
                Console::log(std::string("Right-click detected on point: ") + entry.name.toStdString() +
                             std::string(" at (x: ") + std::to_string(entityPos.x()) +
                             std::string(", y: ") + std::to_string(entityPos.y()) + ")");
            }
        }
        // Check if the right-click is on a polygon
        else if (entry.name.startsWith("TempPolygon")) {
            isHit = isPointInPolygon(event->pos(), entry.mesh->polygen, QPointF(entry.position->x(), entry.position->y()), gislib);
            distance = QVector2D(event->pos() - entityPos).length();
        }
        // Check if the right-click is on a polyline
        else if (entry.name.startsWith("TempPolyline")) {
            for (size_t i = 0; i < entry.mesh->polygen.size() - 1; ++i) {
                QPointF v1 = gislib->geoToCanvas(entry.mesh->polygen[i]->y + entry.position->y(), entry.mesh->polygen[i]->x + entry.position->x());
                QPointF v2 = gislib->geoToCanvas(entry.mesh->polygen[i + 1]->y + entry.position->y(), entry.mesh->polygen[i + 1]->x + entry.position->x());
                if (isPointNearLineSegment(event->pos(), v1, v2, tolerance)) {
                    isHit = true;
                    distance = QVector2D(event->pos() - entityPos).length();
                    break;
                }
            }
        }
        // Check for other shapes (circle, rectangle, bitmap)
        else {
            qreal w, h;
            if (entry.name.startsWith("TempCircle")) {
                w = h = entry.size->x() * 25;
                distance = QVector2D(event->pos() - entityPos).length();
                isHit = (distance < w / 2.0);
            } else if (entry.name.startsWith("TempRectangle") || !entry.bitmapPath.isEmpty()) {
                w = entry.size->x() * 25;
                h = entry.size->y() * 25;
                isHit = (event->pos().x() >= entityPos.x() - w / 2 &&
                         event->pos().x() <= entityPos.x() + w / 2 &&
                         event->pos().y() >= entityPos.y() - h / 2 &&
                         event->pos().y() <= entityPos.y() + h / 2);
                distance = QVector2D(event->pos() - entityPos).length();
            } else {
                continue;
            }
        }
        if (isHit && distance < minDistance) {
            minDistance = distance;
            closestShapeId = entry.name;
            closestEntry = &entry;
            closestIt = it;  // Capture the iterator
        }
    }
    if (!closestShapeId.isEmpty() && closestEntry && closestIt != tempMeshes.end()) {  // Added check for valid iterator
        QMenu contextMenu(this);
        // Add edit option for shapes and bitmaps
        if (closestShapeId.startsWith("TempPolyline") ||
            closestShapeId.startsWith("TempCircle") ||
            closestShapeId.startsWith("TempRectangle") ||
            closestShapeId.startsWith("TempPolygon") ||
            !closestEntry->bitmapPath.isEmpty()) {
            QAction* editAction = contextMenu.addAction("Edit");
            connect(editAction, &QAction::triggered, this, [=]() {
                currentMode = EditShape;
                editingShapeId = closestShapeId;
                setCursor(Qt::SizeAllCursor);
                resizeHandles.clear();
                if (closestShapeId.startsWith("TempRectangle") || !closestEntry->bitmapPath.isEmpty()) {
                    float w = closestEntry->size->x() * 25;
                    float h = closestEntry->size->y() * 25;
                    QPointF centerCanvas = gislib->geoToCanvas(closestEntry->position->y(), closestEntry->position->x());
                    resizeHandles = {
                        centerCanvas + QPointF(-w/2, -h/2),
                        centerCanvas + QPointF(w/2, -h/2),
                        centerCanvas + QPointF(w/2, h/2),
                        centerCanvas + QPointF(-w/2, h/2)
                    };
                } else if (closestShapeId.startsWith("TempCircle")) {
                    QPointF centerCanvas = gislib->geoToCanvas(closestEntry->position->y(), closestEntry->position->x());
                    resizeHandles = { centerCanvas + QPointF(closestEntry->size->x() * 25, 0) };
                } else if (closestShapeId.startsWith("TempPolygon") || closestShapeId.startsWith("TempPolyline")) {
                    for (Vector* v : closestEntry->mesh->polygen) {
                        QPointF canvasPoint = gislib->geoToCanvas(v->y + closestEntry->position->y(), v->x + closestEntry->position->x());
                        resizeHandles.push_back(canvasPoint);
                    }
                }
                Console::log("Entered EditShape mode for: " + closestShapeId.toStdString());
                update();
            });
        }
        QAction* deleteAction = contextMenu.addAction(
            closestShapeId.startsWith("TempPolyline") ||
                    closestShapeId.startsWith("TempCircle") ||
                    closestShapeId.startsWith("TempRectangle") ||
                    closestShapeId.startsWith("TempPolygon") ||
                    closestShapeId.startsWith("TempPoint") ? "Delete Shape" :
                closestShapeId.startsWith("TempText") ? "Delete Text" :
                "Delete Bitmap"
            );
        connect(deleteAction, &QAction::triggered, this, [=]() {
            // Delete immediately without confirmation dialog
            delete closestEntry->position;
            delete closestEntry->rotation;
            delete closestEntry->size;
            delete closestEntry->velocity;
            if (closestEntry->mesh) {
                for (Vector* v : closestEntry->mesh->polygen) {
                    delete v;
                }
                delete closestEntry->mesh->color;
                delete closestEntry->mesh;
            }
            delete closestEntry->collider;
            delete closestEntry->trajectory;
            // Erase using the stored iterator instead of looping by name
            tempMeshes.erase(closestIt);
            Console::log(std::string("Deleted ") +
                         (closestShapeId.startsWith("TempText") ? "text: " :
                              closestShapeId.startsWith("TempPolyline") ||
                                  closestShapeId.startsWith("TempCircle") ||
                                  closestShapeId.startsWith("TempRectangle") ||
                                  closestShapeId.startsWith("TempPolygon") ||
                                  closestShapeId.startsWith("TempPoint") ? "shape: " :
                              "bitmap: ") +
                         closestShapeId.toStdString());
            update();
        });
        contextMenu.exec(event->globalPos());
        return;
    }
}

bool CanvasWidget::isPointInPolygon(const QPointF& point, const std::vector<Vector*>& vertices, const QPointF& centroidGeo, GISlib* gislib) {
    int i, j, n = vertices.size();
    bool inside = false;
    for (i = 0, j = n - 1; i < n; j = i++) {
        // Convert vertices from geo coordinates (relative to centroid) to canvas coordinates
        QPointF vi = gislib->geoToCanvas(vertices[i]->y + centroidGeo.y(), vertices[i]->x + centroidGeo.x());
        QPointF vj = gislib->geoToCanvas(vertices[j]->y + centroidGeo.y(), vertices[j]->x + centroidGeo.x());
        // Ray-casting algorithm
        if (((vi.y() > point.y()) != (vj.y() > point.y())) &&
            (point.x() < (vj.x() - vi.x()) * (point.y() - vi.y()) / (vj.y() - vi.y()) + vi.x())) {
            inside = !inside;
        }
    }
    return inside;
}

void CanvasWidget::handleMouseMove(QMouseEvent *event) {
    if (!gislib) {
        Console::error("GISlib is not initialized");
        return;
    }

    // First check for shape dragging
    if (isDraggingShape && !draggingShapeId.isEmpty()) {
        handleShapeDragging(event);
        return;
    }

    // Pehle user image dragging
    if (isDraggingUserImage && !draggingUserImageId.isEmpty()) {
        handleUserImageDragging(event);
        return;
    }

    // NEW: Bitmap dragging (works for both preset and user selected bitmaps)
    if (isDraggingBitmap && !draggingBitmapId.isEmpty()) {
        handleBitmapDragging(event);
        return;
    }

    if (isPanning) {
        QPoint delta = event->pos() - lastMousePos;
        canvasOffset += delta;
        lastMousePos = event->pos();
        update();
        return;
    }
    if (currentMode == MeasureDistance) {
        return;
    }
    if (currentMode == EditShape && isResizingShape && selectedHandleIndex >= 0 && !editingShapeId.isEmpty()) {
        for (auto& entry : tempMeshes) {
            if (entry.name == editingShapeId) {
                if (!entry.position || !entry.size || !entry.mesh) {
                    Console::error("Invalid MeshEntry data for " + editingShapeId.toStdString());
                    return;
                }
                QPointF newPos = event->pos();
                QPointF geoPos = gislib->canvasToGeo(newPos);
                QPointF centerCanvas = gislib->geoToCanvas(entry.position->y(), entry.position->x());
                if (entry.name.startsWith("TempRectangle") || !entry.bitmapPath.isEmpty()) { // NEW: Handle bitmaps
                    float w = entry.size->x() * 25;
                    float h = entry.size->y() * 25;
                    QPointF delta = newPos - dragStartPos;
                    float newWidth = w;
                    float newHeight = h;
                    if (selectedHandleIndex == 0) { // Top-left
                        newWidth -= delta.x();
                        newHeight -= delta.y();
                        entry.position->setX(entry.position->x() + delta.x() / 50.0f);
                        entry.position->setY(entry.position->y() + delta.y() / 50.0f);
                    } else if (selectedHandleIndex == 1) { // Top-right
                        newWidth += delta.x();
                        newHeight -= delta.y();
                        entry.position->setY(entry.position->y() + delta.y() / 50.0f);
                    } else if (selectedHandleIndex == 2) { // Bottom-right
                        newWidth += delta.x();
                        newHeight += delta.y();
                    } else if (selectedHandleIndex == 3) { // Bottom-left
                        newWidth -= delta.x();
                        newHeight += delta.y();
                        entry.position->setX(entry.position->x() + delta.x() / 50.0f);
                    }
                    newWidth = std::max(1.0f, newWidth / 25.0f);
                    newHeight = std::max(1.0f, newHeight / 25.0f);
                    entry.size->setX(newWidth);
                    entry.size->setY(newHeight);
                    if (entry.name.startsWith("TempRectangle")) { // Update rectangle vertices
                        entry.mesh->polygen.clear();
                        float halfWidth = newWidth * 0.01f;
                        float halfHeight = newHeight * 0.005f;
                        entry.mesh->polygen = {
                            new Vector(-halfWidth, -halfHeight, 0),
                            new Vector(halfWidth, -halfHeight, 0),
                            new Vector(halfWidth, halfHeight, 0),
                            new Vector(-halfWidth, halfHeight, 0)
                        };
                    }
                    resizeHandles = {
                        centerCanvas + QPointF(-newWidth*25/2, -newHeight*25/2),
                        centerCanvas + QPointF(newWidth*25/2, -newHeight*25/2),
                        centerCanvas + QPointF(newWidth*25/2, newHeight*25/2),
                        centerCanvas + QPointF(-newWidth*25/2, newHeight*25/2)
                    };
                } else if (entry.name.startsWith("TempCircle")) {
                    float newRadius = QVector2D(newPos - centerCanvas).length() / 25.0f;
                    newRadius = std::max(1.0f, newRadius);
                    entry.size->setX(newRadius);
                    entry.size->setY(newRadius);
                    resizeHandles = { centerCanvas + QPointF(newRadius*25, 0) };
                } else if (entry.name.startsWith("TempPolygon") || entry.name.startsWith("TempPolyline")) { // Restored TempPolygon
                    if (selectedHandleIndex < (int)entry.mesh->polygen.size()) {
                        Vector* vertex = entry.mesh->polygen[selectedHandleIndex];
                        if (!vertex) {
                            Console::error("Null vertex at index " + std::to_string(selectedHandleIndex));
                            return;
                        }
                        vertex->x = geoPos.x() - entry.position->x();
                        vertex->y = geoPos.y() - entry.position->y();
                        resizeHandles[selectedHandleIndex] = newPos;
                    } else {
                        Console::error("Invalid vertex index: " + std::to_string(selectedHandleIndex));
                    }
                }
                dragStartPos = newPos;
                update();
                return;
            }
        }
        return;
    }
    if (isDrawingTrajectory && isDraggingWaypoint && selectedWaypointIndex >= 0 &&
        selectedWaypointIndex < (int)currentTrajectory.size()) {
        QPointF geoPos = gislib->canvasToGeo(event->pos());
        Waypoints* wp = currentTrajectory[selectedWaypointIndex];
        wp->position->z = geoPos.x();
        wp->position->x = geoPos.y();
        update();
        return;
    }
    if (Meshes.find(selectedEntityId) == Meshes.end()) return;

    auto& entry = Meshes[selectedEntityId];
    if (!entry.position || !entry.size || !entry.rotation) {
        Console::error("Invalid MeshEntry data for " + selectedEntityId);
        return;
    }
    QPointF delta = event->pos() - dragStartPos;

    float dx = event->pos().x() - entry.transform->translation().z() - canvasOffset.x();
    float dy = event->pos().y() - entry.transform->translation().x() - canvasOffset.y();

    if (qAbs(dx) < 10 && qAbs(dy) < 10) {
        //activeDragAxis = "both";
    } else if (qAbs(dx - 50) < 25 && qAbs(dy) < 10) {
        //activeDragAxis = "x";
    } else if (qAbs(dy - 50) < 25 && qAbs(dx) < 10) {
        //activeDragAxis = "y";
    }
    //qDebug()<< dx <<","<<dy<<","<<activeDragAxis<<","<<canvasOffset.x()<<","<<canvasOffset.y()<<","<<entry.transform->translation().z()<<","<<entry.transform->translation().x()<<","<<event->pos();

    if (!selectedEntityId.empty() && !activeDragAxis.isEmpty()) {
        if (currentMode == Translate) {
            QPointF newGeoPos = gislib->canvasToGeo(event->pos());
            if (activeDragAxis == "x") {
                QVector3D po(entry.transform->translation().x(),0,newGeoPos.x());
                entry.transform->setTranslation(po);
            } else if (activeDragAxis == "y") {
                QVector3D po(newGeoPos.y(),0,entry.transform->translation().z());
                entry.transform->setTranslation(po);
            } else if (activeDragAxis == "both") {
                QVector3D po(newGeoPos.y(),0,newGeoPos.x());
                entry.transform->setTranslation(po);
            }
            emit MoveEntity(QString::fromStdString(selectedEntityId));
        } else if (currentMode == Rotate) {
            QPointF basePos = gislib->geoToCanvas(entry.transform->translation().x(), entry.transform->translation().z());
            qreal angle_new = qAtan2(event->pos().y() - basePos.y(), event->pos().x() - basePos.x());
            qreal angle_old = qAtan2(dragStartPos.y() - basePos.y(), dragStartPos.x() - basePos.x());
            float angle_change = qRadiansToDegrees(angle_new - angle_old);
            entry.transform->setRotation(QQuaternion::fromEulerAngles(QVector3D(0,entry.transform->rotation().toEulerAngles().y()-angle_change,0)));
            emit MoveEntity(QString::fromStdString(selectedEntityId));
        } else if (currentMode == Scale) {
            QPointF basePos = gislib->geoToCanvas(entry.transform->translation().x(), entry.transform->translation().z());
            qreal dist_new = QVector2D(event->pos() - basePos).length();
            qreal dist_old = QVector2D(dragStartPos - basePos).length();
            qreal scaleFactor = dist_new / dist_old;
            if (activeDragAxis == "x") {
                QVector3D sc(0,0,entry.transform->scale3D().z() * scaleFactor);
                entry.transform->setScale3D(sc);
            } else if (activeDragAxis == "y") {
                QVector3D sc(entry.transform->scale3D().x() * scaleFactor,0,0);
                entry.transform->setScale3D(sc);
            } else if (activeDragAxis == "both") {
                QVector3D sc(entry.transform->scale3D().x() * scaleFactor,0,entry.transform->scale3D().z() * scaleFactor);
                entry.transform->setScale3D(sc);
            }
            emit MoveEntity(QString::fromStdString(selectedEntityId));
        }
        dragStartPos = event->pos();
        update();
    }
}

void CanvasWidget::handleMouseRelease(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {

        // Stop all types of dragging
        if (isDraggingShape) {
            stopShapeDragging();
        }

        // Dono alag stop karo
        if (isDraggingUserImage) {
            stopUserImageDragging();
        }

        // NEW: Stop bitmap dragging (works for both types)
        if (isDraggingBitmap) {
            stopBitmapDragging();
        }
        activeDragAxis = "";
        isDraggingWaypoint = false;
        isResizingShape = false;
        selectedHandleIndex = -1;
        selectEntity = false;
        updateTrajectoryData();
    }
    if (currentMode == MeasureDistance) {
        return; // Avoid extra updates during measure mode
    }
    if (event->button() == Qt::MiddleButton) {
        isPanning = false;
        setCursor(Qt::ArrowCursor);
        Console::log("Panning stopped");
    }
    update();
}

void CanvasWidget::handleKeyPress(QKeyEvent *event) {
    Console::log("Key pressed: " + std::to_string(event->key()));

    // NEW: Cancel bitmap dragging with Escape key (works for both types)
    if (event->key() == Qt::Key_Escape && isDraggingBitmap) {
        stopBitmapDragging();
        return;
    }
    if (event->key() == Qt::Key_Escape && currentMode == MeasureDistance) {
        setTransformMode(Translate);
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        if (currentMode == EditShape) {
            currentMode = Translate;
            editingShapeId = "";
            selectedHandleIndex = -1;
            isResizingShape = false;
            resizeHandles.clear();
            setCursor(Qt::ArrowCursor);
            Console::log("Exited EditShape mode");
            update();
            return;
        }
        if (currentMode == DrawTrajectory) {
            Console::log("Calling saveTrajectory");
            saveTrajectory();
            setTrajectoryDrawingMode(false);
            deselectWaypoint();
        } else if (currentMode == DrawShape) {
            if (selectedShape == "Polygon" && tempPolygonVertices.size() >= 3) {
                MeshEntry entry;
                entry.name = "TempPolygon";
                float avgX = 0, avgY = 0;
                for (const Vector* v : tempPolygonVertices) {
                    avgX += v->x;
                    avgY += v->y;
                }
                avgX /= tempPolygonVertices.size();
                avgY /= tempPolygonVertices.size();
                entry.position = new QVector3D(avgX, avgY, 0);
                entry.rotation = new QQuaternion();
                float minX = std::numeric_limits<float>::max();
                float maxX = std::numeric_limits<float>::lowest();
                float minY = std::numeric_limits<float>::max();
                float maxY = std::numeric_limits<float>::lowest();
                for (const Vector* v : tempPolygonVertices) {
                    minX = std::min(minX, v->x);
                    maxX = std::max(maxX, v->x);
                    minY = std::min(minY, v->y);
                    maxY = std::max(maxY, v->y);
                }
                entry.size = new QVector3D(1.0f, 1.0f, 1.0f);
                entry.velocity = new QVector3D(0, 0, 0);
                entry.trajectory = nullptr;
                entry.collider = nullptr;
                entry.mesh = new Mesh();
                if (!entry.mesh) {
                    Console::log("Error: Failed to allocate Mesh");
                    return;
                }
                entry.mesh->color = new QColor(Qt::red);
                if (!entry.mesh->color) {
                    Console::log("Error: Failed to allocate QColor");
                    delete entry.mesh;
                    return;
                }
                entry.mesh->lineWidth = 2;
                entry.mesh->closePath = true;
                for (Vector* v : tempPolygonVertices) {
                    entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
                }
                tempMeshes.push_back(entry);
                for (Vector* v : tempPolygonVertices) {
                    delete v;
                }
                tempPolygonVertices.clear();
                tempPolygonCanvasPoints.clear();
                QString logMsg = QString("Finalized polygon with %1 vertices via Escape key, centroid: (x: %2, y: %3)")
                                     .arg(entry.mesh->polygen.size())
                                     .arg(avgX)
                                     .arg(avgY);
                Console::log(logMsg.toStdString());
                setShapeDrawingMode(false, "");
                update();
            } else if (selectedShape == "Line" && tempLineVertices.size() >= 2) {
                static int polylineCounter = 0;
                MeshEntry entry;
                entry.name = QString("TempPolyline_%1").arg(polylineCounter++);
                float avgX = 0, avgY = 0;
                for (const Vector* v : tempLineVertices) {
                    avgX += v->x;
                    avgY += v->y;
                }
                avgX /= tempLineVertices.size();
                avgY /= tempLineVertices.size();
                entry.position = new QVector3D(avgX, avgY, 0);
                entry.rotation = new QQuaternion();
                entry.size = new QVector3D(1.0f, 1.0f, 1.0f);
                entry.velocity = new QVector3D(0, 0, 0);
                entry.trajectory = nullptr;
                entry.collider = nullptr;
                entry.mesh = new Mesh();
                if (!entry.mesh) {
                    Console::log("Error: Failed to allocate Mesh");
                    return;
                }
                entry.mesh->color = new QColor(Qt::red);
                if (!entry.mesh->color) {
                    Console::log("Error: Failed to allocate QColor");
                    delete entry.mesh;
                    return;
                }
                entry.mesh->lineWidth = 2;
                entry.mesh->closePath = false;
                for (Vector* v : tempLineVertices) {
                    entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
                }
                tempMeshes.push_back(entry);
                for (Vector* v : tempLineVertices) {
                    delete v;
                }
                tempLineVertices.clear();
                tempLineCanvasPoints.clear();
                QString logMsg = QString("Finalized polyline %1 with %2 vertices via Escape key")
                                     .arg(entry.name)
                                     .arg(entry.mesh->polygen.size());
                Console::log(logMsg.toStdString());
                setShapeDrawingMode(false, "");
                update();
            } else if (selectedShape == "Polygon" || selectedShape == "Line") {
                for (Vector* v : tempPolygonVertices) {
                    delete v;
                }
                tempPolygonVertices.clear();
                tempPolygonCanvasPoints.clear();
                for (Vector* v : tempLineVertices) {
                    delete v;
                }
                tempLineVertices.clear();
                tempLineCanvasPoints.clear();
                setShapeDrawingMode(false, "");
                Console::log("Shape drawing cancelled due to insufficient vertices");
                update();
            } else if (currentMode == PlaceBitmap) {
                isPlacingBitmap = false;
                selectedBitmapType = "";
                setTransformMode(Translate);
                Console::log("Exited PlaceBitmap mode");
            }
        }
    } else if (event->key() == Qt::Key_5) {
        setTransformMode(MeasureDistance);
        Console::log("Mode set to MeasureDistance");
    } else if (event->key() == Qt::Key_1) {
        setTransformMode(Translate);
        Console::log("Mode set to Translate");
    } else if (event->key() == Qt::Key_2) {
        setTransformMode(Rotate);
        Console::log("Mode set to Rotate");
    } else if (event->key() == Qt::Key_3) {
        setTransformMode(Scale);
        Console::log("Mode set to Scale");
    } else if (event->key() == Qt::Key_4) {
        simulate = !simulate;
        Console::log("Simulation mode: " + std::string(simulate ? "enabled" : "disabled"));
    } else if (event->key() == Qt::Key_Delete && currentMode == DrawTrajectory &&
               selectedWaypointIndex >= 0 && selectedWaypointIndex < (int)currentTrajectory.size()) {
        delete currentTrajectory[selectedWaypointIndex]->position;
        delete currentTrajectory[selectedWaypointIndex];
        currentTrajectory.erase(currentTrajectory.begin() + selectedWaypointIndex);
        deselectWaypoint();
        updateTrajectoryData();
        Console::log("Deleted selected waypoint");
        update();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (currentMode == DrawShape && (!tempPolygonCanvasPoints.empty() || !tempLineCanvasPoints.empty())) {
            if (selectedShape == "Polygon" && tempPolygonVertices.size() >= 3) {
                MeshEntry entry;
                entry.name = "TempPolygon";
                float avgX = 0, avgY = 0;
                for (const Vector* v : tempPolygonVertices) {
                    avgX += v->x;
                    avgY += v->y;
                }
                avgX /= tempPolygonVertices.size();
                avgY /= tempPolygonVertices.size();
                entry.position = new QVector3D(avgX, avgY, 0);
                entry.rotation = new QQuaternion();
                float minX = std::numeric_limits<float>::max();
                float maxX = std::numeric_limits<float>::lowest();
                float minY = std::numeric_limits<float>::max();
                float maxY = std::numeric_limits<float>::lowest();
                for (const Vector* v : tempPolygonVertices) {
                    minX = std::min(minX, v->x);
                    maxX = std::max(maxX, v->x);
                    minY = std::min(minY, v->y);
                    maxY = std::max(maxY, v->y);
                }
                entry.size = new QVector3D(1.0f, 1.0f, 1.0f);
                entry.velocity = new QVector3D(0, 0, 0);
                entry.trajectory = nullptr;
                entry.collider = nullptr;
                entry.mesh = new Mesh();
                if (!entry.mesh) {
                    Console::log("Error: Failed to allocate Mesh");
                    return;
                }
                entry.mesh->color = new QColor(Qt::red);
                if (!entry.mesh->color) {
                    Console::log("Error: Failed to allocate QColor");
                    delete entry.mesh;
                    return;
                }
                entry.mesh->lineWidth = 2;
                entry.mesh->closePath = true;
                for (Vector* v : tempPolygonVertices) {
                    entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
                }
                tempMeshes.push_back(entry);
                for (Vector* v : tempPolygonVertices) {
                    delete v;
                }
                tempPolygonVertices.clear();
                tempPolygonCanvasPoints.clear();
                QString logMsg = QString("Finalized polygon with %1 vertices via Enter key, centroid: (x: %2, y: %3)")
                                     .arg(entry.mesh->polygen.size())
                                     .arg(avgX)
                                     .arg(avgY);
                Console::log(logMsg.toStdString());
                setShapeDrawingMode(false, "");
                update();
            } else if (selectedShape == "Line" && tempLineVertices.size() >= 2) {
                static int polylineCounter = 0;
                MeshEntry entry;
                entry.name = QString("TempPolyline_%1").arg(polylineCounter++);
                float avgX = 0, avgY = 0;
                for (const Vector* v : tempLineVertices) {
                    avgX += v->x;
                    avgY += v->y;
                }
                avgX /= tempLineVertices.size();
                avgY /= tempLineVertices.size();
                entry.position = new QVector3D(avgX, avgY, 0);
                entry.rotation = new QQuaternion();
                entry.size = new QVector3D(1.0f, 1.0f, 1.0f);
                entry.velocity = new QVector3D(0, 0, 0);
                entry.trajectory = nullptr;
                entry.collider = nullptr;
                entry.mesh = new Mesh();
                if (!entry.mesh) {
                    Console::log("Error: Failed to allocate Mesh");
                    return;
                }
                entry.mesh->color = new QColor(Qt::red);
                if (!entry.mesh->color) {
                    Console::log("Error: Failed to allocate QColor");
                    delete entry.mesh;
                    return;
                }
                entry.mesh->lineWidth = 2;
                entry.mesh->closePath = false;
                for (Vector* v : tempLineVertices) {
                    entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
                }
                tempMeshes.push_back(entry);
                for (Vector* v : tempLineVertices) {
                    delete v;
                }
                tempLineVertices.clear();
                tempLineCanvasPoints.clear();
                QString logMsg = QString("Finalized polyline %1 with %2 vertices via Enter key")
                                     .arg(entry.name)
                                     .arg(entry.mesh->polygen.size());
                Console::log(logMsg.toStdString());
                setShapeDrawingMode(false, "");
                update();
            }
        }
    }
    update();
}

void CanvasWidget::handlePaint(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    drawGridLines(painter);
    drawTrajectory(painter);
    drawMesh(painter);
    drawImage(painter);
    drawSelectionOutline(painter);
    drawCollider(painter);
    drawAirbases(painter);// for preset layer

    // Draw selection outline for dragged shape
    if (isDraggingShape && !draggingShapeId.isEmpty()) {
        for (const auto& entry : tempMeshes) {
            if (entry.name == draggingShapeId) {
                painter.save();
                QPen pen(Qt::green, 2, Qt::DashLine);
                painter.setPen(pen);
                painter.setBrush(Qt::NoBrush);

                QPointF canvasPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());

                if (entry.name.startsWith("TempCircle")) {
                    // Calculate exact circle radius in canvas coordinates
                    QPointF radiusPointGeo(entry.position->x() + entry.size->x(), entry.position->y());
                    QPointF radiusPointCanvas = gislib->geoToCanvas(radiusPointGeo.y(), radiusPointGeo.x());
                    float canvasRadius = sqrt(pow(radiusPointCanvas.x() - canvasPos.x(), 2) +
                                              pow(radiusPointCanvas.y() - canvasPos.y(), 2));

                    painter.drawEllipse(canvasPos, canvasRadius, canvasRadius);
                }
                else if (entry.name.startsWith("TempRectangle")) {
                    // Calculate exact rectangle size in canvas coordinates
                    QPointF topLeftGeo(entry.position->x() - entry.size->x()/2, entry.position->y() - entry.size->y()/2);
                    QPointF bottomRightGeo(entry.position->x() + entry.size->x()/2, entry.position->y() + entry.size->y()/2);

                    QPointF topLeftCanvas = gislib->geoToCanvas(topLeftGeo.y(), topLeftGeo.x());
                    QPointF bottomRightCanvas = gislib->geoToCanvas(bottomRightGeo.y(), bottomRightGeo.x());

                    float width = bottomRightCanvas.x() - topLeftCanvas.x();
                    float height = bottomRightCanvas.y() - topLeftCanvas.y();

                    painter.drawRect(QRectF(topLeftCanvas.x(), topLeftCanvas.y(), width, height));
                }
                else if (entry.name.startsWith("TempPolygon") || entry.name.startsWith("TempPolyline")) {
                    QPolygonF polygon;
                    for (Vector* point : entry.mesh->polygen) {
                        // Convert relative vertices to absolute canvas coordinates
                        QPointF vertexGeo(point->x + entry.position->x(), point->y + entry.position->y());
                        QPointF vertexCanvas = gislib->geoToCanvas(vertexGeo.y(), vertexGeo.x());
                        polygon << vertexCanvas;
                    }
                    if (entry.name.startsWith("TempPolygon")) {
                        painter.drawPolygon(polygon);
                    } else {
                        painter.drawPolyline(polygon);
                    }
                }
                else if (entry.name.startsWith("TempPoint")) {
                    // Point is just a small circle around the position
                    painter.drawEllipse(canvasPos, 8, 8);
                }

                painter.restore();
                break;
            }
        }
    }

    // // NEW: Draw selection outline for dragged bitmap
    if (isDraggingBitmap && !draggingBitmapId.isEmpty()) {
        for (const auto& entry : tempMeshes) {
            if (entry.name == draggingBitmapId && !entry.bitmapPath.isEmpty()) {
                QPointF canvasPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
                float w = entry.size->x() * 25;
                float h = entry.size->y() * 25;

                painter.save();
                QPen pen(Qt::yellow, 2, Qt::DashLine);
                painter.setPen(pen);
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(QRectF(canvasPos.x() - w/2, canvasPos.y() - h/2, w, h));
                painter.restore();

                break;
            }
        }
    }


    if (currentMode == MeasureDistance && !measurePoints.empty()) {
        painter.save();
        QPen pen(Qt::yellow, 2, Qt::DashLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        QPolygonF poly;
        for (const auto& geo : measurePoints) {
            QPointF canvas = gislib->geoToCanvas(geo.y(), geo.x());
            poly << canvas;
            painter.setBrush(Qt::yellow);
            painter.drawEllipse(canvas, 5, 5);
            painter.setBrush(Qt::NoBrush);
        }
        if (poly.size() > 1) {
            painter.drawPolyline(poly);
        }
        painter.setFont(QFont("Arial", 10));
        painter.setPen(Qt::black);
        double factor = measureDialog ? measureDialog->getCurrentConversionFactor() : 1.0;
        QString unit = measureDialog ? measureDialog->getCurrentUnitString() : "m";
        for (int i = 1; i < measurePoints.size(); ++i) {
            double dist = gislib->calculateDistance(measurePoints[i-1], measurePoints[i]);
            QPointF mid = (poly[i-1] + poly[i]) / 2.0;
            QString text = QString("%1 %2").arg(dist * factor, 0, 'f', 2).arg(unit);
            painter.drawText(mid + QPointF(10, -10), text);
        }
        painter.restore();
    }

    if (isDrawingTrajectory && !currentTrajectory.empty()) {
        painter.save();
        QPen pen(Qt::magenta, 2, Qt::DashLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        QPolygonF polyline;
        for (size_t i = 0; i < currentTrajectory.size(); ++i) {
            Waypoints* waypoint = currentTrajectory[i];
            QPointF screenPos = gislib->geoToCanvas(waypoint->position->x, waypoint->position->z);
            polyline << screenPos;
            painter.setBrush(Qt::magenta);
            int pointSize = (i == selectedWaypointIndex) ? 6 : 4;
            QPen pointPen = (i == selectedWaypointIndex) ? QPen(Qt::yellow, 2) : QPen(Qt::magenta, 1);
            painter.setPen(pointPen);
            painter.drawEllipse(screenPos, pointSize, pointSize);
            painter.setBrush(Qt::NoBrush);
        }
        if (polyline.size() > 1) {
            painter.setPen(QPen(Qt::magenta, 2, Qt::DashLine));
            painter.drawPolyline(polyline);
        }
        painter.restore();
    }

    // Draw resize handles in EditShape mode
    if (currentMode == EditShape && !editingShapeId.isEmpty()) {
        painter.save();
        painter.setPen(QPen(Qt::blue, 2));
        painter.setBrush(Qt::blue);
        for (size_t i = 0; i < resizeHandles.size(); ++i) {
            painter.drawRect(QRectF(resizeHandles[i] - QPointF(4, 4), QSizeF(8, 8)));
            if (i == selectedHandleIndex) {
                painter.setPen(QPen(Qt::yellow, 2));
                painter.drawRect(QRectF(resizeHandles[i] - QPointF(5, 5), QSizeF(10, 10)));
                painter.setPen(QPen(Qt::blue, 2));
            }
        }
        painter.restore();
    }

    drawSceneInformation(painter);
    drawEntityInformation(painter);
    drawTransformGizmo(painter);

    frameCount++;
}

void CanvasWidget::keyPressEvent(QKeyEvent *event) {
    handleKeyPress(event);
}

void CanvasWidget::paintEvent(QPaintEvent *event) {
    handlePaint(event);
}

void CanvasWidget::mousePressEvent(QMouseEvent *event) {
    handleMousePress(event);
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *event) {
    handleMouseMove(event);
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *event) {
    handleMouseRelease(event);
}


void CanvasWidget::drawGridLines(QPainter& painter) {
    int alpha = (gridOpacity * 255) / 100;

    if (showXGrid) {
        painter.setPen(QColor(128, 128, 128, alpha));
        for (int x = 0; x < width(); x += 20)
            painter.drawLine(x, 0, x, height());
    }

    if (showYGrid) {
        painter.setPen(QColor(128, 128, 128, alpha));
        for (int y = 0; y < height(); y += 20)
            painter.drawLine(0, y, width(), y);
    }
}


void CanvasWidget::drawSceneInformation(QPainter& painter) {
    painter.save();
    painter.resetTransform();

    QFont font("Arial", 10, QFont::Bold);
    painter.setFont(font);
    painter.setPen(QColor(255, 255, 255, 200));

    if (showInformation) {
        QString modeText = QString(" %1").arg(simulate ? "Simulation" : "Editor");

        painter.drawText(QPointF(width() - 100, 20), modeText);
    }

    if (showFPS) {
        QString fpsText = QString("FPS: %1").arg(fps*10);

        painter.drawText(QPointF(10, 20), fpsText);
    }

    painter.restore();
}

void CanvasWidget::drawEntityInformation(QPainter& painter) {
    if (!showInformation || simulate || selectedEntityId.empty()) return;
    auto it = Meshes.find(selectedEntityId);
    if (it == Meshes.end()) return;

    painter.save();
    painter.resetTransform();

    QString name = it->second.name;
    auto& pos = it->second.position;
    QString text;

    if (currentMode == Translate || currentMode == DrawTrajectory) {
        text = QString("pos: lon %1, lat %2")
                   .arg(pos->x(), 0, 'f', 4) // Longitude
                   .arg(pos->y(), 0, 'f', 4); // Latitude
    } else if (currentMode == Rotate) {
        auto& rot = Meshes[selectedEntityId].rotation;
        text = QString("rot: x %1, y %2, z %3")
                   .arg(rot->toEulerAngles().x())
                   .arg(rot->toEulerAngles().y())
                   .arg(rot->toEulerAngles().z());
    } else if (currentMode == Scale) {
        auto& size = Meshes[selectedEntityId].size;
        text = QString("size: x %1, y %2, z %3")
                   .arg(size->x())
                   .arg(size->y())
                   .arg(size->z());
    }
    if (currentMode == DrawTrajectory) {
        text += QString(" | Waypoints: %1").arg(currentTrajectory.size());
    }


    QFont font("Arial", 10, QFont::Bold);
    painter.setFont(font);
    painter.setPen(QColor(255, 255, 255, 200));
    // Fixed screen coordinates ka upyog karein
    painter.drawText(QPointF(10, height() - 50), name);
    painter.drawText(QPointF(10, height() - 30), text);
    painter.drawText(QPointF(10, height() - 10), QString("Mode: %1")
                                                     .arg(currentMode == Translate ? "Translate" : currentMode == Rotate ? "Rotate" : currentMode == Scale ? "Scale" : "Draw Trajectory"));

    painter.restore();
}

void CanvasWidget::drawTransformGizmo(QPainter& painter) {
    if (simulate || currentMode == DrawTrajectory || selectedEntityId.empty()) return;
    auto it = Meshes.find(selectedEntityId);
    if (it == Meshes.end()) return;

    auto& entry = it->second;


    QPointF base = gislib->geoToCanvas(entry.transform->translation().x(), entry.transform->translation().z());


    QPointF xAxisHandlePos = base + QPointF(50, 0);
    QPointF yAxisHandlePos = base + QPointF(0, 50);


    if (currentMode == Translate) {

        painter.setPen(QPen(Qt::blue, (activeDragAxis == "x") ? 4 : 2));
        painter.drawLine(base, xAxisHandlePos);
        painter.setPen(QPen(Qt::green, (activeDragAxis == "y") ? 4 : 2));
        painter.drawLine(base, yAxisHandlePos);
    } else if (currentMode == Rotate) {
        painter.setPen(QPen(QColor(255, 255, 0, 150), 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(base, 40, 40);
        float radius = 40;

        float rad = qDegreesToRadians(-entry.transform->rotation().toEulerAngles().z());
        QPointF endpoint(base.x() + radius * cos(rad), base.y() - radius * sin(rad));
        painter.drawLine(base, endpoint);
    } else if (currentMode == Scale) {
        painter.setPen(QPen(Qt::red, 2));
        painter.setBrush(Qt::red);
        painter.drawRect(QRectF(base + QPointF(45, -3), QSizeF(10, 6)));
        painter.setBrush(Qt::green);
        painter.drawRect(QRectF(base + QPointF(-3, 45), QSizeF(6, 10)));
    }
}


void CanvasWidget::drawSelectionOutline(QPainter& painter) {
    if (!showOutline || simulate) return;
    if (Meshes.find(selectedEntityId) == Meshes.end()) return;

    auto& entry = Meshes[selectedEntityId];
    auto& pos = entry.position;
    auto& size = entry.size;

    float w = size->z() * 25;
    float h = size->x() * 25;
    QRectF outlineRect(pos->z() - w / 2.0f, pos->x() - h / 2.0f, w, h);

    QPen pen(Qt::yellow);
    pen.setWidth(2);
    pen.setStyle(Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(outlineRect);
}

void CanvasWidget::drawCollider(QPainter& painter) {
    if (!showColliders) return;
    for (const auto& [id, entry] : Meshes) {
        if (!entry.collider || !entry.position) continue;

        painter.save();
        painter.translate(entry.position->z(), entry.position->x());

        QPen pen(Qt::cyan);
        pen.setStyle(Qt::DashDotLine);
        pen.setWidth(2);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        using ColliderType = Constants::ColliderType;
        switch (entry.collider->collider) {
        case ColliderType::Sphere:
            painter.drawEllipse(QPointF(0, 0), entry.collider->Radius * entry.size->length(), entry.collider->Radius * entry.size->length());
            break;
        case ColliderType::Box:
            painter.drawRect(QRectF(-entry.collider->Width/2, -entry.collider->Height/2,
                                    entry.collider->Width, entry.collider->Height));
            break;
        case ColliderType::Cyclinder:
            painter.drawRoundedRect(QRectF(-entry.collider->Width/2, -entry.collider->Height/2,
                                           entry.collider->Width, entry.collider->Height), 10, 10);
            break;
        default:
            break;
        }

        painter.restore();
    }
}

void CanvasWidget::drawMesh(QPainter& painter) {
    if (!showMesh) {
        return;
    }
    if (!painter.isActive()) {
        return;
    }

    for (const auto& entry : tempMeshes) {
        Mesh* mesh = entry.mesh;
        if (!mesh || !mesh->color) {
            Console::log("Warning: Invalid mesh or color in tempMeshes for " + entry.name.toStdString());
            continue;
        }

        // Render text for TempText entries
        if (entry.name.startsWith("TempText") && !entry.text.isEmpty()) {
            QPointF canvasPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
            painter.save();
            QFont font("Arial", 12, QFont::Bold);
            painter.setFont(font);
            painter.setPen(*mesh->color);
            painter.drawText(canvasPos, entry.text);
            painter.restore();
            continue;
        }

        // Handle bitmap images
        if (!entry.bitmapPath.isEmpty()) {
            QPixmap img(entry.bitmapPath);
            if (!img.isNull()) {
                QPointF canvasPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
                QPointF sizePointGeo(entry.position->x() + entry.size->x(), entry.position->y() + entry.size->y());
                QPointF sizePointCanvas = gislib->geoToCanvas(sizePointGeo.y(), sizePointGeo.x());
                float canvasWidth = qAbs(sizePointCanvas.x() - canvasPos.x()) * 2;
                float canvasHeight = qAbs(sizePointCanvas.y() - canvasPos.y()) * 2;
                QPixmap scaled = img.scaled(canvasWidth, canvasHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                painter.save();
                painter.translate(canvasPos);
                painter.rotate(entry.rotation->z()*(180/3.14159265359));
                painter.drawPixmap(QPointF(-scaled.width() / 2.0f, -scaled.height() / 2.0f), scaled);
                painter.restore();
            } else {
                Console::log("Error: Failed to load bitmap image at path: " + entry.bitmapPath.toStdString());
            }
            continue;
        }

        // Also update point drawing:
        if (entry.name.startsWith("TempPoint")) {
            if (mesh->polygen.empty()) {
                Console::log("Warning: Point mesh has no position for " + entry.name.toStdString());
                continue;
            }

            // Get position from entry position (not from polygen)
            QPointF canvasPoint = gislib->geoToCanvas(entry.position->y(), entry.position->x());

            painter.setPen(QPen(*mesh->color, mesh->lineWidth, Qt::SolidLine));
            painter.setBrush(*mesh->color);
            painter.drawEllipse(canvasPoint, 3, 3);
            continue;
        }

        if (entry.name.startsWith("TempCircle")) {
            if (mesh->polygen.empty()) {
                Console::log("Warning: Circle mesh has no center point for " + entry.name.toStdString());
                continue;
            }

            // Get center position from the entry position (not from polygen)
            QPointF canvasCenter = gislib->geoToCanvas(entry.position->y(), entry.position->x());

            // Calculate radius in canvas coordinates
            QPointF radiusPointGeo(entry.position->x() + entry.size->x(), entry.position->y());
            QPointF radiusPointCanvas = gislib->geoToCanvas(radiusPointGeo.y(), radiusPointGeo.x());
            float canvasRadius = sqrt(pow(radiusPointCanvas.x() - canvasCenter.x(), 2) +
                                      pow(radiusPointCanvas.y() - canvasCenter.y(), 2));

            if (canvasRadius > 0 && canvasRadius < 10000) {
                painter.setPen(QPen(*mesh->color, mesh->lineWidth, Qt::SolidLine));
                painter.setBrush(QColor(255, 0, 0, 50));
                painter.drawEllipse(canvasCenter, canvasRadius, canvasRadius);
            }
            continue;
        }

        // Handle rectangles, polylines, and polygons
        QPolygonF polygon;
        QPointF canvasPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
        float angle = qDegreesToRadians(entry.rotation->z()*(180/3.14159265359));
        float cosA = cos(angle);
        float sinA = sin(angle);
        for (Vector* point : mesh->polygen) {
            if (!point) {
                Console::log("Warning: Null point in mesh for " + entry.name.toStdString());
                continue;
            }
            float geoX = point->x;
            float geoY = point->y;
            if (entry.name.startsWith("TempRectangle")) {
                geoX *= entry.size->x() / 0.02f;
                geoY *= entry.size->y() / 0.01f;
            }
            QPointF canvasPoint = gislib->geoToCanvas(geoY + entry.position->y(), geoX + entry.position->x());
            float x = canvasPoint.x() - canvasPos.x();
            float y = canvasPoint.y() - canvasPos.y();
            float rotatedX = x * cosA - y * sinA;
            float rotatedY = x * sinA + y * cosA;
            float finalX = canvasPos.x() + rotatedX;
            float finalY = canvasPos.y() + rotatedY;
            polygon << QPointF(finalX, finalY);

        }
        if (polygon.isEmpty()) {

            Console::log("Warning: Polygon/Polyline/Rectangle is empty, cannot draw for " + entry.name.toStdString());
            continue;
        }
        painter.setPen(QPen(*mesh->color, mesh->lineWidth, Qt::SolidLine));
        if (entry.name.startsWith("TempPolygon") || entry.name.startsWith("TempRectangle")) {
            painter.setBrush(QColor(255, 0, 0, 50));
            painter.drawPolygon(polygon);
        } else {
            painter.setBrush(Qt::NoBrush);
            painter.drawPolyline(polygon);
        }
    }

    if (!tempPolygonVertices.empty()) {
        QPolygonF previewPolygon;
        for (const Vector* vertex : tempPolygonVertices) {
            QPointF canvasPoint = gislib->geoToCanvas(vertex->y, vertex->x);
            previewPolygon << canvasPoint;

        }
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter.setBrush(QColor(255, 0, 0, 50));
        if (previewPolygon.size() > 1) {
            painter.drawPolyline(previewPolygon);
            for (const QPointF& point : previewPolygon) {
                painter.drawEllipse(point, 3, 3);
            }
        }
        if (previewPolygon.size() >= 3) {
            painter.drawPolygon(previewPolygon);

        }
    }

    if (!tempLineVertices.empty()) {
        QPolygonF previewLine;
        for (const Vector* vertex : tempLineVertices) {
            QPointF canvasPoint = gislib->geoToCanvas(vertex->y, vertex->x);
            previewLine << canvasPoint;

        }
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        if (previewLine.size() == 1) {
            painter.drawEllipse(previewLine[0], 3, 3);
        } else if (previewLine.size() >= 2) {
            painter.drawPolyline(previewLine);
            for (const QPointF& point : previewLine) {
                painter.drawEllipse(point, 3, 3);
            }

        }
    }
}

void CanvasWidget::drawImage(QPainter& painter) {
    for (auto& [id, entry] : Meshes) {
        Mesh* mesh = entry.mesh;
        if (!mesh) continue;

        QPixmap img(mesh->Sprite->data());
        if (!img.isNull()) {
            QPixmap scaled = img.scaled(entry.transform->scale3D().z() * 50, entry.transform->scale3D().x() * 50,
                                        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            QPointF point = gislib->geoToCanvas(entry.transform->translation().x(), entry.transform->translation().z());
            float x = point.x();
            float y = point.y();
            float angle = entry.transform->rotation().toEulerAngles().y();

            painter.save();
            painter.translate(x, y);
            painter.rotate(-angle);
            painter.drawPixmap(QPointF(-scaled.width() / 2.0f, -scaled.height() / 2.0f), scaled);
            painter.restore();
        } else {
            qDebug() << "Image load failed!";
        }
    }
}

void CanvasWidget::drawTrajectory(QPainter& painter) {
    for (const auto& [id, entry] : Meshes) {
        // Skip if trajectory is null or not active
        if (!entry.trajectory || !entry.trajectory->Active || entry.trajectory->Trajectories.empty()) {

            continue;
        }

        painter.save();
        QPen pen(Qt::cyan, 2);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        QPolygonF polyline;

        // Iterate through waypoints and convert to canvas coordinates
        for (const Waypoints* waypoint : entry.trajectory->Trajectories) {
            if (!waypoint || !waypoint->position) {
                Console::log("Warning: Null waypoint or position in trajectory for entity: " + id);
                continue;
            }
            QPointF point = gislib->geoToCanvas(waypoint->position->x, waypoint->position->z);
            polyline << point;
            painter.setBrush(Qt::cyan);
            painter.drawEllipse(point, 3, 3);
            painter.setBrush(Qt::NoBrush);
        }

        // Draw polyline only if there are multiple waypoints
        if (polyline.size() > 1) {
            painter.drawPolyline(polyline);
            //Console::log("Drawing trajectory for entity: " + id + " with " +std::to_string(polyline.size()) + " waypoints");
        } else {
            Console::log("Not enough waypoints to draw trajectory for entity: " + id);
        }

        painter.restore();
    }
}

void CanvasWidget::toggleLayerVisibility(const QString& layer, bool visible) {
    if (layer == "Collider") {
        showColliders = visible;
    } else if (layer == "Mesh") {
        showMesh = visible;
    } else if (layer == "Outline") {
        showOutline = visible;
    } else if (layer == "Information") {
        showInformation = visible;
    } else if (layer == "FPS") {
        showFPS = visible;
    }
    update();
}

void CanvasWidget::setShapeDrawingMode(bool enabled, const QString& shapeType) {
    isDrawingTrajectory = false;
    selectedShape = shapeType;
    if (enabled) {
        // Don't set currentMode to DrawShape if we're in EditShape mode
        if (currentMode != EditShape) {
            currentMode = DrawShape;
        }
        setCursor(Qt::CrossCursor);
        Console::log("Shape drawing mode enabled: " + shapeType.toStdString());

        // Clear temporary data only for multi-point shapes
        if (shapeType == "Polygon") {
            for (Vector* v : tempPolygonVertices) {
                delete v;
            }
            tempPolygonVertices.clear();
            tempPolygonCanvasPoints.clear();
            Console::log("Cleared previous polygon vertices");
        } else if (shapeType == "Line") {
            for (Vector* v : tempLineVertices) {
                delete v;
            }
            tempLineVertices.clear();
            tempLineCanvasPoints.clear();
            Console::log("Cleared previous line vertices");
        }
    } else {
        // Only exit DrawShape mode if we're not in EditShape mode
        if (currentMode == DrawShape) {
            currentMode = Translate;
        }
        selectedShape = "";
        setCursor(Qt::ArrowCursor);
        Console::log("Shape drawing mode disabled");

        // Clear temporary data
        for (Vector* v : tempPolygonVertices) {
            delete v;
        }
        tempPolygonVertices.clear();
        tempPolygonCanvasPoints.clear();

        for (Vector* v : tempLineVertices) {
            delete v;
        }
        tempLineVertices.clear();
        tempLineCanvasPoints.clear();
    }
    update();
}

void CanvasWidget::onBitmapImageSelected(const QString& filePath) {
    static int userImageCounter = 0;
    MeshEntry entry;
    entry.name = QString("UserImage_%1").arg(userImageCounter++); // Different prefix
    QPointF centerCanvas(width() / 2.0f, height() / 2.0f);
    QPointF geoPos = gislib->canvasToGeo(centerCanvas);
    entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
    entry.rotation = new QQuaternion();
    entry.size = new QVector3D(8.0, 8.0, 1); // Slightly larger size for user images
    entry.velocity = new QVector3D(0, 0, 0);
    entry.trajectory = nullptr;
    entry.collider = nullptr;
    entry.bitmapPath = filePath;
    entry.mesh = new Mesh();
    if (!entry.mesh) return;
    entry.mesh->color = new QColor(Qt::white);
    entry.mesh->lineWidth = 1;
    entry.mesh->closePath = false;

    tempMeshes.push_back(entry);

    QString logMsg = QString("USER UPLOADED IMAGE: %1 at (lon: %2, lat: %3)")
                         .arg(entry.name)
                         .arg(geoPos.x(), 0, 'f', 6)
                         .arg(geoPos.y(), 0, 'f', 6);
    Console::log(logMsg.toStdString());

    update();
}

QString CanvasWidget::getBitmapImagePath(const QString& bitmapType) {
    QMap<QString, QString> bitmapMap = {
        {"Hospital", ":/icons/images/hospital.png"},
        {"School", ":/icons/images/school.png"},
        {"Forest Area", ":/icons/images/forest-area.png"}
    };
    return bitmapMap.value(bitmapType, "");
}

void CanvasWidget::onBitmapSelected(const QString& bitmapType) {
    selectedBitmapType = bitmapType;
    isPlacingBitmap = true;
    currentMode = PlaceBitmap;
    setCursor(Qt::CrossCursor);
    Console::log("Bitmap placement mode enabled for: " + bitmapType.toStdString());
    update();
}

void CanvasWidget::deselectWaypoint() {
    selectedWaypointIndex = -1;
    isDraggingWaypoint = false;
    Console::log("Deselected waypoint");
    update();
}

int CanvasWidget::findNearestWaypoint(QPointF canvasPos) {
    const float tolerance = 10.0f; // Pixel tolerance for selecting a waypoint
    int nearestIndex = -1;
    float minDistance = std::numeric_limits<float>::max();

    for (size_t i = 0; i < currentTrajectory.size(); ++i) {
        Waypoints* wp = currentTrajectory[i];
        QPointF wpCanvas = gislib->geoToCanvas(wp->position->x, wp->position->z);
        float distance = QVector2D(canvasPos - wpCanvas).length();
        if (distance < tolerance && distance < minDistance) {
            minDistance = distance;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

void CanvasWidget::updateTrajectoryData() {
    if (selectedEntityId.empty()) {
        Console::error("Cannot update trajectory: No entity selected");
        return;
    }

    QJsonArray waypointsArray;
    for (const Waypoints* wp : currentTrajectory) {
        QJsonObject wpObj;
        QJsonObject posObj;
        posObj["type"] = "vector";
        posObj["x"] = wp->position->x;
        posObj["y"] = wp->position->y;
        posObj["z"] = wp->position->z;
        wpObj["position"] = posObj;
        waypointsArray.append(wpObj);
    }

    emit trajectoryUpdated(QString::fromStdString(selectedEntityId), waypointsArray);
    Console::log("Updated trajectory data for entity: " + selectedEntityId + " with " +
                 std::to_string(currentTrajectory.size()) + " waypoints");
    QJsonDocument doc(waypointsArray);

}

QJsonObject CanvasWidget::toJson() const {
    QJsonObject json;

    json["selectedBitmapType"] = selectedBitmapType;
    json["isPlacingBitmap"] = isPlacingBitmap;

    QJsonArray polygonVerticesArray;
    for (const Vector* vertex : tempPolygonVertices) {
        QJsonObject vObj;
        vObj["x"] = vertex->x;
        vObj["y"] = vertex->y;
        vObj["z"] = vertex->z;
        polygonVerticesArray.append(vObj);
    }
    json["tempPolygonVertices"] = polygonVerticesArray;

    QJsonArray polygonCanvasPointsArray;
    for (const QPointF& point : tempPolygonCanvasPoints) {
        QJsonObject pObj;
        pObj["x"] = point.x();
        pObj["y"] = point.y();
        polygonCanvasPointsArray.append(pObj);
    }
    json["tempPolygonCanvasPoints"] = polygonCanvasPointsArray;

    QJsonArray lineVerticesArray;
    for (const Vector* vertex : tempLineVertices) {
        QJsonObject vObj;
        vObj["x"] = vertex->x;
        vObj["y"] = vertex->y;
        vObj["z"] = vertex->z;
        lineVerticesArray.append(vObj);
    }
    json["tempLineVertices"] = lineVerticesArray;

    QJsonArray lineCanvasPointsArray;
    for (const QPointF& point : tempLineCanvasPoints) {
        QJsonObject pObj;
        pObj["x"] = point.x();
        pObj["y"] = point.y();
        lineCanvasPointsArray.append(pObj);
    }
    json["tempLineCanvasPoints"] = lineCanvasPointsArray;

    QJsonArray tempMeshesArray;
    for (const MeshEntry& entry : tempMeshes) {
        QJsonObject meshObj;
        meshObj["name"] = entry.name;
        meshObj["text"] = entry.text; // Serialize the text field for TempText entr

        QJsonObject posObj;
        posObj["x"] = entry.position->x();
        posObj["y"] = entry.position->y();
        posObj["z"] = entry.position->z();
        meshObj["position"] = posObj;

        QJsonObject rotObj;
        rotObj["x"] = entry.rotation->x();
        rotObj["y"] = entry.rotation->y();
        rotObj["z"] = entry.rotation->z();
        meshObj["rotation"] = rotObj;

        QJsonObject sizeObj;
        sizeObj["x"] = entry.size->x();
        sizeObj["y"] = entry.size->y();
        sizeObj["z"] = entry.size->z();
        meshObj["size"] = sizeObj;

        QJsonObject velObj;
        velObj["x"] = entry.velocity->x();
        velObj["y"] = entry.velocity->y();
        velObj["z"] = entry.velocity->z();
        meshObj["velocity"] = velObj;

        if (!entry.bitmapPath.isEmpty()) {
            meshObj["bitmapPath"] = entry.bitmapPath;
        }

        if (entry.mesh) {
            QJsonObject meshData;
            meshData["lineWidth"] = entry.mesh->lineWidth;
            meshData["closePath"] = entry.mesh->closePath;
            if (entry.mesh->color) {
                QJsonObject colorObj;
                colorObj["r"] = entry.mesh->color->red();
                colorObj["g"] = entry.mesh->color->green();
                colorObj["b"] = entry.mesh->color->blue();
                colorObj["a"] = entry.mesh->color->alpha();
                meshData["color"] = colorObj;
            }
            QJsonArray polygenArray;
            for (const Vector* point : entry.mesh->polygen) {
                QJsonObject pointObj;
                pointObj["x"] = point->x;
                pointObj["y"] = point->y;
                pointObj["z"] = point->z;
                polygenArray.append(pointObj);
            }
            meshData["polygen"] = polygenArray;
            meshObj["mesh"] = meshData;
        }

        tempMeshesArray.append(meshObj);
    }
    json["tempMeshes"] = tempMeshesArray;

    // use airbase layer data by waris
    json["showAirbases"] = showAirbases;
    QJsonArray airbasesArray;
    for (const auto& pos : airbasePositions) {
        QJsonObject airbase;
        airbase["lon"] = pos.first;
        airbase["lat"] = pos.second;
        airbasesArray.append(airbase);
    }
    json["airbasePositions"] = airbasesArray;

    return json;
}

void CanvasWidget::fromJson(const QJsonObject& json) {
    for (Waypoints* wp : currentTrajectory) {
        delete wp->position;
        delete wp;
    }
    currentTrajectory.clear();

    for (Vector* v : tempPolygonVertices) {
        delete v;
    }
    tempPolygonVertices.clear();
    tempPolygonCanvasPoints.clear();

    for (Vector* v : tempLineVertices) {
        delete v;
    }
    tempLineVertices.clear();
    tempLineCanvasPoints.clear();

    for (MeshEntry& entry : tempMeshes) {
        delete entry.position;
        delete entry.rotation;
        delete entry.size;
        delete entry.velocity;
        if (entry.mesh) {
            for (Vector* v : entry.mesh->polygen) {
                delete v;
            }
            delete entry.mesh->color;
            delete entry.mesh;
        }
        delete entry.collider;
        delete entry.trajectory;
    }
    tempMeshes.clear();

    // use for save data in json for airbase
    airbasePositions.clear();
    showAirbases = false;

    // Restore airbase layer data
    if (json.contains("showAirbases")) {
        showAirbases = json["showAirbases"].toBool();
    }
    if (json.contains("airbasePositions") && json["airbasePositions"].isArray()) {
        QJsonArray airbasesArray = json["airbasePositions"].toArray();
        for (const QJsonValue& value : airbasesArray) {
            if (value.isObject()) {
                QJsonObject airbase = value.toObject();
                double lon = airbase["lon"].toDouble();
                double lat = airbase["lat"].toDouble();
                airbasePositions.emplace_back(lon, lat);
            }
        }
    }

    if (json.contains("selectedBitmapType")) {
        selectedBitmapType = json["selectedBitmapType"].toString();
    }

    if (json.contains("isPlacingBitmap")) {
        isPlacingBitmap = json["isPlacingBitmap"].toBool();
    }

    if (json.contains("tempPolygonVertices") && json["tempPolygonVertices"].isArray()) {
        QJsonArray verticesArray = json["tempPolygonVertices"].toArray();
        for (const QJsonValue& val : verticesArray) {
            QJsonObject vObj = val.toObject();
            tempPolygonVertices.push_back(new Vector(
                vObj["x"].toDouble(),
                vObj["y"].toDouble(),
                vObj["z"].toDouble()
                ));
        }
    }

    if (json.contains("tempPolygonCanvasPoints") && json["tempPolygonCanvasPoints"].isArray()) {
        QJsonArray pointsArray = json["tempPolygonCanvasPoints"].toArray();
        for (const QJsonValue& val : pointsArray) {
            QJsonObject pObj = val.toObject();
            tempPolygonCanvasPoints.push_back(QPointF(
                pObj["x"].toDouble(),
                pObj["y"].toDouble()
                ));
        }
    }

    if (json.contains("tempLineVertices") && json["tempLineVertices"].isArray()) {
        QJsonArray verticesArray = json["tempLineVertices"].toArray();
        for (const QJsonValue& val : verticesArray) {
            QJsonObject vObj = val.toObject();
            tempLineVertices.push_back(new Vector(
                vObj["x"].toDouble(),
                vObj["y"].toDouble(),
                vObj["z"].toDouble()
                ));
        }
    }

    if (json.contains("tempLineCanvasPoints") && json["tempLineCanvasPoints"].isArray()) {
        QJsonArray pointsArray = json["tempLineCanvasPoints"].toArray();
        for (const QJsonValue& val : pointsArray) {
            QJsonObject pObj = val.toObject();
            tempLineCanvasPoints.push_back(QPointF(
                pObj["x"].toDouble(),
                pObj["y"].toDouble()
                ));
        }
    }

    if (json.contains("tempMeshes") && json["tempMeshes"].isArray()) {
        QJsonArray tempMeshesArray = json["tempMeshes"].toArray();
        for (const QJsonValue& val : tempMeshesArray) {
            QJsonObject meshObj = val.toObject();
            MeshEntry entry;

            entry.name = meshObj["name"].toString();
            entry.text = meshObj["text"].toString(); // Deserialize text field for TempText entries

            if (meshObj.contains("position")) {
                QJsonObject posObj = meshObj["position"].toObject();
                entry.position = new QVector3D(
                    posObj["x"].toDouble(),
                    posObj["y"].toDouble(),
                    posObj["z"].toDouble()
                    );
            } else {
                entry.position = new QVector3D(0, 0, 0);
            }

            if (meshObj.contains("rotation")) {
                QJsonObject rotObj = meshObj["rotation"].toObject();
                entry.rotation = new QQuaternion(QQuaternion::fromEulerAngles(
                    rotObj["x"].toDouble(),
                    rotObj["y"].toDouble(),
                    rotObj["z"].toDouble()
                    ));
            } else {
                entry.rotation = new QQuaternion();
            }

            if (meshObj.contains("size")) {
                QJsonObject sizeObj = meshObj["size"].toObject();
                entry.size = new QVector3D(
                    sizeObj["x"].toDouble(),
                    sizeObj["y"].toDouble(),
                    sizeObj["z"].toDouble()
                    );
            } else {
                entry.size = new QVector3D(1, 1, 1);
            }

            if (meshObj.contains("velocity")) {
                QJsonObject velObj = meshObj["velocity"].toObject();
                entry.velocity = new QVector3D(
                    velObj["x"].toDouble(),
                    velObj["y"].toDouble(),
                    velObj["z"].toDouble()
                    );
            } else {
                entry.velocity = new QVector3D(0, 0, 0);
            }

            if (meshObj.contains("bitmapPath")) {
                entry.bitmapPath = meshObj["bitmapPath"].toString();
            }

            if (meshObj.contains("mesh")) {
                QJsonObject meshData = meshObj["mesh"].toObject();
                entry.mesh = new Mesh();
                entry.mesh->lineWidth = meshData["lineWidth"].toInt();
                entry.mesh->closePath = meshData["closePath"].toBool();
                if (meshData.contains("color")) {
                    QJsonObject colorObj = meshData["color"].toObject();
                    entry.mesh->color = new QColor(
                        colorObj["r"].toInt(),
                        colorObj["g"].toInt(),
                        colorObj["b"].toInt(),
                        colorObj["a"].toInt()
                        );
                } else {
                    entry.mesh->color = new QColor(Qt::red);
                }
                if (meshData.contains("polygen")) {
                    QJsonArray polygenArray = meshData["polygen"].toArray();
                    for (const QJsonValue& pointVal : polygenArray) {
                        QJsonObject pointObj = pointVal.toObject();
                        entry.mesh->polygen.push_back(new Vector(
                            pointObj["x"].toDouble(),
                            pointObj["y"].toDouble(),
                            pointObj["z"].toDouble()
                            ));
                    }
                }
            } else {
                entry.mesh = nullptr;
            }

            entry.collider = nullptr;
            entry.trajectory = nullptr;

            tempMeshes.push_back(entry);
        }
    }

    update();
}

void CanvasWidget::onDistanceMeasured(double distance, QPointF startPoint, QPointF endPoint) {
    double distanceKilometers = distance / 1000.0; // Convert to kilometers
    QString msg = QString("Measured distance: %1 meters (%2 km) from (lon: %3, lat: %4) to (lon: %5, lat: %6)")
                      .arg(distance, 0, 'f', 2)
                      .arg(distanceKilometers, 0, 'f', 2)
                      .arg(startPoint.x(), 0, 'f', 6)
                      .arg(startPoint.y(), 0, 'f', 6)
                      .arg(endPoint.x(), 0, 'f', 6)
                      .arg(endPoint.y(), 0, 'f', 6);
    Console::log(msg.toStdString());
    if (measureDialog) {
        measureDialog->raise(); // Ensure dialog stays on top
    }
    QMessageBox::information(this, "Distance Measured", msg);
}

// preset layers for airbase by waris
void CanvasWidget::onPresetLayerSelected(const QString& preset) {
    if (preset == "Airbase") {
        if (!showAirbases) {
            loadAirbaseData();
            showAirbases = true;
            Console::log("Airbase layer enabled: Rendering " + std::to_string(airbasePositions.size()) + " airbases");
        } else {
            showAirbases = false;
            Console::log("Airbase layer disabled");
        }
        update();
    }
}

// preset layers for airbase by waris
void CanvasWidget::loadAirbaseData() {
    if (!airbasePositions.empty()) return;  // Already loaded

    // Static data from sources [web:0, web:2, web:5, web:8]
    airbasePositions = {
        {77.4301, 28.7172},  // Hindon
        {76.7770, 30.3020},  // Ambala
        {75.3800, 31.5400},  // Adampur
        {70.0420, 22.5270},  // Jamnagar
        {73.0219, 26.2842},  // Jodhpur
        {78.1670, 26.2530},  // Gwalior
        {78.0133, 27.1556},  // Agra
        {79.4200, 28.3670},  // Bareilly
        {74.9830, 33.8830},  // Awantipur
        {77.6270, 34.0827},  // Leh
        {74.7833, 34.0833},  // Srinagar
        {73.9200, 18.5800},  // Pune
        {77.1500, 10.6750},  // Sulur
        {76.9167, 8.4833},   // Thiruvananthapuram
        {94.4670, 26.6170},  // Jorhat
        {87.2830, 22.1500},  // Kalaikunda
        {88.3500, 22.7500},  // Barrackpore
        {89.3500, 26.7500},  // Hasimara
        {92.8167, 9.1500},   // Car Nicobar
        {70.0420, 22.5270},   // Jamnagar (example duplicate)

        // Pakistan Airbases [web:1, web:3]
        {73.0992, 33.6027},  // PAF Base Mushaf, Sargodha, Punjab
        {71.9789, 33.8730},  // PAF Base Minhas, Kamra, Punjab
        {73.0500, 31.3667},  // PAF Base Rafiqui, Shorkot, Punjab
        {71.5200, 33.9667},  // PAF Base Peshawar, Khyber Pakhtunkhwa
        {67.0333, 24.8936},  // PAF Base Masroor, Karachi, Sindh
        {72.2667, 30.1833},  // PAF Base Multan, Punjab
        {74.4036, 31.6119},  // PAF Base Lahore, Punjab
        {70.8967, 30.2033},  // PAF Base Dera Ghazi Khan, Punjab
        {66.9375, 25.2764},  // PAF Base Samungli, Quetta, Balochistan
        {69.2667, 28.9667}   // PAF Base Shahbaz, Jacobabad, Sindh
    };
    Console::log("Loaded " + std::to_string(airbasePositions.size()) + " Indian and Pakistan airbases");
}

// preset layers for airbase by waris
void CanvasWidget::drawAirbases(QPainter& painter) {
    if (!showAirbases || airbasePositions.empty() || airbasePixmap.isNull()) {
        return;
    }
    painter.save();
    const int iconSize = 20;
    QPixmap scaledIcon = airbasePixmap.scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    for (const auto& pos : airbasePositions) {
        double lon = pos.first;
        double lat = pos.second;
        QPointF canvasPos = gislib->geoToCanvas(lat, lon);
        painter.drawPixmap(QPointF(canvasPos.x() - iconSize / 2.0, canvasPos.y() - iconSize / 2.0), scaledIcon);
    }
    painter.restore();
    Console::log("Rendered " + std::to_string(airbasePositions.size()) + " airbase icons");
}
// for geojson function
void CanvasWidget::importGeoJsonLayer(const QString &filePath) {
    if (!gislib) {
        Console::error("GISlib not available for GeoJSON import");
        return;
    }

    Console::log("Importing GeoJSON layer from: " + filePath.toStdString());

    // Forward the import request to GISlib
    gislib->importGeoJsonLayer(filePath);

    // Track the imported layer
    QString layerName = QFileInfo(filePath).completeBaseName();
    geoJsonLayers[layerName] = true; // Set layer as visible by default
    Console::log("GeoJSON layer imported successfully: " + layerName.toStdString());

    // Emit signal to notify UI about new layer
    emit geoJsonLayerAdded(layerName);

    // Trigger update to render the new layer
    update();
}
/* Add new slot in CanvasWidget.cpp */
void CanvasWidget::onGeoJsonLayerToggled(const QString &layerName, bool visible) {
    if (gislib) {
        gislib->toggleVectorLayerVisibility(layerName, visible);
        geoJsonLayers[layerName] = visible;
        Console::log("Toggled GeoJSON layer '" + layerName.toStdString() + "' to " + (visible ? "visible" : "hidden"));
        update();
    } else {
        Console::error("Cannot toggle GeoJSON layer: GISlib not initialized");
    }
}

void CanvasWidget::onMeasurementTypeChanged(bool isEll) {
    gislib->setEllipsoidal(isEll);
    measureDialog->clearMeasurements();
    for (int i = 1; i < measurePoints.size(); ++i) {
        double dist = gislib->calculateDistance(measurePoints[i-1], measurePoints[i]);
        QPointF curr = measurePoints[i];
        measureDialog->addMeasurement(curr.x(), curr.y(), dist);
    }
    update();
}

void CanvasWidget::startDistanceMeasurement() {
    setTransformMode(MeasureDistance);
    if (measureDialog) measureDialog->clearMeasurements();
    measurePoints.clear();
    Console::log("Distance measurement mode started");
}

void CanvasWidget::addMeasurePoint(double lon, double lat) {
    if (currentMode != MeasureDistance) startDistanceMeasurement();
    QPointF geo(lon, lat);
    measurePoints.append(geo);

    // Add measurement to dialog
    if (measureDialog && measurePoints.size() >= 2) {
        double dist = gislib->calculateDistance(measurePoints[measurePoints.size() - 2],
                                                measurePoints.last());
        measureDialog->addMeasurement(lon, lat, dist);
    }
    update();
}

double CanvasWidget::getLastSegmentDistance() const {
    if (measurePoints.size() < 2) return 0.0;
    return gislib->calculateDistance(measurePoints[measurePoints.size() - 2],
                                     measurePoints.last());
}

double CanvasWidget::getTotalDistance() const {
    double total = 0.0;
    for (int i = 1; i < measurePoints.size(); ++i)
        total += gislib->calculateDistance(measurePoints[i-1], measurePoints[i]);
    return total;
}

void CanvasWidget::clearMeasurementPoints() {
    measurePoints.clear();
    if (measureDialog) measureDialog->clearMeasurements();
    update();
}

void CanvasWidget::setMeasurementUnit(const QString &unit) {
    measurementUnit = unit;

    if (unit == "m") conversionFactor = 1.0;
    else if (unit == "km") conversionFactor = 0.001;
    else if (unit == "ft") conversionFactor = 3.28084;
    else if (unit == "mi") conversionFactor = 0.000621371;
    else if (unit == "deg") conversionFactor = 1.0; // optional for degrees

    update(); // triggers repaint using the new unit
}
bool CanvasWidget::handleBitmapSelection(QMouseEvent *event) {
    const qreal selectionTolerance = 25.0;

    // Only preset bitmaps check karo (Hospital, School, Forest Area)
    for (auto& entry : tempMeshes) {
        if (!entry.name.startsWith("UserImage_") && !entry.bitmapPath.isEmpty()) {
            QPointF bitmapCenterCanvas = gislib->geoToCanvas(entry.position->y(), entry.position->x());

            float w = entry.size->x() * 25;
            float h = entry.size->y() * 25;

            QRectF bitmapRect(
                bitmapCenterCanvas.x() - w/2,
                bitmapCenterCanvas.y() - h/2,
                w,
                h
                );

            if (bitmapRect.contains(event->pos())) {
                isDraggingBitmap = true;
                draggingBitmapId = entry.name;
                bitmapDragStartPos = event->pos();

                setCursor(Qt::ClosedHandCursor);
                Console::log("Preset bitmap selected for dragging: " + entry.name.toStdString());

                selectedEntityId = "";
                activeDragAxis = "";
                isDraggingUserImage = false; // User image dragging cancel

                update();
                return true;
            }
        }
    }

    return false;
}

// Bitmap dragging implementation
void CanvasWidget::handleBitmapDragging(QMouseEvent *event) {
    // Temporary bitmaps mein search karo
    for (auto& entry : tempMeshes) {
        if (entry.name == draggingBitmapId && !entry.bitmapPath.isEmpty()) {
            // Calculate drag delta
            QPointF delta = event->pos() - bitmapDragStartPos;

            // Convert delta to geographic coordinates
            QPointF currentGeoPos = gislib->canvasToGeo(bitmapDragStartPos);
            QPointF newGeoPos = gislib->canvasToGeo(event->pos());

            // Update bitmap position
            entry.position->setX(newGeoPos.x()); // Longitude
            entry.position->setY(newGeoPos.y()); // Latitude

            // Update drag start position for smooth dragging
            bitmapDragStartPos = event->pos();

            Console::log("Dragging bitmap: " + entry.name.toStdString() +
                         " to (lon: " + std::to_string(newGeoPos.x()) +
                         ", lat: " + std::to_string(newGeoPos.y()) + ")");

            update();
            return;
        }
    }

    // Agar bitmap nahi mila to dragging stop karo
    stopBitmapDragging();
}

// Bitmap dragging stop function
void CanvasWidget::stopBitmapDragging() {
    if (isDraggingBitmap) {
        Console::log("Stopped dragging bitmap: " + draggingBitmapId.toStdString());
        isDraggingBitmap = false;
        draggingBitmapId = "";
        setCursor(Qt::ArrowCursor);
        update();
    }
}
void CanvasWidget::stopUserImageDragging() {
    if (isDraggingUserImage) {
        Console::log("Stopped dragging user image: " + draggingUserImageId.toStdString());
        isDraggingUserImage = false;
        draggingUserImageId = "";
        setCursor(Qt::ArrowCursor);
        update();
    }
}
void CanvasWidget::handleUserImageDragging(QMouseEvent *event) {
    if (!isDraggingUserImage || draggingUserImageId.isEmpty()) return;

    for (auto& entry : tempMeshes) {
        if (entry.name == draggingUserImageId && entry.name.startsWith("UserImage_")) {
            QPointF delta = event->pos() - userImageDragStartPos;
            QPointF currentCanvasPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
            QPointF newCanvasPos = currentCanvasPos + delta;
            QPointF newGeoPos = gislib->canvasToGeo(newCanvasPos);

            // Update position
            entry.position->setX(newGeoPos.x()); // Longitude
            entry.position->setY(newGeoPos.y()); // Latitude

            userImageDragStartPos = event->pos();

            QString posInfo = QString("Dragging user image: %1 to (lon: %2, lat: %3)")
                                  .arg(entry.name)
                                  .arg(newGeoPos.x(), 0, 'f', 6)
                                  .arg(newGeoPos.y(), 0, 'f', 6);
            Console::log(posInfo.toStdString());

            update();
            return;
        }
    }

    stopUserImageDragging();
}

bool CanvasWidget::handleUserImageSelection(QMouseEvent *event) {
    const qreal selectionTolerance = 25.0;

    // Only user-uploaded images check karo (jo "UserImage_" se start hote hain)
    for (auto& entry : tempMeshes) {
        if (entry.name.startsWith("UserImage_") && !entry.bitmapPath.isEmpty()) {
            QPointF imageCenterCanvas = gislib->geoToCanvas(entry.position->y(), entry.position->x());

            float w = entry.size->x() * 25;
            float h = entry.size->y() * 25;

            QRectF imageRect(
                imageCenterCanvas.x() - w/2,
                imageCenterCanvas.y() - h/2,
                w,
                h
                );

            if (imageRect.contains(event->pos())) {
                isDraggingUserImage = true;
                draggingUserImageId = entry.name;
                userImageDragStartPos = event->pos();

                setCursor(Qt::ClosedHandCursor);
                Console::log("User image selected for dragging: " + entry.name.toStdString());

                // Deselect other entities
                selectedEntityId = "";
                activeDragAxis = "";
                isDraggingBitmap = false; // Preset bitmap dragging cancel

                update();
                return true;
            }
        }
    }

    return false;
}

bool CanvasWidget::handleShapeSelection(QMouseEvent *event) {
    const qreal selectionTolerance = 10.0;

    // Only prevent selection if we're in DrawShape mode for Line or Polygon
    if (currentMode == DrawShape && (selectedShape == "Line" || selectedShape == "Polygon")) {
        return false;
    }

    // Check all temporary shapes
    for (auto& entry : tempMeshes) {
        if (entry.name.startsWith("Temp") && !entry.name.startsWith("TempText") &&
            !entry.name.startsWith("TempBitmap") && entry.bitmapPath.isEmpty()) {

            QPointF shapeCenterCanvas = gislib->geoToCanvas(entry.position->y(), entry.position->x());
            bool isHit = false;

            // Check different shape types for hit detection
            if (entry.name.startsWith("TempCircle")) {
                // Circle hit detection
                QPointF radiusPointGeo(entry.position->x() + entry.size->x(), entry.position->y());
                QPointF radiusPointCanvas = gislib->geoToCanvas(radiusPointGeo.y(), radiusPointGeo.x());
                float canvasRadius = sqrt(pow(radiusPointCanvas.x() - shapeCenterCanvas.x(), 2) +
                                          pow(radiusPointCanvas.y() - shapeCenterCanvas.y(), 2));

                float distance = QVector2D(event->pos() - shapeCenterCanvas).length();
                isHit = (distance <= canvasRadius + selectionTolerance);
            }
            else if (entry.name.startsWith("TempRectangle")) {
                // Rectangle hit detection
                QPointF topLeftGeo(entry.position->x() - entry.size->x()/2, entry.position->y() - entry.size->y()/2);
                QPointF bottomRightGeo(entry.position->x() + entry.size->x()/2, entry.position->y() + entry.size->y()/2);

                QPointF topLeftCanvas = gislib->geoToCanvas(topLeftGeo.y(), topLeftGeo.x());
                QPointF bottomRightCanvas = gislib->geoToCanvas(bottomRightGeo.y(), bottomRightGeo.x());

                QRectF shapeRect(topLeftCanvas.x() - selectionTolerance,
                                 topLeftCanvas.y() - selectionTolerance,
                                 (bottomRightCanvas.x() - topLeftCanvas.x()) + 2*selectionTolerance,
                                 (bottomRightCanvas.y() - topLeftCanvas.y()) + 2*selectionTolerance);

                isHit = shapeRect.contains(event->pos());
            }
            else if (entry.name.startsWith("TempPolygon")) {
                // POLYGON HIT DETECTION - FIXED
                QPolygonF polygon;

                // Convert all vertices to canvas coordinates
                for (Vector* relVertex : entry.mesh->polygen) {
                    // Convert relative vertex to absolute geographic coordinates
                    double absLon = relVertex->x + entry.position->x();
                    double absLat = relVertex->y + entry.position->y();

                    // Convert to canvas coordinates
                    QPointF canvasPoint = gislib->geoToCanvas(absLat, absLon);
                    polygon << canvasPoint;
                }

                // Use Qt's built-in polygon hit test
                isHit = polygon.containsPoint(event->pos(), Qt::OddEvenFill);

                Console::log("Polygon hit test: " + QString(isHit ? "HIT" : "MISS").toStdString());
            }
            else if (entry.name.startsWith("TempPolyline")) {
                // LINE HIT DETECTION - FIXED
                const qreal lineTolerance = 8.0;
                bool lineHit = false;

                for (size_t i = 0; i < entry.mesh->polygen.size() - 1; ++i) {
                    Vector* v1 = entry.mesh->polygen[i];
                    Vector* v2 = entry.mesh->polygen[i + 1];

                    if (!v1 || !v2) continue;

                    // Convert RELATIVE vertices to ABSOLUTE geographic coordinates
                    double absLon1 = v1->x + entry.position->x();
                    double absLat1 = v1->y + entry.position->y();
                    double absLon2 = v2->x + entry.position->x();
                    double absLat2 = v2->y + entry.position->y();

                    // Convert to canvas coordinates
                    QPointF canvas1 = gislib->geoToCanvas(absLat1, absLon1);
                    QPointF canvas2 = gislib->geoToCanvas(absLat2, absLon2);

                    lineHit = isPointNearLineSegment(event->pos(), canvas1, canvas2, lineTolerance);

                    if (lineHit) {
                        Console::log("Line segment " + std::to_string(i) + " HIT");
                        break;
                    }
                }

                isHit = lineHit;
                Console::log("Line hit test: " + QString(isHit ? "HIT" : "MISS").toStdString());
            }
            else if (entry.name.startsWith("TempPoint")) {
                // Point hit detection
                float distance = QVector2D(event->pos() - shapeCenterCanvas).length();
                isHit = (distance <= selectionTolerance);
            }

            if (isHit) {
                isDraggingShape = true;
                draggingShapeId = entry.name;
                shapeDragStartPos = event->pos();

                setCursor(Qt::ClosedHandCursor);
                Console::log("Shape selected for dragging: " + entry.name.toStdString());

                // Deselect other entities
                selectedEntityId = "";
                activeDragAxis = "";
                isDraggingBitmap = false;
                isDraggingUserImage = false;

                update();
                return true;
            }
        }
    }

    return false;
}

void CanvasWidget::handleShapeDragging(QMouseEvent *event) {
    if (!isDraggingShape || draggingShapeId.isEmpty()) return;

    for (auto& entry : tempMeshes) {
        if (entry.name == draggingShapeId) {
            // Calculate drag delta in geographic coordinates
            QPointF currentGeoPos = gislib->canvasToGeo(shapeDragStartPos);
            QPointF newGeoPos = gislib->canvasToGeo(event->pos());

            // Calculate the geographic offset
            double deltaLon = newGeoPos.x() - currentGeoPos.x();
            double deltaLat = newGeoPos.y() - currentGeoPos.y();

            // Update shape position
            entry.position->setX(entry.position->x() + deltaLon);
            entry.position->setY(entry.position->y() + deltaLat);

            // For polygons, vertices are relative to position, so no need to update them
            // The drawing will automatically use the new position

            // Update drag start position for next move event
            shapeDragStartPos = event->pos();

            QString posInfo = QString("Dragging shape: %1 to (lon: %2, lat: %3)")
                                  .arg(entry.name)
                                  .arg(entry.position->x(), 0, 'f', 6)
                                  .arg(entry.position->y(), 0, 'f', 6);
            Console::log(posInfo.toStdString());

            update();
            return;
        }
    }

    // If shape not found, stop dragging
    stopShapeDragging();
}

// NEW: Stop shape dragging
void CanvasWidget::stopShapeDragging() {
    if (isDraggingShape) {
        Console::log("Stopped dragging shape: " + draggingShapeId.toStdString());
        isDraggingShape = false;
        draggingShapeId = "";
        setCursor(Qt::ArrowCursor);
        update();
    }
}
