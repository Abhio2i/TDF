
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
#include <QCheckBox>

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
    bool getDeveloperMode() const;

    // Global access functions declarations
    static int getGlobalFPS();
    static int getGlobalGUIFPS();
    static int getGlobalSimulationFPS();
    static int getGlobalPhysicsFPS();
    static QString getGlobalImageSize();
    static QString getImageSizeInPixels();

    static void setGlobalFPS(int fps);
    static void setGlobalGUIFPS(int guifps);
    static void setGlobalSimulationFPS(int simfps);
    static void setGlobalPhysicsFPS(int physicsfps);
    static void setGlobalImageSize(const QString& size);
    static void resetGlobalSettings();
    static bool getGlobalDeveloperMode();
    static void setGlobalDeveloperMode(bool enabled);

signals:
    void fpsState(int value);
    void guiFPSState(int value);
    void simulationFPSState(int value);
    void physicsFPSState(int value);
    void canvasIconState(int value);
    void developerModeState(bool enabled);

private slots:
    void onOkClicked();
    void onCancelClicked();
    void validateInputs();


private:
    void setupUI();
    void setupConnections();
    void initializeWidgets();
    void cleanupWidgets();
    bool validateFPSInput(QLineEdit* edit, QLabel* errorLabel, const QString& fieldName);
    bool validateImageSizeInput();

    QGroupBox *settingsGroup;
    QCheckBox *developerModeCheckBox;
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

    static bool s_developerMode;
    static bool s_initialized;

    // Validators
    QIntValidator *fpsValidator;
    QIntValidator *guiFPSValidator;
    QIntValidator *simulationFPSValidator;
    QIntValidator *physicsFPSValidator;
};

#endif // APPLICATIONDIALOG_H
