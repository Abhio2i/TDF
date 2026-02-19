/* ========================================================================= */
/* File: applicationdialog.h                                                */
/* Purpose: Dialog for configuring application-wide settings and parameters */
// Written by Arti Rajpoot
/* ========================================================================= */

#ifndef APPLICATIONDIALOG_H
#define APPLICATIONDIALOG_H

#include <QDialog>          // For dialog base class
#include <QWidget>          // For widget base class
#include <QLineEdit>        // For text input fields
#include <QPushButton>      // For button controls
#include <QLabel>           // For text labels
#include <QVBoxLayout>      // For vertical layout
#include <QHBoxLayout>      // For horizontal layout
#include <QFormLayout>      // For form layout
#include <QIntValidator>    // For integer input validation
#include <QFrame>           // For frame containers
#include <QGroupBox>        // For grouping widgets
#include <QCheckBox>        // For checkbox controls

// %%% Class Definition %%%
/* Dialog for managing application-wide configuration settings */
class ApplicationDialog : public QDialog
{
    Q_OBJECT

public:
    // %%% Constructor %%%
    /* Initialize application settings dialog */
    explicit ApplicationDialog(QWidget *parent = nullptr);

    // %%% Destructor %%%
    /* Clean up dialog resources */
    ~ApplicationDialog();

    // %%% Accessor Methods %%%
    /* Get frames per second setting */
    int getFPS() const;
    /* Get GUI frames per second setting */
    int getGUIFPS() const;
    /* Get simulation frames per second setting */
    int getSimulationFPS() const;
    /* Get physics frames per second setting */
    int getPhysicsFPS() const;
    /* Get image size setting */
    QString getImageSize() const;
    /* Get developer mode state */
    bool getDeveloperMode() const;

    // %%% Global Static Accessors %%%
    /* Get global FPS setting */
    static int getGlobalFPS();
    /* Get global GUI FPS setting */
    static int getGlobalGUIFPS();
    /* Get global simulation FPS setting */
    static int getGlobalSimulationFPS();
    /* Get global physics FPS setting */
    static int getGlobalPhysicsFPS();
    /* Get global image size setting */
    static QString getGlobalImageSize();
    /* Get image size converted to pixels */
    static QString getImageSizeInPixels();

    // %%% Global Static Mutators %%%
    /* Set global FPS setting */
    static void setGlobalFPS(int fps);
    /* Set global GUI FPS setting */
    static void setGlobalGUIFPS(int guifps);
    /* Set global simulation FPS setting */
    static void setGlobalSimulationFPS(int simfps);
    /* Set global physics FPS setting */
    static void setGlobalPhysicsFPS(int physicsfps);
    /* Set global image size setting */
    static void setGlobalImageSize(const QString& size);
    /* Reset all global settings to defaults */
    static void resetGlobalSettings();
    /* Get global developer mode state */
    static bool getGlobalDeveloperMode();
    /* Set global developer mode state */
    static void setGlobalDeveloperMode(bool enabled);

signals:
    // %%% Setting Change Signals %%%
    /* Signal FPS setting change */
    void fpsState(int value);
    /* Signal GUI FPS setting change */
    void guiFPSState(int value);
    /* Signal simulation FPS setting change */
    void simulationFPSState(int value);
    /* Signal physics FPS setting change */
    void physicsFPSState(int value);
    /* Signal canvas icon setting change */
    void canvasIconState(int value);
    /* Signal developer mode state change */
    void developerModeState(bool enabled);

private slots:
    // %%% Action Slots %%%
    /* Handle OK button click */
    void onOkClicked();
    /* Handle Cancel button click */
    void onCancelClicked();
    /* Validate all input fields */
    void validateInputs();

private:
    // %%% UI Setup Methods %%%
    /* Set up user interface components */
    void setupUI();
    /* Set up signal-slot connections */
    void setupConnections();
    /* Initialize widget values */
    void initializeWidgets();
    /* Clean up widget resources */
    void cleanupWidgets();

    // %%% Validation Methods %%%
    /* Validate FPS input field */
    bool validateFPSInput(QLineEdit* edit, QLabel* errorLabel, const QString& fieldName);
    /* Validate image size input */
    bool validateImageSizeInput();

    // %%% UI Component Members %%%
    QGroupBox *settingsGroup;          // Group box for settings
    QCheckBox *developerModeCheckBox;  // Developer mode checkbox
    QLineEdit *fpsEdit;                // FPS input field
    QLineEdit *guiFPSEdit;             // GUI FPS input field
    QLineEdit *simulationFPSEdit;      // Simulation FPS input field
    QLineEdit *physicsFPSEdit;         // Physics FPS input field
    QLineEdit *imageSizeEdit;          // Image size input field
    QPushButton *okButton;             // OK confirmation button
    QPushButton *cancelButton;         // Cancel button

    // %%% Error Display Members %%%
    QLabel *fpsErrorLabel;             // FPS error message label
    QLabel *guiFPSErrorLabel;          // GUI FPS error message label
    QLabel *simulationFPSErrorLabel;   // Simulation FPS error message label
    QLabel *physicsFPSErrorLabel;      // Physics FPS error message label
    QLabel *imageSizeErrorLabel;       // Image size error message label

    // %%% Static Configuration Members %%%
    static bool s_developerMode;       // Global developer mode flag
    static bool s_initialized;         // Initialization state flag

    // %%% Validator Members %%%
    QIntValidator *fpsValidator;               // FPS input validator
    QIntValidator *guiFPSValidator;            // GUI FPS input validator
    QIntValidator *simulationFPSValidator;     // Simulation FPS input validator
    QIntValidator *physicsFPSValidator;        // Physics FPS input validator
};

#endif // APPLICATIONDIALOG_H
