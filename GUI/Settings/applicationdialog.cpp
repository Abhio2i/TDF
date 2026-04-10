/* ========================================================================= */
/* File: applicationdialog.cpp                                               */
/* Purpose: Implements application settings dialog with 3 tabs               */
//               Written by Arti Rajpoot
/* ========================================================================= */

#include "applicationdialog.h"
#include "applicationdialog-styles.h"
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
#include <QFileDialog>
#include <QDir>
#include <Setup.h>
#include "tests/applicationdialogtest/applicationdialog_test.h"
#include <QTimer>

// %%% Static Member Initialization %%%
bool    ApplicationDialog::s_developerMode  = false;
bool    ApplicationDialog::s_initialized    = false;
bool    ApplicationDialog::s_databaseEnabled = false;
QString ApplicationDialog::s_databasePath   = QString();

// ─────────────────────────────────────────────────────────────────────────────
// Helper lambdas
// ─────────────────────────────────────────────────────────────────────────────
namespace {
bool isConfigAvailable() {
    return MainWindow::scenarioconfig != nullptr;
}
bool isMainThread() {
    return QThread::currentThread() == QCoreApplication::instance()->thread();
}
}

// ─────────────────────────────────────────────────────────────────────────────
// Static accessors — existing ones unchanged
// ─────────────────────────────────────────────────────────────────────────────
int ApplicationDialog::getGlobalFPS()
{
    if (!isConfigAvailable()) return 60;
    return MainWindow::scenarioconfig->getSavedFPS();
}
int ApplicationDialog::getGlobalGUIFPS()
{
    if (!isConfigAvailable()) return 60;
    return MainWindow::scenarioconfig->getSavedGUIFPS();
}
int ApplicationDialog::getGlobalSimulationFPS()
{
    if (!isConfigAvailable()) return 60;
    return MainWindow::scenarioconfig->getSavedSimulationFPS();
}
int ApplicationDialog::getGlobalPhysicsFPS()
{
    if (!isConfigAvailable()) return 60;
    return MainWindow::scenarioconfig->getSavedPhysicsFPS();
}
QString ApplicationDialog::getGlobalImageSize()
{
    if (!isConfigAvailable()) return "100px";
    return MainWindow::scenarioconfig->getSavedImageSize();
}
QString ApplicationDialog::getImageSizeInPixels()
{
    QString size = getGlobalImageSize();
    QRegularExpression re("(\\d+)(?:\\s*px)?", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(size);
    if (match.hasMatch()) return match.captured(1);
    return "100";
}

// ─────────────────────────────────────────────────────────────────────────────
// NEW — Database static accessors
// ─────────────────────────────────────────────────────────────────────────────
bool ApplicationDialog::getGlobalDatabaseEnabled()
{
    return s_databaseEnabled;
}
QString ApplicationDialog::getGlobalDatabasePath()
{
    return s_databasePath;
}
void ApplicationDialog::setGlobalDatabaseEnabled(bool enabled)
{
    s_databaseEnabled = enabled;
}
void ApplicationDialog::setGlobalDatabasePath(const QString& path)
{
    s_databasePath = path;
}

// ─────────────────────────────────────────────────────────────────────────────
// Static mutators — unchanged
// ─────────────────────────────────────────────────────────────────────────────
void ApplicationDialog::setGlobalFPS(int fps)
{
    if (!isMainThread() || !isConfigAvailable()) return;
    if (fps >= 1 && fps <= 1000) {
        MainWindow::scenarioconfig->saveAppSettings(
            fps,
            MainWindow::scenarioconfig->getSavedGUIFPS(),
            MainWindow::scenarioconfig->getSavedSimulationFPS(),
            MainWindow::scenarioconfig->getSavedPhysicsFPS(),
            MainWindow::scenarioconfig->getSavedImageSize());
    }
}
void ApplicationDialog::setGlobalGUIFPS(int guifps)
{
    if (!isMainThread() || !isConfigAvailable()) return;
    if (guifps >= 1 && guifps <= 1000) {
        MainWindow::scenarioconfig->saveAppSettings(
            MainWindow::scenarioconfig->getSavedFPS(), guifps,
            MainWindow::scenarioconfig->getSavedSimulationFPS(),
            MainWindow::scenarioconfig->getSavedPhysicsFPS(),
            MainWindow::scenarioconfig->getSavedImageSize());
    }
}
void ApplicationDialog::setGlobalSimulationFPS(int simfps)
{
    if (!isMainThread() || !isConfigAvailable()) return;
    if (simfps >= 1 && simfps <= 1000) {
        MainWindow::scenarioconfig->saveAppSettings(
            MainWindow::scenarioconfig->getSavedFPS(),
            MainWindow::scenarioconfig->getSavedGUIFPS(), simfps,
            MainWindow::scenarioconfig->getSavedPhysicsFPS(),
            MainWindow::scenarioconfig->getSavedImageSize());
    }
}
void ApplicationDialog::setGlobalPhysicsFPS(int physicsfps)
{
    if (!isMainThread() || !isConfigAvailable()) return;
    if (physicsfps >= 1 && physicsfps <= 1000) {
        MainWindow::scenarioconfig->saveAppSettings(
            MainWindow::scenarioconfig->getSavedFPS(),
            MainWindow::scenarioconfig->getSavedGUIFPS(),
            MainWindow::scenarioconfig->getSavedSimulationFPS(), physicsfps,
            MainWindow::scenarioconfig->getSavedImageSize());
    }
}
void ApplicationDialog::setGlobalImageSize(const QString& size)
{
    if (!isMainThread() || !isConfigAvailable() || size.isEmpty()) return;
    QString input = size.trimmed();
    QRegularExpression re("(\\d+)(?:\\s*px)?", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(input);
    if (match.hasMatch()) {
        bool ok;
        int pixelInt = match.captured(1).toInt(&ok);
        if (ok && pixelInt >= 1 && pixelInt <= 10000) {
            MainWindow::scenarioconfig->saveAppSettings(
                MainWindow::scenarioconfig->getSavedFPS(),
                MainWindow::scenarioconfig->getSavedGUIFPS(),
                MainWindow::scenarioconfig->getSavedSimulationFPS(),
                MainWindow::scenarioconfig->getSavedPhysicsFPS(),
                match.captured(1) + "px");
        }
    }
}
void ApplicationDialog::resetGlobalSettings()
{
    if (!isMainThread() || !isConfigAvailable()) return;
    MainWindow::scenarioconfig->saveAppSettings(60, 60, 60, 60, "100px");
}
bool ApplicationDialog::getGlobalDeveloperMode()  { return s_developerMode; }
void ApplicationDialog::setGlobalDeveloperMode(bool enabled)
{
    if (s_developerMode != enabled) s_developerMode = enabled;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────
ApplicationDialog::ApplicationDialog(QWidget *parent)
    : QDialog(parent)
    , settingsGroup(nullptr), developerModeCheckBox(nullptr)
    , fpsEdit(nullptr), guiFPSEdit(nullptr)
    , simulationFPSEdit(nullptr), physicsFPSEdit(nullptr), imageSizeEdit(nullptr)
    , okButton(nullptr), cancelButton(nullptr)
    , fpsErrorLabel(nullptr), guiFPSErrorLabel(nullptr)
    , simulationFPSErrorLabel(nullptr), physicsFPSErrorLabel(nullptr)
    , imageSizeErrorLabel(nullptr)
    , databaseEnabledCheckBox(nullptr), databasePathEdit(nullptr)
    , browseDatabaseButton(nullptr), resetDatabaseButton(nullptr)
    , databasePathLabel(nullptr)
    , tabWidget(nullptr)
    , fpsValidator(nullptr), guiFPSValidator(nullptr)
    , simulationFPSValidator(nullptr), physicsFPSValidator(nullptr)
{
    setStyleSheet(ApplicationDialogStyles::Dialog);
    if (!isMainThread()) return;

    setWindowTitle("Application Settings");
    setModal(true);
    resize(430, 400);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QTimer::singleShot(0, this, [this]() {
        try {
            setupUI();
            setupConnections();
            s_initialized = true;
        } catch (const std::exception& e) {
            qCritical() << "[ApplicationDialog] Failed to initialize:" << e.what();
            reject();
        }
    });
    runUnitTestsOnce();
}

ApplicationDialog::~ApplicationDialog()
{
    disconnect();
    delete fpsValidator;        fpsValidator        = nullptr;
    delete guiFPSValidator;     guiFPSValidator     = nullptr;
    delete simulationFPSValidator; simulationFPSValidator = nullptr;
    delete physicsFPSValidator; physicsFPSValidator = nullptr;
    s_initialized = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Tab builders
// ─────────────────────────────────────────────────────────────────────────────

/* ── General tab ─────────────────────────────────────────────────────────── */
QWidget* ApplicationDialog::buildGeneralTab()
{
    QWidget *page = new QWidget();
    QVBoxLayout *outer = new QVBoxLayout(page);
    outer->setContentsMargins(12, 12, 12, 12);
    outer->setSpacing(10);

    settingsGroup = new QGroupBox("General Settings", page);
    settingsGroup->setObjectName("settingsGroup");
    settingsGroup->setStyleSheet(ApplicationDialogStyles::GroupBox);

    QFormLayout *form = new QFormLayout();
    form->setSpacing(10);
    form->setContentsMargins(15, 15, 15, 15);

    // Developer Mode
    QLabel *devLabel = new QLabel("Developer Mode:", settingsGroup);
    devLabel->setStyleSheet(ApplicationDialogStyles::FormLabel);
    developerModeCheckBox = new QCheckBox(settingsGroup);
    developerModeCheckBox->setChecked(getGlobalDeveloperMode());
    developerModeCheckBox->setStyleSheet(ApplicationDialogStyles::CheckBox);
    QHBoxLayout *devRow = new QHBoxLayout();
    devRow->addWidget(developerModeCheckBox);
    devRow->addStretch();
    form->addRow(devLabel, devRow);

    // Separator
    QFrame *sep = new QFrame(); sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(ApplicationDialogStyles::Separator);
    form->addRow(sep);

    // Helper lambda to add a FPS row
    auto addFPSRow = [&](const QString& labelText, QLineEdit*& edit, QLabel*& errLabel,
                         QIntValidator*& validator, int savedVal) {
        QLabel *lbl = new QLabel(labelText, settingsGroup);
        lbl->setStyleSheet(ApplicationDialogStyles::FormLabel);
        edit = new QLineEdit(settingsGroup);
        edit->setText(QString::number(savedVal));
        edit->setPlaceholderText("e.g., 60");
        edit->setStyleSheet(ApplicationDialogStyles::LineEdit);
        validator = new QIntValidator(1, 1000, this);
        edit->setValidator(validator);
        errLabel = new QLabel("", settingsGroup);
        errLabel->setVisible(false);
        errLabel->setStyleSheet(ApplicationDialogStyles::ErrorLabel);
        form->addRow(lbl, edit);
        form->addRow(errLabel);
    };

    int savedFPS   = isConfigAvailable() ? MainWindow::scenarioconfig->getSavedFPS()            : 60;
    int savedGUI   = isConfigAvailable() ? MainWindow::scenarioconfig->getSavedGUIFPS()         : 60;
    int savedSim   = isConfigAvailable() ? MainWindow::scenarioconfig->getSavedSimulationFPS()  : 60;
    int savedPhys  = isConfigAvailable() ? MainWindow::scenarioconfig->getSavedPhysicsFPS()     : 60;
    QString savedImg = isConfigAvailable() ? MainWindow::scenarioconfig->getSavedImageSize()    : "100px";

    addFPSRow("Main FPS:",       fpsEdit,           fpsErrorLabel,           fpsValidator,           savedFPS);
    addFPSRow("GUI FPS:",        guiFPSEdit,        guiFPSErrorLabel,        guiFPSValidator,        savedGUI);
    addFPSRow("Simulation FPS:", simulationFPSEdit, simulationFPSErrorLabel, simulationFPSValidator, savedSim);
    addFPSRow("Physics FPS:",    physicsFPSEdit,    physicsFPSErrorLabel,    physicsFPSValidator,    savedPhys);

    // Image Size
    QLabel *imgLbl = new QLabel("Image Size:", settingsGroup);
    imgLbl->setStyleSheet(ApplicationDialogStyles::FormLabel);
    imageSizeEdit = new QLineEdit(settingsGroup);
    imageSizeEdit->setText(savedImg);
    imageSizeEdit->setPlaceholderText("e.g., 100px");
    imageSizeEdit->setStyleSheet(ApplicationDialogStyles::LineEdit);
    QRegularExpression pxRe("^\\d+\\s*px$", QRegularExpression::CaseInsensitiveOption);
    imageSizeEdit->setValidator(new QRegularExpressionValidator(pxRe, this));
    imageSizeErrorLabel = new QLabel("", settingsGroup);
    imageSizeErrorLabel->setVisible(false);
    imageSizeErrorLabel->setStyleSheet(ApplicationDialogStyles::ErrorLabel);
    form->addRow(imgLbl, imageSizeEdit);
    form->addRow(imageSizeErrorLabel);

    settingsGroup->setLayout(form);
    outer->addWidget(settingsGroup);
    outer->addStretch();
    return page;
}

/* ── Database tab ────────────────────────────────────────────────────────── */
QWidget* ApplicationDialog::buildDatabaseTab()
{
    QWidget *page = new QWidget();
    QVBoxLayout *outer = new QVBoxLayout(page);
    outer->setContentsMargins(12, 12, 12, 12);
    outer->setSpacing(12);

    QGroupBox *dbGroup = new QGroupBox("Database Settings", page);
    dbGroup->setStyleSheet(ApplicationDialogStyles::GroupBox);
    QVBoxLayout *vl = new QVBoxLayout(dbGroup);
    vl->setContentsMargins(15, 20, 15, 15);
    vl->setSpacing(14);

    // ── Enable / Disable checkbox ─────────────────────────────────────────
    QHBoxLayout *enableRow = new QHBoxLayout();
    QLabel *enableLbl = new QLabel("Default Database:", dbGroup);
    enableLbl->setStyleSheet(ApplicationDialogStyles::FormLabel);
    databaseEnabledCheckBox = new QCheckBox(dbGroup);
    databaseEnabledCheckBox->setChecked(s_databaseEnabled);
    databaseEnabledCheckBox->setStyleSheet(ApplicationDialogStyles::CheckBox);
    enableRow->addWidget(enableLbl);
    enableRow->addWidget(databaseEnabledCheckBox);
    enableRow->addStretch();
    vl->addLayout(enableRow);

    // Thin separator
    QFrame *sep = new QFrame(); sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(ApplicationDialogStyles::Separator);
    vl->addWidget(sep);

    // ── Path label ───────────────────────────────────────────────────────
    QLabel *pathTitleLbl = new QLabel("Database Path:", dbGroup);
    pathTitleLbl->setStyleSheet(ApplicationDialogStyles::FormLabel);
    vl->addWidget(pathTitleLbl);

    // Path display (read-only line edit)
    databasePathEdit = new QLineEdit(dbGroup);
    databasePathEdit->setText(s_databasePath.isEmpty() ? "No path set" : s_databasePath);
    databasePathEdit->setReadOnly(true);
    databasePathEdit->setStyleSheet(ApplicationDialogStyles::LineEdit);
    databasePathEdit->setPlaceholderText("No database path configured");
    vl->addWidget(databasePathEdit);

    // ── Buttons row ───────────────────────────────────────────────────────
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    browseDatabaseButton = new QPushButton("Browse…", dbGroup);
    browseDatabaseButton->setStyleSheet(ApplicationDialogStyles::OkButton);
    browseDatabaseButton->setFixedHeight(30);

    resetDatabaseButton = new QPushButton("Reset Path", dbGroup);
    resetDatabaseButton->setStyleSheet(ApplicationDialogStyles::CancelButton);
    resetDatabaseButton->setFixedHeight(30);

    btnRow->addWidget(browseDatabaseButton);
    btnRow->addWidget(resetDatabaseButton);
    btnRow->addStretch();
    vl->addLayout(btnRow);

    vl->addStretch();
    outer->addWidget(dbGroup);
    outer->addStretch();
    return page;
}

/* ── Physics tab (placeholder) ───────────────────────────────────────────── */
QWidget* ApplicationDialog::buildPhysicsTab()
{
    QWidget *page = new QWidget();
    QVBoxLayout *outer = new QVBoxLayout(page);
    outer->setContentsMargins(12, 12, 12, 12);

    QGroupBox *physGroup = new QGroupBox("Physics Settings", page);
    physGroup->setStyleSheet(ApplicationDialogStyles::GroupBox);
    QVBoxLayout *vl = new QVBoxLayout(physGroup);
    vl->setContentsMargins(15, 20, 15, 15);

    QLabel *comingSoon = new QLabel("Physics configuration coming soon.", physGroup);
    comingSoon->setAlignment(Qt::AlignCenter);
    comingSoon->setStyleSheet("color: #7f8c8d; font-size: 13px; padding: 20px;");
    vl->addWidget(comingSoon);
    vl->addStretch();

    outer->addWidget(physGroup);
    outer->addStretch();
    return page;
}

// ─────────────────────────────────────────────────────────────────────────────
// setupUI — builds tab widget + OK/Cancel bar
// ─────────────────────────────────────────────────────────────────────────────
void ApplicationDialog::setupUI()
{
    if (!isMainThread()) return;

    // ── Tab widget ────────────────────────────────────────────────────────
    tabWidget = new QTabWidget(this);
    tabWidget->setStyleSheet(R"(
        QTabWidget::pane {
            border: 1px solid #27446d;
            background-color: #0F2636;
            border-radius: 4px;
        }
        QTabBar::tab {
            background-color: #1A3652;
            color: #B0C4D8;
            padding: 8px 20px;
            border: none;
            border-bottom: 2px solid transparent;
            font-size: 12px;
            min-width: 80px;
        }
        QTabBar::tab:selected {
            background-color: #0F2636;
            color: #FFFFFF;
            border-bottom: 2px solid #0078D4;
            font-weight: bold;
        }
        QTabBar::tab:hover:!selected {
            background-color: #203E5A;
            color: #FFFFFF;
        }
    )");

    tabWidget->addTab(buildGeneralTab(),  "General");
    tabWidget->addTab(buildDatabaseTab(), "Database");
    tabWidget->addTab(buildPhysicsTab(),  "Physics");

    // ── OK / Cancel buttons ───────────────────────────────────────────────
    okButton = new QPushButton("OK", this);
    okButton->setDefault(true);
    okButton->setEnabled(false);
    okButton->setStyleSheet(ApplicationDialogStyles::OkButton);

    cancelButton = new QPushButton("Cancel", this);
    cancelButton->setStyleSheet(ApplicationDialogStyles::CancelButton);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(okButton);
    btnLayout->addWidget(cancelButton);
    btnLayout->setSpacing(10);
    btnLayout->setContentsMargins(0, 8, 0, 0);

    // ── Main layout ───────────────────────────────────────────────────────
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);
    mainLayout->addWidget(tabWidget);
    mainLayout->addLayout(btnLayout);
}

// ─────────────────────────────────────────────────────────────────────────────
// setupConnections
// ─────────────────────────────────────────────────────────────────────────────
// void ApplicationDialog::setupConnections()
// {
//     if (!okButton || !cancelButton) return;

//     connect(okButton,     &QPushButton::clicked, this, &ApplicationDialog::onOkClicked);
//     connect(cancelButton, &QPushButton::clicked, this, &ApplicationDialog::onCancelClicked);

//     // General tab
//     if (developerModeCheckBox)
//         connect(developerModeCheckBox, &QCheckBox::stateChanged, this, &ApplicationDialog::validateInputs);
//     auto wire = [this](QLineEdit* e) {
//         if (e) {
//             connect(e, &QLineEdit::textChanged,  this, &ApplicationDialog::validateInputs);
//             connect(e, &QLineEdit::returnPressed, this, &ApplicationDialog::onOkClicked);
//         }
//     };
//     wire(fpsEdit); wire(guiFPSEdit); wire(simulationFPSEdit);
//     wire(physicsFPSEdit); wire(imageSizeEdit);

//     // Database tab — ADD THESE TWO MISSING LINES
//     if (databaseEnabledCheckBox)
//         connect(databaseEnabledCheckBox, &QCheckBox::stateChanged,
//                 this, &ApplicationDialog::validateInputs);

//     if (browseDatabaseButton)
//         connect(browseDatabaseButton, &QPushButton::clicked, this, &ApplicationDialog::onBrowseDatabasePath);
//     if (resetDatabaseButton)
//         connect(resetDatabaseButton,  &QPushButton::clicked, this, &ApplicationDialog::onResetDatabasePath);

//     // Trigger initial validation so OK enables if fields are already valid
//     validateInputs();
// }
void ApplicationDialog::setupConnections()
{
    if (!okButton || !cancelButton) return;

    connect(okButton,     &QPushButton::clicked, this, &ApplicationDialog::onOkClicked);
    connect(cancelButton, &QPushButton::clicked, this, &ApplicationDialog::onCancelClicked);

    // General tab
    if (developerModeCheckBox)
        connect(developerModeCheckBox, &QCheckBox::stateChanged, this, &ApplicationDialog::validateInputs);
    auto wire = [this](QLineEdit* e) {
        if (e) {
            connect(e, &QLineEdit::textChanged,  this, &ApplicationDialog::validateInputs);
            connect(e, &QLineEdit::returnPressed, this, &ApplicationDialog::onOkClicked);
        }
    };
    wire(fpsEdit); wire(guiFPSEdit); wire(simulationFPSEdit);
    wire(physicsFPSEdit); wire(imageSizeEdit);

    // Database tab connections
    if (databaseEnabledCheckBox)
        connect(databaseEnabledCheckBox, &QCheckBox::stateChanged,
                this, &ApplicationDialog::validateInputs);

    if (browseDatabaseButton) {
        // Disconnect any existing connections first
        disconnect(browseDatabaseButton, &QPushButton::clicked, this, nullptr);
        // Connect to our updated slot
        connect(browseDatabaseButton, &QPushButton::clicked, this, &ApplicationDialog::onBrowseDatabasePath);
    }

    if (resetDatabaseButton) {
        disconnect(resetDatabaseButton, &QPushButton::clicked, this, nullptr);
        connect(resetDatabaseButton,  &QPushButton::clicked, this, &ApplicationDialog::onResetDatabasePath);
    }

    // Trigger initial validation so OK enables if fields are already valid
    validateInputs();
}


void ApplicationDialog::onBrowseDatabasePath()
{
    QString startDir = s_databasePath.isEmpty() ? QDir::homePath() : QFileInfo(s_databasePath).absolutePath();
    QString path = QFileDialog::getOpenFileName(
        this,
        "Select Database File",
        startDir,
        "Database Files (*.db);;All Files (*.*)"
        );

    if (!path.isEmpty() && databasePathEdit) {
        // Update the path display
        databasePathEdit->setText(path);

        // Automatically enable the checkbox if it's not enabled
        if (databaseEnabledCheckBox && !databaseEnabledCheckBox->isChecked()) {
            databaseEnabledCheckBox->setChecked(true);
        }

        // Save the settings immediately
        bool dbEnabled = databaseEnabledCheckBox->isChecked();
        QString dbPath = path;

        setGlobalDatabaseEnabled(dbEnabled);
        setGlobalDatabasePath(dbPath);

        // Update ScenarioConfig
        if (MainWindow::scenarioconfig) {
            MainWindow::scenarioconfig->saveDatabaseSettings(dbEnabled, dbPath);
        }

        // ✅ FIXED: Don't load directly - just emit signal and show message
        emit databaseSettingsChanged(dbEnabled, dbPath);

        qDebug() << "Database selected:" << path;

        // Show a message that database will be loaded when switching editors

    }
}


void ApplicationDialog::onResetDatabasePath()
{
    if (databasePathEdit) {
        // Reset the path display to default
        QString defaultPath = "Aircraft.db (Default)";
        databasePathEdit->setText(defaultPath);

        // Disable the checkbox (optional - aap chahe to enable bhi rakh sakte hain)
        if (databaseEnabledCheckBox) {
            databaseEnabledCheckBox->setChecked(true); // Default database enable rahega
        }

        // Update static members
        setGlobalDatabaseEnabled(true);
        setGlobalDatabasePath("");  // Empty path means use default Aircraft.db

        // Update ScenarioConfig
        if (MainWindow::scenarioconfig) {
            MainWindow::scenarioconfig->saveDatabaseSettings(true, "");
        }

        // Load default Aircraft.db into current editor's library
        MainWindow* mainWin = MainWindow::instance();
        if (mainWin) {
            QMainWindow* currentEditor = mainWin->getCurrentEditor();

            // Show loading overlay
            mainWin->showLoadingOverlay("Loading Default Aircraft Database...");
            QCoreApplication::processEvents();

            // Get default Aircraft.db path
            QString aircraftDbPath = TDFManager::instance()->getAircraftDbPath();

            // Load into appropriate editor
            if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(currentEditor)) {
                // Load default database into scenario editor's library
                QFile file(aircraftDbPath);
                if (file.open(QIODevice::ReadOnly)) {
                    QByteArray data = file.readAll();
                    file.close();

                    QJsonParseError err;
                    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
                    if (err.error == QJsonParseError::NoError && doc.isObject()) {
                        QJsonObject obj = doc.object();
                        if (obj.contains("hierarchy")) {
                            // ✅ Load default hierarchy
                            scEditor->library->fromJson(obj["hierarchy"].toObject());

                            // Update tree view
                            if (scEditor->libTreeView) {
                                scEditor->libTreeView->setLibraryFileName("Aircraft.db (Default)");
                                scEditor->libTreeView->getTreeWidget()->update();
                                scEditor->libTreeView->getTreeWidget()->collapseAll();
                            }

                            qDebug() << "Default Aircraft.db loaded into Scenario Editor library";
                        }
                    }
                }
            }
            else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor)) {
                // Load default database into runtime editor's library
                QFile file(aircraftDbPath);
                if (file.open(QIODevice::ReadOnly)) {
                    QByteArray data = file.readAll();
                    file.close();

                    QJsonParseError err;
                    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
                    if (err.error == QJsonParseError::NoError && doc.isObject()) {
                        QJsonObject obj = doc.object();
                        if (obj.contains("hierarchy")) {
                            // ✅ Load default hierarchy
                            rtEditor->library->fromJson(obj["hierarchy"].toObject());

                            // Update tree view
                            if (rtEditor->libTreeView) {
                                rtEditor->libTreeView->setLibraryFileName("Aircraft.db (Default)");
                                rtEditor->libTreeView->getTreeWidget()->update();
                                rtEditor->libTreeView->getTreeWidget()->collapseAll();
                            }

                            qDebug() << "Default Aircraft.db loaded into Runtime Editor library";
                        }
                    }
                }
            }

            mainWin->hideLoadingOverlay();


        }

        // Emit signal
        emit databaseSettingsChanged(true, "");

        qDebug() << "Database reset to default Aircraft.db";
    }
}
// ─────────────────────────────────────────────────────────────────────────────
// Validation
// ─────────────────────────────────────────────────────────────────────────────
bool ApplicationDialog::validateFPSInput(QLineEdit* edit, QLabel* errorLabel, const QString& fieldName)
{
    if (!edit || !errorLabel) return false;
    QString text = edit->text().trimmed();
    if (text.isEmpty()) {
        errorLabel->setText(QString("Please enter %1 value").arg(fieldName));
        errorLabel->setVisible(true);
        edit->setStyleSheet(ApplicationDialogStyles::LineEditError);
        return false;
    }
    bool ok; int value = text.toInt(&ok);
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

bool ApplicationDialog::validateImageSizeInput()
{
    if (!imageSizeEdit || !imageSizeErrorLabel) return false;
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
    int pv = match.captured(1).toInt();
    if (pv < 1 || pv > 10000) {
        imageSizeErrorLabel->setText("Size must be between 1px and 10000px");
        imageSizeErrorLabel->setVisible(true);
        imageSizeEdit->setStyleSheet(ApplicationDialogStyles::LineEditError);
        return false;
    }
    imageSizeErrorLabel->setVisible(false);
    imageSizeEdit->setStyleSheet(ApplicationDialogStyles::LineEdit);
    return true;
}

void ApplicationDialog::validateInputs()
{
    // if (!s_initialized) return;
    bool valid = true;
    if (!validateFPSInput(fpsEdit,           fpsErrorLabel,           "Main FPS"))       valid = false;
    if (!validateFPSInput(guiFPSEdit,        guiFPSErrorLabel,        "GUI FPS"))        valid = false;
    if (!validateFPSInput(simulationFPSEdit, simulationFPSErrorLabel, "Simulation FPS")) valid = false;
    if (!validateFPSInput(physicsFPSEdit,    physicsFPSErrorLabel,    "Physics FPS"))    valid = false;
    if (!validateImageSizeInput())                                                        valid = false;
    if (okButton) okButton->setEnabled(valid);
}

// ─────────────────────────────────────────────────────────────────────────────
// OK / Cancel
// ─────────────────────────────────────────────────────────────────────────────
void ApplicationDialog::onOkClicked()
{
    if (!isMainThread()) return;
    validateInputs();
    if (!okButton || !okButton->isEnabled()) {
        QMessageBox::warning(this, "Validation Error", "Please fix all errors before saving.");
        return;
    }

    try {
        // ── Save General settings ─────────────────────────────────────────
        int fps       = getFPS();
        int guifps    = getGUIFPS();
        int simfps    = getSimulationFPS();
        int physicsfps= getPhysicsFPS();
        QString imgSz = getImageSize();
        bool devMode  = getDeveloperMode();

        setGlobalFPS(fps);
        setGlobalGUIFPS(guifps);
        setGlobalSimulationFPS(simfps);
        setGlobalPhysicsFPS(physicsfps);
        setGlobalImageSize(imgSz);
        setGlobalDeveloperMode(devMode);

        emit fpsState(fps);
        emit guiFPSState(guifps);
        emit simulationFPSState(simfps);
        emit physicsFPSState(physicsfps);

        QRegularExpression re("(\\d+)(?:\\s*px)?", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(imgSz);
        if (m.hasMatch()) {
            bool ok; int px = m.captured(1).toInt(&ok);
            if (ok) emit canvasIconState(px);
        }
        emit developerModeState(devMode);

        // ── Save Database settings ────────────────────────────────────────
        if (databaseEnabledCheckBox && databasePathEdit) {
            bool dbEnabled = databaseEnabledCheckBox->isChecked();
            QString dbPath = databasePathEdit->text();
            if (dbPath == "No path set") dbPath.clear();
            setGlobalDatabaseEnabled(dbEnabled);
            setGlobalDatabasePath(dbPath);
            emit databaseSettingsChanged(dbEnabled, dbPath);
        }
        accept();
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Save Error", QString("Failed to save settings: %1").arg(e.what()));
    }
}
void ApplicationDialog::onCancelClicked()
{
    reject();
}

// ─────────────────────────────────────────────────────────────────────────────
// Getters (read from UI widgets)
// ─────────────────────────────────────────────────────────────────────────────
int     ApplicationDialog::getFPS()          const { return fpsEdit           ? fpsEdit->text().toInt()           : 60; }
int     ApplicationDialog::getGUIFPS()       const { return guiFPSEdit        ? guiFPSEdit->text().toInt()        : 60; }
int     ApplicationDialog::getSimulationFPS()const { return simulationFPSEdit ? simulationFPSEdit->text().toInt() : 60; }
int     ApplicationDialog::getPhysicsFPS()   const { return physicsFPSEdit    ? physicsFPSEdit->text().toInt()    : 60; }
QString ApplicationDialog::getImageSize()    const { return imageSizeEdit     ? imageSizeEdit->text()             : "100px"; }
bool    ApplicationDialog::getDeveloperMode()const { return developerModeCheckBox ? developerModeCheckBox->isChecked() : false; }
void ApplicationDialog::runUnitTestsOnce()
{
    static bool testsRun = false;
    if (testsRun) return;
    testsRun = true;

    QTimer::singleShot(0, []() {
        Console* console = nullptr;
        MainWindow* mw = MainWindow::instance();
        if (mw && mw->databaseEditor && mw->databaseEditor->console) {
            console = mw->databaseEditor->console;
        }
        if (!console) {
            qDebug() << "ApplicationDialog: console not available, cannot run tests";
            return;
        }

        // Create a temporary ApplicationDialog (no parent, won't show)
        ApplicationDialog* testDialog = new ApplicationDialog(nullptr);
        runApplicationDialogTests(testDialog, console);
        testDialog->deleteLater();
    });
}
