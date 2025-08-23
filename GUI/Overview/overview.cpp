#include "overview.h"
#include <QVBoxLayout>

Overview::Overview(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    label = new QLabel("Overview", this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    setLayout(layout);
}
