// CustomResizableOverlayDock.h
#ifndef CUSTOMRESIZABLEOVERLAYDOCK_H
#define CUSTOMRESIZABLEOVERLAYDOCK_H

#include <QDockWidget>
#include <QWidget>
#include <QMouseEvent>

class CustomResizableOverlayDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit CustomResizableOverlayDock(const QString &title, QWidget *parent = nullptr);
    enum HandlePosition { Left, Right };
    HandlePosition handlePos = Right;

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
private:
    QPoint dragPosition;
    bool resizing = false;
    Qt::Edges resizeEdge = Qt::Edges();
    static constexpr int resizeMargin = 8;

    Qt::Edges getResizeEdge(const QPoint &pos) const;
    void updateCursor(const QPoint &pos);
};

#endif
