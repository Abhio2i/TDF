
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

    // Override so content widget also gets mouseTracking + eventFilter
    void setWidget(QWidget *widget);

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
};

#endif // CUSTOMRESIZABLEOVERLAYDOCK_H
