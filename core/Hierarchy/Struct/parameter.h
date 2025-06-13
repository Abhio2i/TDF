#ifndef PARAMETER_H

#define PARAMETER_H
#include<QObject>
#include "./constants.h"
class Parameter: public QObject
{
    Q_OBJECT
public:
    Parameter();
    std::string Name;
    Constants::ParameterType type;
    std::variant<int, float, double, std::string, bool, char> value;

    void setValue();
    void getValue();

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);

};

#endif // PARAMETER_H
