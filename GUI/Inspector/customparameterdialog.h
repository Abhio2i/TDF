
#ifndef CUSTOMPARAMETERDIALOG_H
#define CUSTOMPARAMETERDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class CustomParameterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CustomParameterDialog(QWidget *parent = nullptr);
    QString getParameterName() const;
    QString getParameterType() const;
    QString getParameterValue() const;

private slots:
    void validateInput();

private:
    QLineEdit *nameEdit;
    QComboBox *typeCombo;
    QLineEdit *valueEdit;
};

#endif // CUSTOMPARAMETERDIALOG_H
