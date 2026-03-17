#include "ifcondition.h"
#include "ui_ifcondition.h"
#include <QDebug>
IFCondition::IFCondition(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::IFCondition)
{
    ui->setupUi(this);
     this->layout()->setAlignment(Qt::AlignTop);
    action = new Actions();
    leftInput = new Inputs();
    rightInput = new Inputs();
    ui->leftinput->addWidget(leftInput);
    ui->rightinput->addWidget(rightInput);
    ui->horizontalLayout->addWidget(action);
}

IFCondition::~IFCondition()
{
    delete ui;
    delete action;
    delete leftInput;
    delete rightInput;
    qDebug()<<"condition delete";
}

int IFCondition::Execute(){
    QString left = leftInput->Execute();
    QString right = rightInput->Execute();
    if(left == right){
        return action->Execute();
    }
    return -1;
}


