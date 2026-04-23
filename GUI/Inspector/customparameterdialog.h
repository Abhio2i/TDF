/* =============================================================================
 * FILE:         customparameterdialog.h
 * MODULE:       Custom Parameter Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the CustomParameterDialog class which provides a modal
 *               dialog for adding custom parameters (name, type, value). It is
 *               used within the Inspector panel to allow users to extend entity
 *               or component definitions with user-defined key-value pairs.
 *
 * REQUIREMENTS: REQ-CUSTOMPARAM-010  Custom parameter input dialog
 *               REQ-CUSTOMPARAM-011  Input field for parameter name
 *               REQ-CUSTOMPARAM-012  Dropdown for parameter type selection
 *               REQ-CUSTOMPARAM-013  Input field for parameter value
 *               REQ-CUSTOMPARAM-014  Input validation before acceptance
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-CUSTOMPARAM-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef CUSTOMPARAMETERDIALOG_H
#define CUSTOMPARAMETERDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QStackedWidget>
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
    void onTypeChanged(const QString &type);

private:
    QLineEdit *nameEdit;
    QComboBox *typeCombo;
    QStackedWidget *valueStack;
    QLineEdit *valueEdit;
    QCheckBox *valueCheckBox;
};

#endif // CUSTOMPARAMETERDIALOG_H
