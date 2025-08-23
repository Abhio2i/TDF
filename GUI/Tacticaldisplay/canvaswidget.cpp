#include "canvaswidget.h"
#include "qjsondocument.h"
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


CanvasWidget::CanvasWidget(QWidget *parent) : QWidget(parent) {
    // Existing initialization...
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
    QTimer* fpsUpdateTimer = new QTimer(this);
    connect(fpsUpdateTimer, &QTimer::timeout, this, [this]() {
        fps = frameCount;
        frameCount = 0;
    });
    connect(gislib, &GISlib::distanceMeasured, this, &CanvasWidget::onDistanceMeasured);
    fpsUpdateTimer->start(100);
    update();
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

void CanvasWidget::editor() {
    simulate = false;
    Console::log("Editor mode enabled");
}

void CanvasWidget::setTransformMode(TransformMode mode) {
    currentMode = mode;
    if (mode == MeasureDistance) {
        gislib->startDistanceMeasurement();
        setCursor(Qt::CrossCursor);
        Console::log("Distance measurement mode enabled");
    } else {
        if (gislib->isMeasuringDistance()) {
            gislib->endDistanceMeasurement();
        }
        if (mode != DrawTrajectory) {
            if (isDrawingTrajectory) {
                setTrajectoryDrawingMode(false);
            }
        }
        setCursor(Qt::ArrowCursor);
        Console::log("Transform mode set to: " + std::to_string(mode));
    }
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

    Console::log("Attempting to save trajectory for entity: " + selectedEntityId);

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

    // Clear existing trajectory waypoints
    for (Waypoints* wp : entry.trajectory->Trajectories) {
        delete wp->position;
        delete wp;
    }
    entry.trajectory->Trajectories.clear();

    // Save current trajectory waypoints
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
    if (event->button() == Qt::LeftButton) {
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
            entry.position = new Vector(geoPos.x(), geoPos.y(), 0); // Geo coordinates (lon, lat)
            entry.rotation = new Vector(0, 0, 0);
            entry.size = new Vector(4.0, 4.0, 1); // Default size in geo coordinates
            entry.velocity = new Vector(0, 0, 0);
            entry.trajectory = nullptr;
            entry.collider = nullptr;
            entry.bitmapPath = bitmapPath;

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
            // Let GISlib handle the mouse press for distance measurement
            return;
        }

        if (currentMode == DrawShape) {
            QPointF geoPos = gislib->canvasToGeo(event->pos());
            Console::log("Click position: (lon: " + std::to_string(geoPos.x()) +
                         ", lat: " + std::to_string(geoPos.y()) + ")");
            QPointF canvasPos = gislib->geoToCanvas(geoPos.y(), geoPos.x());
            Console::log("Canvas position: (x: " + std::to_string(canvasPos.x()) +
                         ", y: " + std::to_string(canvasPos.y()) + ")");

            if (selectedShape == "Rectangle") {
                MeshEntry entry;
                entry.name = "TempRectangle";
                entry.position = new Vector(geoPos.x(), geoPos.y(), 0); // Store geo coords
                entry.rotation = new Vector(0, 0, 0);
                entry.size = new Vector(0.1, 0.1, 1); // Geo size (for reference)
                entry.velocity = new Vector(0, 0, 0);
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

                float halfWidth = 0.5f;
                float halfHeight = 0.2f;
                entry.mesh->polygen = {
                    new Vector(-halfWidth, -halfHeight, 0), // Bottom-left
                    new Vector(halfWidth, -halfHeight, 0),  // Bottom-right
                    new Vector(halfWidth, halfHeight, 0),   // Top-right
                    new Vector(-halfWidth, halfHeight, 0)   // Top-left
                };

                tempMeshes.push_back(entry);

                Console::log("Created temporary rectangle at (lon: " + std::to_string(geoPos.x()) +
                             ", lat: " + std::to_string(geoPos.y()) + ")");
                update();
                return;
            } else if (selectedShape == "Polygon") {
                if (event->type() == QEvent::MouseButtonDblClick && tempPolygonVertices.size() >= 3) {
                    MeshEntry entry;
                    entry.name = "TempPolygon";
                    float avgX = 0, avgY = 0;
                    for (const Vector* v : tempPolygonVertices) {
                        avgX += v->x;
                        avgY += v->y;
                    }
                    avgX /= tempPolygonVertices.size();
                    avgY /= tempPolygonVertices.size();
                    entry.position = new Vector(avgX, avgY, 0);
                    entry.rotation = new Vector(0, 0, 0);
                    entry.size = new Vector(0.1, 0.1, 1);
                    entry.velocity = new Vector(0, 0, 0);
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
                        entry.mesh->polygen.push_back(new Vector((v->x - avgX) / 0.1f, (v->y - avgY) / 0.1f, 0));
                    }

                    tempMeshes.push_back(entry);

                    for (Vector* v : tempPolygonVertices) {
                        delete v;
                    }
                    tempPolygonVertices.clear();
                    tempPolygonCanvasPoints.clear();

                    Console::log("Created temporary polygon with " + std::to_string(entry.mesh->polygen.size()) + " vertices");
                    update();
                    return;
                } else if (event->type() == QEvent::MouseButtonPress) {
                    if (tempPolygonVertices.empty()) {
                        Console::log("Starting new polygon at (lon: " + std::to_string(geoPos.x()) +
                                     ", lat: " + std::to_string(geoPos.y()) + ")");
                    }
                    tempPolygonVertices.push_back(new Vector(geoPos.x(), geoPos.y(), 0));
                    tempPolygonCanvasPoints.push_back(canvasPos);
                    Console::log("Added polygon vertex at (lon: " + std::to_string(geoPos.x()) +
                                 ", lat: " + std::to_string(geoPos.y()) + ")");
                    update();
                    return;
                }
            } else if (selectedShape == "Line") {
                if (event->type() == QEvent::MouseButtonDblClick && tempLineVertices.size() >= 2) {
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
                    entry.position = new Vector(avgX, avgY, 0);
                    entry.rotation = new Vector(0, 0, 0);
                    entry.size = new Vector(1.0, 1.0, 1);
                    entry.velocity = new Vector(0, 0, 0);
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
                        entry.mesh->polygen.push_back(new Vector(v->x, v->y, 0));
                    }

                    tempMeshes.push_back(entry);

                    for (Vector* v : tempLineVertices) {
                        delete v;
                    }
                    tempLineVertices.clear();
                    tempLineCanvasPoints.clear();

                    QString logMsg = QString("Created temporary polyline %1 with %2 vertices")
                                         .arg(entry.name)
                                         .arg(entry.mesh->polygen.size());
                    Console::log(logMsg.toStdString());
                    update();
                    return;
                } else if (event->type() == QEvent::MouseButtonPress) {
                    if (tempLineVertices.empty()) {
                        Console::log("Starting new polyline at (lon: " + std::to_string(geoPos.x()) +
                                     ", lat: " + std::to_string(geoPos.y()) + ")");
                    }
                    tempLineVertices.push_back(new Vector(geoPos.x(), geoPos.y(), 0));
                    tempLineCanvasPoints.push_back(canvasPos);
                    Console::log("Added polyline vertex at (lon: " + std::to_string(geoPos.x()) +
                                 ", lat: " + std::to_string(geoPos.y()) + ")");
                    update();
                    return;
                }
            } else if (selectedShape == "Circle") {
                static int circleCounter = 0;
                MeshEntry entry;
                entry.name = QString("TempCircle_%1").arg(circleCounter++);
                entry.position = new Vector(geoPos.x(), geoPos.y(), 0);
                entry.rotation = new Vector(0, 0, 0);
                entry.size = new Vector(8, 8, 1);
                entry.velocity = new Vector(0, 0, 0);
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

                entry.mesh->polygen.push_back(new Vector(geoPos.x(), geoPos.y(), 0));

                tempMeshes.push_back(entry);

                QString logMsg = QString("Created temporary circle %1 at (lon: %2, lat: %3) with geo radius: %4")
                                     .arg(entry.name)
                                     .arg(geoPos.x())
                                     .arg(geoPos.y())
                                     .arg(entry.size->x);
                Console::log(logMsg.toStdString());
                update();
                return;
            } else if (selectedShape == "Points") {
                static int pointCounter = 0;
                MeshEntry entry;
                entry.name = QString("TempPoint_%1").arg(pointCounter++);
                entry.position = new Vector(geoPos.x(), geoPos.y(), 0);
                entry.rotation = new Vector(0, 0, 0);
                entry.size = new Vector(0, 0, 1);
                entry.velocity = new Vector(0, 0, 0);
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
                entry.mesh->lineWidth = 1;
                entry.mesh->closePath = true;

                entry.mesh->polygen.push_back(new Vector(geoPos.x(), geoPos.y(), 0));

                tempMeshes.push_back(entry);

                QString logMsg = QString("Created temporary point %1 at (lon: %2, lat: %3)")
                                     .arg(entry.name)
                                     .arg(geoPos.x())
                                     .arg(geoPos.y());
                Console::log(logMsg.toStdString());
                update();
                return;
            }
        }
        if (currentMode == DrawTrajectory && isDrawingTrajectory && !selectedEntityId.empty()) {
            QPointF geoPos = gislib->canvasToGeo(event->pos());
            int nearestIndex = findNearestWaypoint(event->pos());

            if (nearestIndex >= 0) {
                // Select existing waypoint
                selectWaypoint(nearestIndex);
                isDraggingWaypoint = true;
                Console::log("Selected existing waypoint at index: " + std::to_string(nearestIndex));
            } else {
                // Add new waypoint
                Waypoints* waypoint = new Waypoints();
                waypoint->position = new Vector(geoPos.x(), geoPos.y(), 0);
                currentTrajectory.push_back(waypoint);
                selectWaypoint(currentTrajectory.size() - 1);
                Console::log("Added new waypoint at (lon: " + std::to_string(waypoint->position->x) +
                             ", lat: " + std::to_string(waypoint->position->y) + ")");
                updateTrajectoryData();
            }
            update();
            return;
        }
        if (!selectedEntityId.empty()) {
            auto it = Meshes.find(selectedEntityId);
            if (it != Meshes.end()) {
                auto& entry = it->second;
                QPointF basePos = gislib->geoToCanvas(entry.position->y, entry.position->x);
                const float handleTolerance = 15.0f;

                if (currentMode == Translate) {
                    QPointF xAxisHandle = basePos + QPointF(50, 0);
                    QPointF yAxisHandle = basePos + QPointF(0, 50);
                    if (QVector2D(event->pos() - xAxisHandle).length() < handleTolerance) {
                        activeDragAxis = "x";
                        dragStartPos = event->pos();
                        Console::log("Dragging X-axis of entity: " + selectedEntityId);
                        update();
                        return;
                    }
                    if (QVector2D(event->pos() - yAxisHandle).length() < handleTolerance) {
                        activeDragAxis = "y";
                        dragStartPos = event->pos();
                        Console::log("Dragging Y-axis of entity: " + selectedEntityId);
                        update();
                        return;
                    }
                } else if (currentMode == Rotate) {
                    float distFromCenter = QVector2D(event->pos() - basePos).length();
                    if (qAbs(distFromCenter - 40.0f) < handleTolerance) {
                        activeDragAxis = "rotate";
                        dragStartPos = event->pos();
                        Console::log("Gizmo selected for rotation.");
                        update();
                        return;
                    }
                } else if (currentMode == Scale) {
                    QRectF xHandleRect(basePos.x() + 45, basePos.y() - 5, 10, 10);
                    QRectF yHandleRect(basePos.x() - 5, basePos.y() + 45, 10, 10);
                    if (xHandleRect.contains(event->pos())) {
                        activeDragAxis = "scale-x";
                        dragStartPos = event->pos();
                        Console::log("Gizmo X-axis selected for scaling.");
                        update();
                        return;
                    }
                    if (yHandleRect.contains(event->pos())) {
                        activeDragAxis = "scale-y";
                        dragStartPos = event->pos();
                        Console::log("Gizmo Y-axis selected for scaling.");
                        update();
                        return;
                    }
                }
            }
        }
        bool entityWasClicked = false;
        for (auto& [id, entry] : Meshes) {
            QPointF entityPos = gislib->geoToCanvas(entry.position->y, entry.position->x);
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

    if (event->button() == Qt::RightButton && currentMode == DrawTrajectory && isDrawingTrajectory) {
        int nearestIndex = findNearestWaypoint(event->pos());
        if (nearestIndex >= 0) {
            selectWaypoint(nearestIndex);
            QMenu contextMenu(this);
            QAction* deleteAction = contextMenu.addAction("Delete Waypoint");
            connect(deleteAction, &QAction::triggered, this, [=]() {
                if (selectedWaypointIndex >= 0 && selectedWaypointIndex < (int)currentTrajectory.size()) {
                    // Create confirmation dialog
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

    update();
}

void CanvasWidget::handleMouseMove(QMouseEvent *event) {
    if (isPanning) {
        QPoint delta = event->pos() - lastMousePos;
        canvasOffset += delta;
        lastMousePos = event->pos();
        update();
        return;
    }

    if (currentMode == MeasureDistance) {
        // Let GISlib handle the mouse move for distance measurement
        return;
    }

    if (isDrawingTrajectory && isDraggingWaypoint && selectedWaypointIndex >= 0 &&
        selectedWaypointIndex < (int)currentTrajectory.size()) {
        // Move the selected waypoint
        QPointF geoPos = gislib->canvasToGeo(event->pos());
        Waypoints* wp = currentTrajectory[selectedWaypointIndex];
        wp->position->x = geoPos.x();
        wp->position->y = geoPos.y();
        Console::log("Moved waypoint " + std::to_string(selectedWaypointIndex) +
                     " to (" + std::to_string(geoPos.x()) + ", " + std::to_string(geoPos.y()) + ")");
        updateTrajectoryData();
        update();
        return;
    }
    if (Meshes.find(selectedEntityId) == Meshes.end()) return;

    auto& entry = Meshes[selectedEntityId];
    QPointF delta = event->pos() - dragStartPos;

    float dx = event->pos().x() - entry.position->x - canvasOffset.x();
    float dy = event->pos().y() - entry.position->y - canvasOffset.y();

    if (qAbs(dx) < 10 && qAbs(dy) < 10) {
        activeDragAxis = "both";
    } else if (qAbs(dx - 50) < 25 && qAbs(dy) < 10) {
        activeDragAxis = "x";
    } else if (qAbs(dy - 50) < 25 && qAbs(dx) < 10) {
        activeDragAxis = "y";
    }

    if (!selectedEntityId.empty() && !activeDragAxis.isEmpty()) {
        auto& entry = Meshes[selectedEntityId];

        if (currentMode == Translate) {
            // Nayi mouse position ko geographical coordinates mein convert karein
            QPointF newGeoPos = gislib->canvasToGeo(event->pos());

            // Pichli position ke geo coordinates
            QPointF oldGeoPos = gislib->canvasToGeo(dragStartPos);

            // Delta calculate karein
            double deltaLon = newGeoPos.x() - oldGeoPos.x();
            double deltaLat = newGeoPos.y() - oldGeoPos.y();

            // Sirf active axis ke anusaar position update karein
            if (activeDragAxis == "x") {
                entry.position->x += deltaLon; // Longitude
            } else if (activeDragAxis == "y") {
                entry.position->y += deltaLat; // Latitude
            }


        } else if (currentMode == Rotate) {
            QPointF basePos = gislib->geoToCanvas(entry.position->y, entry.position->x);
            // Center se purane aur naye mouse position tak ka angle calculate karein
            qreal angle_new = qAtan2(event->pos().y() - basePos.y(), event->pos().x() - basePos.x());
            qreal angle_old = qAtan2(dragStartPos.y() - basePos.y(), dragStartPos.x() - basePos.x());
            // Angle delta ko degrees mein convert karke add karein
            entry.rotation->z -= qRadiansToDegrees(angle_new - angle_old);

        } else if (currentMode == Scale) {
            QPointF basePos = gislib->geoToCanvas(entry.position->y, entry.position->x);
            qreal dist_new = QVector2D(event->pos() - basePos).length();
            qreal dist_old = QVector2D(dragStartPos - basePos).length();
            qreal scaleFactor = dist_new / dist_old;

            if (activeDragAxis == "x") {
                entry.size->x *= scaleFactor;
            } else if (activeDragAxis == "y") {
                entry.size->y *= scaleFactor;
            }
        }

        dragStartPos = event->pos();
        update();
    }
}

void CanvasWidget::handleMouseRelease(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        activeDragAxis = "";
        isDraggingWaypoint = false;
        selectEntity = false;
        updateTrajectoryData(); // Ensure final position is saved
    }

    if (currentMode == MeasureDistance) {
        // Let GISlib handle the mouse release for distance measurement
        return;
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

    if (event->key() == Qt::Key_5) {
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
    } else if (event->key() == Qt::Key_Escape) {
        Console::log("Escape key pressed, currentMode: " + std::to_string(currentMode));
        if (currentMode == DrawTrajectory) {
            Console::log("Calling saveTrajectory");
            saveTrajectory();
            setTrajectoryDrawingMode(false);
            deselectWaypoint();
        } else if (currentMode == DrawShape) {
            if (!tempPolygonCanvasPoints.empty() || !tempLineCanvasPoints.empty()) {
                if (selectedShape == "Polygon" || selectedShape == "Line") {
                    // Finalize shape drawing
                    QString entityId = QString::fromStdString(selectedEntityId);
                    QJsonObject shapeData;
                    QJsonArray vertices;
                    std::vector<QPointF>& points = (selectedShape == "Polygon") ? tempPolygonCanvasPoints : tempLineCanvasPoints;
                    std::vector<Vector*>& verticesVec = (selectedShape == "Polygon") ? tempPolygonVertices : tempLineVertices;
                    for (const QPointF& point : points) {
                        QPointF geoPos = gislib->canvasToGeo(point);
                        QJsonObject vertex;
                        vertex["x"] = geoPos.x();
                        vertex["y"] = geoPos.y();
                        vertex["z"] = 0.0;
                        vertices.append(vertex);
                    }
                    shapeData["vertices"] = vertices;
                    shapeData["type"] = selectedShape;
                    emit selectEntitybyCursor(entityId);
                    // Update hierarchy with shape data (assuming a signal or method exists)
                    for (Vector* v : verticesVec) {
                        delete v;
                    }
                    verticesVec.clear();
                    points.clear();
                    setShapeDrawingMode(false, "");
                }
            }
        } else if (currentMode == PlaceBitmap) {
            isPlacingBitmap = false;
            selectedBitmapType = "";
            setTransformMode(Translate);
            Console::log("Exited PlaceBitmap mode");
        } else if (currentMode == MeasureDistance) {
            setTransformMode(Translate); // Exit measurement mode
            Console::log("Exited MeasureDistance mode");
        }
    } else if (event->key() == Qt::Key_Delete && currentMode == DrawTrajectory &&
               selectedWaypointIndex >= 0 && selectedWaypointIndex < (int)currentTrajectory.size()) {
        // Delete the selected waypoint
        delete currentTrajectory[selectedWaypointIndex]->position;
        delete currentTrajectory[selectedWaypointIndex];
        currentTrajectory.erase(currentTrajectory.begin() + selectedWaypointIndex);
        deselectWaypoint();
        updateTrajectoryData();
        Console::log("Deleted selected waypoint");
        update();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (currentMode == DrawShape && (!tempPolygonCanvasPoints.empty() || !tempLineCanvasPoints.empty())) {
            if (selectedShape == "Polygon" || selectedShape == "Line") {
                // Finalize shape drawing
                QString entityId = QString::fromStdString(selectedEntityId);
                QJsonObject shapeData;
                QJsonArray vertices;
                std::vector<QPointF>& points = (selectedShape == "Polygon") ? tempPolygonCanvasPoints : tempLineCanvasPoints;
                std::vector<Vector*>& verticesVec = (selectedShape == "Polygon") ? tempPolygonVertices : tempLineVertices;
                for (const QPointF& point : points) {
                    QPointF geoPos = gislib->canvasToGeo(point);
                    QJsonObject vertex;
                    vertex["x"] = geoPos.x();
                    vertex["y"] = geoPos.y();
                    vertex["z"] = 0.0;
                    vertices.append(vertex);
                }
                shapeData["vertices"] = vertices;
                shapeData["type"] = selectedShape;
                emit selectEntitybyCursor(entityId);
                // Update hierarchy with shape data (assuming a signal or method exists)
                for (Vector* v : verticesVec) {
                    delete v;
                }
                verticesVec.clear();
                points.clear();
                setShapeDrawingMode(false, "");
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

    if (isDrawingTrajectory && !currentTrajectory.empty()) {
        painter.save();
        QPen pen(Qt::magenta, 2, Qt::DashLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        QPolygonF polyline;
        for (size_t i = 0; i < currentTrajectory.size(); ++i) {
            Waypoints* waypoint = currentTrajectory[i];
            QPointF screenPos = gislib->geoToCanvas(waypoint->position->y, waypoint->position->x);
            polyline << screenPos;
            painter.setBrush(Qt::magenta);
            int pointSize = (i == selectedWaypointIndex) ? 6 : 4; // Larger size for selected waypoint
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

bool isColliding(const MeshEntry& a, const MeshEntry& b) {
    float ax = a.position->x, ay = a.position->y;
    float aw = a.size->x * 25, ah = a.size->y * 25;

    float bx = b.position->x, by = b.position->y;
    float bw = b.size->x * 25, bh = b.size->y * 25;

    return (std::abs(ax - bx) < (aw + bw) / 2.0f) &&
           (std::abs(ay - by) < (ah + bh) / 2.0f);
}

void CanvasWidget::applyGravityAndBounce(float deltaTime) {
    const float gravity = 1980.0f;
    const float bounceFactor = 0.9f;

    for (auto& [id, entry] : Meshes) {
        if (!entry.position || !entry.velocity || !entry.size) continue;

        entry.velocity->y += gravity * deltaTime;
        entry.position->x += entry.velocity->x * deltaTime;
        entry.position->y += entry.velocity->y * deltaTime;

        float w = entry.size->x * 25;
        float h = entry.size->y * 25;

        float bottom = entry.position->y + h / 2.0f;
        if (bottom >= height()) {
            entry.position->y = height() - h / 2.0f;
            entry.velocity->y = -entry.velocity->y * bounceFactor;
            if (qAbs(entry.velocity->y) < 10.0f) entry.velocity->y = 0.0f;
        }

        float top = entry.position->y - h / 2.0f;
        if (top <= 0) {
            entry.position->y = h / 2.0f;
            entry.velocity->y = -entry.velocity->y * bounceFactor;
        }

        float left = entry.position->x - w / 2.0f;
        if (left <= 0) {
            entry.position->x = w / 2.0f;
            entry.velocity->x = -entry.velocity->x * bounceFactor;
        }

        float right = entry.position->x + w / 2.0f;
        if (right >= width()) {
            entry.position->x = width() - w / 2.0f;
            entry.velocity->x = -entry.velocity->x * bounceFactor;
        }
    }

    std::vector<std::string> ids;
    for (const auto& [id, _] : Meshes) {
        ids.push_back(id);
    }

    for (size_t i = 0; i < ids.size(); ++i) {
        for (size_t j = i + 1; j < ids.size(); ++j) {
            MeshEntry& a = Meshes[ids[i]];
            MeshEntry& b = Meshes[ids[j]];

            if (isColliding(a, b)) {
                std::swap(a.velocity->x, b.velocity->x);
                std::swap(a.velocity->y, b.velocity->y);
            }
        }
    }
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
    painter.save(); // Painter state save karein
    painter.resetTransform(); // Transform reset karein taaki drawing screen coordinates mein ho

    QFont font("Arial", 10, QFont::Bold);
    painter.setFont(font);
    painter.setPen(QColor(255, 255, 255, 200));

    if (showInformation) {
        QString modeText = QString(" %1").arg(simulate ? "Simulation" : "Editor");
        // FIX: canvasOffset hataya gaya, fixed screen coordinates ka upyog kiya
        painter.drawText(QPointF(width() - 100, 20), modeText);
    }

    if (showFPS) {
        QString fpsText = QString("FPS: %1").arg(fps*10);
        // FIX: canvasOffset hataya gaya
        painter.drawText(QPointF(10, 20), fpsText);
    }

    painter.restore(); // Painter state restore karein
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
    // Position ko behtar format mein dikhayein
    if (currentMode == Translate || currentMode == DrawTrajectory) {
        text = QString("pos: lon %1, lat %2")
        .arg(pos->x, 0, 'f', 4) // Longitude
            .arg(pos->y, 0, 'f', 4); // Latitude
    } else if (currentMode == Rotate) {
        auto& rot = Meshes[selectedEntityId].rotation;
        text = QString("rot: x %1, y %2, z %3")
                   .arg(rot->x)
                   .arg(rot->y)
                   .arg(rot->z);
    } else if (currentMode == Scale) {
        auto& size = Meshes[selectedEntityId].size;
        text = QString("size: x %1, y %2, z %3")
                   .arg(size->x)
                   .arg(size->y)
                   .arg(size->z);
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

    // Entity ki geographical position ko screen position (pixels) mein badlein
    QPointF base = gislib->geoToCanvas(entry.position->y, entry.position->x);

    // Gizmo handles ki screen positions
    QPointF xAxisHandlePos = base + QPointF(50, 0);
    QPointF yAxisHandlePos = base + QPointF(0, 50);

    // Baaki ka code (drawing logic) waisa hi rahega, lekin ab 'base' screen coordinates mein hai
    if (currentMode == Translate) {
        // Active axis ke hisaab se handle ko mota (thicker) dikhayein
        painter.setPen(QPen(Qt::blue, (activeDragAxis == "x") ? 4 : 2));
        painter.drawLine(base, xAxisHandlePos);
        painter.setPen(QPen(Qt::green, (activeDragAxis == "y") ? 4 : 2));
        painter.drawLine(base, yAxisHandlePos);
    } else if (currentMode == Rotate) {
        painter.setPen(QPen(QColor(255, 255, 0, 150), 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(base, 40, 40);
        float radius = 40;
        // Rotation angle entry se lein
        float rad = qDegreesToRadians(-entry.rotation->z);
        QPointF endpoint(base.x() + radius * cos(rad), base.y() - radius * sin(rad));
        painter.drawLine(base, endpoint);
    } else if (currentMode == Scale) {
        painter.setPen(QPen(Qt::red, 2));
        painter.setBrush(Qt::red);
        painter.drawRect(QRectF(base + QPointF(45, -3), QSizeF(10, 6))); // X-axis handle
        painter.setBrush(Qt::green);
        painter.drawRect(QRectF(base + QPointF(-3, 45), QSizeF(6, 10))); // Y-axis handle
    }
}


void CanvasWidget::drawSelectionOutline(QPainter& painter) {
    if (!showOutline || simulate) return;
    if (Meshes.find(selectedEntityId) == Meshes.end()) return;

    auto& entry = Meshes[selectedEntityId];
    auto& pos = entry.position;
    auto& size = entry.size;

    float w = size->x * 25;
    float h = size->y * 25;
    QRectF outlineRect(pos->x - w / 2.0f, pos->y - h / 2.0f, w, h);

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
        painter.translate(entry.position->x, entry.position->y);

        QPen pen(Qt::cyan);
        pen.setStyle(Qt::DashDotLine);
        pen.setWidth(2);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        using ColliderType = Constants::ColliderType;
        switch (entry.collider->collider) {
        case ColliderType::Sphere:
            painter.drawEllipse(QPointF(0, 0), entry.collider->Radius * entry.size->magnitude(), entry.collider->Radius * entry.size->magnitude());
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
            Console::log("Warning: Invalid mesh or color in tempMeshes");
            continue;
        }

        // Handle bitmap images
        if (!entry.bitmapPath.isEmpty()) {
            QPixmap img(entry.bitmapPath);
            if (!img.isNull()) {
                // Convert geo position to canvas coordinates
                QPointF canvasPos = gislib->geoToCanvas(entry.position->y, entry.position->x);

                // Calculate size in canvas coordinates
                // Assuming size->x and size->y are in geo coordinates
                QPointF sizePointGeo(entry.position->x + entry.size->x, entry.position->y + entry.size->y);
                QPointF sizePointCanvas = gislib->geoToCanvas(sizePointGeo.y(), sizePointGeo.x());
                float canvasWidth = qAbs(sizePointCanvas.x() - canvasPos.x()) * 2;
                float canvasHeight = qAbs(sizePointCanvas.y() - canvasPos.y()) * 2;

                // Scale the image
                QPixmap scaled = img.scaled(canvasWidth, canvasHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);

                // Apply rotation
                painter.save();
                painter.translate(canvasPos);
                painter.rotate(entry.rotation->z);
                painter.drawPixmap(QPointF(-scaled.width() / 2.0f, -scaled.height() / 2.0f), scaled);
                painter.restore();

                QString logMsg = QString("Rendering bitmap %1 at canvas position: (x: %2, y: %3)")
                                     .arg(entry.name)
                                     .arg(canvasPos.x())
                                     .arg(canvasPos.y());
                Console::log(logMsg.toStdString());
            } else {
                Console::log("Error: Failed to load bitmap image at path: " + entry.bitmapPath.toStdString());
            }
            continue; // Skip other rendering logic for bitmaps
        }

        if (entry.name.startsWith("TempPoint")) {
            if (mesh->polygen.empty()) {
                Console::log("Warning: Point mesh has no position");
                continue;
            }
            Vector* point = mesh->polygen[0];
            if (!point) {
                Console::log("Warning: Null point in point mesh");
                continue;
            }
            QPointF canvasPoint = gislib->geoToCanvas(point->y, point->x);
            QString logMsg = QString("Rendering point %1 at canvas position: (x: %2, y: %3)")
                                 .arg(entry.name)
                                 .arg(canvasPoint.x())
                                 .arg(canvasPoint.y());
            Console::log(logMsg.toStdString());
            painter.setPen(QPen(*mesh->color, mesh->lineWidth, Qt::SolidLine));
            painter.setBrush(*mesh->color);
            painter.drawEllipse(canvasPoint, 3, 3);
        }

        else if (entry.name.startsWith("TempCircle")) {
            if (mesh->polygen.empty()) {
                Console::log("Warning: Circle mesh has no center point");
                continue;
            }
            Vector* center = mesh->polygen[0];
            if (!center) {
                Console::log("Warning: Null center point in circle mesh");
                continue;
            }
            QPointF canvasCenter = gislib->geoToCanvas(center->y, center->x);
            QString logMsg = QString("Rendering circle %1 at canvas position: (x: %2, y: %3)")
                                 .arg(entry.name)
                                 .arg(canvasCenter.x())
                                 .arg(canvasCenter.y());
            Console::log(logMsg.toStdString());
            QPointF radiusPointGeo(center->x + entry.size->x, center->y);
            QPointF radiusPointCanvas = gislib->geoToCanvas(radiusPointGeo.y(), radiusPointGeo.x());
            float canvasRadius = sqrt(pow(radiusPointCanvas.x() - canvasCenter.x(), 2) +
                                      pow(radiusPointCanvas.y() - canvasCenter.y(), 2));
            if (canvasRadius > 0 && canvasRadius < 10000) {
                painter.setPen(QPen(*mesh->color, mesh->lineWidth, Qt::SolidLine));
                painter.setBrush(QColor(255, 0, 0, 50));
                logMsg = QString("Drawing circle %1 with radius: %2")
                             .arg(entry.name)
                             .arg(canvasRadius);
                Console::log(logMsg.toStdString());
                painter.drawEllipse(canvasCenter, canvasRadius, canvasRadius);
            } else {
                Console::log("Warning: Circle radius " + std::to_string(canvasRadius) + " is invalid or too large");
            }
        }
        // Existing code for polylines
        else if (entry.name.startsWith("TempPolyline")) {
            QPolygonF polygon;
            for (Vector* point : mesh->polygen) {
                if (!point) {
                    Console::log("Warning: Null point in polyline mesh");
                    continue;
                }
                QPointF canvasPoint = gislib->geoToCanvas(point->y, point->x);
                polygon << canvasPoint;
                QString logMsg = QString("Polyline point %1: (x: %2, y: %3)")
                                     .arg(entry.name)
                                     .arg(canvasPoint.x())
                                     .arg(canvasPoint.y());
                Console::log(logMsg.toStdString());
            }
            if (polygon.size() < 2) {
                Console::log("Warning: Polyline " + entry.name.toStdString() + " has fewer than 2 points");
                continue;
            }
            painter.setPen(QPen(*mesh->color, mesh->lineWidth, Qt::SolidLine));
            painter.setBrush(Qt::NoBrush);
            QString logMsg = QString("Drawing polyline %1 with %2 vertices")
                                 .arg(entry.name)
                                 .arg(polygon.size());
            Console::log(logMsg.toStdString());
            painter.drawPolyline(polygon);
        }
        // Existing code for rectangles and polygons
        else {
            QPolygonF polygon;
            QPointF canvasPos;
            if (gislib) {
                canvasPos = gislib->geoToCanvas(entry.position->y, entry.position->x);
            } else {
                Console::log("Error: gislib is null, using canvas center");
                canvasPos = QPointF(width() / 2, height() / 2);
            }
            Console::log("Rendering shape at canvas position: (x: " + std::to_string(canvasPos.x()) +
                         ", y: " + std::to_string(canvasPos.y()) + ")");
            float angle = qDegreesToRadians(entry.rotation->z);
            float cosA = cos(angle);
            float sinA = sin(angle);
            float pixelSizeX = 50.0f;
            float pixelSizeY = 50.0f;
            for (Vector* point : mesh->polygen) {
                if (!point) {
                    Console::log("Warning: Null point in mesh polygen");
                    continue;
                }
                float x = point->x * pixelSizeX;
                float y = point->y * pixelSizeY;
                float rotatedX = x * cosA - y * sinA;
                float rotatedY = x * sinA + y * cosA;
                float finalX = canvasPos.x() + rotatedX;
                float finalY = canvasPos.y() + rotatedY;
                polygon << QPointF(finalX, finalY);
                Console::log("Polygon point: (x: " + std::to_string(finalX) +
                             ", y: " + std::to_string(finalY) + ")");
            }
            if (polygon.isEmpty()) {
                Console::log("Warning: Polygon is empty, cannot draw");
                continue;
            }
            painter.setPen(QPen(*mesh->color, mesh->lineWidth, Qt::SolidLine));
            painter.setBrush(QColor(255, 0, 0, 50));
            Console::log("Drawing shape with color: red, lineWidth: " + std::to_string(mesh->lineWidth));
            painter.drawPolygon(polygon);
        }
    }

    // Draw preview of polygon being created
    if (!tempPolygonVertices.empty()) {
        QPolygonF previewPolygon;
        for (const Vector* vertex : tempPolygonVertices) {
            QPointF canvasPoint = gislib->geoToCanvas(vertex->y, vertex->x);
            previewPolygon << canvasPoint;
            Console::log("Preview polygon point: (x: " + std::to_string(canvasPoint.x()) +
                         ", y: " + std::to_string(canvasPoint.y()) + ")");
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

    // Draw preview of polyline being created
    if (!tempLineVertices.empty()) {
        QPolygonF previewLine;
        for (const Vector* vertex : tempLineVertices) {
            QPointF canvasPoint = gislib->geoToCanvas(vertex->y, vertex->x);
            previewLine << canvasPoint;
            Console::log("Preview polyline point: (x: " + std::to_string(canvasPoint.x()) +
                         ", y: " + std::to_string(canvasPoint.y()) + ")");
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
            QPixmap scaled = img.scaled(entry.size->x * 50, entry.size->y * 50,
                                        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            QPointF point = gislib->geoToCanvas(entry.position->y, entry.position->x);
            float x = point.x();
            float y = point.y();
            float angle = entry.rotation->z;

            painter.save();
            painter.translate(x, y);
            painter.rotate(angle);
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
            Console::log("Skipping trajectory draw for entity: " + id + " (trajectory null, inactive, or empty)");
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
            QPointF point = gislib->geoToCanvas(waypoint->position->y, waypoint->position->x);
            polyline << point;
            painter.setBrush(Qt::cyan);
            painter.drawEllipse(point, 3, 3);
            painter.setBrush(Qt::NoBrush);
        }

        // Draw polyline only if there are multiple waypoints
        if (polyline.size() > 1) {
            painter.drawPolyline(polyline);
            Console::log("Drawing trajectory for entity: " + id + " with " +
                         std::to_string(polyline.size()) + " waypoints");
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
        currentMode = DrawShape;
        setCursor(Qt::CrossCursor);
        Console::log("Shape drawing mode enabled: " + shapeType.toStdString());
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
        currentMode = Translate;
        selectedShape = "";
        setCursor(Qt::ArrowCursor);
        Console::log("Shape drawing mode disabled");

        // Clear temporary polygon data
        for (Vector* v : tempPolygonVertices) {
            delete v;
        }
        tempPolygonVertices.clear();
        tempPolygonCanvasPoints.clear();

        // Clear temporary line data
        for (Vector* v : tempLineVertices) {
            delete v;
        }
        tempLineVertices.clear();
        tempLineCanvasPoints.clear();
    }
    update();
}


void CanvasWidget::onBitmapImageSelected(const QString& filePath) {
    static int bitmapCounter = 0;
    MeshEntry entry;
    entry.name = QString("TempBitmap_%1").arg(bitmapCounter++);
    QPointF centerCanvas(width() / 2.0f, height() / 2.0f);
    QPointF geoPos = gislib->canvasToGeo(centerCanvas);
    entry.position = new Vector(geoPos.x(), geoPos.y(), 0);
    entry.rotation = new Vector(0, 0, 0);
    entry.size = new Vector(7.1, 7.1, 1);
    entry.velocity = new Vector(0, 0, 0);
    entry.trajectory = nullptr;
    entry.collider = nullptr;
    entry.bitmapPath = filePath;
    entry.mesh = new Mesh();
    if (!entry.mesh) {
        Console::log("Error: Failed to allocate Mesh for bitmap");
        return;
    }
    entry.mesh->color = new QColor(Qt::white);
    entry.mesh->lineWidth = 1;
    entry.mesh->closePath = false;

    tempMeshes.push_back(entry);

    QString logMsg = QString("Added bitmap image %1 at (lon: %2, lat: %3) with path: %4")
                         .arg(entry.name)
                         .arg(geoPos.x())
                         .arg(geoPos.y())
                         .arg(filePath);
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
        QPointF wpCanvas = gislib->geoToCanvas(wp->position->y, wp->position->x);
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

    // Serialize tempPolygonVertices
    QJsonArray polygonVerticesArray;
    for (const Vector* vertex : tempPolygonVertices) {
        QJsonObject vObj;
        vObj["x"] = vertex->x;
        vObj["y"] = vertex->y;
        vObj["z"] = vertex->z;
        polygonVerticesArray.append(vObj);
    }
    json["tempPolygonVertices"] = polygonVerticesArray;

    // Serialize tempPolygonCanvasPoints
    QJsonArray polygonCanvasPointsArray;
    for (const QPointF& point : tempPolygonCanvasPoints) {
        QJsonObject pObj;
        pObj["x"] = point.x();
        pObj["y"] = point.y();
        polygonCanvasPointsArray.append(pObj);
    }
    json["tempPolygonCanvasPoints"] = polygonCanvasPointsArray;

    // Serialize tempLineVertices
    QJsonArray lineVerticesArray;
    for (const Vector* vertex : tempLineVertices) {
        QJsonObject vObj;
        vObj["x"] = vertex->x;
        vObj["y"] = vertex->y;
        vObj["z"] = vertex->z;
        lineVerticesArray.append(vObj);
    }
    json["tempLineVertices"] = lineVerticesArray;

    // Serialize tempLineCanvasPoints
    QJsonArray lineCanvasPointsArray;
    for (const QPointF& point : tempLineCanvasPoints) {
        QJsonObject pObj;
        pObj["x"] = point.x();
        pObj["y"] = point.y();
        lineCanvasPointsArray.append(pObj);
    }
    json["tempLineCanvasPoints"] = lineCanvasPointsArray;

    // Serialize tempMeshes (includes Rectangle, Circle, Point, and Bitmap)
    QJsonArray tempMeshesArray;
    for (const MeshEntry& entry : tempMeshes) {
        QJsonObject meshObj;
        meshObj["name"] = entry.name;

        // Serialize position
        QJsonObject posObj;
        posObj["x"] = entry.position->x;
        posObj["y"] = entry.position->y;
        posObj["z"] = entry.position->z;
        meshObj["position"] = posObj;

        // Serialize rotation
        QJsonObject rotObj;
        rotObj["x"] = entry.rotation->x;
        rotObj["y"] = entry.rotation->y;
        rotObj["z"] = entry.rotation->z;
        meshObj["rotation"] = rotObj;

        // Serialize size
        QJsonObject sizeObj;
        sizeObj["x"] = entry.size->x;
        sizeObj["y"] = entry.size->y;
        sizeObj["z"] = entry.size->z;
        meshObj["size"] = sizeObj;

        // Serialize velocity
        QJsonObject velObj;
        velObj["x"] = entry.velocity->x;
        velObj["y"] = entry.velocity->y;
        velObj["z"] = entry.velocity->z;
        meshObj["velocity"] = velObj;

        // Serialize bitmapPath if present
        if (!entry.bitmapPath.isEmpty()) {
            meshObj["bitmapPath"] = entry.bitmapPath;
        }

        // Serialize mesh data
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

    return json;
}

void CanvasWidget::fromJson(const QJsonObject& json) {
    // Clear existing data
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

    // Clear tempMeshes
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

    // Deserialize selectedBitmapType and isPlacingBitmap
    if (json.contains("selectedBitmapType")) {
        selectedBitmapType = json["selectedBitmapType"].toString();
    }

    if (json.contains("isPlacingBitmap")) {
        isPlacingBitmap = json["isPlacingBitmap"].toBool();
    }

    // Deserialize tempPolygonVertices
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

    // Deserialize tempPolygonCanvasPoints
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

    // Deserialize tempLineVertices
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

    // Deserialize tempLineCanvasPoints
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

    // Deserialize tempMeshes
    if (json.contains("tempMeshes") && json["tempMeshes"].isArray()) {
        QJsonArray tempMeshesArray = json["tempMeshes"].toArray();
        for (const QJsonValue& val : tempMeshesArray) {
            QJsonObject meshObj = val.toObject();
            MeshEntry entry;

            entry.name = meshObj["name"].toString();

            // Deserialize position
            if (meshObj.contains("position")) {
                QJsonObject posObj = meshObj["position"].toObject();
                entry.position = new Vector(
                    posObj["x"].toDouble(),
                    posObj["y"].toDouble(),
                    posObj["z"].toDouble()
                    );
            } else {
                entry.position = new Vector(0, 0, 0);
            }

            // Deserialize rotation
            if (meshObj.contains("rotation")) {
                QJsonObject rotObj = meshObj["rotation"].toObject();
                entry.rotation = new Vector(
                    rotObj["x"].toDouble(),
                    rotObj["y"].toDouble(),
                    rotObj["z"].toDouble()
                    );
            } else {
                entry.rotation = new Vector(0, 0, 0);
            }

            // Deserialize size
            if (meshObj.contains("size")) {
                QJsonObject sizeObj = meshObj["size"].toObject();
                entry.size = new Vector(
                    sizeObj["x"].toDouble(),
                    sizeObj["y"].toDouble(),
                    sizeObj["z"].toDouble()
                    );
            } else {
                entry.size = new Vector(1, 1, 1);
            }

            // Deserialize velocity
            if (meshObj.contains("velocity")) {
                QJsonObject velObj = meshObj["velocity"].toObject();
                entry.velocity = new Vector(
                    velObj["x"].toDouble(),
                    velObj["y"].toDouble(),
                    velObj["z"].toDouble()
                    );
            } else {
                entry.velocity = new Vector(0, 0, 0);
            }

            // Deserialize bitmapPath
            if (meshObj.contains("bitmapPath")) {
                entry.bitmapPath = meshObj["bitmapPath"].toString();
            }

            // Deserialize mesh data
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
    QMessageBox::information(this, "Distance Measured", msg);
}
