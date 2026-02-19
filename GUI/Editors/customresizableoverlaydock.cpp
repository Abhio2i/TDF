// CustomResizableOverlayDock.cpp
#include "customresizableoverlaydock.h"
#include <QCursor>
#include <QPainter>
// #include <QPainter>
#include <QPen>
CustomResizableOverlayDock::CustomResizableOverlayDock(const QString &title, QWidget *parent)
    : QDockWidget(title, parent)
{
    // Sirf close button ke liye window flags
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);

    // Dock widget features - sirf close button allowed karo
    setFeatures(QDockWidget::DockWidgetClosable);

    setAttribute(Qt::WA_TranslucentBackground, false);

    // Background color set karo
    setStyleSheet("QDockWidget { background-color: #252525; }"
                  "QDockWidget::title { background-color: #333; color: white; }");
}

void CustomResizableOverlayDock::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        resizeEdge = getResizeEdge(event->pos());
        if (resizeEdge != Qt::Edges()) {
            resizing = true;
            dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        } else {
            // Move the widget
            dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    QDockWidget::mousePressEvent(event);
}

void CustomResizableOverlayDock::mouseMoveEvent(QMouseEvent *event)
{
    if (!event->buttons().testFlag(Qt::LeftButton)) {
        updateCursor(event->pos());
        return;
    }

    if (resizing) {
        QRect geo = geometry();
        QPoint delta = event->globalPos() - (geo.topLeft() + dragPosition);

        if (resizeEdge.testFlag(Qt::RightEdge)) {
            geo.setWidth(qMax(minimumWidth(), geo.width() + delta.x()));
        }
        if (resizeEdge.testFlag(Qt::BottomEdge)) {
            geo.setHeight(qMax(minimumHeight(), geo.height() + delta.y()));
        }
        if (resizeEdge.testFlag(Qt::LeftEdge)) {
            geo.setLeft(geo.left() + delta.x());
            geo.setWidth(geo.width() - delta.x());
        }
        if (resizeEdge.testFlag(Qt::TopEdge)) {
            geo.setTop(geo.top() + delta.y());
            geo.setHeight(geo.height() - delta.y());
        }

        setGeometry(geo);
        dragPosition = event->globalPos() - geo.topLeft(); // update for smooth resize
        event->accept();
    } else {
        // Normal move
        move(event->globalPos() - dragPosition);
        event->accept();
    }
}

void CustomResizableOverlayDock::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        resizing = false;
        resizeEdge = Qt::Edges();
        updateCursor(event->pos());
    }
    QDockWidget::mouseReleaseEvent(event);
}

Qt::Edges CustomResizableOverlayDock::getResizeEdge(const QPoint &pos) const
{
    Qt::Edges edges;
    QRect r = rect();

    if (pos.x() <= resizeMargin)                    edges |= Qt::LeftEdge;
    if (pos.x() >= r.width() - resizeMargin - 1)    edges |= Qt::RightEdge;
    if (pos.y() <= resizeMargin)                    edges |= Qt::TopEdge;
    if (pos.y() >= r.height() - resizeMargin - 1)   edges |= Qt::BottomEdge;

    return edges;
}

void CustomResizableOverlayDock::updateCursor(const QPoint &pos)
{
    Qt::Edges edge = getResizeEdge(pos);
    if (edge == (Qt::RightEdge | Qt::BottomEdge))
        setCursor(Qt::SizeFDiagCursor);
    else if (edge == (Qt::LeftEdge | Qt::BottomEdge))
        setCursor(Qt::SizeBDiagCursor);
    else if (edge.testFlag(Qt::RightEdge) || edge.testFlag(Qt::LeftEdge))
        setCursor(Qt::SizeHorCursor);
    else if (edge.testFlag(Qt::BottomEdge) || edge.testFlag(Qt::TopEdge))
        setCursor(Qt::SizeVerCursor);
    else
        setCursor(Qt::ArrowCursor);
}
void CustomResizableOverlayDock::moveEvent(QMoveEvent *event)
{
    QPoint oldPos = event->oldPos();
    QPoint newPos = event->pos();
    emit moved(oldPos, newPos);
    QDockWidget::moveEvent(event);
}

void CustomResizableOverlayDock::resizeEvent(QResizeEvent *event)
{
    QSize oldSize = event->oldSize();
    QSize newSize = event->size();
    emit resized(oldSize, newSize);
    QDockWidget::resizeEvent(event);
}
// void CustomResizableOverlayDock::paintEvent(QPaintEvent *event)
// {
//     QDockWidget::paintEvent(event); // Default drawing
//     QPainter painter(this);

//     // Sidebar/Dock ki boundary dikhane ke liye ek patli border
//     painter.setPen(QPen(QColor(60, 60, 60), 1)); // Dark grey border
//     painter.drawRect(0, 0, width() - 1, height() - 1);

//     // --- Resize Indicator Draw Karna ---
//     // Right side par ek "Vertical Bar" dikhane ke liye
//     painter.setPen(QPen(QColor(100, 100, 100, 150), 3)); // Light grey line
//     painter.drawLine(width() - 3, 10, width() - 3, height() - 10);

//     // Bottom-right corner mein 3 chote dots (Grabber design)
//     painter.setPen(QPen(QColor(150, 150, 150), 2));
//     painter.drawPoint(width() - 5, height() - 5);
//     painter.drawPoint(width() - 9, height() - 5);
//     painter.drawPoint(width() - 5, height() - 9);
// }
void CustomResizableOverlayDock::paintEvent(QPaintEvent *event)
{
    QDockWidget::paintEvent(event);
    QPainter painter(this);

    // Sky Blue Color set karein
    QColor skyBlue(0, 191, 255);
    painter.setPen(QPen(skyBlue, 3)); // 3px thickness

    if (handlePos == Left) {
        // Left side par line draw karein (Inspector/Library ke liye)
        painter.drawLine(2, 10, 2, height() - 10);

        // Bottom-left mein 3 dots
        painter.drawPoint(5, height() - 5);
        painter.drawPoint(9, height() - 5);
        painter.drawPoint(5, height() - 9);
    } else {
        // Right side par line draw karein (Hierarchy ke liye)
        painter.drawLine(width() - 3, 10, width() - 3, height() - 10);

        // Bottom-right mein 3 dots
        painter.drawPoint(width() - 5, height() - 5);
        painter.drawPoint(width() - 9, height() - 5);
        painter.drawPoint(width() - 5, height() - 9);
    }
}
