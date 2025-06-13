#ifndef FORMATIONPOSITION_H
#define FORMATIONPOSITION_H
#include <QObject>
#include <core/Hierarchy/entity.h>
#include "./vector.h"
#include "./geocords.h"

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
