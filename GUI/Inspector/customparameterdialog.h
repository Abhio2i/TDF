/* ========================================================================= */
/* File: customparameterdialog.h                                            */
/* Purpose: Defines dialog for adding custom parameters                      */
/* ========================================================================= */

#ifndef CUSTOMPARAMETERDIALOG_H
#define CUSTOMPARAMETERDIALOG_H

#include <QDialog>                                // For dialog base class
#include <QLineEdit>                              // For text input widget
#include <QComboBox>                              // For combo box widget
#include <QPushButton>                            // For push button widget
#include <QVBoxLayout>                            // For vertical layout
#include <QLabel>                                 // For label widget

// %%% Class Definition %%%
/* Dialog for custom parameter input */
class CustomParameterDialog : public QDialog
{
    Q_OBJECT

public:
    // Initialize dialog
    explicit CustomParameterDialog(QWidget *parent = nullptr);
    // Get parameter name
    QString getParameterName() const;
    // Get parameter type
    QString getParameterType() const;
    // Get parameter value
    QString getParameterValue() const;

private slots:
    // Validate input fields
    void validateInput();

private:
    // %%% UI Components %%%
    // Name input field
    QLineEdit *nameEdit;
    // Type selection combo
    QComboBox *typeCombo;
    // Value input field
    QLineEdit *valueEdit;
};

#endif // CUSTOMPARAMETERDIALOG_H
