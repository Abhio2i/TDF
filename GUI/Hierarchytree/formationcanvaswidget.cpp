#include "formationcanvaswidget.h"
#include <QPainterPath>
#include <QtMath>

FormationCanvasWidget::FormationCanvasWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(420, 420);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet("background-color: #0A1A28; border: 1px solid #27446d;");

    // Bottom controls
    QVBoxLayout *mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(4, 4, 4, 4);
    mainLay->addStretch();

    QHBoxLayout *btnLay = new QHBoxLayout();
    m_btnClear = new QPushButton("Clear");
    m_btnUndo  = new QPushButton("Undo");
    m_infoLabel = new QLabel("Click to add point | Drag to move | Right-click remove");
    m_infoLabel->setStyleSheet("color:#A0C0D0; font-size:11px;");

    m_btnClear->setFixedWidth(70);
    m_btnUndo->setFixedWidth(70);
    m_btnClear->setStyleSheet("QPushButton{background:#1A3652;color:white;border:1px solid #27446d;padding:4px;} QPushButton:hover{background:#27446d;}");
    m_btnUndo->setStyleSheet(m_btnClear->styleSheet());

    btnLay->addWidget(m_infoLabel);
    btnLay->addStretch();
    btnLay->addWidget(m_btnUndo);
    btnLay->addWidget(m_btnClear);
    mainLay->addLayout(btnLay);

    connect(m_btnClear, &QPushButton::clicked, this, &FormationCanvasWidget::onClearClicked);
    connect(m_btnUndo,  &QPushButton::clicked, this, &FormationCanvasWidget::onUndoClicked);
}

void FormationCanvasWidget::setPositions(const QJsonArray &positions)
{
    m_points.clear();
    for (const QJsonValue &v : positions) {
        QJsonObject o = v.toObject();
        m_points.append(QPointF(o.value("x").toDouble(), o.value("y").toDouble()));
    }
    update();
}

QJsonArray FormationCanvasWidget::getPositions() const
{
    QJsonArray arr;
    for (const QPointF &p : m_points) {
        QJsonObject o;
        o["x"] = p.x();
        o["y"] = p.y();
        arr.append(o);
    }
    return arr;
}

void FormationCanvasWidget::clearPoints()
{
    m_points.clear();
    emit positionsChanged(getPositions());
    update();
}

void FormationCanvasWidget::onClearClicked()
{
    clearPoints();
}

void FormationCanvasWidget::onUndoClicked()
{
    if (!m_points.isEmpty()) {
        m_points.removeLast();
        emit positionsChanged(getPositions());
        update();
    }
}

QPointF FormationCanvasWidget::screenToWorld(const QPoint &screen) const
{
    QPointF center(width() / 2.0, height() / 2.0 - 20);
    return QPointF((screen.x() - center.x()) / m_scale,
                   (center.y() - screen.y()) / m_scale);
}

QPoint FormationCanvasWidget::worldToScreen(const QPointF &world) const
{
    QPointF center(width() / 2.0, height() / 2.0 - 20);
    return QPoint(qRound(center.x() + world.x() * m_scale),
                  qRound(center.y() - world.y() * m_scale));
}

int FormationCanvasWidget::hitTest(const QPoint &screen, qreal radius) const
{
    for (int i = 0; i < m_points.size(); ++i) {
        QPoint sp = worldToScreen(m_points[i]);
        if (QLineF(sp, screen).length() <= radius)
            return i;
    }
    return -1;
}
void FormationCanvasWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPointF center(width() / 2.0, height() / 2.0 - 20);

    // Background grid
    p.setPen(QPen(QColor(30, 50, 70), 1));
    qreal gridPx = m_gridSize * m_scale;
    for (qreal x = fmod(center.x(), gridPx); x < width(); x += gridPx)
        p.drawLine(QPointF(x, 0), QPointF(x, height()));
    for (qreal y = fmod(center.y(), gridPx); y < height(); y += gridPx)
        p.drawLine(QPointF(0, y), QPointF(width(), y));

    // Axes
    p.setPen(QPen(QColor(60, 100, 140), 1.5));
    p.drawLine(QPointF(0, center.y()), QPointF(width(), center.y()));
    p.drawLine(QPointF(center.x(), 0), QPointF(center.x(), height()));

    // Origin (Mothership)
    p.setBrush(QColor(0, 180, 255));
    p.setPen(QPen(Qt::white, 2));
    p.drawEllipse(center, 10, 10);
    p.setPen(Qt::white);
    p.drawText(center + QPointF(14, 4), "Mothership");

    // Formation points (icons only, no connecting lines)
    if (!m_points.isEmpty()) {
        for (int i = 0; i < m_points.size(); ++i) {
            QPoint sp = worldToScreen(m_points[i]);

            qreal s = 10.0;
            QPainterPath planePath;
            planePath.moveTo(sp + QPointF(0.00 * s, -1.00 * s));   // nose tip
            planePath.lineTo(sp + QPointF(0.14 * s, -0.55 * s));   // right nose shoulder
            planePath.lineTo(sp + QPointF(1.00 * s,  0.05 * s));   // right wingtip
            planePath.lineTo(sp + QPointF(0.18 * s,  0.18 * s));   // right wing trailing inner
            planePath.lineTo(sp + QPointF(0.14 * s,  0.55 * s));   // right fuselage taper
            planePath.lineTo(sp + QPointF(0.42 * s,  0.85 * s));   // right tail wingtip
            planePath.lineTo(sp + QPointF(0.08 * s,  0.65 * s));   // right tail inner
            planePath.lineTo(sp + QPointF(0.00 * s,  0.92 * s));   // tail center
            planePath.lineTo(sp + QPointF(-0.08 * s,  0.65 * s));  // left tail inner
            planePath.lineTo(sp + QPointF(-0.42 * s,  0.85 * s));  // left tail wingtip
            planePath.lineTo(sp + QPointF(-0.14 * s,  0.55 * s));  // left fuselage taper
            planePath.lineTo(sp + QPointF(-0.18 * s,  0.18 * s));  // left wing trailing inner
            planePath.lineTo(sp + QPointF(-1.00 * s,  0.05 * s));  // left wingtip
            planePath.lineTo(sp + QPointF(-0.14 * s, -0.55 * s));  // left nose shoulder
            planePath.closeSubpath();

            p.setBrush(QColor(255, 180, 0));
            p.setPen(QPen(Qt::white, 1.0));
            p.drawPath(planePath);

            p.setPen(Qt::white);
            QString label = QString("%1 (%2, %3)")
                                .arg(i + 1)
                                .arg(m_points[i].x(), 0, 'f', 1)
                                .arg(m_points[i].y(), 0, 'f', 1);
            p.drawText(sp + QPoint(14, -6), label);
        }
    }
}
void FormationCanvasWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        int idx = hitTest(event->pos());
        if (idx >= 0) {
            m_draggingIndex = idx;
        } else {
            // Add new point
            QPointF world = screenToWorld(event->pos());
            m_points.append(world);
            emit positionsChanged(getPositions());
            update();
        }
    } else if (event->button() == Qt::RightButton) {
        int idx = hitTest(event->pos());
        if (idx >= 0) {
            m_points.removeAt(idx);
            emit positionsChanged(getPositions());
            update();
        }
    }
}

void FormationCanvasWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_draggingIndex >= 0 && m_draggingIndex < m_points.size()) {
        m_points[m_draggingIndex] = screenToWorld(event->pos());
        emit positionsChanged(getPositions());
        update();
    }
}

void FormationCanvasWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_draggingIndex = -1;
}

void FormationCanvasWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}
