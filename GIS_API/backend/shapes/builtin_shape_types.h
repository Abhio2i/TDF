#ifndef BUILTIN_SHAPE_TYPES_H
#define BUILTIN_SHAPE_TYPES_H
#include "backend/shapes/ishape_type.h"
void registerBuiltInShapeTypes();


class CircleShapeType : public IShapeType {
public:
    QString typeId() const override { return QStringLiteral("circle"); }
    QString displayName() const override { return QStringLiteral("Circle"); }
    bool isBuiltIn() const override { return true; }

    bool onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    bool onMouseMove(MapWidget* map, QMouseEvent* e, const QVector<QPointF>& points) override;
    bool onMouseRelease(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    QJsonObject finishDrawing(const QVector<QPointF>& points) override;

    void draw(QPainter& p, const QJsonObject& props, const MapWidget* map) const override;
    void drawPreview(QPainter& p, const QVector<QPointF>& points, const MapWidget* map) const override;
    void drawEditHandles(QPainter& p, const QJsonObject& props, const MapWidget* map) const override;

    bool hitTest(const QJsonObject& props, const QPointF& geoPos, double tolerance) const override;
    int  findNearestEditPoint(const QJsonObject& props, const QPointF& canvasPos, const MapWidget* map) const override;
    void updateEditPoint(QJsonObject& props, int index, const QPointF& newGeoPos) override;
};

class RectangleShapeType : public IShapeType {
public:
    QString typeId() const override { return QStringLiteral("rectangle"); }
    QString displayName() const override { return QStringLiteral("Rectangle"); }
    bool isBuiltIn() const override { return true; }

    bool onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    bool onMouseMove(MapWidget* map, QMouseEvent* e, const QVector<QPointF>& points) override;
    bool onMouseRelease(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    QJsonObject finishDrawing(const QVector<QPointF>& points) override;

    void draw(QPainter& p, const QJsonObject& props, const MapWidget* map) const override;
    void drawPreview(QPainter& p, const QVector<QPointF>& points, const MapWidget* map) const override;
    void drawEditHandles(QPainter& p, const QJsonObject& props, const MapWidget* map) const override;

    bool hitTest(const QJsonObject& props, const QPointF& geoPos, double tolerance) const override;
    int  findNearestEditPoint(const QJsonObject& props, const QPointF& canvasPos, const MapWidget* map) const override;
    void updateEditPoint(QJsonObject& props, int index, const QPointF& newGeoPos) override;
};

class PolygonShapeType : public IShapeType {
public:
    QString typeId() const override { return QStringLiteral("polygon"); }
    QString displayName() const override { return QStringLiteral("Polygon"); }
    bool isBuiltIn() const override { return true; }

    bool onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    bool onMouseMove(MapWidget* map, QMouseEvent* e, const QVector<QPointF>& points) override;
    bool onMouseRelease(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    QJsonObject finishDrawing(const QVector<QPointF>& points) override;

    void draw(QPainter& p, const QJsonObject& props, const MapWidget* map) const override;
    void drawPreview(QPainter& p, const QVector<QPointF>& points, const MapWidget* map) const override;
    void drawEditHandles(QPainter& p, const QJsonObject& props, const MapWidget* map) const override;

    bool hitTest(const QJsonObject& props, const QPointF& geoPos, double tolerance) const override;
    int  findNearestEditPoint(const QJsonObject& props, const QPointF& canvasPos, const MapWidget* map) const override;
    void updateEditPoint(QJsonObject& props, int index, const QPointF& newGeoPos) override;
};

class LineShapeType : public IShapeType {
public:
    QString typeId() const override { return QStringLiteral("line"); }
    QString displayName() const override { return QStringLiteral("Line"); }
    bool isBuiltIn() const override { return true; }

    bool onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    bool onMouseMove(MapWidget* map, QMouseEvent* e, const QVector<QPointF>& points) override;
    bool onMouseRelease(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    QJsonObject finishDrawing(const QVector<QPointF>& points) override;

    void draw(QPainter& p, const QJsonObject& props, const MapWidget* map) const override;
    void drawPreview(QPainter& p, const QVector<QPointF>& points, const MapWidget* map) const override;
    void drawEditHandles(QPainter& p, const QJsonObject& props, const MapWidget* map) const override;

    bool hitTest(const QJsonObject& props, const QPointF& geoPos, double tolerance) const override;
    int  findNearestEditPoint(const QJsonObject& props, const QPointF& canvasPos, const MapWidget* map) const override;
    void updateEditPoint(QJsonObject& props, int index, const QPointF& newGeoPos) override;
};

class PointShapeType : public IShapeType {
public:
    QString typeId() const override { return QStringLiteral("point"); }
    QString displayName() const override { return QStringLiteral("Point"); }
    bool isBuiltIn() const override { return true; }

    bool onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    bool onMouseMove(MapWidget* map, QMouseEvent* e, const QVector<QPointF>& points) override;
    bool onMouseRelease(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    QJsonObject finishDrawing(const QVector<QPointF>& points) override;

    void draw(QPainter& p, const QJsonObject& props, const MapWidget* map) const override;
    void drawPreview(QPainter& p, const QVector<QPointF>& points, const MapWidget* map) const override;

    bool hitTest(const QJsonObject& props, const QPointF& geoPos, double tolerance) const override;
};

class TextShapeType : public IShapeType {
public:
    QString typeId() const override { return QStringLiteral("text"); }
    QString displayName() const override { return QStringLiteral("Text"); }
    bool isBuiltIn() const override { return true; }

    bool onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    bool onMouseMove(MapWidget* map, QMouseEvent* e, const QVector<QPointF>& points) override;
    bool onMouseRelease(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    QJsonObject finishDrawing(const QVector<QPointF>& points) override;

    void draw(QPainter& p, const QJsonObject& props, const MapWidget* map) const override;
    void drawPreview(QPainter& p, const QVector<QPointF>& points, const MapWidget* map) const override;

    bool hitTest(const QJsonObject& props, const QPointF& geoPos, double tolerance) const override;
};

class BitmapShapeType : public IShapeType {
public:
    QString typeId() const override { return QStringLiteral("bitmap"); }
    QString displayName() const override { return QStringLiteral("Bitmap"); }
    bool isBuiltIn() const override { return true; }

    bool onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    bool onMouseMove(MapWidget* map, QMouseEvent* e, const QVector<QPointF>& points) override;
    bool onMouseRelease(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    QJsonObject finishDrawing(const QVector<QPointF>& points) override;

    void draw(QPainter& p, const QJsonObject& props, const MapWidget* map) const override;
    void drawPreview(QPainter& p, const QVector<QPointF>& points, const MapWidget* map) const override;
    void drawEditHandles(QPainter& p, const QJsonObject& props, const MapWidget* map) const override;

    bool hitTest(const QJsonObject& props, const QPointF& geoPos, double tolerance) const override;
    int  findNearestEditPoint(const QJsonObject& props, const QPointF& canvasPos, const MapWidget* map) const override;
    void updateEditPoint(QJsonObject& props, int index, const QPointF& newGeoPos) override;
};


class RasterImageShapeType : public IShapeType {
public:
    QString typeId() const override { return QStringLiteral("raster_image"); }
    QString displayName() const override { return QStringLiteral("Raster Image"); }
    bool isBuiltIn() const override { return true; }

    bool onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    bool onMouseMove(MapWidget* map, QMouseEvent* e, const QVector<QPointF>& points) override;
    bool onMouseRelease(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) override;
    QJsonObject finishDrawing(const QVector<QPointF>& points) override;

    void draw(QPainter& p, const QJsonObject& props, const MapWidget* map) const override;
    void drawPreview(QPainter& p, const QVector<QPointF>& points, const MapWidget* map) const override;
    void drawEditHandles(QPainter& p, const QJsonObject& props, const MapWidget* map) const override;

    bool hitTest(const QJsonObject& props, const QPointF& geoPos, double tolerance) const override;
    int  findNearestEditPoint(const QJsonObject& props, const QPointF& canvasPos, const MapWidget* map) const override;
    void updateEditPoint(QJsonObject& props, int index, const QPointF& newGeoPos) override;
};

#endif // BUILTIN_SHAPE_TYPES_H
