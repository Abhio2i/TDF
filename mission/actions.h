#ifndef ACTIONS_H
#define ACTIONS_H

#include <QWidget>

namespace Ui {
class Actions;
}

class Actions : public QWidget
{
    Q_OBJECT

public:
    enum Action{
        none,
        goHome,
        print,
        takeoff,
        Jump,
        end
    };
    explicit Actions(QWidget *parent = nullptr);
    ~Actions();
    Action action = Actions::Action::none;
    int Execute();

private slots:
    void on_action_currentIndexChanged(int index);

private:
    Ui::Actions *ui;
};

#endif // ACTIONS_H
