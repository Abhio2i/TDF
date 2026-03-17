#include "task.h"
#include "mission/taskgroup.h"
#include "ui_task.h"
#include <QDebug>
#include <QTimer>

Task::Task(QWidget *parent, TaskGroup *master)
    : QWidget(parent)
    , ui(new Ui::Task)
{
    ui->setupUi(this);
     this->layout()->setAlignment(Qt::AlignTop);
    action = new Actions();
    condition = new IFCondition();
    ui->verticalLayout->addWidget(action);
    ui->verticalLayout_4->addWidget(condition);
    label = ui->label;
    run(false);
}

Task::~Task()
{
    delete ui;
    delete action;
    delete condition;
    delete label;
    qDebug()<<"task delete";
}

void Task::Execute(){
    int Index = index+1;
    int waitTime = 200;
    if(command == Task::Command::Action){
        int results = action->Execute();
        qDebug()<<"Action";
        if(action->action == Actions::Action::end){//end command
            qDebug()<<"END";
            run(true);
            return;
        }else
        if(action->action == Actions::Action::Jump){//jump command
            Index = results;
        }
    }else
    if(command == Task::Command::Condition){
        int results = condition->Execute();
        qDebug()<<"Condition";
        if(condition->action->action == Actions::Action::end){//end command
            qDebug()<<"END";
            run(true);
            return;
        }else
        if(condition->action->action == Actions::Action::Jump){//jump command
            Index = results;
        }
    }else
    if(command == Task::Command::Jump){
        qDebug()<<"Jump";
        Index = ui->JumpspinBox->value();
    }else
    if(command == Task::Command::Wait){
        qDebug()<<"Wait";
        waitTime += ui->waitspinBox->value() * 1000;
    }


    qDebug()<<label->text()<<" Execute"<<QString::number(index);
    run(false);
    QTimer::singleShot(waitTime, this, [=]() {
        run(true);
        emit Next(Index);
    });

}

void Task::run(bool complete){
    ui->progressWidget->setCurrentIndex(complete?0:1);
}

void Task::on_command_currentIndexChanged(int index)
{
    ui->stackedWidget->setCurrentIndex(index);
    command = (Command)index;
}

void Task::on_removeButton_clicked()
{
    emit on_delete(this);
}

