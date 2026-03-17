#include "taskgroup.h"
#include "ui_taskgroup.h"
#include <QListWidgetItem>

TaskGroup::TaskGroup(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TaskGroup)
{
    ui->setupUi(this);
    this->layout()->setAlignment(Qt::AlignTop);
    ui->horizontalLayout->setAlignment(Qt::AlignLeft);
}

TaskGroup::~TaskGroup()
{
    delete ui;
}

void TaskGroup::on_addtask_clicked()
{
    Task* task = new Task();
    task->label->setText("Task_"+QString::number(taskList.size()+1));
    QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
    task->item = item;
    item->setSizeHint(task->sizeHint());
    ui->listWidget->setItemWidget(item, task);
    connect(task,&Task::on_delete,this,&TaskGroup::on_itemRemove);
    connect(task,&Task::Next,this,&TaskGroup::execute);
    task->label->setText("Task_" + QString::number(ui->listWidget->count()));
    task->index = ui->listWidget->count()-1;

}

void TaskGroup::on_itemRemove(Task* task){
    ui->listWidget->removeItemWidget(task->item);
    delete task->item;
    task->deleteLater();
    for(int i=0;i<ui->listWidget->count();i++){
        QListWidgetItem* item = ui->listWidget->item(i);
        Task* task = qobject_cast<Task*>(ui->listWidget->itemWidget(item));
        if(task) {
            task->label->setText("Task_" + QString::number(i + 1));
                task->index = i;
        }
    }

}

void TaskGroup::reset(){
    currentTask = 0;
    allowrun = false;
}

void TaskGroup::run(){
    allowrun = true;
    execute(currentTask);
}

void TaskGroup::pause(){
    allowrun = false;
}

void TaskGroup::on_run_clicked()
{
    execute(0);
}

void TaskGroup::execute(int i){
    QListWidgetItem* item = ui->listWidget->item(i);
    Task* task = qobject_cast<Task*>(ui->listWidget->itemWidget(item));
    if(task && allowrun) {
        currentTask = i;
        task->Execute();
    }
}

