/* ========================================================================= */
/* File: applicationdialog.h                                                */
/* Purpose: Dialog for configuring application-wide settings and parameters */
// Written by Arti Rajpoot
/* ========================================================================= */

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
#include <QTabWidget>

class ApplicationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ApplicationDialog(QWidget *parent = nullptr);
    ~ApplicationDialog();

    // %%% Accessor Methods %%%
    int getFPS() const;
    int getGUIFPS() const;
    int getSimulationFPS() const;
    int getPhysicsFPS() const;
    QString getImageSize() const;
    bool getDeveloperMode() const;

    // %%% Global Static Accessors %%%
    static int getGlobalFPS();
    static int getGlobalGUIFPS();
    static int getGlobalSimulationFPS();
    static int getGlobalPhysicsFPS();
    static QString getGlobalImageSize();
    static QString getImageSizeInPixels();

    // %%% Database Static Accessors %%%
    static bool getGlobalDatabaseEnabled();
    static QString getGlobalDatabasePath();
    static void setGlobalDatabaseEnabled(bool enabled);
    static void setGlobalDatabasePath(const QString& path);

    // %%% Global Static Mutators %%%
    static void setGlobalFPS(int fps);
    static void setGlobalGUIFPS(int guifps);
    static void setGlobalSimulationFPS(int simfps);
    static void setGlobalPhysicsFPS(int physicsfps);
    static void setGlobalImageSize(const QString& size);
    static void resetGlobalSettings();
    static bool getGlobalDeveloperMode();
    static void setGlobalDeveloperMode(bool enabled);
    static void loadSelectedDatabase();

signals:
    void fpsState(int value);
    void guiFPSState(int value);
    void simulationFPSState(int value);
    void physicsFPSState(int value);
    void canvasIconState(int value);
    void developerModeState(bool enabled);
    void databaseSettingsChanged(bool enabled, const QString& path);

private slots:
    void onOkClicked();
    void onCancelClicked();
    void validateInputs();
    void onResetDatabasePath();
    void onBrowseDatabasePath();

private:
    // %%% UI Setup Methods %%%
    void setupUI();
    void setupConnections();
    void initializeWidgets();
    void cleanupWidgets();

    // %%% Tab builders ─────────────────────────────────────
    QWidget* buildGeneralTab();
    QWidget* buildDatabaseTab();
    QWidget* buildPhysicsTab();

    // %%% Validation %%%
    bool validateFPSInput(QLineEdit* edit, QLabel* errorLabel, const QString& fieldName);
    bool validateImageSizeInput();

    // %%% UI Components — General tab %%%
    QGroupBox  *settingsGroup;
    QCheckBox  *developerModeCheckBox;
    QLineEdit  *fpsEdit;
    QLineEdit  *guiFPSEdit;
    QLineEdit  *simulationFPSEdit;
    QLineEdit  *physicsFPSEdit;
    QLineEdit  *imageSizeEdit;
    QPushButton *okButton;
    QPushButton *cancelButton;

    // %%% Error labels — General tab %%%
    QLabel *fpsErrorLabel;
    QLabel *guiFPSErrorLabel;
    QLabel *simulationFPSErrorLabel;
    QLabel *physicsFPSErrorLabel;
    QLabel *imageSizeErrorLabel;

    // %%% UI Components — Database tab ─────────────────────
    QCheckBox  *databaseEnabledCheckBox;
    QLineEdit  *databasePathEdit;
    QPushButton *browseDatabaseButton;
    QPushButton *resetDatabaseButton;
    QLabel     *databasePathLabel;

    // %%% Tab widget ───────────────────────────────────────
    QTabWidget *tabWidget;

    // %%% Static members %%%
    static bool    s_developerMode;
    static bool    s_initialized;
    static bool    s_databaseEnabled;
    static QString s_databasePath;

    // %%% Validators %%%
    QIntValidator *fpsValidator;
    QIntValidator *guiFPSValidator;
    QIntValidator *simulationFPSValidator;
    QIntValidator *physicsFPSValidator;
};

#endif // APPLICATIONDIALOG_H
