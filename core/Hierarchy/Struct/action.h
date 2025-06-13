#ifndef ACTION_H
#define ACTION_H
#include <QObject>
#include "./constants.h"

class Action: public QObject
{
    Q_OBJECT
public:
    Action();
    Constants::ActionType actionType;

    void toJson();
    void fromJson();

signals:
    void action();
};

#endif // ACTION_H
