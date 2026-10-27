//============================================================================
// FILE:    esmppiwidget.h
// MODULE:  ESM (Electronic Support Measures) Display
// PROJECT: Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
//
// DESCRIPTION: Circular 360-degree PPI scope. Pure view: it reads the shared
//   ESMObservationModel and never touches the ESM sensor or the simulation.
//
//   Bearing convention (same as ESM::scan): degrees clockwise from north,
//   000 at the top of the scope.
//   Range is the value exposed by ESMTarget::radius (km); the outer ring is
//   the configured ESM range.
//============================================================================
#ifndef ESMPPIWIDGET_H
#define ESMPPIWIDGET_H

#include <QPoint>
#include <QPointF>
#include <QString>
#include <QWidget>

class ESMObservationModel;
struct ESMObservation;

class ESMPpiWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ESMPpiWidget(ESMObservationModel* model, QWidget* parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /* Number of dashed rings between the centre and the outer (range) ring. */
    void setRingCount(int count);
    int ringCount() const { return ringCount_; }

    /* Highlight an observation (empty key = none). Does not emit. */
    void setSelectedKey(const QString& key);

signals:
    /* User clicked a plot (key) or empty space (empty key). */
    void observationSelected(const QString& key);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private slots:
    void onModelChanged();

private:
    bool scopeGeometry(QPointF& centre, double& radius) const;
    bool plotPosition(const ESMObservation& o, const QPointF& centre, double radius,
                      QPointF& out) const;
    int hitTest(const QPoint& pos) const;

    void drawScope(class QPainter& p, const QPointF& c, double R) const;
    void drawObservations(class QPainter& p, const QPointF& c, double R) const;

    ESMObservationModel* model_ = nullptr;
    int ringCount_ = 3;
    int hovered_ = -1;
    QString selectedKey_;
};

#endif // ESMPPIWIDGET_H