
#include "applicationdialog.h"
#include <QMessageBox>
#include <GUI/mainwindow.h>
#include <core/Config/scenarioconfig.h>
#include <QDebug>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QFont>
#include <QPalette>
#include <QStyle>
#include <QApplication>
#include <QCheckBox>
#include <QTimer>
#include <QThread>

// Static member initialization
bool ApplicationDialog::s_developerMode = false;
bool ApplicationDialog::s_initialized = false;

// Helper function for config availability
namespace {
bool isConfigAvailable() {
    if (!MainWindow::scenarioconfig) {

        return false;
    }
    return true;
}

bool isMainThread() {
    return QThread::currentThread() == QCoreApplication::instance()->thread();
}
}

// Static Global Settings Functions
int ApplicationDialog::getGlobalFPS()
{
    if (!isConfigAvailable()) {

        return 60;
    }
    return MainWindow::scenarioconfig->getSavedFPS();
}

int ApplicationDialog::getGlobalGUIFPS()
{
    if (!isConfigAvailable()) {

        return 60;
    }
    return MainWindow::scenarioconfig->getSavedGUIFPS();
}

int ApplicationDialog::getGlobalSimulationFPS()
{
    if (!isConfigAvailable()) {

        return 60;
    }
    return MainWindow::scenarioconfig->getSavedSimulationFPS();
}

int ApplicationDialog::getGlobalPhysicsFPS()
{
    if (!isConfigAvailable()) {

        return 60;
    }
    return MainWindow::scenarioconfig->getSavedPhysicsFPS();
}

QString ApplicationDialog::getGlobalImageSize()
{
    if (!isConfigAvailable()) {

        return "100px";
    }
    return MainWindow::scenarioconfig->getSavedImageSize();
}

QString ApplicationDialog::getImageSizeInPixels()
{
    QString size = getGlobalImageSize();
    QRegularExpression re("(\\d+)(?:\\s*px)?", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(size);
    if (match.hasMatch()) {
        return match.captured(1);
    }

    return "100";
}

void ApplicationDialog::setGlobalFPS(int fps)
{
    if (!isMainThread()) {

        return;
    }

    if (!isConfigAvailable()) {

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

        } catch (const std::exception& e) {

        }
    } else {

    }
}

void ApplicationDialog::setGlobalGUIFPS(int guifps)
{
    if (!isMainThread()) {

        return;
    }

    if (!isConfigAvailable()) {

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

        } catch (const std::exception& e) {

        }
    } else {

    }
}

void ApplicationDialog::setGlobalSimulationFPS(int simfps)
{
    if (!isMainThread()) {

        return;
    }

    if (!isConfigAvailable()) {

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

        } catch (const std::exception& e) {

        }
    } else {

    }
}

void ApplicationDialog::setGlobalPhysicsFPS(int physicsfps)
{
    if (!isMainThread()) {

        return;
    }

    if (!isConfigAvailable()) {

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

        } catch (const std::exception& e) {

        }
    } else {

    }
}

void ApplicationDialog::setGlobalImageSize(const QString& size)
{
    if (!isMainThread()) {

        return;
    }

    if (!isConfigAvailable()) {

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

                } else {

                }
            } else {

            }
        } catch (const std::exception& e) {

        }
    }
}

void ApplicationDialog::resetGlobalSettings()
{
    if (!isMainThread()) {

        return;
    }

    if (!isConfigAvailable()) {

        return;
    }

    try {
        int g_fps = 60;
        int g_guifps = 60;
        int g_simfps = 60;
        int g_physicsfps = 60;
        QString g_imageSize = "100px";

        MainWindow::scenarioconfig->saveAppSettings(g_fps, g_guifps, g_simfps, g_physicsfps, g_imageSize);

    } catch (const std::exception& e) {

    }
}

bool ApplicationDialog::getGlobalDeveloperMode()
{
    return s_developerMode;
}

void ApplicationDialog::setGlobalDeveloperMode(bool enabled)
{
    if (s_developerMode != enabled) {
        s_developerMode = enabled;

    }
}

// Constructor
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

    if (!isMainThread()) {

        return;
    }

    try {
        if (!MainWindow::scenarioconfig) {

        }

        setWindowTitle("Application Settings");
        setModal(true);
        resize(400, 450);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        QTimer::singleShot(0, this, [this]() {
            try {
                setupUI();
                setupConnections();
                s_initialized = true;

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

// Destructor
ApplicationDialog::~ApplicationDialog()
{

    disconnect();
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

    s_initialized = false;
}

void ApplicationDialog::setupUI()
{

    if (!isMainThread()) {

        return;
    }
    try {

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
        settingsGroup = new QGroupBox("Settings", this);
        settingsGroup->setObjectName("settingsGroup");
        QFormLayout *formLayout = new QFormLayout();
        formLayout->setSpacing(10);
        formLayout->setContentsMargins(15, 15, 15, 15);
        QLabel *developerModeLabel = new QLabel("Developer Mode:", settingsGroup);
        developerModeLabel->setToolTip("Enable developer features and debugging tools");
        developerModeCheckBox = new QCheckBox(settingsGroup);
        developerModeCheckBox->setChecked(savedDeveloperMode);
        developerModeCheckBox->setObjectName("developerModeCheckBox");
        developerModeCheckBox->setToolTip("Toggle developer mode on/off");
        QHBoxLayout *devModeLayout = new QHBoxLayout();
        devModeLayout->setContentsMargins(0, 0, 0, 0);
        devModeLayout->addWidget(developerModeCheckBox);
        devModeLayout->addStretch();
        formLayout->addRow(developerModeLabel, devModeLayout);
        QFrame *separatorLine = new QFrame();
        separatorLine->setFrameShape(QFrame::HLine);
        separatorLine->setFrameShadow(QFrame::Sunken);
        separatorLine->setStyleSheet("background-color: #dee2e6;");
        formLayout->addRow(separatorLine);
        QLabel *fpsLabel = new QLabel("Main FPS:", settingsGroup);
        fpsLabel->setToolTip("Main frames per second (1-1000)");
        fpsEdit = new QLineEdit(settingsGroup);
        fpsEdit->setObjectName("fpsEdit");
        fpsEdit->setText(QString::number(savedFPS));
        fpsEdit->setPlaceholderText("e.g., 60");
        fpsEdit->setToolTip("Enter Main FPS value between 1 and 1000");
        fpsValidator = new QIntValidator(1, 1000, this);
        fpsEdit->setValidator(fpsValidator);
        fpsErrorLabel = new QLabel("", settingsGroup);
        fpsErrorLabel->setObjectName("fpsErrorLabel");
        fpsErrorLabel->setVisible(false);
        fpsErrorLabel->setStyleSheet("color: #dc3545; font-size: 11px;");
        QLabel *guiFPSLabel = new QLabel("GUI FPS:", settingsGroup);
        guiFPSLabel->setToolTip("GUI refresh rate (1-1000)");
        guiFPSEdit = new QLineEdit(settingsGroup);
        guiFPSEdit->setObjectName("guiFPSEdit");
        guiFPSEdit->setText(QString::number(savedGUIFPS));
        guiFPSEdit->setPlaceholderText("e.g., 60");
        guiFPSEdit->setToolTip("Enter GUI FPS value between 1 and 1000");
        guiFPSValidator = new QIntValidator(1, 1000, this);
        guiFPSEdit->setValidator(guiFPSValidator);
        guiFPSErrorLabel = new QLabel("", settingsGroup);
        guiFPSErrorLabel->setObjectName("guiFPSErrorLabel");
        guiFPSErrorLabel->setVisible(false);
        guiFPSErrorLabel->setStyleSheet("color: #dc3545; font-size: 11px;");
        QLabel *simulationFPSLabel = new QLabel("Simulation FPS:", settingsGroup);
        simulationFPSLabel->setToolTip("Simulation update rate (1-1000)");
        simulationFPSEdit = new QLineEdit(settingsGroup);
        simulationFPSEdit->setObjectName("simulationFPSEdit");
        simulationFPSEdit->setText(QString::number(savedSimulationFPS));
        simulationFPSEdit->setPlaceholderText("e.g., 60");
        simulationFPSEdit->setToolTip("Enter Simulation FPS value between 1 and 1000");
        simulationFPSValidator = new QIntValidator(1, 1000, this);
        simulationFPSEdit->setValidator(simulationFPSValidator);
        simulationFPSErrorLabel = new QLabel("", settingsGroup);
        simulationFPSErrorLabel->setObjectName("simulationFPSErrorLabel");
        simulationFPSErrorLabel->setVisible(false);
        simulationFPSErrorLabel->setStyleSheet("color: #dc3545; font-size: 11px;");

        // Physics FPS
        QLabel *physicsFPSLabel = new QLabel("Physics FPS:", settingsGroup);
        physicsFPSLabel->setToolTip("Physics engine update rate (1-1000)");

        physicsFPSEdit = new QLineEdit(settingsGroup);
        physicsFPSEdit->setObjectName("physicsFPSEdit");
        physicsFPSEdit->setText(QString::number(savedPhysicsFPS));
        physicsFPSEdit->setPlaceholderText("e.g., 60");
        physicsFPSEdit->setToolTip("Enter Physics FPS value between 1 and 1000");

        physicsFPSValidator = new QIntValidator(1, 1000, this);
        physicsFPSEdit->setValidator(physicsFPSValidator);

        physicsFPSErrorLabel = new QLabel("", settingsGroup);
        physicsFPSErrorLabel->setObjectName("physicsFPSErrorLabel");
        physicsFPSErrorLabel->setVisible(false);
        physicsFPSErrorLabel->setStyleSheet("color: #dc3545; font-size: 11px;");

        // Image Size
        QLabel *imageSizeLabel = new QLabel("Image Size:", settingsGroup);
        imageSizeLabel->setToolTip("Size in pixels (e.g., 100px, 250px)");

        imageSizeEdit = new QLineEdit(settingsGroup);
        imageSizeEdit->setObjectName("imageSizeEdit");
        imageSizeEdit->setText(savedImageSize);
        imageSizeEdit->setPlaceholderText("e.g., 100px");
        imageSizeEdit->setToolTip("Enter size in pixels (1px to 10000px)");

        QRegularExpression pxRegEx("^\\d+\\s*px$", QRegularExpression::CaseInsensitiveOption);
        QValidator *pxValidator = new QRegularExpressionValidator(pxRegEx, this);
        imageSizeEdit->setValidator(pxValidator);

        imageSizeErrorLabel = new QLabel("", settingsGroup);
        imageSizeErrorLabel->setObjectName("imageSizeErrorLabel");
        imageSizeErrorLabel->setVisible(false);
        imageSizeErrorLabel->setStyleSheet("color: #dc3545; font-size: 11px;");

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

        // Buttons
        okButton = new QPushButton("OK", this);
        okButton->setObjectName("okButton");
        okButton->setDefault(true);
        okButton->setEnabled(false);

        cancelButton = new QPushButton("Cancel", this);
        cancelButton->setObjectName("cancelButton");

        // Button layout
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(10);
        buttonLayout->setContentsMargins(0, 10, 0, 0);
        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);

        // Main layout
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(15);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->addWidget(settingsGroup);
        mainLayout->addLayout(buttonLayout);
        setStyleSheet(R"(
            QDialog {
                background-color: #f8f9fa;
                font-family: 'Segoe UI', Arial, sans-serif;
            }
            QGroupBox {
                font-weight: bold;
                border: 1px solid #dee2e6;
                border-radius: 6px;
                margin-top: 12px;
                padding-top: 12px;
                background-color: white;
            }
            QLineEdit {
                border: 1px solid #ced4da;
                border-radius: 4px;
                padding: 6px;
                min-width: 120px;
            }
            QLineEdit:focus {
                border: 1px solid #80bdff;
            }
            QPushButton {
                border: none;
                border-radius: 4px;
                padding: 8px 16px;
                min-width: 80px;
            }
            QPushButton#okButton {
                background-color: #28a745;
                color: white;
            }
            QPushButton#okButton:hover {
                background-color: #218838;
            }
            QPushButton#okButton:disabled {
                background-color: #6c757d;
            }
            QPushButton#cancelButton {
                background-color: #6c757d;
                color: white;
            }
            QPushButton#cancelButton:hover {
                background-color: #545b62;
            }
        )");
    } catch (const std::exception& e) {
        qCritical() << "[ApplicationDialog] Exception in setupUI:" << e.what();
        throw;
    }
}

void ApplicationDialog::setupConnections()
{

    if (!okButton || !cancelButton) {
        qCritical() << "[ApplicationDialog] Buttons not initialized!";
        return;
    }

    // Button connections
    connect(okButton, &QPushButton::clicked, this, &ApplicationDialog::onOkClicked);
    connect(cancelButton, &QPushButton::clicked, this, &ApplicationDialog::onCancelClicked);
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
        edit->setStyleSheet("border: 1px solid #dc3545;");
        return false;
    }

    bool ok;
    int value = text.toInt(&ok);
    if (!ok || value < 1 || value > 1000) {
        errorLabel->setText(QString("%1 must be between 1 and 1000").arg(fieldName));
        errorLabel->setVisible(true);
        edit->setStyleSheet("border: 1px solid #dc3545;");
        return false;
    }

    errorLabel->setVisible(false);
    edit->setStyleSheet("border: 1px solid #ced4da;");
    return true;
}

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
        imageSizeEdit->setStyleSheet("border: 1px solid #dc3545;");
        return false;
    }

    QRegularExpression re("^(\\d+)\\s*px$", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(text);

    if (!match.hasMatch()) {
        imageSizeErrorLabel->setText("Format: number followed by 'px' (e.g., 100px)");
        imageSizeErrorLabel->setVisible(true);
        imageSizeEdit->setStyleSheet("border: 1px solid #dc3545;");
        return false;
    }

    int pixelValue = match.captured(1).toInt();
    if (pixelValue < 1 || pixelValue > 10000) {
        imageSizeErrorLabel->setText("Size must be between 1px and 10000px");
        imageSizeErrorLabel->setVisible(true);
        imageSizeEdit->setStyleSheet("border: 1px solid #dc3545;");
        return false;
    }

    imageSizeErrorLabel->setVisible(false);
    imageSizeEdit->setStyleSheet("border: 1px solid #ced4da;");
    return true;
}

void ApplicationDialog::validateInputs()
{

    if (!s_initialized) {
        qWarning() << "[ApplicationDialog] Not initialized yet";
        return;
    }

    bool isValid = true;
    // Validate each input
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

    if (okButton) {
        okButton->setEnabled(isValid);
    }
}

void ApplicationDialog::onOkClicked()
{

    if (!isMainThread()) {
        qCritical() << "[ApplicationDialog] onOkClicked called from non-main thread!";
        return;
    }
    validateInputs();

    if (!okButton || !okButton->isEnabled()) {
        qWarning() << "[ApplicationDialog] OK button disabled, validation failed";
        QMessageBox::warning(this, "Validation Error",
                             "Please fix all errors before saving.");
        return;
    }
    try {

        int fps = getFPS();
        int guifps = getGUIFPS();
        int simfps = getSimulationFPS();
        int physicsfps = getPhysicsFPS();
        QString imageSize = getImageSize();
        bool newDevMode = getDeveloperMode();
        // Save settings
        setGlobalFPS(fps);
        setGlobalGUIFPS(guifps);
        setGlobalSimulationFPS(simfps);
        setGlobalPhysicsFPS(physicsfps);
        setGlobalImageSize(imageSize);
        setGlobalDeveloperMode(newDevMode);
        // Emit signals
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
        accept();

    } catch (const std::exception& e) {
        qCritical() << "[ApplicationDialog] Exception in onOkClicked:" << e.what();
        QMessageBox::critical(this, "Save Error",
                              QString("Failed to save settings: %1").arg(e.what()));
    }
}

void ApplicationDialog::onCancelClicked()
{

    reject();
}

int ApplicationDialog::getFPS() const
{
    if (!fpsEdit) return 60;
    return fpsEdit->text().toInt();
}

int ApplicationDialog::getGUIFPS() const
{
    if (!guiFPSEdit) return 60;
    return guiFPSEdit->text().toInt();
}

int ApplicationDialog::getSimulationFPS() const
{
    if (!simulationFPSEdit) return 60;
    return simulationFPSEdit->text().toInt();
}

int ApplicationDialog::getPhysicsFPS() const
{
    if (!physicsFPSEdit) return 60;
    return physicsFPSEdit->text().toInt();
}

QString ApplicationDialog::getImageSize() const
{
    if (!imageSizeEdit) return "100px";
    return imageSizeEdit->text();
}

bool ApplicationDialog::getDeveloperMode() const
{
    if (!developerModeCheckBox) return false;
    return developerModeCheckBox->isChecked();
}
