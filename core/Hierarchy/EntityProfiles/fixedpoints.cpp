#include "fixedpoints.h"

FixedPoints::FixedPoints(Hierarchy *h):Entity(h) {}

QJsonObject FixedPoints::toJson() const {
    QJsonObject obj;
    return obj;
}

void FixedPoints::fromJson(const QJsonObject& obj) {
}
