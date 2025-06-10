#ifndef FORMATION_H
#define FORMATION_H
#include <QObject>
#include <core/Struct/constants.h>
#include <core/Struct/formationposition.h>

class Formation: public QObject
{
    Q_OBJECT
public:
    Formation();
    Constants::FormationType formationType;
    int count;
    FormationPosition *mothership;
    std::unordered_map<std::string, FormationPosition> *formationPositions;

    void toJson();
    void fromJson();
};

#endif // FORMATION_H
