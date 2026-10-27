#ifndef TYPES_H
#define TYPES_H

#include <QString>
#include <QPointF>
#include <QColor>
#include <QVector>
#include <QRectF>
#include <QSize>

struct ShapeData {
    QString id;
    QString type;
    double longitude;
    double latitude;
    double rotation = 0.0;
    QColor color;
    int lineWidth;

    // Circle
    double radius;

    // Rectangle / bitmap size
    double width;
    double height;

    // Polygon/Line
    QVector<QPointF> points;

    // Text
    QString text;
    int fontSize;

    // Bitmap image path
    QString imagePath;

    // Layer
    QString layerId;

    ShapeData()
        : longitude(0.0)
        , latitude(0.0)
        , rotation(0.0)
        , color(Qt::red)
        , lineWidth(2)
        , radius(0.0)
        , width(0.0)
        , height(0.0)
        , fontSize(14)
    {}
};

struct LayerData {
    QString id;
    QString name;
    QColor color;
    float opacity;
    bool visible;
    bool locked;
    QVector<QString> shapeIds;

    LayerData()
        : color(Qt::white)
        , opacity(1.0f)
        , visible(true)
        , locked(false)
    {}
};

struct MapState {
    double centerLat;
    double centerLon;
    int zoom;
    QSize widgetSize;

    MapState()
        : centerLat(20.5937)
        , centerLon(78.9629)
        , zoom(6)
        , widgetSize(800, 600)
    {}
};

#endif // TYPES_H
