
#ifndef APPLICATIONDIALOG_H
#define APPLICATIONDIALOG_H

#include <QDialog>
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QIntValidator>
#include <QFrame>
#include <QGroupBox>

class ApplicationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ApplicationDialog(QWidget *parent = nullptr);
    ~ApplicationDialog();

    int getFPS() const;
    int getGUIFPS() const;
    int getSimulationFPS() const;
    int getPhysicsFPS() const;
    QString getImageSize() const;


    // Global access functions declarations
    int getGlobalFPS();
    int getGlobalGUIFPS();
    int getGlobalSimulationFPS();
    int getGlobalPhysicsFPS();
    QString getGlobalImageSize();
    QString getImageSizeInPixels();

    void setGlobalFPS(int fps);
    void setGlobalGUIFPS(int guifps);
    void setGlobalSimulationFPS(int simfps);
    void setGlobalPhysicsFPS(int physicsfps);
    void setGlobalImageSize(const QString& size);
    void resetGlobalSettings();

signals:
    void fpsState(int value);
    void guiFPSState(int value);
    void simulationFPSState(int value);
    void physicsFPSState(int value);
    void canvasIconState(int value);

private slots:
    void onOkClicked();
    void onCancelClicked();
    void validateInputs();

private:
    void setupUI();
    void setupConnections();

    QGroupBox *settingsGroup;
    QLineEdit *fpsEdit;
    QLineEdit *guiFPSEdit;
    QLineEdit *simulationFPSEdit;
    QLineEdit *physicsFPSEdit;
    QLineEdit *imageSizeEdit;
    QPushButton *okButton;
    QPushButton *cancelButton;

    QLabel *fpsErrorLabel;
    QLabel *guiFPSErrorLabel;
    QLabel *simulationFPSErrorLabel;
    QLabel *physicsFPSErrorLabel;
    QLabel *imageSizeErrorLabel;
};

#endif // APPLICATIONDIALOG_H
