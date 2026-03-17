#include "mission/actions.h"
#include "mission/inputs.h"
#ifndef IFCONDITION_H
#define IFCONDITION_H

#include <QWidget>

namespace Ui {
class IFCondition;
}

class IFCondition : public QWidget
{
    Q_OBJECT

public:
    explicit IFCondition(QWidget *parent = nullptr);
    ~IFCondition();
    Inputs* leftInput;
    Inputs* rightInput;
    Actions* action;

    int Execute();

private:
    Ui::IFCondition *ui;

};

#endif // IFCONDITION_H
