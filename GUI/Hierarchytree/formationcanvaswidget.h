#ifndef FORMATIONCANVASWIDGET_H
#define FORMATIONCANVASWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QJsonArray>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class FormationCanvasWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FormationCanvasWidget(QWidget *parent = nullptr);

    // Load existing positions (from FormationPositions)
    void setPositions(const QJsonArray &positions);
    QJsonArray getPositions() const;

    // Clear all points
    void clearPoints();

signals:
    void positionsChanged(const QJsonArray &positions);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onClearClicked();
    void onUndoClicked();

private:
    QVector<QPointF> m_points;          // relative offsets (meters or units)
    int m_draggingIndex = -1;
    QPointF m_origin;                   // center of canvas (mothership)
    qreal m_scale = 8.0;                // pixels per unit
    qreal m_gridSize = 10.0;            // units

    QPointF screenToWorld(const QPoint &screen) const;
    QPoint  worldToScreen(const QPointF &world) const;
    int     hitTest(const QPoint &screen, qreal radius = 10.0) const;

    // UI
    QLabel *m_infoLabel = nullptr;
    QPushButton *m_btnClear = nullptr;
    QPushButton *m_btnUndo = nullptr;
};

#endif // FORMATIONCANVASWIDGET_H
