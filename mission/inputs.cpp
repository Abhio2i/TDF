#include "inputs.h"
#include "ui_inputs.h"

Inputs::Inputs(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Inputs)
{
    ui->setupUi(this);
    this->layout()->setAlignment(Qt::AlignTop);
}

Inputs::~Inputs()
{
    delete ui;
}

void Inputs::on_comboBox_currentIndexChanged(int index)
{
    ui->stackedWidget->setCurrentIndex(index);
    input = (Input)index;
}

QString Inputs::Execute(){
    if(input == Inputs::Input::action){
        return "1";
    }else
        if(input == Inputs::Input::True){
            return "1";
        }else
            if(input == Inputs::Input::False){
                return "0";
            }else
                if(input == Inputs::Input::Int){
                    return QString::number(ui->intfield->value());
                }else
                    if(input == Inputs::Input::Float){
                        return QString::number(ui->floatField->value());
                    }else
                        if(input == Inputs::Input::String){
                            return ui->stringField->text();
                        }

    return "0";
}

