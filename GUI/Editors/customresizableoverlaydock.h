/* =============================================================================
 * FILE:         customresizableoverlaydock.h
 * MODULE:       Custom Resizable Overlay Dock
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the CustomResizableOverlayDock class which extends
 *               QDockWidget with resize handles (left/right), lock functionality
 *               to fix position/size, and overlay capability. Supports moving,
 *               resizing via mouse drag, lock toggling, and emits signals on
 *               move, resize, and lock state changes.
 *
 * REQUIREMENTS: REQ-DOCK-010  Custom dock widget with resize handles
 *               REQ-DOCK-011  Resize margin detection and cursor update
 *               REQ-DOCK-012  Lock/unlock position and size
 *               REQ-DOCK-013  Lock button in title bar
 *               REQ-DOCK-014  Signals for moved, resized, lockToggled
 *               REQ-DOCK-015  Overlay capability (stays on top)
 *               REQ-DOCK-016  Event filter for title bar interactions
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-DOCK-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#ifndef CUSTOMRESIZABLEOVERLAYDOCK_H
#define CUSTOMRESIZABLEOVERLAYDOCK_H

#include <QDockWidget>
#include <QWidget>
#include <QMouseEvent>
#include <QToolButton>
#include <QHBoxLayout>
#include <QLabel>

class CustomResizableOverlayDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit CustomResizableOverlayDock(const QString &title, QWidget *parent = nullptr);

    enum HandlePosition { Left, Right };
    HandlePosition handlePos = Right;
    bool isLocked() const { return m_locked; }
    void setLocked(bool locked);
    void enableLockButton();
    void setWindowTitle(const QString &title);
    void setWidget(QWidget *widget);
    void setTitleStripe(bool enabled, const QColor &color = QColor("#00BFFF"));


protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void moved(QPoint oldPos, QPoint newPos);
    void resized(QSize oldSize, QSize newSize);
    void lockToggled(bool locked);

private slots:
    void onLockButtonClicked();

private:
    QPoint dragPosition;
    bool resizing = false;
    Qt::Edges resizeEdge = Qt::Edges();
    static constexpr int resizeMargin = 12;
    Qt::Edges getResizeEdge(const QPoint &pos) const;
    void updateCursor(const QPoint &pos);
    bool m_locked = false;
    QToolButton *m_lockButton = nullptr;
    void setupTitleBar(const QString &title);
    bool m_stripeEnabled = false;
    QColor m_stripeColor = QColor("#00BFFF");
};

#endif // CUSTOMRESIZABLEOVERLAYDOCK_H
