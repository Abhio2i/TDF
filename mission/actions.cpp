#include "actions.h"
#include "qdebug.h"
#include "ui_actions.h"

Actions::Actions(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Actions)
{
    ui->setupUi(this);
    this->layout()->setAlignment(Qt::AlignTop);
    ui->numberField->hide();
    ui->stringField->hide();
}

Actions::~Actions()
{
    delete ui;
}

int Actions::Execute(){
    if(action == Actions::Action::none){

    }else
    if(action == Actions::Action::goHome){

    }else
    if(action == Actions::Action::takeoff){

    }else
    if(action == Actions::Action::print){
        qDebug()<< ui->stringField->text();
    }else
    if(action == Actions::Action::Jump){
        return ui->numberField->value();
    }else
    if(action == Actions::Action::end){
        return -2;
    }
    return -1;
}

void Actions::on_action_currentIndexChanged(int index)
{
    action = (Action)index;
    if(action == Actions::Action::none){
        ui->numberField->hide();
        ui->stringField->hide();
    }else
    if(action == Actions::Action::goHome){
        ui->numberField->hide();
        ui->stringField->hide();
    }else
    if(action == Actions::Action::takeoff){
        ui->numberField->hide();
        ui->stringField->hide();
    }else
    if(action == Actions::Action::print){
        ui->numberField->hide();
        ui->stringField->show();
    }else
    if(action == Actions::Action::Jump){
        ui->numberField->show();
        ui->stringField->hide();
    }else
    if(action == Actions::Action::end){
        ui->numberField->hide();
        ui->stringField->hide();
    }

}

