#ifndef PARAMETER_H
#define PARAMETER_H
#include<QObject>
#include<core/Struct/constants.h>
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

    void toJson();
    void fromJson();
};

#endif // PARAMETER_H
