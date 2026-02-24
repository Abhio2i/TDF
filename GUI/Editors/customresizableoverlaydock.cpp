#include "customresizableoverlaydock.h"
#include <QCursor>
#include <QPainter>
#include <QPen>
#include <QApplication>
CustomResizableOverlayDock::CustomResizableOverlayDock(const QString &title, QWidget *parent)
    : QDockWidget(title, parent)
{
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
    setFeatures(QDockWidget::DockWidgetClosable);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setStyleSheet("QDockWidget { background-color: #252525; }"
                  "QDockWidget::title { background-color: #333; color: white; }");

    setMouseTracking(true);
    qApp->installEventFilter(this);
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
            dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    QDockWidget::mousePressEvent(event);
}
void CustomResizableOverlayDock::mouseMoveEvent(QMouseEvent *event)
{
    if (resizing) {
        QRect rect = geometry();
        QPoint globalPos = event->globalPos();
        QPoint posInParent = parentWidget() ? parentWidget()->mapFromGlobal(globalPos) : globalPos;
        if (resizeEdge.testFlag(Qt::TopEdge)) {
            int oldBottom = rect.bottom();
            int newTop = posInParent.y();
            int newHeight = oldBottom - newTop;

            if (newHeight > minimumHeight()) {
                rect.setTop(newTop);
            }
        }

        else if (resizeEdge.testFlag(Qt::BottomEdge)) {
            rect.setBottom(posInParent.y());
        }


        if (resizeEdge.testFlag(Qt::LeftEdge)) {
            int oldRight = rect.right();
            int newLeft = posInParent.x();
            if ((oldRight - newLeft) > minimumWidth()) {
                rect.setLeft(newLeft);
            }
        }

        else if (resizeEdge.testFlag(Qt::RightEdge)) {
            rect.setRight(posInParent.x());
        }

        setGeometry(rect);
        event->accept();
    }
    else if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - dragPosition);
        event->accept();
    }
    else {
        updateCursor(event->pos());
        QDockWidget::mouseMoveEvent(event);
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
    if (edge.testFlag(Qt::LeftEdge) || edge.testFlag(Qt::RightEdge))
        setCursor(Qt::SizeHorCursor);
    else if (edge.testFlag(Qt::BottomEdge) || edge.testFlag(Qt::TopEdge))
        setCursor(Qt::SizeVerCursor);
    else {
        setCursor(Qt::ArrowCursor);
    }
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

void CustomResizableOverlayDock::paintEvent(QPaintEvent *event)
{
    QDockWidget::paintEvent(event);
    QPainter painter(this);
    if (handlePos == Left) {
        painter.drawLine(2, 10, 2, height() - 10);
        painter.drawPoint(5, height() - 5);
        painter.drawPoint(9, height() - 5);
        painter.drawPoint(5, height() - 9);
    } else {
        painter.drawLine(width() - 3, 10, width() - 3, height() - 10);
        painter.drawPoint(width() - 5, height() - 5);
        painter.drawPoint(width() - 9, height() - 5);
        painter.drawPoint(width() - 5, height() - 9);
    }
}
bool CustomResizableOverlayDock::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint localPos = mapFromGlobal(mouseEvent->globalPos());
        if (rect().contains(localPos)) {
            updateCursor(localPos);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }
    return QDockWidget::eventFilter(watched, event);
}

