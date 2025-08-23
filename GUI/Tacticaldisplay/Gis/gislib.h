#ifndef GISLIB_H
#define GISLIB_H

#include "gisnetwork.h"
#include "qjsonarray.h"
#include "qwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPaintEvent>
#include <qgscoordinatereferencesystem.h>
Q_DECLARE_METATYPE(QMouseEvent*)
Q_DECLARE_METATYPE(QKeyEvent*)
class GISlib : public QWidget {
    Q_OBJECT
public:
    GISlib(QWidget *parent = nullptr);
    QString mouseLat, mouseLon;
    void serachPlace(const QString& query);
    void setLayers(const QStringList& layerNames);
    void setCenter(double lat, double lon);
    void setZoom(int level);
    // void addCustomMap(const QString& layerName, int zoomMin, int zoomMax, const QString& tileUrl);
    void addCustomMap(const QString& layerName, int zoomMin, int zoomMax, const QString& tileUrl, qreal opacity = 1.0); // Added opacity paramete
    double lonToX(double lon, int zoom);
    double latToY(double lat, int zoom);
    QPointF geoToCanvas(double lat, double lon);
    QPointF canvasToGeo(QPointF p);
    void wheelEvents(QWheelEvent *event);
    void setInitialOffset(QPointF offset); // add declaration
    void setCoordinateSystem(const QString& crsId);
    void startDistanceMeasurement();
    void endDistanceMeasurement();
    double calculateDistance(QPointF point1, QPointF point2); // point1, point2 are in (lon, lat)
    bool isMeasuringDistance() const { return measuringDistance; }

protected:
    void resizeEvent(QResizeEvent *event) override;

signals:
void mouseCords(double lat, double lon, const QString& crsId);
    void centerChanged(double lat, double lon);
    void zoomChanged(int zoom);
    void keyPressed(QKeyEvent *event);
    void mousePressed(QMouseEvent *event);
    void mouseMoved(QMouseEvent *event);
    void mouseReleased(QMouseEvent *event);
    void painted(QPaintEvent *event);
    void distanceMeasured(double distance, QPointF startPoint, QPointF endPoint);

private:
    QString getSubdomain(int x, int y, const QString& layer);
    QString tileUrl(const QString& layer, int x, int y, int z);
    QString getTileKey(const QString& layer, int z, int x, int y);
    void requestTile(const QString& layer, int x, int y, int z, int retries = 3);

private:
    struct Marker {
        double lat, lon;
        QColor color;
    };
    struct CustomMap {
        int zoomMin;
        int zoomMax;
        QString tileUrl;
               qreal opacity;
    };
    bool flipkeyaxis = false;
    GISNetwork* net;
    QPixmap networkImage;
    bool dragging = false;
    QStringList activeLayers;
    QPoint lastMouse;
    Marker marker;
    QJsonArray geoOutline;
    QList<Marker> markers;
    QHash<QString, QPixmap> tileCache;
    QSet<QString> pendingTileKeys;
    QHash<QString, int> tileRetries;
    // QMap<QString, QImage> tileCache;
    QMap<QString, CustomMap> customMaps;
    double centerLat, centerLon;
    int pendingTiles = 0;
    int zoom;
    // double lonToX(double lon, int zoom);
    // double latToY(double lat, int zoom);
    QPointF initialOffset = QPointF(0, 0); // in pixels
    double xToLon(double x, int zoom);
    double yToLat(double y, int zoom);
    QString toDMS(double deg, bool isLat);
    static constexpr int maxCacheSize = 1000;
    QgsCoordinateReferenceSystem currentCrs;
    bool measuringDistance = false;
    QPointF measureStartPoint; // Geo coordinates (lon, lat)
    QPointF measureEndPoint;   // Geo coordinates (lon, lat)

public slots:
    void receiveImage(QString url, QByteArray data);
    void receivePlace(QString url, QByteArray data);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};

#endif // GISLIB_H

