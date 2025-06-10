#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <QObject>
#include <core/Struct/vector.h>
#include <core/Struct/geocords.h>
#include <core/Components/component.h>
#include <QJsonObject>
class Transform: public QObject, public Component
{
    Q_OBJECT
public:
    Transform();
    ComponentType Typo() const override { return ComponentType::Transform; }
    bool Active;
    std::string ID;

    Geocords *geocord;
    Vector *position;
    Vector *rotation;
    Vector *size;

    Vector *localPosition;
    Vector *localRotation;
    Vector *localSize;

    void translate(Vector *vector);
    void rotate(Vector *vector);
    void scale(Vector *vector);

    Vector forward();
    Vector right();
    Vector up();
    Vector back();
    Vector left();
    Vector down();

    Vector inverseTransformDirection(const Vector& worldDir);


    QJsonObject toJson()const override;
    void fromJson(const QJsonObject &obj) override;

};

#endif // TRANSFORM_H
