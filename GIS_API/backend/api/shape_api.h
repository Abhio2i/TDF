
#ifndef SHAPE_API_H
#define SHAPE_API_H

#include <QObject>
#include <QString>
#include <QPointF>
#include <QColor>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include "common/types.h"

class ShapeApi : public QObject {
    Q_OBJECT

public:
    static ShapeApi* instance();

    // CREATE
    Q_INVOKABLE QString addShape(const QJsonObject& shapeData);
    Q_INVOKABLE QString addCircle(double lon, double lat, double radius, const QColor& color = Qt::red);
    Q_INVOKABLE QString addRectangle(double lon, double lat, double w, double h, const QColor& color = Qt::red);
    Q_INVOKABLE QString addPolygon(const QVector<QPointF>& points, const QColor& color = Qt::red);
    Q_INVOKABLE QString addLine(const QVector<QPointF>& points, const QColor& color = Qt::red);
    Q_INVOKABLE QString addPoint(double lon, double lat, const QColor& color = Qt::red);
    Q_INVOKABLE QString addText(double lon, double lat, const QString& text, const QColor& color = Qt::white);

    // DELETE
    Q_INVOKABLE bool deleteShape(const QString& id);
    Q_INVOKABLE bool deleteAllShapes();
    Q_INVOKABLE int deleteShapesByType(const QString& type);

    // EDIT
    Q_INVOKABLE bool updateShape(const QString& id, const QJsonObject& data);
    Q_INVOKABLE bool updateShapeColor(const QString& id, const QColor& color);
    Q_INVOKABLE bool updateShapeWidth(const QString& id, int width);

    // MOVE
    Q_INVOKABLE bool moveShape(const QString& id, double newLon, double newLat);
    Q_INVOKABLE bool moveShapeBy(const QString& id, double deltaLon, double deltaLat);

    // GET
    Q_INVOKABLE QJsonObject getShapeProperties(const QString& id) const;
    Q_INVOKABLE QJsonArray getAllShapes() const;
    Q_INVOKABLE int getShapeCount() const;
    Q_INVOKABLE QString getShapeType(const QString& id) const;
    Q_INVOKABLE QString findShapeAt(const QPointF& geoPos) const;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);
    void loadShapes(const QJsonArray& shapes, bool clearExisting = true);
    void importFromLegacyTactical(const QJsonObject& tactical);

signals:
    void shapeAdded(const QString& id);
    void shapeUpdated(const QString& id);
    void shapeDeleted(const QString& id);
    void allShapesDeleted();

private:
    explicit ShapeApi(QObject* parent = nullptr);
    ~ShapeApi();
    static ShapeApi* m_instance;
    void assignToActiveLayer(const QString& shapeId);

    ShapeData shapeFromJson(const QJsonObject& json) const;
    ShapeData shapeFromJson(const QJsonObject& json, const ShapeData& base) const;
    QJsonObject shapeToJson(const ShapeData& shape) const;
};

#endif // SHAPE_API_H
