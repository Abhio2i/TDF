
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
    void setLayer(const QString& layerName);
    void setCenter(double lat, double lon);
    void setZoom(int level);

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
    QString tileUrl(int x, int y, int z);
    QString getTileKey(int z, int x, int y);
    void requestTile(int x, int y, int z, int retries = 3);

private:
    struct Marker {
        double lat, lon;
        QColor color;
    };
    bool flipkeyaxis = false;
    GISNetwork* net;
    QPixmap networkImage;
    bool dragging = false;
    QString currentLayer;
    QPoint lastMouse;
    Marker marker;
    QJsonArray geoOutline;
    QList<Marker> markers;
    QMap<QString, QImage> tileCache;
    double centerLat, centerLon;
    int pendingTiles = 0;
    int zoom;
    double lonToX(double lon, int zoom);
    double latToY(double lat, int zoom);
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
