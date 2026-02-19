/* ========================================================================= */
/* File: applicationdialog.cpp                                               */
/* Purpose: Implements application settings dialog with configuration        */
//               Written by Arti Rajpoot
/* ========================================================================= */

#include "applicationdialog.h"                     // For application dialog class
#include "applicationdialog-styles.h"              // Include separate CSS file
#include <QMessageBox>                            // For message boxes
#include <GUI/mainwindow.h>                       // For main window reference
#include <core/Config/scenarioconfig.h>           // For scenario configuration
#include <QDebug>                                 // For debugging output
#include <QCoreApplication>                       // For application instance
#include <QRegularExpression>                     // For regex validation
#include <QRegularExpressionValidator>            // For regex validators
#include <QFont>                                  // For font handling
#include <QPalette>                               // For color palettes
#include <QStyle>                                 // For widget styling
#include <QApplication>                           // For application instance
#include <QCheckBox>                              // For checkbox widget
#include <QTimer>                                 // For timer operations
#include <QThread>                                // For thread management

// %%% Static Member Initialization %%%
/* Initialize static class members */
bool ApplicationDialog::s_developerMode = false;   // Developer mode state
bool ApplicationDialog::s_initialized = false;     // Initialization flag

// %%% Helper Functions %%%
/* Check if configuration is available */
namespace {
bool isConfigAvailable() {
    if (!MainWindow::scenarioconfig) {
        // Configuration not loaded
        return false;
    }
    return true;
}

/* Check if current thread is main thread */
bool isMainThread() {
    return QThread::currentThread() == QCoreApplication::instance()->thread();
}
}

// %%% Static Global Settings Functions %%%
/* Get global Main FPS setting */
int ApplicationDialog::getGlobalFPS()
{
    if (!isConfigAvailable()) {
        // Return default if config unavailable
        return 60;
    }
    return MainWindow::scenarioconfig->getSavedFPS();
}

/* Get global GUI FPS setting */
int ApplicationDialog::getGlobalGUIFPS()
{
    if (!isConfigAvailable()) {
        // Return default if config unavailable
        return 60;
    }
    return MainWindow::scenarioconfig->getSavedGUIFPS();
}

/* Get global Simulation FPS setting */
int ApplicationDialog::getGlobalSimulationFPS()
{
    if (!isConfigAvailable()) {
        // Return default if config unavailable
        return 60;
    }
    return MainWindow::scenarioconfig->getSavedSimulationFPS();
}

/* Get global Physics FPS setting */
int ApplicationDialog::getGlobalPhysicsFPS()
{
    if (!isConfigAvailable()) {
        // Return default if config unavailable
        return 60;
    }
    return MainWindow::scenarioconfig->getSavedPhysicsFPS();
}

/* Get global image size setting */
QString ApplicationDialog::getGlobalImageSize()
{
    if (!isConfigAvailable()) {
        // Return default if config unavailable
        return "100px";
    }
    return MainWindow::scenarioconfig->getSavedImageSize();
}

/* Get image size as pixel value */
QString ApplicationDialog::getImageSizeInPixels()
{
    QString size = getGlobalImageSize();
    QRegularExpression re("(\\d+)(?:\\s*px)?", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(size);
    if (match.hasMatch()) {
        return match.captured(1);
    }

    // Return default if parsing fails
    return "100";
}

/* Set global Main FPS setting */
void ApplicationDialog::setGlobalFPS(int fps)
{
    if (!isMainThread()) {
        // Thread safety check
        return;
    }

    if (!isConfigAvailable()) {
        // Configuration unavailable
        return;
    }

    if (fps >= 1 && fps <= 1000) {
        try {
            MainWindow::scenarioconfig->saveAppSettings(
                fps,
                MainWindow::scenarioconfig->getSavedGUIFPS(),
                MainWindow::scenarioconfig->getSavedSimulationFPS(),
                MainWindow::scenarioconfig->getSavedPhysicsFPS(),
                MainWindow::scenarioconfig->getSavedImageSize()
                );
            // Settings saved successfully
        } catch (const std::exception& e) {
            // Handle save exception
        }
    } else {
        // Invalid FPS value
    }
}

/* Set global GUI FPS setting */
void ApplicationDialog::setGlobalGUIFPS(int guifps)
{
    if (!isMainThread()) {
        // Thread safety check
        return;
    }

    if (!isConfigAvailable()) {
        // Configuration unavailable
        return;
    }

    if (guifps >= 1 && guifps <= 1000) {
        try {
            MainWindow::scenarioconfig->saveAppSettings(
                MainWindow::scenarioconfig->getSavedFPS(),
                guifps,
                MainWindow::scenarioconfig->getSavedSimulationFPS(),
                MainWindow::scenarioconfig->getSavedPhysicsFPS(),
                MainWindow::scenarioconfig->getSavedImageSize()
                );
            // Settings saved successfully
        } catch (const std::exception& e) {
            // Handle save exception
        }
    } else {
        // Invalid GUI FPS value
    }
}

/* Set global Simulation FPS setting */
void ApplicationDialog::setGlobalSimulationFPS(int simfps)
{
    if (!isMainThread()) {
        // Thread safety check
        return;
    }

    if (!isConfigAvailable()) {
        // Configuration unavailable
        return;
    }

    if (simfps >= 1 && simfps <= 1000) {
        try {
            MainWindow::scenarioconfig->saveAppSettings(
                MainWindow::scenarioconfig->getSavedFPS(),
                MainWindow::scenarioconfig->getSavedGUIFPS(),
                simfps,
                MainWindow::scenarioconfig->getSavedPhysicsFPS(),
                MainWindow::scenarioconfig->getSavedImageSize()
                );
            // Settings saved successfully
        } catch (const std::exception& e) {
            // Handle save exception
        }
    } else {
        // Invalid Simulation FPS value
    }
}

/* Set global Physics FPS setting */
void ApplicationDialog::setGlobalPhysicsFPS(int physicsfps)
{
    if (!isMainThread()) {
        // Thread safety check
        return;
    }

    if (!isConfigAvailable()) {
        // Configuration unavailable
        return;
    }

    if (physicsfps >= 1 && physicsfps <= 1000) {
        try {
            MainWindow::scenarioconfig->saveAppSettings(
                MainWindow::scenarioconfig->getSavedFPS(),
                MainWindow::scenarioconfig->getSavedGUIFPS(),
                MainWindow::scenarioconfig->getSavedSimulationFPS(),
                physicsfps,
                MainWindow::scenarioconfig->getSavedImageSize()
                );
            // Settings saved successfully
        } catch (const std::exception& e) {
            // Handle save exception
        }
    } else {
        // Invalid Physics FPS value
    }
}

/* Set global image size setting */
void ApplicationDialog::setGlobalImageSize(const QString& size)
{
    if (!isMainThread()) {
        // Thread safety check
        return;
    }

    if (!isConfigAvailable()) {
        // Configuration unavailable
        return;
    }

    if (!size.isEmpty()) {
        try {
            QString input = size.trimmed();
            QRegularExpression re("(\\d+)(?:\\s*px)?", QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = re.match(input);

            if (match.hasMatch()) {
                QString g_pixelValue = match.captured(1);
                bool ok;
                int pixelInt = g_pixelValue.toInt(&ok);

                if (ok && pixelInt >= 1 && pixelInt <= 10000) {
                    MainWindow::scenarioconfig->saveAppSettings(
                        MainWindow::scenarioconfig->getSavedFPS(),
                        MainWindow::scenarioconfig->getSavedGUIFPS(),
                        MainWindow::scenarioconfig->getSavedSimulationFPS(),
                        MainWindow::scenarioconfig->getSavedPhysicsFPS(),
                        g_pixelValue + "px"
                        );
                    // Image size saved successfully
                } else {
                    // Invalid pixel value
                }
            } else {
                // Invalid format
            }
        } catch (const std::exception& e) {
            // Handle save exception
        }
    }
}

/* Reset all global settings to defaults */
void ApplicationDialog::resetGlobalSettings()
{
    if (!isMainThread()) {
        // Thread safety check
        return;
    }

    if (!isConfigAvailable()) {
        // Configuration unavailable
        return;
    }

    try {
        int g_fps = 60;
        int g_guifps = 60;
        int g_simfps = 60;
        int g_physicsfps = 60;
        QString g_imageSize = "100px";

        MainWindow::scenarioconfig->saveAppSettings(g_fps, g_guifps, g_simfps, g_physicsfps, g_imageSize);
        // Settings reset successfully
    } catch (const std::exception& e) {
        // Handle reset exception
    }
}

/* Get global developer mode state */
bool ApplicationDialog::getGlobalDeveloperMode()
{
    return s_developerMode;
}

/* Set global developer mode state */
void ApplicationDialog::setGlobalDeveloperMode(bool enabled)
{
    if (s_developerMode != enabled) {
        s_developerMode = enabled;
        // Developer mode changed
    }
}

// %%% Constructor %%%
/* Initialize application settings dialog */
ApplicationDialog::ApplicationDialog(QWidget *parent)
    : QDialog(parent)
    , settingsGroup(nullptr)
    , developerModeCheckBox(nullptr)
    , fpsEdit(nullptr)
    , guiFPSEdit(nullptr)
    , simulationFPSEdit(nullptr)
    , physicsFPSEdit(nullptr)
    , imageSizeEdit(nullptr)
    , okButton(nullptr)
    , cancelButton(nullptr)
    , fpsErrorLabel(nullptr)
    , guiFPSErrorLabel(nullptr)
    , simulationFPSErrorLabel(nullptr)
    , physicsFPSErrorLabel(nullptr)
    , imageSizeErrorLabel(nullptr)
    , fpsValidator(nullptr)
    , guiFPSValidator(nullptr)
    , simulationFPSValidator(nullptr)
    , physicsFPSValidator(nullptr)
{
    // Apply dark theme to dialog
    setStyleSheet(ApplicationDialogStyles::Dialog);

    // Check thread safety
    if (!isMainThread()) {
        // Not in main thread
        return;
    }

    try {
        if (!MainWindow::scenarioconfig) {
            // Configuration not loaded
        }

        // Configure dialog window
        setWindowTitle("Application Settings");
        setModal(true);
        resize(400, 450);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        // Deferred initialization
        QTimer::singleShot(0, this, [this]() {
            try {
                setupUI();
                setupConnections();
                s_initialized = true;
                // Dialog initialized successfully
            } catch (const std::exception& e) {
                qCritical() << "[ApplicationDialog] Failed to initialize:" << e.what();
                QMessageBox::critical(this, "Initialization Error",
                                      QString("Failed to initialize dialog: %1").arg(e.what()));
                reject();
            }
        });
    } catch (const std::exception& e) {
        qCritical() << "[ApplicationDialog] Exception in constructor:" << e.what();
        throw;
    }
}

// %%% Destructor %%%
/* Clean up dialog resources */
ApplicationDialog::~ApplicationDialog()
{
    // Disconnect all signals
    disconnect();

    // Clean up validators
    if (fpsValidator) {
        delete fpsValidator;
        fpsValidator = nullptr;
    }
    if (guiFPSValidator) {
        delete guiFPSValidator;
        guiFPSValidator = nullptr;
    }
    if (simulationFPSValidator) {
        delete simulationFPSValidator;
        simulationFPSValidator = nullptr;
    }
    if (physicsFPSValidator) {
        delete physicsFPSValidator;
        physicsFPSValidator = nullptr;
    }

    // Reset initialization flag
    s_initialized = false;
}

// %%% UI Setup %%%
/* Create and configure dialog UI elements */
void ApplicationDialog::setupUI()
{
    // Check thread safety
    if (!isMainThread()) {
        // Not in main thread
        return;
    }

    try {
        // Load saved settings or use defaults
        int savedFPS = 60;
        int savedGUIFPS = 60;
        int savedSimulationFPS = 60;
        int savedPhysicsFPS = 60;
        QString savedImageSize = "100px";
        bool savedDeveloperMode = getGlobalDeveloperMode();

        if (MainWindow::scenarioconfig) {
            savedFPS = MainWindow::scenarioconfig->getSavedFPS();
            savedGUIFPS = MainWindow::scenarioconfig->getSavedGUIFPS();
            savedSimulationFPS = MainWindow::scenarioconfig->getSavedSimulationFPS();
            savedPhysicsFPS = MainWindow::scenarioconfig->getSavedPhysicsFPS();
            savedImageSize = MainWindow::scenarioconfig->getSavedImageSize();
        }

        // Create settings group box
        settingsGroup = new QGroupBox("Settings", this);
        settingsGroup->setObjectName("settingsGroup");
        settingsGroup->setStyleSheet(ApplicationDialogStyles::GroupBox);

        // Create form layout
        QFormLayout *formLayout = new QFormLayout();
        formLayout->setSpacing(10);
        formLayout->setContentsMargins(15, 15, 15, 15);

        // Developer mode setting
        QLabel *developerModeLabel = new QLabel("Developer Mode:", settingsGroup);
        developerModeLabel->setStyleSheet(ApplicationDialogStyles::FormLabel);
        developerModeLabel->setToolTip("Enable developer features and debugging tools");

        developerModeCheckBox = new QCheckBox(settingsGroup);
        developerModeCheckBox->setChecked(savedDeveloperMode);
        developerModeCheckBox->setObjectName("developerModeCheckBox");
        developerModeCheckBox->setToolTip("Toggle developer mode on/off");
        developerModeCheckBox->setStyleSheet(ApplicationDialogStyles::CheckBox);

        QHBoxLayout *devModeLayout = new QHBoxLayout();
        devModeLayout->setContentsMargins(0, 0, 0, 0);
        devModeLayout->addWidget(developerModeCheckBox);
        devModeLayout->addStretch();

        formLayout->addRow(developerModeLabel, devModeLayout);

        // Separator line
        QFrame *separatorLine = new QFrame();
        separatorLine->setFrameShape(QFrame::HLine);
        separatorLine->setStyleSheet(ApplicationDialogStyles::Separator);
        formLayout->addRow(separatorLine);

        // Main FPS setting
        QLabel *fpsLabel = new QLabel("Main FPS:", settingsGroup);
        fpsLabel->setStyleSheet(ApplicationDialogStyles::FormLabel);
        fpsLabel->setToolTip("Main frames per second (1-1000)");

        fpsEdit = new QLineEdit(settingsGroup);
        fpsEdit->setObjectName("fpsEdit");
        fpsEdit->setText(QString::number(savedFPS));
        fpsEdit->setPlaceholderText("e.g., 60");
        fpsEdit->setToolTip("Enter Main FPS value between 1 and 1000");
        fpsEdit->setStyleSheet(ApplicationDialogStyles::LineEdit);

        fpsValidator = new QIntValidator(1, 1000, this);
        fpsEdit->setValidator(fpsValidator);

        fpsErrorLabel = new QLabel("", settingsGroup);
        fpsErrorLabel->setObjectName("fpsErrorLabel");
        fpsErrorLabel->setVisible(false);
        fpsErrorLabel->setStyleSheet(ApplicationDialogStyles::ErrorLabel);

        // GUI FPS setting
        QLabel *guiFPSLabel = new QLabel("GUI FPS:", settingsGroup);
        guiFPSLabel->setStyleSheet(ApplicationDialogStyles::FormLabel);
        guiFPSLabel->setToolTip("GUI refresh rate (1-1000)");

        guiFPSEdit = new QLineEdit(settingsGroup);
        guiFPSEdit->setObjectName("guiFPSEdit");
        guiFPSEdit->setText(QString::number(savedGUIFPS));
        guiFPSEdit->setPlaceholderText("e.g., 60");
        guiFPSEdit->setToolTip("Enter GUI FPS value between 1 and 1000");
        guiFPSEdit->setStyleSheet(ApplicationDialogStyles::LineEdit);

        guiFPSValidator = new QIntValidator(1, 1000, this);
        guiFPSEdit->setValidator(guiFPSValidator);

        guiFPSErrorLabel = new QLabel("", settingsGroup);
        guiFPSErrorLabel->setObjectName("guiFPSErrorLabel");
        guiFPSErrorLabel->setVisible(false);
        guiFPSErrorLabel->setStyleSheet(ApplicationDialogStyles::ErrorLabel);

        // Simulation FPS setting
        QLabel *simulationFPSLabel = new QLabel("Simulation FPS:", settingsGroup);
        simulationFPSLabel->setStyleSheet(ApplicationDialogStyles::FormLabel);
        simulationFPSLabel->setToolTip("Simulation update rate (1-1000)");

        simulationFPSEdit = new QLineEdit(settingsGroup);
        simulationFPSEdit->setObjectName("simulationFPSEdit");
        simulationFPSEdit->setText(QString::number(savedSimulationFPS));
        simulationFPSEdit->setPlaceholderText("e.g., 60");
        simulationFPSEdit->setToolTip("Enter Simulation FPS value between 1 and 1000");
        simulationFPSEdit->setStyleSheet(ApplicationDialogStyles::LineEdit);

        simulationFPSValidator = new QIntValidator(1, 1000, this);
        simulationFPSEdit->setValidator(simulationFPSValidator);

        simulationFPSErrorLabel = new QLabel("", settingsGroup);
        simulationFPSErrorLabel->setObjectName("simulationFPSErrorLabel");
        simulationFPSErrorLabel->setVisible(false);
        simulationFPSErrorLabel->setStyleSheet(ApplicationDialogStyles::ErrorLabel);

        // Physics FPS setting
        QLabel *physicsFPSLabel = new QLabel("Physics FPS:", settingsGroup);
        physicsFPSLabel->setStyleSheet(ApplicationDialogStyles::FormLabel);
        physicsFPSLabel->setToolTip("Physics engine update rate (1-1000)");

        physicsFPSEdit = new QLineEdit(settingsGroup);
        physicsFPSEdit->setObjectName("physicsFPSEdit");
        physicsFPSEdit->setText(QString::number(savedPhysicsFPS));
        physicsFPSEdit->setPlaceholderText("e.g., 60");
        physicsFPSEdit->setToolTip("Enter Physics FPS value between 1 and 1000");
        physicsFPSEdit->setStyleSheet(ApplicationDialogStyles::LineEdit);

        physicsFPSValidator = new QIntValidator(1, 1000, this);
        physicsFPSEdit->setValidator(physicsFPSValidator);

        physicsFPSErrorLabel = new QLabel("", settingsGroup);
        physicsFPSErrorLabel->setObjectName("physicsFPSErrorLabel");
        physicsFPSErrorLabel->setVisible(false);
        physicsFPSErrorLabel->setStyleSheet(ApplicationDialogStyles::ErrorLabel);

        // Image Size setting
        QLabel *imageSizeLabel = new QLabel("Image Size:", settingsGroup);
        imageSizeLabel->setStyleSheet(ApplicationDialogStyles::FormLabel);
        imageSizeLabel->setToolTip("Size in pixels (e.g., 100px, 250px)");

        imageSizeEdit = new QLineEdit(settingsGroup);
        imageSizeEdit->setObjectName("imageSizeEdit");
        imageSizeEdit->setText(savedImageSize);
        imageSizeEdit->setPlaceholderText("e.g., 100px");
        imageSizeEdit->setToolTip("Enter size in pixels (1px to 10000px)");
        imageSizeEdit->setStyleSheet(ApplicationDialogStyles::LineEdit);

        QRegularExpression pxRegEx("^\\d+\\s*px$", QRegularExpression::CaseInsensitiveOption);
        QValidator *pxValidator = new QRegularExpressionValidator(pxRegEx, this);
        imageSizeEdit->setValidator(pxValidator);

        imageSizeErrorLabel = new QLabel("", settingsGroup);
        imageSizeErrorLabel->setObjectName("imageSizeErrorLabel");
        imageSizeErrorLabel->setVisible(false);
        imageSizeErrorLabel->setStyleSheet(ApplicationDialogStyles::ErrorLabel);

        // Add rows to form layout
        formLayout->addRow(fpsLabel, fpsEdit);
        formLayout->addRow(fpsErrorLabel);
        formLayout->addRow(guiFPSLabel, guiFPSEdit);
        formLayout->addRow(guiFPSErrorLabel);
        formLayout->addRow(simulationFPSLabel, simulationFPSEdit);
        formLayout->addRow(simulationFPSErrorLabel);
        formLayout->addRow(physicsFPSLabel, physicsFPSEdit);
        formLayout->addRow(physicsFPSErrorLabel);
        formLayout->addRow(imageSizeLabel, imageSizeEdit);
        formLayout->addRow(imageSizeErrorLabel);

        settingsGroup->setLayout(formLayout);

        // Create action buttons
        okButton = new QPushButton("OK", this);
        okButton->setObjectName("okButton");
        okButton->setDefault(true);
        okButton->setEnabled(false);
        okButton->setStyleSheet(ApplicationDialogStyles::OkButton);

        cancelButton = new QPushButton("Cancel", this);
        cancelButton->setObjectName("cancelButton");
        cancelButton->setStyleSheet(ApplicationDialogStyles::CancelButton);

        // Create button layout
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(10);
        buttonLayout->setContentsMargins(0, 10, 0, 0);
        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);

        // Create main layout
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(15);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->addWidget(settingsGroup);
        mainLayout->addLayout(buttonLayout);

    } catch (const std::exception& e) {
        qCritical() << "[ApplicationDialog] Exception in setupUI:" << e.what();
        throw;
    }
}

// %%% Connection Setup %%%
/* Set up signal-slot connections */
void ApplicationDialog::setupConnections()
{
    // Validate button initialization
    if (!okButton || !cancelButton) {
        qCritical() << "[ApplicationDialog] Buttons not initialized!";
        return;
    }

    // Button click connections
    connect(okButton, &QPushButton::clicked, this, &ApplicationDialog::onOkClicked);
    connect(cancelButton, &QPushButton::clicked, this, &ApplicationDialog::onCancelClicked);

    // Developer mode change connection
    if (developerModeCheckBox) {
        connect(developerModeCheckBox, &QCheckBox::stateChanged,
                this, &ApplicationDialog::validateInputs);
    }

    // Input validation connections
    if (fpsEdit) {
        connect(fpsEdit, &QLineEdit::textChanged, this, &ApplicationDialog::validateInputs);
        connect(fpsEdit, &QLineEdit::returnPressed, this, &ApplicationDialog::onOkClicked);
    }
    if (guiFPSEdit) {
        connect(guiFPSEdit, &QLineEdit::textChanged, this, &ApplicationDialog::validateInputs);
        connect(guiFPSEdit, &QLineEdit::returnPressed, this, &ApplicationDialog::onOkClicked);
    }
    if (simulationFPSEdit) {
        connect(simulationFPSEdit, &QLineEdit::textChanged, this, &ApplicationDialog::validateInputs);
        connect(simulationFPSEdit, &QLineEdit::returnPressed, this, &ApplicationDialog::onOkClicked);
    }
    if (physicsFPSEdit) {
        connect(physicsFPSEdit, &QLineEdit::textChanged, this, &ApplicationDialog::validateInputs);
        connect(physicsFPSEdit, &QLineEdit::returnPressed, this, &ApplicationDialog::onOkClicked);
    }
    if (imageSizeEdit) {
        connect(imageSizeEdit, &QLineEdit::textChanged, this, &ApplicationDialog::validateInputs);
        connect(imageSizeEdit, &QLineEdit::returnPressed, this, &ApplicationDialog::onOkClicked);
    }
}

// %%% Validation Methods %%%
/* Validate FPS input field */
bool ApplicationDialog::validateFPSInput(QLineEdit* edit, QLabel* errorLabel, const QString& fieldName)
{
    if (!edit || !errorLabel) {
        qWarning() << "[ApplicationDialog] Invalid widgets for validation";
        return false;
    }

    QString text = edit->text().trimmed();
    if (text.isEmpty()) {
        errorLabel->setText(QString("Please enter %1 value").arg(fieldName));
        errorLabel->setVisible(true);
        edit->setStyleSheet(ApplicationDialogStyles::LineEditError);
        return false;
    }

    bool ok;
    int value = text.toInt(&ok);
    if (!ok || value < 1 || value > 1000) {
        errorLabel->setText(QString("%1 must be between 1 and 1000").arg(fieldName));
        errorLabel->setVisible(true);
        edit->setStyleSheet(ApplicationDialogStyles::LineEditError);
        return false;
    }

    errorLabel->setVisible(false);
    edit->setStyleSheet(ApplicationDialogStyles::LineEdit);
    return true;
}

/* Validate image size input field */
bool ApplicationDialog::validateImageSizeInput()
{
    if (!imageSizeEdit || !imageSizeErrorLabel) {
        qWarning() << "[ApplicationDialog] Image size widgets not initialized";
        return false;
    }

    QString text = imageSizeEdit->text().trimmed();
    if (text.isEmpty()) {
        imageSizeErrorLabel->setText("Please enter image size");
        imageSizeErrorLabel->setVisible(true);
        imageSizeEdit->setStyleSheet(ApplicationDialogStyles::LineEditError);
        return false;
    }

    QRegularExpression re("^(\\d+)\\s*px$", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(text);

    if (!match.hasMatch()) {
        imageSizeErrorLabel->setText("Format: number followed by 'px' (e.g., 100px)");
        imageSizeErrorLabel->setVisible(true);
        imageSizeEdit->setStyleSheet(ApplicationDialogStyles::LineEditError);
        return false;
    }

    int pixelValue = match.captured(1).toInt();
    if (pixelValue < 1 || pixelValue > 10000) {
        imageSizeErrorLabel->setText("Size must be between 1px and 10000px");
        imageSizeErrorLabel->setVisible(true);
        imageSizeEdit->setStyleSheet(ApplicationDialogStyles::LineEditError);
        return false;
    }

    imageSizeErrorLabel->setVisible(false);
    imageSizeEdit->setStyleSheet(ApplicationDialogStyles::LineEdit);
    return true;
}

/* Validate all input fields */
void ApplicationDialog::validateInputs()
{
    // Check initialization state
    if (!s_initialized) {
        qWarning() << "[ApplicationDialog] Not initialized yet";
        return;
    }

    bool isValid = true;

    // Validate each input field
    if (!validateFPSInput(fpsEdit, fpsErrorLabel, "Main FPS")) {
        isValid = false;
    }
    if (!validateFPSInput(guiFPSEdit, guiFPSErrorLabel, "GUI FPS")) {
        isValid = false;
    }
    if (!validateFPSInput(simulationFPSEdit, simulationFPSErrorLabel, "Simulation FPS")) {
        isValid = false;
    }
    if (!validateFPSInput(physicsFPSEdit, physicsFPSErrorLabel, "Physics FPS")) {
        isValid = false;
    }
    if (!validateImageSizeInput()) {
        isValid = false;
    }

    // Update OK button state
    if (okButton) {
        okButton->setEnabled(isValid);
    }
}

// %%% Event Handlers %%%
/* Handle OK button click */
void ApplicationDialog::onOkClicked()
{
    // Check thread safety
    if (!isMainThread()) {
        qCritical() << "[ApplicationDialog] onOkClicked called from non-main thread!";
        return;
    }

    // Validate inputs before saving
    validateInputs();

    if (!okButton || !okButton->isEnabled()) {
        qWarning() << "[ApplicationDialog] OK button disabled, validation failed";
        QMessageBox::warning(this, "Validation Error",
                             "Please fix all errors before saving.");
        return;
    }

    try {
        // Get current values
        int fps = getFPS();
        int guifps = getGUIFPS();
        int simfps = getSimulationFPS();
        int physicsfps = getPhysicsFPS();
        QString imageSize = getImageSize();
        bool newDevMode = getDeveloperMode();

        // Save settings globally
        setGlobalFPS(fps);
        setGlobalGUIFPS(guifps);
        setGlobalSimulationFPS(simfps);
        setGlobalPhysicsFPS(physicsfps);
        setGlobalImageSize(imageSize);
        setGlobalDeveloperMode(newDevMode);

        // Emit change signals
        emit fpsState(fps);
        emit guiFPSState(guifps);
        emit simulationFPSState(simfps);
        emit physicsFPSState(physicsfps);

        // Parse and emit image size
        QRegularExpression re("(\\d+)(?:\\s*px)?", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = re.match(imageSize);
        if (match.hasMatch()) {
            bool ok;
            int pixelInt = match.captured(1).toInt(&ok);
            if (ok) {
                emit canvasIconState(pixelInt);
            }
        }

        emit developerModeState(newDevMode);

        // Close dialog with accept
        accept();

    } catch (const std::exception& e) {
        qCritical() << "[ApplicationDialog] Exception in onOkClicked:" << e.what();
        QMessageBox::critical(this, "Save Error",
                              QString("Failed to save settings: %1").arg(e.what()));
    }
}

/* Handle Cancel button click */
void ApplicationDialog::onCancelClicked()
{
    // Close dialog with reject
    reject();
}

// %%% Getter Methods %%%
/* Get Main FPS value from UI */
int ApplicationDialog::getFPS() const
{
    if (!fpsEdit) return 60;
    return fpsEdit->text().toInt();
}

/* Get GUI FPS value from UI */
int ApplicationDialog::getGUIFPS() const
{
    if (!guiFPSEdit) return 60;
    return guiFPSEdit->text().toInt();
}

/* Get Simulation FPS value from UI */
int ApplicationDialog::getSimulationFPS() const
{
    if (!simulationFPSEdit) return 60;
    return simulationFPSEdit->text().toInt();
}

/* Get Physics FPS value from UI */
int ApplicationDialog::getPhysicsFPS() const
{
    if (!physicsFPSEdit) return 60;
    return physicsFPSEdit->text().toInt();
}

/* Get Image Size value from UI */
QString ApplicationDialog::getImageSize() const
{
    if (!imageSizeEdit) return "100px";
    return imageSizeEdit->text();
}

/* Get Developer Mode state from UI */
bool ApplicationDialog::getDeveloperMode() const
{
    if (!developerModeCheckBox) return false;
    return developerModeCheckBox->isChecked();
}
