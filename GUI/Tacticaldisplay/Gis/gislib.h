

#ifndef GISLIB_H
#define GISLIB_H

#include "gisnetwork.h"
#include "qjsonarray.h"
#include "qwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPaintEvent>

class GISlib : public QWidget {
    Q_OBJECT
public:
    GISlib(QWidget *parent = nullptr);
    QString mouseLat, mouseLon;
    void serachPlace(const QString& query);
    void setLayers(const QStringList& layerNames);
    void setCenter(double lat, double lon);
    void setZoom(int level);
    void addCustomMap(const QString& layerName, int zoomMin, int zoomMax, const QString& tileUrl);
    double lonToX(double lon, int zoom);
    double latToY(double lat, int zoom);
signals:
    void mouseCords(QString lat, QString lon);
    void centerChanged(double lat, double lon);
    void zoomChanged(int zoom);
    void keyPressed(QKeyEvent *event);
    void mousePressed(QMouseEvent *event);
    void mouseMoved(QMouseEvent *event);
    void mouseReleased(QMouseEvent *event);
    void painted(QPaintEvent *event);


private:
      QString getSubdomain(int x, int y, const QString& layer);
    QString tileUrl(const QString& layer, int x, int y, int z);
    QString getTileKey(const QString& layer, int z, int x, int y);
    void requestTile(const QString& layer, int x, int y, int z, int retries = 3);

// private:
//     struct Marker {
//         double lat, lon;
//         QColor color;
//     };
//     struct CustomMap {
//         int zoomMin;
//         int zoomMax;
//         QString tileUrl;
//     };
//     bool flipkeyaxis = false;
//     GISNetwork* net;
//     QPixmap networkImage;
//     bool dragging = false;
//     QStringList activeLayers;
//     QPoint lastMouse;
//     Marker marker;
//     QJsonArray geoOutline;
//     QList<Marker> markers;
//     QMap<QString, QImage> tileCache;
//     QMap<QString, CustomMap> customMaps;
//     double centerLat, centerLon;
//     int pendingTiles = 0;
//     int zoom;
//     double lonToX(double lon, int zoom);
//     double latToY(double lat, int zoom);
//     double xToLon(double x, int zoom);
//     double yToLat(double y, int zoom);
//     QString toDMS(double deg, bool isLat);
//     static constexpr int maxCacheSize = 1000;


private:
    struct Marker {
        double lat, lon;
        QColor color;
    };
    struct CustomMap {
        int zoomMin;
        int zoomMax;
        QString tileUrl;
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
    double xToLon(double x, int zoom);
    double yToLat(double y, int zoom);
    QString toDMS(double deg, bool isLat);
    static constexpr int maxCacheSize = 1000;

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

