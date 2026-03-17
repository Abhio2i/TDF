#ifndef TASKGROUP_H
#define TASKGROUP_H

#include "mission/task.h"
#include <QWidget>

namespace Ui {
class TaskGroup;
}

class TaskGroup : public QWidget
{
    Q_OBJECT

public:
    explicit TaskGroup(QWidget *parent = nullptr);
    ~TaskGroup();
    std::vector<Task*> taskList;
    int currentTask = 0;
    bool allowrun = true;

signals:
    void goHome();
    void activateSensors();
    void deactivateSensors();
    void makeFormation();
    void deformation();

public slots:
    void run();
    void reset();
    void pause();
private slots:
    void on_addtask_clicked();
    void on_itemRemove(Task* task);

    void on_run_clicked();
    void execute(int i);

private:
    Ui::TaskGroup *ui;
};

#endif // TASKGROUP_H
