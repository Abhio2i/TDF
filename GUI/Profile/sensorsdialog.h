#ifndef SENSORSDIALOG_H
#define SENSORSDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>

class SensorsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SensorsDialog(QWidget *parent = nullptr);

    // Getter methods for checkbox states
    bool isSensorsChecked() const;
    bool isEsmChecked() const;
    bool isCsmChecked() const;
    bool isRadioChecked() const;
    bool isIffChecked() const;

    // Get selected sensors as list
    QStringList getSelectedSensors() const;

private:
    // Checkboxes for sensors
    QCheckBox *sensorsCheckBox;
    QCheckBox *esmCheckBox;
    QCheckBox *csmCheckBox;
    QCheckBox *radioCheckBox;
    QCheckBox *iffCheckBox;

    // Buttons
    QPushButton *okButton;
    QPushButton *cancelButton;

    // Layouts
    QVBoxLayout *mainLayout;
    QGroupBox *sensorsGroupBox;
    QVBoxLayout *sensorsLayout;
    QHBoxLayout *buttonLayout;
    QDialogButtonBox *buttonBox;

    void setupUI();
    void setupConnections();
};

#endif // SENSORSDIALOG_H
