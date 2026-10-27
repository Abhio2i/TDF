#ifndef SHAPE_MANAGER_H
#define SHAPE_MANAGER_H

#include <QObject>
#include <QMap>
#include <QVector>
#include "common/types.h"

class ShapeManager : public QObject {
    Q_OBJECT
    
public:
    static ShapeManager* instance();
    ~ShapeManager();
    
    // CRUD operations
    QString addShape(const ShapeData& shape);
    bool updateShape(const QString& id, const ShapeData& shape);
    bool deleteShape(const QString& id);
    bool deleteAllShapes();
    int deleteShapesByType(const QString& type);
    
    // Get operations
    ShapeData getShape(const QString& id) const;
    QVector<ShapeData> getAllShapes() const;
    int getShapeCount() const;
    QString getShapeType(const QString& id) const;
    QPointF getShapePosition(const QString& id) const;
    bool moveShape(const QString& id, double newLon, double newLat);
    QString findShapeAt(const QPointF& geoPos) const;
    QVector<QString> findShapesInRect(const QRectF& rect) const;
    
signals:
    void shapeAdded(const QString& id);
    void shapeUpdated(const QString& id);
    void shapeDeleted(const QString& id);
    void allShapesDeleted();
    
private:
    explicit ShapeManager(QObject* parent = nullptr);
    static ShapeManager* m_instance;
    QMap<QString, ShapeData> m_shapes;
    QVector<QString> m_shapeOrder;
    int m_counter;
    QString generateId();
    bool pointOnSegment(const QPointF& p, const QPointF& p1, const QPointF& p2) const;
    bool pointNearSegment(const QPointF& p, const QPointF& p1, const QPointF& p2, double tolerance) const;
};

#endif // SHAPE_MANAGER_H
