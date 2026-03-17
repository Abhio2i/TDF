#ifndef TASK_H
#define TASK_H

#include "mission/actions.h"
#include "mission/ifcondition.h"
#include "qlabel.h"
#include "qlistwidget.h"
#include <QWidget>

namespace Ui {
class Task;
}
class TaskGroup;
class Task : public QWidget
{
    Q_OBJECT

public:
    explicit Task(QWidget *parent = nullptr, TaskGroup *master = nullptr);
    ~Task();
    int index = 0;
    enum Command {
        Action,
        Condition,
        Jump,
        Wait
    };
    Command command = Task::Command::Action;
    Actions* action;
    IFCondition* condition;
    QWidget* widget;
    QLabel* label;
    QListWidgetItem* item;

    bool complete = false;
    void Execute();
    void run(bool complete);

private slots:
    void on_command_currentIndexChanged(int index);
    void on_removeButton_clicked();

signals:
    void Next(int Index);
    void on_delete(Task* task);
private:
    Ui::Task *ui;
    TaskGroup *master;
};

#endif // TASK_H
