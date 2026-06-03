#ifndef SONOBUOYPANEL_H
#define SONOBUOYPANEL_H

#include "core/Hierarchy/Components/sensorprofile.h"
#include "core/Hierarchy/EntityProfiles/weapons/sonobuoy.h"
#include "qcombobox.h"
#include "qpushbutton.h"
#include "qsvgrenderer.h"
#include "qwidget.h"
#include <QObject>
#include <core/Debug/profiler.h>

class SonoBuoyPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SonoBuoyPanel(QWidget *parent = nullptr);
    // Set hierarchy instance
    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    // Get size hint
    QSize sizeHint() const override;
    // Get minimum size
    int range = 25;
    QSize minimumSize() const;
    // Set radar range
    void setRange(float value) { range = value; }
    // Get height for width
    int heightForWidth(int width) const override;
    // Select entity
    void selectEntity(Entity* entity);
    // Remove entity by ID
    void RemoveEntity(QString ID);
    // Update radar display
    void updateRadar();
    // Sensor instance
    Sonobuoy* sensor = nullptr;
    QVector<Sonobuoy*> sensorlist;
    // Entity platform
    Platform* entity = nullptr;
private slots:
    void onZoomIn();
    void onZoomOut();
    void onSensorSelected(int index);

private:
    QPushButton *zoomInButton;
    QPushButton *zoomOutButton;
    double zoomLevel = 100; // Zoom factor store karne ke liye

protected:
    // Handle paint events
    void paintEvent(QPaintEvent *event) override;
    // Handle mouse move for hover detection
    void mouseMoveEvent(QMouseEvent *event) override;
    // Handle mouse leave
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // %%% Display Properties %%%
    // Aspect ratio for display
    const double ASPECT_RATIO = 16.0/9.0;
    // Padding for display
    int padding = 18;
    // Number of radar rings
    int ringCount = 3;
    // Major tick interval
    int majorTickEvery = 30;
    // Minor ticks per major
    int minorTicksPerMajor = 5;
    // Radar green color
    QColor radarGreen = QColor(0, 255, 0);
    // Current angle
    int ang = 0;
    // List of targets
    QVector<Target> targets;
    // Entity ID
    QString id = "";
    // Hierarchy instance
    Hierarchy* hierarchy = nullptr;

    // Hover tracking
    int hoveredTargetIndex = -1;
    QPoint lastMousePos;
    QComboBox* sensorDropdown = nullptr;
    void updateDropdown();
    // %%% Drawing Methods %%%
    // Draw background
    void drawBackground(QPainter &p);
    // Draw radar ring
    void drawRadarRing(QPainter &p, const QPoint &center, int outerRadius);
    // Draw ticks and labels
    void drawTicksAndLabels(QPainter &p, const QPoint &center, int outerRadius);
    // Draw concentric circles
    void drawConcentricCircles(QPainter &p, const QPoint &center, int outerRadius);
    // Draw center mark
    void drawCenterMark(QPainter &p, const QPoint &center);
    // Draw top marker
    void drawTopMarker(QPainter &p, const QPoint &center, int outerRadius);
    // Draw target and path
    void drawTargetAndPath(QPainter &painter);
private:
    QSvgRenderer m_svgRenderer;
};

#endif // SONOBUOYPANEL_H
