/* =============================================================================
 * FILE:         customresizableoverlaydock.cpp
 * MODULE:       Custom Resizable Overlay Dock
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the CustomResizableOverlayDock class which extends
 *               QDockWidget with resize handles (left/right), lock functionality
 *               to fix position/size, and overlay capability. Supports moving,
 *               resizing via mouse drag, lock toggling, and emits signals on
 *               move, resize, and lock state changes. The implementation includes
 *               a custom title bar with a lock button and close button, edge
 *               detection for resizing, cursor updates, and event filtering.
 *
 * REQUIREMENTS: Implements REQ-DOCK-010 through REQ-DOCK-016
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-DOCK-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#include "customresizableoverlaydock.h"
#include <QCursor>
#include <QPainter>
#include <QPen>
#include <QApplication>
#include <QToolButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

CustomResizableOverlayDock::CustomResizableOverlayDock(const QString &title, QWidget *parent)
    : QDockWidget(title, parent)
{
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
    setFeatures(QDockWidget::DockWidgetClosable);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setStyleSheet("QDockWidget { background-color: #252525; }"
                  "QDockWidget::title { background-color: #333; color: white; }");
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    installEventFilter(this);
    setupTitleBar(title);
}

// ─── setWidget override ───────────────────────────────────────────────────────

void CustomResizableOverlayDock::setWidget(QWidget *widget)
{
    QDockWidget::setWidget(widget);
    if (!widget) return;
    widget->setMouseTracking(true);
    widget->setAttribute(Qt::WA_Hover, true);
    widget->installEventFilter(this);
    for (QWidget *child : widget->findChildren<QWidget*>()) {
        child->setMouseTracking(true);
        child->setAttribute(Qt::WA_Hover, true);
        child->installEventFilter(this);
    }
}

// ─── Mouse Press ─────────────────────────────────────────────────────────────
void CustomResizableOverlayDock::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        resizeEdge = getResizeEdge(event->pos());
        if (resizeEdge != Qt::Edges()) {
            resizing = true;
            dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
            return;
        } else {
            dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    QDockWidget::mousePressEvent(event);
}

// ─── Mouse Move ──────────────────────────────────────────────────────────────
void CustomResizableOverlayDock::mouseMoveEvent(QMouseEvent *event)
{
    if (resizing) {
        QRect rect = geometry();
        QPoint globalPos  = event->globalPos();
        QPoint posInParent = parentWidget()
                                 ? parentWidget()->mapFromGlobal(globalPos)
                                 : globalPos;
        if (resizeEdge.testFlag(Qt::TopEdge)) {
            int oldBottom = rect.bottom();
            int newTop    = posInParent.y();
            if ((oldBottom - newTop) > minimumHeight())
                rect.setTop(newTop);
        } else if (resizeEdge.testFlag(Qt::BottomEdge)) {
            rect.setBottom(posInParent.y());
        }

        if (resizeEdge.testFlag(Qt::LeftEdge)) {
            int oldRight = rect.right();
            int newLeft  = posInParent.x();
            if ((oldRight - newLeft) > minimumWidth())
                rect.setLeft(newLeft);
        } else if (resizeEdge.testFlag(Qt::RightEdge)) {
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
        QPoint localPos = mapFromGlobal(event->globalPos());
        updateCursor(localPos);
        event->accept();
    }
}
// ─── Mouse Release ───────────────────────────────────────────────────────────
void CustomResizableOverlayDock::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        resizing   = false;
        resizeEdge = Qt::Edges();

        QPoint localPos = mapFromGlobal(QCursor::pos());
        updateCursor(localPos);
    }
    QDockWidget::mouseReleaseEvent(event);
}

// ─── Edge Detection ──────────────────────────────────────────────────────────
Qt::Edges CustomResizableOverlayDock::getResizeEdge(const QPoint &pos) const
{
    Qt::Edges edges;
    QRect r = rect();

    if (pos.x() <= resizeMargin)                  edges |= Qt::LeftEdge;
    if (pos.x() >= r.width()  - resizeMargin - 1) edges |= Qt::RightEdge;
    if (pos.y() <= resizeMargin)                  edges |= Qt::TopEdge;
    if (pos.y() >= r.height() - resizeMargin - 1) edges |= Qt::BottomEdge;
    return edges;
}

// ─── Cursor Update ───────────────────────────────────────────────────────────
void CustomResizableOverlayDock::updateCursor(const QPoint &pos)
{
    Qt::Edges edge = getResizeEdge(pos);
    Qt::CursorShape shape = Qt::ArrowCursor;
    if (edge.testFlag(Qt::LeftEdge) || edge.testFlag(Qt::RightEdge))
        shape = Qt::SizeHorCursor;
    else if (edge.testFlag(Qt::TopEdge) || edge.testFlag(Qt::BottomEdge))
        shape = Qt::SizeVerCursor;
    this->setCursor(shape);
    if (QWidget *tb = titleBarWidget()) {
        tb->setCursor(shape);
        for (QWidget *child : tb->findChildren<QWidget*>())
            child->setCursor(shape);
    }
    if (QWidget *w = widget()) {
        w->setCursor(shape);
        for (QWidget *child : w->findChildren<QWidget*>())
            child->setCursor(shape);
    }
}

// ─── Event Filter ────────────────────────────────────────────────────────────
bool CustomResizableOverlayDock::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseMove || event->type() == QEvent::HoverMove) {
        QPoint localPos = this->mapFromGlobal(QCursor::pos());
        if (this->rect().contains(localPos)) {
            updateCursor(localPos);
            if (resizing) return true;
        } else {
            if (!resizing) this->setCursor(Qt::ArrowCursor);
        }
    }
    return QObject::eventFilter(watched, event);
}

// ─── Move / Resize Events ────────────────────────────────────────────────────
void CustomResizableOverlayDock::moveEvent(QMoveEvent *event)
{
    emit moved(event->oldPos(), event->pos());
    QDockWidget::moveEvent(event);

}

void CustomResizableOverlayDock::resizeEvent(QResizeEvent *event)
{
    emit resized(event->oldSize(), event->size());
    QDockWidget::resizeEvent(event);
}

// ─── Paint ───────────────────────────────────────────────────────────────────
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

// ─── Title Bar Setup ─────────────────────────────────────────────────────────
void CustomResizableOverlayDock::setupTitleBar(const QString &title)
{
    QWidget *titleBar = new QWidget(this);
    titleBar->setObjectName("customTitleBar");
    titleBar->setStyleSheet("background-color: #1A3A4F;");
    QHBoxLayout *layout = new QHBoxLayout(titleBar);
    layout->setContentsMargins(8, 2, 4, 2);
    layout->setSpacing(4);
    QLabel *titleLabel = new QLabel(title, titleBar);
    titleLabel->setObjectName("dockTitleLabel");
    titleLabel->setStyleSheet(
        "color: white; font-weight: bold; font-size: 12px; background: transparent;");
    layout->addWidget(titleLabel);
    layout->addStretch();
    m_lockButton = new QToolButton(titleBar);
    m_lockButton->setText("🔓");
    m_lockButton->setToolTip("Lock sensor view to current entity");
    m_lockButton->setCheckable(true);
    m_lockButton->setChecked(false);
    m_lockButton->setFixedSize(22, 22);
    m_lockButton->setVisible(false);
    m_lockButton->setStyleSheet(
        "QToolButton { background: transparent; border: none; color: #aaaaaa;"
        "              font-size: 13px; padding: 0px; }"
        "QToolButton:hover { background: rgba(255,255,255,0.1);"
        "                    border-radius: 3px; color: #00BFFF; }"
        "QToolButton:checked { color: #00BFFF;"
        "                      background: rgba(0,191,255,0.15);"
        "                      border-radius: 3px; }");
    connect(m_lockButton, &QToolButton::clicked,
            this, &CustomResizableOverlayDock::onLockButtonClicked);
    layout->addWidget(m_lockButton);
    QToolButton *closeButton = new QToolButton(titleBar);
    closeButton->setText("✕");
    closeButton->setFixedSize(22, 22);
    closeButton->setStyleSheet(
        "QToolButton { background: transparent; border: none; color: #aaaaaa;"
        "              font-size: 13px; padding: 0px; }"
        "QToolButton:hover { background: #c0392b; border-radius: 3px; color: white; }");
    connect(closeButton, &QToolButton::clicked, this, &QDockWidget::close);
    layout->addWidget(closeButton);
    titleBar->setLayout(layout);
    setTitleBarWidget(titleBar);

    titleBar->setMouseTracking(true);
    titleBar->setAttribute(Qt::WA_Hover, true);
    titleBar->installEventFilter(this);

    for (QWidget *child : titleBar->findChildren<QWidget*>()) {
        child->setMouseTracking(true);
        child->setAttribute(Qt::WA_Hover, true);
        child->installEventFilter(this);
    }
}

// ─── setWindowTitle ──────────────────────────────────────────────────────────
void CustomResizableOverlayDock::setWindowTitle(const QString &title)
{
    QDockWidget::setWindowTitle(title);
    if (QWidget *tb = titleBarWidget()) {
        if (QLabel *label = tb->findChild<QLabel*>("dockTitleLabel"))
            label->setText(title);
    }
}

// ─── Lock Button ─────────────────────────────────────────────────────────────
void CustomResizableOverlayDock::enableLockButton()
{
    if (m_lockButton) m_lockButton->setVisible(true);
}

void CustomResizableOverlayDock::onLockButtonClicked()
{
    m_locked = m_lockButton->isChecked();
    QWidget *tb = titleBarWidget();
    if (m_locked) {
        m_lockButton->setText("🔒");
        m_lockButton->setToolTip("Sensor view locked — click to unlock");
        if (tb) tb->setStyleSheet("background-color: #5a1a1a;");
    } else {
        m_lockButton->setText("🔓");
        m_lockButton->setToolTip("Lock sensor view to current entity");
        if (tb) tb->setStyleSheet("background-color: #1A3A4F;");
    }
    emit lockToggled(m_locked);
}

void CustomResizableOverlayDock::setLocked(bool locked)
{
    if (m_locked == locked) return;
    m_locked = locked;
    if (m_lockButton) {
        m_lockButton->setChecked(locked);
        m_lockButton->setText(locked ? "🔒" : "🔓");
    }
    emit lockToggled(m_locked);
}

void CustomResizableOverlayDock::setTitleStripe(bool enabled, const QColor &color)
{
    m_stripeEnabled = enabled;
    m_stripeColor   = color;

    QWidget *tb = titleBarWidget();
    if (!tb) return;

    // Pehle se existing stripe hato
    if (QWidget *old = tb->findChild<QWidget*>("titleStripe"))
        delete old;

    if (!enabled) return;

    QHBoxLayout *layout = qobject_cast<QHBoxLayout*>(tb->layout());
    if (!layout) return;

    QWidget *stripe = new QWidget(tb);
    stripe->setObjectName("titleStripe");
    stripe->setFixedWidth(4);
    stripe->setStyleSheet(QString("background-color: %1;").arg(color.name()));
    stripe->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    // Layout ke bilkul start mein insert karo
    layout->insertWidget(0, stripe);
    layout->setContentsMargins(0, 0, 4, 0);
}
