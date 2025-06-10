
#ifndef OVERVIEW_H
#define OVERVIEW_H

#include <QWidget>
#include <QLabel>

class Overview : public QWidget
{
    Q_OBJECT

public:
    explicit Overview(QWidget *parent = nullptr);  // Capital 'O'

private:
    QLabel *label;
};

#endif // OVERVIEW_H
