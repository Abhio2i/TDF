#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    // Get current settings
    int getFrameRate() const;
    QString getImageSize() const;

    // Set settings
    void setFrameRate(int rate);
    void setImageSize(const QString& size);

private:
    void setupUI();
    void setupConnections();

    // Widgets
    QSpinBox *frameRateSpinBox;
    QLineEdit *imageSizeLineEdit;
    QPushButton *saveButton;
    QPushButton *cancelButton;

    // Labels for validation
    QLabel *frameRateLabel;
    QLabel *imageSizeLabel;

private slots:
    void validateImageSize();
};

#endif // SETTINGSDIALOG_H
