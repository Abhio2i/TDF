
#include "canvaswidget.h"
#include <QTimer>
#include <QDebug>
#include <QMouseEvent>
#include <cmath>

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

    // Timer to update FPS every second
    QTimer* fpsUpdateTimer = new QTimer(this);
    connect(fpsUpdateTimer, &QTimer::timeout, this, [this]() {
        fps = frameCount;
        frameCount = 0;
    });
    fpsUpdateTimer->start(100); // 1 second interval
}

void CanvasWidget::Render(float deltatime) {
    angle += 2.0f;   // increment sweep
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
    update();
}

void CanvasWidget::handleMousePress(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        isPanning = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    } else if (event->button() == Qt::LeftButton) {
        for (auto& [id, entry] : Meshes) {
            float dx = event->pos().x() - entry.position->x - canvasOffset.x();
            float dy = event->pos().y() - entry.position->y - canvasOffset.y();
            if (selectedEntityId != id && (qAbs(dx) < 10 && qAbs(dy) < 10)) {
                selectedEntityId = id;
                emit selectEntitybyCursor(QString::fromStdString(id));
                selectEntity = true;
                return;
            } else if (qAbs(dx - 50) < 25 && qAbs(dy) < 10) {
                activeDragAxis = "x";
                selectedEntityId = id;
                emit selectEntitybyCursor(QString::fromStdString(id));
                dragStartPos = event->pos();
                selectEntity = true;
                return;
            } else if (qAbs(dy - 50) < 25 && qAbs(dx) < 10) {
                activeDragAxis = "y";
                selectedEntityId = id;
                emit selectEntitybyCursor(QString::fromStdString(id));
                dragStartPos = event->pos();
                selectEntity = true;
                return;
            }
        }
        selectedEntityId = "";
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

    // Set active axis based on gizmo hitbox
    if (qAbs(dx) < 10 && qAbs(dy) < 10) {
        activeDragAxis = "both";
    } else if (qAbs(dx - 50) < 25 && qAbs(dy) < 10) {
        activeDragAxis = "x";
    } else if (qAbs(dy - 50) < 25 && qAbs(dx) < 10) {
        activeDragAxis = "y";
    }

    // Do action based on mode
    if (selectEntity && !simulate) {
        if (currentMode == Translate) {
            if (activeDragAxis == "x" || activeDragAxis == "both")
                entry.position->x += delta.x();
            if (activeDragAxis == "y" || activeDragAxis == "both")
                entry.position->y += delta.y();
        } else if (currentMode == Rotate) {
            float angleChange = delta.x() * 0.5f; // sensitivity
            entry.rotation->z += angleChange;
        } else if (currentMode == Scale) {
            if (activeDragAxis == "x" || activeDragAxis == "both")
                entry.size->x += delta.x() * 0.01f; // scaling factor
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
    if (event->key() == Qt::Key_1)
        currentMode = Translate;
    else if (event->key() == Qt::Key_2)
        currentMode = Rotate;
    else if (event->key() == Qt::Key_3)
        currentMode = Scale;
    else if (event->key() == Qt::Key_4)
        simulate = !simulate;
}

void CanvasWidget::handlePaint(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(canvasOffset);

    drawGridLines(painter);
    drawMesh(painter);
    drawImage(painter);
    drawSelectionOutline(painter);
    drawCollider(painter);
    drawSceneInformation(painter);
    drawEntityInformation(painter);
    drawTransformGizmo(painter);

    // Center
    QPointF center(width() / 2.0, height() / 2.0);

    // Radar Dot
    painter.setBrush(Qt::green);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, 10, 10);

    // Radar Sweep Line
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
    }

    QFont font("Arial", 10, QFont::Bold);
    painter.setFont(font);
    painter.setPen(QColor(255, 255, 255, 200));
    painter.drawText(QPointF(10 - canvasOffset.x(), height() - 50 - canvasOffset.y()), name);
    painter.drawText(QPointF(10 - canvasOffset.x(), height() - 10 - canvasOffset.y()), text);
    painter.drawText(QPointF(10 - canvasOffset.x(), height() - 30 - canvasOffset.y()), QString("Mode: %1")
                                                                                           .arg(currentMode == Translate ? "Translate" : currentMode == Rotate ? "Rotate" : "Scale"));
}

void CanvasWidget::drawTransformGizmo(QPainter& painter) {
    if (simulate) return;
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
