#ifndef INPUTS_H
#define INPUTS_H

#include <QWidget>

namespace Ui {
class Inputs;
}

class Inputs : public QWidget
{
    Q_OBJECT

public:
    enum Input{
        action,
        True,
        False,
        Int,
        Float,
        String
    };
    explicit Inputs(QWidget *parent = nullptr);
    ~Inputs();
    Input input = Inputs::Input::action;
    QString Execute();
private slots:
    void on_comboBox_currentIndexChanged(int index);

private:
    Ui::Inputs *ui;
};

#endif // INPUTS_H
