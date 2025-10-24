#ifndef TASK_H
#define TASK_H
#include <QObject>
#include "./condition.h"
#include "./action.h"

class Task: public QObject
{
    Q_OBJECT
public:
    Task();
    std::string Name;
    Condition *condition;
    Action *action;

    void setTaskName();
    void addCondition();
    void addAction();
    void removeAction();
    void removeCondition();

    void toJson();
    void fromJson();
};

#endif // TASK_H
