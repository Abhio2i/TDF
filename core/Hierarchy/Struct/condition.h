#ifndef CONDITION_H
#define CONDITION_H
#include <QObject>
#include "./constants.h"
#include <variant>

class Condition: public QObject
{
    Q_OBJECT
public:
    Condition();
    bool And;
    std::variant<int, float, double, std::string, bool, char> first;
    std::variant<int, float, double, std::string, bool, char> second;
    Constants::OperaterType type;

    void check();

    void toJson();
    void fromJson();
};

#endif // CONDITION_H
