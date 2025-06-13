#ifndef FIXEDPOINTS_H
#define FIXEDPOINTS_H
#include <core/Hierarchy/entity.h>
#include <QObject>
#include <QJsonObject>

class FixedPoints: public Entity
{
    Q_OBJECT
public:
    FixedPoints(Hierarchy* h);

    QJsonObject toJson()const override;
    void fromJson(const QJsonObject &obj) override;
};

#endif // FIXEDPOINTS_H
