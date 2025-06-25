
#include "canvaswidget.h"
#include <QTimer>
#include <QDebug>
#include <QMouseEvent>
#include <cmath>
#include <core/Debug/console.h>

CanvasWidget::CanvasWidget(QWidget *parent) : QWidget(parent) {
    setMinimumSize(300, 300);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint);
    setFocusPolicy(Qt::StrongFocus);
    setUpdatesEnabled(true);
    setMouseTracking(true);
    simulate = false;
    frameCount = 0;
    fps = 0;
    fpsTimer.start();

    QTimer* fpsUpdateTimer = new QTimer(this);
    connect(fpsUpdateTimer, &QTimer::timeout, this, [this]() {
        fps = frameCount;
        frameCount = 0;
    });
    fpsUpdateTimer->start(100);
}

void CanvasWidget::Render(float /*deltatime*/) {
    angle += 2.0f;
    if (angle >= 360.0f) angle = 0;
    update();
}

void CanvasWidget::simulation() {
    simulate = true;
}

void CanvasWidget::editor() {
    simulate = false;
}

void CanvasWidget::setTransformMode(TransformMode mode) {
    currentMode = mode;
    if (mode != DrawTrajectory) {
        setTrajectoryDrawingMode(false);
    }
    update();
}

void CanvasWidget::setTrajectoryDrawingMode(bool enabled) {
    isDrawingTrajectory = enabled;
    if (enabled) {
        currentMode = DrawTrajectory;
        currentTrajectory.clear();
        setCursor(Qt::CrossCursor);
        Console::log("Trajectory drawing mode enabled");
    } else {
        currentMode = Translate;
        setCursor(Qt::ArrowCursor);
        Console::log("Trajectory drawing mode disabled");
    }
    update();
}

void CanvasWidget::saveTrajectory() {
    Console::log("saveTrajectory called");
    if (!isDrawingTrajectory) {
        Console::error("Trajectory drawing mode is not enabled");
        return;
    }
    if (currentTrajectory.empty()) {
        Console::error("No waypoints to save");
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

    entry.trajectory->Trajectories.clear();

    for (Waypoints* waypoint : currentTrajectory) {
        Waypoints* newWaypoint = new Waypoints();
        newWaypoint->position = new Vector(waypoint->position->x, waypoint->position->y, waypoint->position->z);
        entry.trajectory->addTrajectory(newWaypoint);
    }
    entry.trajectory->Active = true;

    Console::log("Saved trajectory with " + std::to_string(currentTrajectory.size()) + " waypoints for entity: " + selectedEntityId);

    QJsonObject trajJson = entry.trajectory->toJson();
    QJsonDocument doc(trajJson);
    Console::log("Trajectory JSON: " + QString(doc.toJson(QJsonDocument::Indented)).toStdString());

    for (Waypoints* waypoint : currentTrajectory) {
        delete waypoint->position;
        delete waypoint;
    }
    currentTrajectory.clear();

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
    Console::log("updateWaypointsFromInspector called for entity: " + entityId.toStdString() + " with " + std::to_string(waypoints.size()) + " waypoints");

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

    for (Waypoints* wp : entry.trajectory->Trajectories) {
        delete wp->position;
        delete wp;
    }
    entry.trajectory->Trajectories.clear();

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
        for (const QJsonValue& val : waypoints) {
            QJsonObject wpObj = val.toObject();
            QJsonObject posObj = wpObj["position"].toObject();
            Waypoints* newWaypoint = new Waypoints();
            newWaypoint->position = new Vector(
                posObj["x"].toDouble(),
                posObj["y"].toDouble(),
                posObj["z"].toDouble()
                );
            currentTrajectory.push_back(newWaypoint);
        }
    }

    update();
    Console::log("Updated trajectory for entity: " + entityId.toStdString() + " with " + std::to_string(entry.trajectory->Trajectories.size()) + " waypoints");
}

void CanvasWidget::handleMousePress(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        isPanning = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    } else if (event->button() == Qt::LeftButton) {
        if (currentMode == DrawTrajectory && isDrawingTrajectory && !selectedEntityId.empty()) {
            Waypoints* waypoint = new Waypoints();
            waypoint->position = new Vector(event->pos().x() - canvasOffset.x(), event->pos().y() - canvasOffset.y(), 0);
            currentTrajectory.push_back(waypoint);
            Console::log("Added waypoint at (" + std::to_string(waypoint->position->x) + ", " + std::to_string(waypoint->position->y) + ")");

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

            QJsonDocument doc(waypointsArray);
            Console::log("Emitting trajectoryUpdated with waypoints: " + QString(doc.toJson(QJsonDocument::Indented)).toStdString());

            emit trajectoryUpdated(QString::fromStdString(selectedEntityId), waypointsArray);
            update();
            return;
        }
        for (auto& [id, entry] : Meshes) {
            float dx = event->pos().x() - entry.position->x - canvasOffset.x();
            float dy = event->pos().y() - entry.position->y - canvasOffset.y();
            if (selectedEntityId != id && (qAbs(dx) < 10 && qAbs(dy) < 10)) {
                selectedEntityId = id;
                emit selectEntitybyCursor(QString::fromStdString(id));
                selectEntity = true;
                Console::log("Selected entity: " + id);
                return;
            } else if (qAbs(dx - 50) < 25 && qAbs(dy) < 10) {
                activeDragAxis = "x";
                selectedEntityId = id;
                emit selectEntitybyCursor(QString::fromStdString(id));
                dragStartPos = event->pos();
                selectEntity = true;
                Console::log("Selected entity (x-axis): " + id);
                return;
            } else if (qAbs(dy - 50) < 25 && qAbs(dx) < 10) {
                activeDragAxis = "y";
                selectedEntityId = id;
                emit selectEntitybyCursor(QString::fromStdString(id));
                dragStartPos = event->pos();
                selectEntity = true;
                Console::log("Selected entity (y-axis): " + id);
                return;
            }
        }
        selectedEntityId = "";
        Console::log("Deselected entity");
    }
}

void CanvasWidget::handleMouseMove(QMouseEvent *event) {
    if (isPanning) {
        QPoint delta = event->pos() - lastMousePos;
        canvasOffset += delta;
        lastMousePos = event->pos();
        update();
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

    if (selectEntity && !simulate && currentMode != DrawTrajectory) {
        if (currentMode == Translate) {
            if (activeDragAxis == "x" || activeDragAxis == "both")
                entry.position->x += delta.x();
            if (activeDragAxis == "y" || activeDragAxis == "both")
                entry.position->y += delta.y();
        } else if (currentMode == Rotate) {
            float angleChange = delta.x() * 0.5f;
            entry.rotation->z += angleChange;
        } else if (currentMode == Scale) {
            if (activeDragAxis == "x" || activeDragAxis == "both")
                entry.size->x += delta.x() * 0.01f;
            if (activeDragAxis == "y" || activeDragAxis == "both")
                entry.size->y += delta.y() * 0.01f;
        } else if (currentMode == Panning) {
            activeDragAxis = "none";
        }
    }

    dragStartPos = event->pos();
    update();
}

void CanvasWidget::handleMouseRelease(QMouseEvent *event) {
    activeDragAxis = "";
    selectEntity = false;
    if (event->button() == Qt::MiddleButton) {
        isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void CanvasWidget::handleKeyPress(QKeyEvent *event) {
    Console::log("Key pressed: " + std::to_string(event->key()));
    if (event->key() == Qt::Key_1) {
        currentMode = Translate;
        Console::log("Mode set to Translate");
    } else if (event->key() == Qt::Key_2) {
        currentMode = Rotate;
        Console::log("Mode set to Rotate");
    } else if (event->key() == Qt::Key_3) {
        currentMode = Scale;
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
        }
    }
}

void CanvasWidget::handlePaint(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(canvasOffset);

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
        for (const Waypoints* waypoint : currentTrajectory) {
            polyline << QPointF(waypoint->position->x, waypoint->position->y);
            painter.setBrush(Qt::magenta);
            painter.drawEllipse(QPointF(waypoint->position->x, waypoint->position->y), 3, 3);
            painter.setBrush(Qt::NoBrush);
        }
        if (polyline.size() > 1) {
            painter.drawPolyline(polyline);
        }
        painter.restore();
    }
    drawSceneInformation(painter);
    drawEntityInformation(painter);
    drawTransformGizmo(painter);

    QPointF center(width() / 2.0, height() / 2.0);
    painter.setBrush(Qt::green);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, 10, 10);

    painter.setPen(QPen(Qt::red, 2));
    float radius = qMin(width(), height()) / 2.5;
    float rad = qDegreesToRadians(angle);
    QPointF endpoint(center.x() + radius * cos(rad), center.y() - radius * sin(rad));
    painter.drawLine(center, endpoint);

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

        // Gravity
        entry.velocity->y += gravity * deltaTime;

        // Move
        entry.position->x += entry.velocity->x * deltaTime;
        entry.position->y += entry.velocity->y * deltaTime;

        float w = entry.size->x * 25;
        float h = entry.size->y * 25;

        // ==== Screen Boundary Collisions ====
        // Bottom
        float bottom = entry.position->y + h / 2.0f;
        if (bottom >= height()) {
            entry.position->y = height() - h / 2.0f;
            entry.velocity->y = -entry.velocity->y * bounceFactor;
            if (qAbs(entry.velocity->y) < 10.0f) entry.velocity->y = 0.0f;
        }

        // Top
        float top = entry.position->y - h / 2.0f;
        if (top <= 0) {
            entry.position->y = h / 2.0f;
            entry.velocity->y = -entry.velocity->y * bounceFactor;
        }

        // Left
        float left = entry.position->x - w / 2.0f;
        if (left <= 0) {
            entry.position->x = w / 2.0f;
            entry.velocity->x = -entry.velocity->x * bounceFactor;
        }

        // Right
        float right = entry.position->x + w / 2.0f;
        if (right >= width()) {
            entry.position->x = width() - w / 2.0f;
            entry.velocity->x = -entry.velocity->x * bounceFactor;
        }
    }

    // ==== Entity vs Entity Collision ====
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
    QFont font("Arial", 10, QFont::Bold);
    painter.setFont(font);
    painter.setPen(QColor(255, 255, 255, 200));

    if (showInformation) {
        QString modeText = QString(" %1").arg(simulate ? "Simulation" : "Editor");
        painter.drawText(QPointF(width() - 100 - canvasOffset.x(), height() - 10 - canvasOffset.y()), modeText);
    }

    if (showFPS) {
        QString fpsText = QString("FPS: %1").arg(fps*10);
        painter.drawText(QPointF(10 - canvasOffset.x(), 20 - canvasOffset.y()), fpsText);
    }
}

void CanvasWidget::drawEntityInformation(QPainter& painter) {
    if (!showInformation || simulate) return;
    if (Meshes.find(selectedEntityId) == Meshes.end()) return;

    QString name = Meshes[selectedEntityId].name;
    auto& pos = Meshes[selectedEntityId].position;
    QString text = QString("pos: x %1, y %2, z %3")
                       .arg(pos->x)
                       .arg(pos->y)
                       .arg(pos->z);
    if (currentMode == Rotate) {
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
    } else if (currentMode == DrawTrajectory) {
        text = QString("Drawing Trajectory: %1 waypoints").arg(currentTrajectory.size());
    }

    QFont font("Arial", 10, QFont::Bold);
    painter.setFont(font);
    painter.setPen(QColor(255, 255, 255, 200));
    painter.drawText(QPointF(10 - canvasOffset.x(), height() - 50 - canvasOffset.y()), name);
    painter.drawText(QPointF(10 - canvasOffset.x(), height() - 10 - canvasOffset.y()), text);
    painter.drawText(QPointF(10 - canvasOffset.x(), height() - 30 - canvasOffset.y()), QString("Mode: %1")
                                                                                           .arg(currentMode == Translate ? "Translate" : currentMode == Rotate ? "Rotate" : currentMode == Scale ? "Scale" : "Draw Trajectory"));
}


void CanvasWidget::drawTransformGizmo(QPainter& painter) {
    if (simulate || currentMode == DrawTrajectory) return;
    if (Meshes.find(selectedEntityId) == Meshes.end()) return;

    auto& pos = Meshes[selectedEntityId].position;
    auto& angle = Meshes[selectedEntityId].rotation->z;
    QPointF base(pos->x, pos->y);

    if (currentMode == Translate) {
        painter.setPen(QPen(Qt::blue, (activeDragAxis == "x") ? 4 : 2));
        painter.drawLine(base, base + QPointF(50, 0)); // X axis
        painter.setPen(QPen(Qt::green, (activeDragAxis == "y") ? 4 : 2));
        painter.drawLine(base, base + QPointF(0, 50)); // Y axis
    } else if (currentMode == Rotate) {
        painter.setPen(QPen(QColor(255, 255, 0, 150), 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(base, 40, 40);
        float radius = 40;
        float rad = qDegreesToRadians(-angle);
        QPointF endpoint(base.x() + radius * cos(rad), base.y() - radius * sin(rad));
        painter.drawLine(base, endpoint);
    } else if (currentMode == Scale) {
        painter.setPen(QPen(Qt::red, 2));
        painter.setBrush(Qt::red);
        painter.drawRect(QRectF(base + QPointF(40, 0), QSizeF(6, 6))); // X box
        painter.setBrush(Qt::green);
        painter.drawRect(QRectF(base + QPointF(0, 40), QSizeF(6, 6))); // Y box
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
    if (!showMesh) return;
    for (auto& [id, entry] : Meshes) {
        Mesh* mesh = entry.mesh;
        if (!mesh) continue;

        QPolygonF polygon;

        float angle = qDegreesToRadians(entry.rotation->z);
        float cosA = cos(angle);
        float sinA = sin(angle);

        for (Vector* point : mesh->polygen) {
            if (!point) continue;

            float x = point->x * entry.size->x;
            float y = point->y * entry.size->y;

            float rotatedX = x * cosA - y * sinA;
            float rotatedY = x * sinA + y * cosA;

            float finalX = entry.position->x + rotatedX;
            float finalY = entry.position->y + rotatedY;

            polygon << QPointF(finalX, finalY);
        }

        painter.setPen(QPen(*mesh->color, mesh->lineWidth));
        painter.setBrush(*mesh->color);

        if (mesh->closePath)
            painter.drawPolygon(polygon);
        else
            painter.drawPolyline(polygon);
    }
}

void CanvasWidget::drawImage(QPainter& painter) {
    for (auto& [id, entry] : Meshes) {
        Mesh* mesh = entry.mesh;
        if (!mesh) continue;

        QPixmap img(mesh->Sprite->data());
        if (!img.isNull()) {
            QPixmap scaled = img.scaled(entry.size->x * 25, entry.size->y * 25,
                                        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

            float x = entry.position->x;
            float y = entry.position->y;
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
        if (!entry.trajectory || !entry.trajectory->Active || entry.trajectory->Trajectories.empty()) continue;

        painter.save();
        QPen pen(Qt::cyan, 2);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        QPolygonF polyline;
        for (const Waypoints* waypoint : entry.trajectory->Trajectories) {
            polyline << QPointF(waypoint->position->x, waypoint->position->y);
            painter.setBrush(Qt::cyan);
            painter.drawEllipse(QPointF(waypoint->position->x, waypoint->position->y), 3, 3);
            painter.setBrush(Qt::NoBrush);
        }
        if (polyline.size() > 1) {
            painter.drawPolyline(polyline);
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


// //=======test for zoom in zoom out

// #include "canvaswidget.h"
// #include <QTimer>
// #include <QDebug>
// #include <QMouseEvent>
// #include <cmath>
// #include <core/Debug/console.h>

// CanvasWidget::CanvasWidget(QWidget *parent) : QWidget(parent) {
//     setMinimumSize(300, 300);
//     setAttribute(Qt::WA_TranslucentBackground);
//     setWindowFlags(Qt::FramelessWindowHint);
//     setFocusPolicy(Qt::StrongFocus);
//     setUpdatesEnabled(true);
//     setMouseTracking(true);
//     simulate = false;
//     frameCount = 0;
//     fps = 0;
//     fpsTimer.start();

//     QTimer* fpsUpdateTimer = new QTimer(this);
//     connect(fpsUpdateTimer, &QTimer::timeout, this, [this]() {
//         fps = frameCount;
//         frameCount = 0;
//     });
//     fpsUpdateTimer->start(100);
// }

// void CanvasWidget::Render(float /*deltatime*/) {
//     angle += 2.0f;
//     if (angle >= 360.0f) angle = 0;
//     update();
// }

// void CanvasWidget::simulation() {
//     simulate = true;
// }

// void CanvasWidget::editor() {
//     simulate = false;
// }

// void CanvasWidget::setTransformMode(TransformMode mode) {
//     currentMode = mode;
//     if (mode != DrawTrajectory) {
//         setTrajectoryDrawingMode(false);
//     }
//     update();
// }

// void CanvasWidget::setTrajectoryDrawingMode(bool enabled) {
//     isDrawingTrajectory = enabled;
//     if (enabled) {
//         currentMode = DrawTrajectory;
//         currentTrajectory.clear();
//         setCursor(Qt::CrossCursor);
//         Console::log("Trajectory drawing mode enabled");
//     } else {
//         currentMode = Translate;
//         setCursor(Qt::ArrowCursor);
//         Console::log("Trajectory drawing mode disabled");
//     }
//     update();
// }

// void CanvasWidget::saveTrajectory() {
//     Console::log("saveTrajectory called");
//     if (!isDrawingTrajectory) {
//         Console::error("Trajectory drawing mode is not enabled");
//         return;
//     }
//     if (currentTrajectory.empty()) {
//         Console::error("No waypoints to save");
//         return;
//     }
//     if (selectedEntityId.empty()) {
//         Console::error("No entity selected for trajectory");
//         return;
//     }

//     Console::log("Attempting to save trajectory for entity: " + selectedEntityId);

//     auto it = Meshes.find(selectedEntityId);
//     if (it == Meshes.end()) {
//         Console::error("Selected entity not found in Meshes: " + selectedEntityId);
//         return;
//     }

//     MeshEntry& entry = it->second;
//     if (!entry.trajectory) {
//         entry.trajectory = new Trajectory();
//         entry.trajectory->ID = selectedEntityId;
//     }

//     entry.trajectory->Trajectories.clear();

//     for (Waypoints* waypoint : currentTrajectory) {
//         Waypoints* newWaypoint = new Waypoints();
//         newWaypoint->position = new Vector(waypoint->position->x, waypoint->position->y, waypoint->position->z);
//         entry.trajectory->addTrajectory(newWaypoint);
//     }
//     entry.trajectory->Active = true;

//     Console::log("Saved trajectory with " + std::to_string(currentTrajectory.size()) + " waypoints for entity: " + selectedEntityId);

//     QJsonObject trajJson = entry.trajectory->toJson();
//     QJsonDocument doc(trajJson);
//     Console::log("Trajectory JSON: " + QString(doc.toJson(QJsonDocument::Indented)).toStdString());

//     for (Waypoints* waypoint : currentTrajectory) {
//         delete waypoint->position;
//         delete waypoint;
//     }
//     currentTrajectory.clear();

//     QJsonArray waypointsArray;
//     for (const Waypoints* wp : entry.trajectory->Trajectories) {
//         QJsonObject wpObj;
//         QJsonObject posObj;
//         posObj["type"] = "vector";
//         posObj["x"] = wp->position->x;
//         posObj["y"] = wp->position->y;
//         posObj["z"] = wp->position->z;
//         wpObj["position"] = posObj;
//         waypointsArray.append(wpObj);
//     }
//     emit trajectoryUpdated(QString::fromStdString(selectedEntityId), waypointsArray);
//     Console::log("Emitted trajectoryUpdated signal for entity: " + selectedEntityId);
// }

// void CanvasWidget::updateWaypointsFromInspector(QString entityId, QJsonArray waypoints) {
//     Console::log("updateWaypointsFromInspector called for entity: " + entityId.toStdString() + " with " + std::to_string(waypoints.size()) + " waypoints");

//     auto it = Meshes.find(entityId.toStdString());
//     if (it == Meshes.end()) {
//         Console::error("Entity not found in Meshes: " + entityId.toStdString());
//         return;
//     }

//     MeshEntry& entry = it->second;
//     if (!entry.trajectory) {
//         entry.trajectory = new Trajectory();
//         entry.trajectory->ID = entityId.toStdString();
//     }

//     for (Waypoints* wp : entry.trajectory->Trajectories) {
//         delete wp->position;
//         delete wp;
//     }
//     entry.trajectory->Trajectories.clear();

//     for (const QJsonValue& val : waypoints) {
//         QJsonObject wpObj = val.toObject();
//         if (!wpObj.contains("position")) {
//             Console::error("Invalid waypoint data: missing position");
//             continue;
//         }
//         QJsonObject posObj = wpObj["position"].toObject();
//         Waypoints* newWaypoint = new Waypoints();
//         newWaypoint->position = new Vector(
//             posObj["x"].toDouble(),
//             posObj["y"].toDouble(),
//             posObj["z"].toDouble()
//             );
//         entry.trajectory->addTrajectory(newWaypoint);
//     }
//     entry.trajectory->Active = !waypoints.isEmpty();

//     if (isDrawingTrajectory && selectedEntityId == entityId.toStdString()) {
//         for (Waypoints* wp : currentTrajectory) {
//             delete wp->position;
//             delete wp;
//         }
//         currentTrajectory.clear();
//         for (const QJsonValue& val : waypoints) {
//             QJsonObject wpObj = val.toObject();
//             QJsonObject posObj = wpObj["position"].toObject();
//             Waypoints* newWaypoint = new Waypoints();
//             newWaypoint->position = new Vector(
//                 posObj["x"].toDouble(),
//                 posObj["y"].toDouble(),
//                 posObj["z"].toDouble()
//                 );
//             currentTrajectory.push_back(newWaypoint);
//         }
//     }

//     update();
//     Console::log("Updated trajectory for entity: " + entityId.toStdString() + " with " + std::to_string(entry.trajectory->Trajectories.size()) + " waypoints");
// }

// void CanvasWidget::handleMousePress(QMouseEvent *event) {
//     if (event->button() == Qt::MiddleButton) {
//         isPanning = true;
//         lastMousePos = event->pos();
//         setCursor(Qt::ClosedHandCursor);
//         return;
//     } else if (event->button() == Qt::LeftButton) {
//         if (currentMode == DrawTrajectory && isDrawingTrajectory && !selectedEntityId.empty()) {
//             Waypoints* waypoint = new Waypoints();
//             // Adjust mouse position for zoom
//             waypoint->position = new Vector((event->pos().x() - canvasOffset.x()) / zoomFactor,
//                                             (event->pos().y() - canvasOffset.y()) / zoomFactor, 0);
//             currentTrajectory.push_back(waypoint);
//             Console::log("Added waypoint at (" + std::to_string(waypoint->position->x) + ", " + std::to_string(waypoint->position->y) + ")");

//             QJsonArray waypointsArray;
//             for (const Waypoints* wp : currentTrajectory) {
//                 QJsonObject wpObj;
//                 QJsonObject posObj;
//                 posObj["type"] = "vector";
//                 posObj["x"] = wp->position->x;
//                 posObj["y"] = wp->position->y;
//                 posObj["z"] = wp->position->z;
//                 wpObj["position"] = posObj;
//                 waypointsArray.append(wpObj);
//             }

//             emit trajectoryUpdated(QString::fromStdString(selectedEntityId), waypointsArray);
//             update();
//             return;
//         }
//         for (auto& [id, entry] : Meshes) {
//             // Adjust coordinates for zoom
//             float dx = (event->pos().x() - canvasOffset.x()) / zoomFactor - entry.position->x;
//             float dy = (event->pos().y() - canvasOffset.y()) / zoomFactor - entry.position->y;
//             if (selectedEntityId != id && (qAbs(dx) < 10 / zoomFactor && qAbs(dy) < 10 / zoomFactor)) {
//                 selectedEntityId = id;
//                 emit selectEntitybyCursor(QString::fromStdString(id));
//                 selectEntity = true;
//                 Console::log("Selected entity: " + id);
//                 return;
//             } else if (qAbs(dx - 50 / zoomFactor) < 25 / zoomFactor && qAbs(dy) < 10 / zoomFactor) {
//                 activeDragAxis = "x";
//                 selectedEntityId = id;
//                 emit selectEntitybyCursor(QString::fromStdString(id));
//                 dragStartPos = event->pos();
//                 selectEntity = true;
//                 Console::log("Selected entity (x-axis): " + id);
//                 return;
//             } else if (qAbs(dy - 50 / zoomFactor) < 25 / zoomFactor && qAbs(dx) < 10 / zoomFactor) {
//                 activeDragAxis = "y";
//                 selectedEntityId = id;
//                 emit selectEntitybyCursor(QString::fromStdString(id));
//                 dragStartPos = event->pos();
//                 selectEntity = true;
//                 Console::log("Selected entity (y-axis): " + id);
//                 return;
//             }
//         }
//         selectedEntityId = "";
//         Console::log("Deselected entity");
//     }
// }

// void CanvasWidget::handleMouseMove(QMouseEvent *event) {
//     if (isPanning) {
//         QPoint delta = event->pos() - lastMousePos;
//         canvasOffset += delta;
//         lastMousePos = event->pos();
//         update();
//     }

//     if (Meshes.find(selectedEntityId) == Meshes.end()) return;

//     auto& entry = Meshes[selectedEntityId];
//     QPointF delta = (event->pos() - dragStartPos) / zoomFactor; // Adjust for zoom

//     float dx = (event->pos().x() - canvasOffset.x()) / zoomFactor - entry.position->x;
//     float dy = (event->pos().y() - canvasOffset.y()) / zoomFactor - entry.position->y;

//     if (qAbs(dx) < 10 / zoomFactor && qAbs(dy) < 10 / zoomFactor) {
//         activeDragAxis = "both";
//     } else if (qAbs(dx - 50 / zoomFactor) < 25 / zoomFactor && qAbs(dy) < 10 / zoomFactor) {
//         activeDragAxis = "x";
//     } else if (qAbs(dy - 50 / zoomFactor) < 25 / zoomFactor && qAbs(dx) < 10 / zoomFactor) {
//         activeDragAxis = "y";
//     }

//     if (selectEntity && !simulate && currentMode != DrawTrajectory) {
//         if (currentMode == Translate) {
//             if (activeDragAxis == "x" || activeDragAxis == "both")
//                 entry.position->x += delta.x();
//             if (activeDragAxis == "y" || activeDragAxis == "both")
//                 entry.position->y += delta.y();
//         } else if (currentMode == Rotate) {
//             float angleChange = delta.x() * 0.5f;
//             entry.rotation->z += angleChange;
//         } else if (currentMode == Scale) {
//             if (activeDragAxis == "x" || activeDragAxis == "both")
//                 entry.size->x += delta.x() * 0.01f;
//             if (activeDragAxis == "y" || activeDragAxis == "both")
//                 entry.size->y += delta.y() * 0.01f;
//         } else if (currentMode == Panning) {
//             activeDragAxis = "none";
//         }
//     }

//     dragStartPos = event->pos();
//     update();
// }

// void CanvasWidget::handleMouseRelease(QMouseEvent *event) {
//     activeDragAxis = "";
//     selectEntity = false;
//     if (event->button() == Qt::MiddleButton) {
//         isPanning = false;
//         setCursor(Qt::ArrowCursor);
//     }
// }

// void CanvasWidget::handleKeyPress(QKeyEvent *event) {
//     Console::log("Key pressed: " + std::to_string(event->key()));
//     if (event->key() == Qt::Key_1) {
//         currentMode = Translate;
//         Console::log("Mode set to Translate");
//     } else if (event->key() == Qt::Key_2) {
//         currentMode = Rotate;
//         Console::log("Mode set to Rotate");
//     } else if (event->key() == Qt::Key_3) {
//         currentMode = Scale;
//         Console::log("Mode set to Scale");
//     } else if (event->key() == Qt::Key_4) {
//         simulate = !simulate;
//         Console::log("Simulation mode: " + std::string(simulate ? "enabled" : "disabled"));
//     } else if (event->key() == Qt::Key_Escape) {
//         Console::log("Escape key pressed, currentMode: " + std::to_string(currentMode));
//         if (currentMode == DrawTrajectory) {
//             Console::log("Calling saveTrajectory");
//             saveTrajectory();
//             setTrajectoryDrawingMode(false);
//         }
//     }
// }

// void CanvasWidget::handlePaint(QPaintEvent *event) {
//     QPainter painter(this);
//     painter.setRenderHint(QPainter::Antialiasing);
//     painter.translate(canvasOffset);
//     painter.scale(zoomFactor, zoomFactor); // Apply zoom to all drawings

//     drawGridLines(painter);
//     drawTrajectory(painter);
//     drawMesh(painter);
//     drawImage(painter);
//     drawSelectionOutline(painter);
//     drawCollider(painter);

//     if (isDrawingTrajectory && !currentTrajectory.empty()) {
//         painter.save();
//         QPen pen(Qt::magenta, 2 / zoomFactor, Qt::DashLine); // Adjust pen width
//         painter.setPen(pen);
//         painter.setBrush(Qt::NoBrush);
//         QPolygonF polyline;
//         for (const Waypoints* waypoint : currentTrajectory) {
//             polyline << QPointF(waypoint->position->x, waypoint->position->y);
//             painter.setBrush(Qt::magenta);
//             painter.drawEllipse(QPointF(waypoint->position->x, waypoint->position->y), 3 / zoomFactor, 3 / zoomFactor);
//             painter.setBrush(Qt::NoBrush);
//         }
//         if (polyline.size() > 1) {
//             painter.drawPolyline(polyline);
//         }
//         painter.restore();
//     }
//     painter.resetTransform(); // Reset transform for UI elements
//     painter.translate(canvasOffset);
//     drawSceneInformation(painter);
//     drawEntityInformation(painter);
//     drawTransformGizmo(painter);

//     painter.resetTransform(); // Reset for center marker
//     QPointF center(width() / 2.0, height() / 2.0);
//     painter.setBrush(Qt::green);
//     painter.setPen(Qt::NoPen);
//     painter.drawEllipse(center, 10 / zoomFactor, 10 / zoomFactor);

//     painter.setPen(QPen(Qt::red, 2 / zoomFactor));
//     float radius = qMin(width(), height()) / 2.5 / zoomFactor;
//     float rad = qDegreesToRadians(angle);
//     QPointF endpoint(center.x() + radius * cos(rad), center.y() - radius * sin(rad));
//     painter.drawLine(center, endpoint);
//     frameCount++;
// }

// void CanvasWidget::keyPressEvent(QKeyEvent *event) {
//     handleKeyPress(event);
// }

// void CanvasWidget::paintEvent(QPaintEvent *event) {
//     handlePaint(event);
// }

// void CanvasWidget::mousePressEvent(QMouseEvent *event) {
//     handleMousePress(event);
// }

// void CanvasWidget::mouseMoveEvent(QMouseEvent *event) {
//     handleMouseMove(event);
// }

// void CanvasWidget::mouseReleaseEvent(QMouseEvent *event) {
//     handleMouseRelease(event);
// }

// bool isColliding(const MeshEntry& a, const MeshEntry& b) {
//     float ax = a.position->x, ay = a.position->y;
//     float aw = a.size->x * 25, ah = a.size->y * 25;

//     float bx = b.position->x, by = b.position->y;
//     float bw = b.size->x * 25, bh = b.size->y * 25;

//     return (std::abs(ax - bx) < (aw + bw) / 2.0f) &&
//            (std::abs(ay - by) < (ah + bh) / 2.0f);
// }

// void CanvasWidget::applyGravityAndBounce(float deltaTime) {
//     const float gravity = 1980.0f;
//     const float bounceFactor = 0.9f;

//     for (auto& [id, entry] : Meshes) {
//         if (!entry.position || !entry.velocity || !entry.size) continue;

//         // Gravity
//         entry.velocity->y += gravity * deltaTime;

//         // Move
//         entry.position->x += entry.velocity->x * deltaTime;
//         entry.position->y += entry.velocity->y * deltaTime;

//         float w = entry.size->x * 25;
//         float h = entry.size->y * 25;

//         // ==== Screen Boundary Collisions ====
//         // Bottom
//         float bottom = entry.position->y + h / 2.0f;
//         if (bottom >= height()) {
//             entry.position->y = height() - h / 2.0f;
//             entry.velocity->y = -entry.velocity->y * bounceFactor;
//             if (qAbs(entry.velocity->y) < 10.0f) entry.velocity->y = 0.0f;
//         }

//         // Top
//         float top = entry.position->y - h / 2.0f;
//         if (top <= 0) {
//             entry.position->y = h / 2.0f;
//             entry.velocity->y = -entry.velocity->y * bounceFactor;
//         }

//         // Left
//         float left = entry.position->x - w / 2.0f;
//         if (left <= 0) {
//             entry.position->x = w / 2.0f;
//             entry.velocity->x = -entry.velocity->x * bounceFactor;
//         }

//         // Right
//         float right = entry.position->x + w / 2.0f;
//         if (right >= width()) {
//             entry.position->x = width() - w / 2.0f;
//             entry.velocity->x = -entry.velocity->x * bounceFactor;
//         }
//     }

//     // ==== Entity vs Entity Collision ====
//     std::vector<std::string> ids;
//     for (const auto& [id, _] : Meshes) {
//         ids.push_back(id);
//     }

//     for (size_t i = 0; i < ids.size(); ++i) {
//         for (size_t j = i + 1; j < ids.size(); ++j) {
//             MeshEntry& a = Meshes[ids[i]];
//             MeshEntry& b = Meshes[ids[j]];

//             if (isColliding(a, b)) {
//                 std::swap(a.velocity->x, b.velocity->x);
//                 std::swap(a.velocity->y, b.velocity->y);
//             }
//         }
//     }
// }
// void CanvasWidget::drawGridLines(QPainter& painter) {
//     int alpha = (gridOpacity * 255) / 100;

//     if (showXGrid) {
//         painter.setPen(QColor(128, 128, 128, alpha));
//         for (int x = 0; x < width(); x += 20)
//             painter.drawLine(x, 0, x, height());
//     }

//     if (showYGrid) {
//         painter.setPen(QColor(128, 128, 128, alpha));
//         for (int y = 0; y < height(); y += 20)
//             painter.drawLine(0, y, width(), y);
//     }
// }

// void CanvasWidget::drawSceneInformation(QPainter& painter) {
//     QFont font("Arial", 10, QFont::Bold);
//     painter.setFont(font);
//     painter.setPen(QColor(255, 255, 255, 200));

//     if (showInformation) {
//         QString modeText = QString(" %1").arg(simulate ? "Simulation" : "Editor");
//         painter.drawText(QPointF(width() - 100 - canvasOffset.x(), height() - 10 - canvasOffset.y()), modeText);
//     }

//     if (showFPS) {
//         QString fpsText = QString("FPS: %1").arg(fps*10);
//         painter.drawText(QPointF(10 - canvasOffset.x(), 20 - canvasOffset.y()), fpsText);
//     }
// }

// void CanvasWidget::drawEntityInformation(QPainter& painter) {
//     if (!showInformation || simulate) return;
//     if (Meshes.find(selectedEntityId) == Meshes.end()) return;

//     QString name = Meshes[selectedEntityId].name;
//     auto& pos = Meshes[selectedEntityId].position;
//     QString text = QString("pos: x %1, y %2, z %3")
//                        .arg(pos->x)
//                        .arg(pos->y)
//                        .arg(pos->z);
//     if (currentMode == Rotate) {
//         auto& rot = Meshes[selectedEntityId].rotation;
//         text = QString("rot: x %1, y %2, z %3")
//                    .arg(rot->x)
//                    .arg(rot->y)
//                    .arg(rot->z);
//     } else if (currentMode == Scale) {
//         auto& size = Meshes[selectedEntityId].size;
//         text = QString("size: x %1, y %2, z %3")
//                    .arg(size->x)
//                    .arg(size->y)
//                    .arg(size->z);
//     } else if (currentMode == DrawTrajectory) {
//         text = QString("Drawing Trajectory: %1 waypoints").arg(currentTrajectory.size());
//     }

//     QFont font("Arial", 10, QFont::Bold);
//     painter.setFont(font);
//     painter.setPen(QColor(255, 255, 255, 200));
//     painter.drawText(QPointF(10 - canvasOffset.x(), height() - 50 - canvasOffset.y()), name);
//     painter.drawText(QPointF(10 - canvasOffset.x(), height() - 10 - canvasOffset.y()), text);
//     painter.drawText(QPointF(10 - canvasOffset.x(), height() - 30 - canvasOffset.y()), QString("Mode: %1")
//                                                                                            .arg(currentMode == Translate ? "Translate" : currentMode == Rotate ? "Rotate" : currentMode == Scale ? "Scale" : "Draw Trajectory"));
// }


// void CanvasWidget::drawTransformGizmo(QPainter& painter) {
//     if (simulate || currentMode == DrawTrajectory) return;
//     if (Meshes.find(selectedEntityId) == Meshes.end()) return;

//     auto& pos = Meshes[selectedEntityId].position;
//     auto& angle = Meshes[selectedEntityId].rotation->z;
//     QPointF base(pos->x, pos->y);

//     if (currentMode == Translate) {
//         painter.setPen(QPen(Qt::blue, (activeDragAxis == "x") ? 4 : 2));
//         painter.drawLine(base, base + QPointF(50, 0)); // X axis
//         painter.setPen(QPen(Qt::green, (activeDragAxis == "y") ? 4 : 2));
//         painter.drawLine(base, base + QPointF(0, 50)); // Y axis
//     } else if (currentMode == Rotate) {
//         painter.setPen(QPen(QColor(255, 255, 0, 150), 2, Qt::DashLine));
//         painter.setBrush(Qt::NoBrush);
//         painter.drawEllipse(base, 40, 40);
//         float radius = 40;
//         float rad = qDegreesToRadians(-angle);
//         QPointF endpoint(base.x() + radius * cos(rad), base.y() - radius * sin(rad));
//         painter.drawLine(base, endpoint);
//     } else if (currentMode == Scale) {
//         painter.setPen(QPen(Qt::red, 2));
//         painter.setBrush(Qt::red);
//         painter.drawRect(QRectF(base + QPointF(40, 0), QSizeF(6, 6))); // X box
//         painter.setBrush(Qt::green);
//         painter.drawRect(QRectF(base + QPointF(0, 40), QSizeF(6, 6))); // Y box
//     }
// }


// void CanvasWidget::drawSelectionOutline(QPainter& painter) {
//     if (!showOutline || simulate) return;
//     if (Meshes.find(selectedEntityId) == Meshes.end()) return;

//     auto& entry = Meshes[selectedEntityId];
//     auto& pos = entry.position;
//     auto& size = entry.size;

//     float w = size->x * 25;
//     float h = size->y * 25;
//     QRectF outlineRect(pos->x - w / 2.0f, pos->y - h / 2.0f, w, h);

//     QPen pen(Qt::yellow);
//     pen.setWidth(2);
//     pen.setStyle(Qt::DashLine);
//     painter.setPen(pen);
//     painter.setBrush(Qt::NoBrush);
//     painter.drawRect(outlineRect);
// }

// void CanvasWidget::drawCollider(QPainter& painter) {
//     if (!showColliders) return;
//     for (const auto& [id, entry] : Meshes) {
//         if (!entry.collider || !entry.position) continue;

//         painter.save();
//         painter.translate(entry.position->x, entry.position->y);

//         QPen pen(Qt::cyan);
//         pen.setStyle(Qt::DashDotLine);
//         pen.setWidth(2);
//         painter.setPen(pen);
//         painter.setBrush(Qt::NoBrush);

//         using ColliderType = Constants::ColliderType;
//         switch (entry.collider->collider) {
//         case ColliderType::Sphere:
//             painter.drawEllipse(QPointF(0, 0), entry.collider->Radius * entry.size->magnitude(), entry.collider->Radius * entry.size->magnitude());
//             break;
//         case ColliderType::Box:
//             painter.drawRect(QRectF(-entry.collider->Width/2, -entry.collider->Height/2,
//                                     entry.collider->Width, entry.collider->Height));
//             break;
//         case ColliderType::Cyclinder:
//             painter.drawRoundedRect(QRectF(-entry.collider->Width/2, -entry.collider->Height/2,
//                                            entry.collider->Width, entry.collider->Height), 10, 10);
//             break;
//         default:
//             break;
//         }

//         painter.restore();
//     }
// }

// void CanvasWidget::drawMesh(QPainter& painter) {
//     if (!showMesh) return;
//     painter.save();
//     painter.scale(zoomFactor, zoomFactor); // Apply zoom factor
//     for (auto& [id, entry] : Meshes) {
//         Mesh* mesh = entry.mesh;
//         if (!mesh) continue;

//         QPolygonF polygon;

//         float angle = qDegreesToRadians(entry.rotation->z);
//         float cosA = cos(angle);
//         float sinA = sin(angle);

//         for (Vector* point : mesh->polygen) {
//             if (!point) continue;

//             float x = point->x * entry.size->x;
//             float y = point->y * entry.size->y;

//             float rotatedX = x * cosA - y * sinA;
//             float rotatedY = x * sinA + y * cosA;

//             float finalX = entry.position->x + rotatedX;
//             float finalY = entry.position->y + rotatedY;

//             polygon << QPointF(finalX, finalY);
//         }

//         painter.setPen(QPen(*mesh->color, mesh->lineWidth / zoomFactor)); // Adjust line width for zoom
//         painter.setBrush(*mesh->color);

//         if (mesh->closePath)
//             painter.drawPolygon(polygon);
//         else
//             painter.drawPolyline(polygon);
//     }
//     painter.restore();
// }

// void CanvasWidget::drawImage(QPainter& painter) {
//     for (auto& [id, entry] : Meshes) {
//         Mesh* mesh = entry.mesh;
//         if (!mesh) continue;

//         QPixmap img(mesh->Sprite->data());
//         if (!img.isNull()) {
//             QPixmap scaled = img.scaled(entry.size->x * 25, entry.size->y * 25,
//                                         Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

//             float x = entry.position->x;
//             float y = entry.position->y;
//             float angle = entry.rotation->z;

//             painter.save();
//             painter.translate(x, y);
//             painter.rotate(angle);
//             painter.drawPixmap(QPointF(-scaled.width() / 2.0f, -scaled.height() / 2.0f), scaled);
//             painter.restore();
//         } else {
//             qDebug() << "Image load failed!";
//         }
//     }
// }

// void CanvasWidget::drawTrajectory(QPainter& painter) {
//     for (const auto& [id, entry] : Meshes) {
//         if (!entry.trajectory || !entry.trajectory->Active || entry.trajectory->Trajectories.empty()) continue;

//         painter.save();
//         QPen pen(Qt::cyan, 2);
//         painter.setPen(pen);
//         painter.setBrush(Qt::NoBrush);
//         QPolygonF polyline;
//         for (const Waypoints* waypoint : entry.trajectory->Trajectories) {
//             polyline << QPointF(waypoint->position->x, waypoint->position->y);
//             painter.setBrush(Qt::cyan);
//             painter.drawEllipse(QPointF(waypoint->position->x, waypoint->position->y), 3, 3);
//             painter.setBrush(Qt::NoBrush);
//         }
//         if (polyline.size() > 1) {
//             painter.drawPolyline(polyline);
//         }
//         painter.restore();
//     }
// }

// void CanvasWidget::toggleLayerVisibility(const QString& layer, bool visible) {
//     if (layer == "Collider") {
//         showColliders = visible;
//     } else if (layer == "Mesh") {
//         showMesh = visible;
//     } else if (layer == "Outline") {
//         showOutline = visible;
//     } else if (layer == "Information") {
//         showInformation = visible;
//     } else if (layer == "FPS") {
//         showFPS = visible;
//     }
//     update();
// }


// void CanvasWidget::onGISZoomChanged(int zoom) {
//     // Map zoom level to a canvas zoom factor (adjust formula as needed)
//     zoomFactor = std::pow(2.0, zoom - 3); // Example: base zoom level 3 maps to zoomFactor 1.0
//     Console::log("Canvas zoom factor set to: " + std::to_string(zoomFactor));
//     update();
// }
