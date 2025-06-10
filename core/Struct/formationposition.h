#ifndef FORMATIONPOSITION_H
#define FORMATIONPOSITION_H
#include <QObject>
#include <core/structure/entity.h>
#include <core/Struct/vector.h>
#include <core/Struct/geocords.h>

class FormationPosition: public QObject
{
    Q_OBJECT
public:
    FormationPosition();

    Entity *entity;
    Vector *Offset;
    Geocords *geoOffset;

    void toJson();
    void fromJson();

};

#endif // FORMATIONPOSITION_H
