
#ifndef ISHAPE_TYPE_H
#define ISHAPE_TYPE_H

#include <QString>
#include <QIcon>
#include <QJsonObject>
#include <QPainter>
#include <QPointF>
#include <QMouseEvent>
#include <QVector>
#include <QWidget>

class MapWidget;

class IShapeType
{
public:
    virtual ~IShapeType() = default;

    // ---------- Identity ----------
    virtual QString typeId() const = 0;
    virtual QString displayName() const = 0;
    virtual QIcon   icon() const { return QIcon(); }
    virtual bool    isBuiltIn() const { return false; }

    // ---------- Drawing Mode ----------
    virtual void onStartDrawing(MapWidget* /*map*/) {}
    virtual bool onMousePress  (MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) = 0;
    virtual bool onMouseMove   (MapWidget* map, QMouseEvent* e, const QVector<QPointF>& points) = 0;
    virtual bool onMouseRelease(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points) = 0;
    virtual QJsonObject finishDrawing(const QVector<QPointF>& points) = 0;

    // ---------- Rendering ----------
    virtual void draw(QPainter& painter,
                      const QJsonObject& props,
                      const MapWidget* map) const = 0;

    virtual void drawPreview(QPainter& painter,
                             const QVector<QPointF>& points,
                             const MapWidget* map) const = 0;

    virtual void drawEditHandles(QPainter& ,
                                 const QJsonObject& ,
                                 const MapWidget*) const {}

    // ---------- Selection Highlight ----------
    virtual void drawSelection(QPainter& painter,
                               const QJsonObject& props,
                               const MapWidget* map) const;

    // ---------- Hit Testing & Editing ----------
    virtual bool hitTest(const QJsonObject& props,
                         const QPointF& geoPos,
                         double tolerance) const = 0;

    virtual int findNearestEditPoint(const QJsonObject& ,
                                     const QPointF& ,
                                     const MapWidget*) const { return -1; }

    virtual void updateEditPoint(QJsonObject& ,
                                 int ,
                                 const QPointF& ) {}

    // ---------- Properties ----------
    virtual void showProperties(QWidget* ,
                                const QString& ,
                                QJsonObject& ) {}

    virtual QJsonObject defaultProperties() const { return QJsonObject(); }
};

#endif // ISHAPE_TYPE_H
