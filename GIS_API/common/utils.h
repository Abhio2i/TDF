#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QUuid>
#include <QPointF>
#include <cmath>
#include "types.h"

namespace Utils {
    inline QString generateId() {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    
    inline double distance(const QPointF& p1, const QPointF& p2) {
        double dx = p1.x() - p2.x();
        double dy = p1.y() - p2.y();
        return std::sqrt(dx*dx + dy*dy);
    }
    
    inline bool isValidShape(const ShapeData& shape) {
        if (shape.type.isEmpty()) return false;
        if (shape.type == "circle" && shape.radius <= 0) return false;
        if (shape.type == "rectangle" && (shape.width <= 0 || shape.height <= 0)) return false;
        if ((shape.type == "polygon" || shape.type == "line") && shape.points.size() < 2) return false;
        return true;
    }
}

#endif // UTILS_H
