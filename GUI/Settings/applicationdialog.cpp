

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

void ApplicationDialog::setGlobalFPS(int fps) {
    if (fps >= 1 && fps <= 1000) {
        // Save all settings using the 5-parameter method
        MainWindow::scenarioconfig->saveAppSettings(
            fps,
            MainWindow::scenarioconfig->getSavedGUIFPS(),
            MainWindow::scenarioconfig->getSavedSimulationFPS(),
            MainWindow::scenarioconfig->getSavedPhysicsFPS(),
            MainWindow::scenarioconfig->getSavedImageSize()
            );
        emit fpsState(fps);
        qDebug() << "Global FPS set to:" << fps;
    } else {
        qWarning() << "Invalid FPS value:" << fps;
    }
}

void ApplicationDialog::setGlobalGUIFPS(int guifps) {
    if (guifps >= 1 && guifps <= 1000) {
        // Save all settings using the 5-parameter method
        MainWindow::scenarioconfig->saveAppSettings(
            MainWindow::scenarioconfig->getSavedFPS(),
            guifps,
            MainWindow::scenarioconfig->getSavedSimulationFPS(),
            MainWindow::scenarioconfig->getSavedPhysicsFPS(),
            MainWindow::scenarioconfig->getSavedImageSize()
            );
        emit guiFPSState(guifps);
        qDebug() << "GUI FPS set to:" << guifps;
    } else {
        qWarning() << "Invalid GUI FPS value:" << guifps;
    }
}

void ApplicationDialog::setGlobalSimulationFPS(int simfps) {
    if (simfps >= 1 && simfps <= 1000) {
        // Save all settings using the 5-parameter method
        MainWindow::scenarioconfig->saveAppSettings(
            MainWindow::scenarioconfig->getSavedFPS(),
            MainWindow::scenarioconfig->getSavedGUIFPS(),
            simfps,
            MainWindow::scenarioconfig->getSavedPhysicsFPS(),
            MainWindow::scenarioconfig->getSavedImageSize()
            );
        emit simulationFPSState(simfps);
        qDebug() << "Simulation FPS set to:" << simfps;
    } else {
        qWarning() << "Invalid Simulation FPS value:" << simfps;
    }
}

void ApplicationDialog::setGlobalPhysicsFPS(int physicsfps) {
    if (physicsfps >= 1 && physicsfps <= 1000) {
        // Save all settings using the 5-parameter method
        MainWindow::scenarioconfig->saveAppSettings(
            MainWindow::scenarioconfig->getSavedFPS(),
            MainWindow::scenarioconfig->getSavedGUIFPS(),
            MainWindow::scenarioconfig->getSavedSimulationFPS(),
            physicsfps,
            MainWindow::scenarioconfig->getSavedImageSize()
            );
        emit physicsFPSState(physicsfps);
        qDebug() << "Physics FPS set to:" << physicsfps;
    } else {
        qWarning() << "Invalid Physics FPS value:" << physicsfps;
    }
}

void ApplicationDialog::setGlobalImageSize(const QString& size) {
    if (!size.isEmpty()) {
        QString input = size.trimmed();

        // Extract pixel value using regex
        QRegularExpression re("(\\d+)(?:\\s*px)?", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = re.match(input);

        if (match.hasMatch()) {
            QString g_pixelValue = match.captured(1);

            // Save all settings using the 5-parameter method
            MainWindow::scenarioconfig->saveAppSettings(
                MainWindow::scenarioconfig->getSavedFPS(),
                MainWindow::scenarioconfig->getSavedGUIFPS(),
                MainWindow::scenarioconfig->getSavedSimulationFPS(),
                MainWindow::scenarioconfig->getSavedPhysicsFPS(),
                g_pixelValue + "px"
                );

            // Convert to int and emit
            bool ok;
            int pixelInt = g_pixelValue.toInt(&ok);
            if (ok) {
                emit canvasIconState(pixelInt);
            } else {
                qWarning() << "Failed to convert pixel value to int:" << g_pixelValue;
            }

            qDebug() << "Image Size set to:" << MainWindow::scenarioconfig->getSavedImageSize();
            qDebug() << "Pixel Value:" << g_pixelValue;
        } else {
            qWarning() << "Invalid image size format:" << size;
        }
    }
}

void ApplicationDialog::resetGlobalSettings() {
    int g_fps = 60;
    int g_guifps = 60;
    int g_simfps = 60;
    int g_physicsfps = 60;
    QString g_imageSize = "100px";


    MainWindow::scenarioconfig->saveAppSettings(g_fps, g_guifps, g_simfps, g_physicsfps, g_imageSize);

    qDebug() << "Global settings reset to defaults:";
    qDebug() << "Main FPS:" << g_fps;
    qDebug() << "GUI FPS:" << g_guifps;
    qDebug() << "Simulation FPS:" << g_simfps;
    qDebug() << "Physics FPS:" << g_physicsfps;
    qDebug() << "Image Size:" << g_imageSize;
}

void cleanupGlobalSettings()
{
    // Cleanup function if needed
}

// ------------------------------------------------------------
// APPLICATION DIALOG IMPLEMENTATION
// ------------------------------------------------------------

ApplicationDialog::ApplicationDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Application Settings");
    setModal(true);
    resize(350, 320);

    // Set dialog properties
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Setup UI
    setupUI();
    setupConnections();
}

ApplicationDialog::~ApplicationDialog()
{
    // Destructor
}

void ApplicationDialog::setupUI()
{
    // Set overall dialog style
    setStyleSheet(R"(
        QDialog {
            background-color: #f8f9fa;
            font-family: 'Segoe UI', Arial, sans-serif;
        }

        QGroupBox {
            font-weight: bold;
            font-size: 12px;
            color: #495057;
            border: 1px solid #dee2e6;
            border-radius: 6px;
            margin-top: 12px;
            padding-top: 12px;
            background-color: white;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }

        QLabel {
            color: #495057;
            font-size: 12px;
        }

        QLineEdit {
            border: 1px solid #ced4da;
            border-radius: 4px;
            padding: 6px;
            font-size: 13px;
            background-color: white;
            min-height: 20px;
            min-width: 150px;
        }

        QLineEdit:focus {
            border: 1px solid #80bdff;
            outline: none;
        }

        QLineEdit.error {
            border: 1px solid #dc3545;
            background-color: #fff5f5;
        }

        QLabel.error {
            color: #dc3545;
            font-size: 11px;
            font-style: italic;
        }

        QPushButton {
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 13px;
            font-weight: 500;
            min-width: 80px;
        }

        QPushButton#okButton {
            background-color: #28a745;
            color: white;
        }

        QPushButton#okButton:hover {
            background-color: #218838;
        }

        QPushButton#okButton:pressed {
            background-color: #1e7e34;
        }

        QPushButton#okButton:disabled {
            background-color: #6c757d;
            color: #adb5bd;
        }

        QPushButton#cancelButton {
            background-color: #6c757d;
            color: white;
        }

        QPushButton#cancelButton:hover {
            background-color: #545b62;
        }

        QPushButton#cancelButton:pressed {
            background-color: #3d4347;
        }
    )");

    // Create settings group box
    settingsGroup = new QGroupBox("Settings", this);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(8);
    formLayout->setContentsMargins(20, 20, 20, 20);

    // Main FPS input
    QLabel *fpsLabel = new QLabel("Main FPS:", settingsGroup);
    fpsLabel->setToolTip("Main frames per second (1-1000)");

    fpsEdit = new QLineEdit(settingsGroup);
    fpsEdit->setText(QString::number(MainWindow::scenarioconfig->getSavedFPS()));
    fpsEdit->setPlaceholderText("e.g., 60");
    fpsEdit->setToolTip("Enter Main FPS value between 1 and 1000");

    QIntValidator *fpsValidator = new QIntValidator(1, 1000, this);
    fpsEdit->setValidator(fpsValidator);

    fpsErrorLabel = new QLabel("", settingsGroup);
    fpsErrorLabel->setObjectName("error");
    fpsErrorLabel->setVisible(false);

    // GUI FPS input
    QLabel *guiFPSLabel = new QLabel("GUI FPS:", settingsGroup);
    guiFPSLabel->setToolTip("GUI refresh rate (1-1000)");

    guiFPSEdit = new QLineEdit(settingsGroup);
    guiFPSEdit->setText(QString::number(MainWindow::scenarioconfig->getSavedGUIFPS()));
    guiFPSEdit->setPlaceholderText("e.g., 60");
    guiFPSEdit->setToolTip("Enter GUI FPS value between 1 and 1000");

    QIntValidator *guiFPSValidator = new QIntValidator(1, 1000, this);
    guiFPSEdit->setValidator(guiFPSValidator);

    guiFPSErrorLabel = new QLabel("", settingsGroup);
    guiFPSErrorLabel->setObjectName("error");
    guiFPSErrorLabel->setVisible(false);

    // Simulation FPS input
    QLabel *simulationFPSLabel = new QLabel("Simulation FPS:", settingsGroup);
    simulationFPSLabel->setToolTip("Simulation update rate (1-1000)");

    simulationFPSEdit = new QLineEdit(settingsGroup);
    simulationFPSEdit->setText(QString::number(MainWindow::scenarioconfig->getSavedSimulationFPS()));
    simulationFPSEdit->setPlaceholderText("e.g., 60");
    simulationFPSEdit->setToolTip("Enter Simulation FPS value between 1 and 1000");

    QIntValidator *simulationFPSValidator = new QIntValidator(1, 1000, this);
    simulationFPSEdit->setValidator(simulationFPSValidator);

    simulationFPSErrorLabel = new QLabel("", settingsGroup);
    simulationFPSErrorLabel->setObjectName("error");
    simulationFPSErrorLabel->setVisible(false);

    // Physics FPS input (NEW)
    QLabel *physicsFPSLabel = new QLabel("Physics FPS:", settingsGroup);
    physicsFPSLabel->setToolTip("Physics engine update rate (1-1000)");

    physicsFPSEdit = new QLineEdit(settingsGroup);
    physicsFPSEdit->setText(QString::number(MainWindow::scenarioconfig->getSavedPhysicsFPS()));
    physicsFPSEdit->setPlaceholderText("e.g., 60");
    physicsFPSEdit->setToolTip("Enter Physics FPS value between 1 and 1000");

    QIntValidator *physicsFPSValidator = new QIntValidator(1, 1000, this);
    physicsFPSEdit->setValidator(physicsFPSValidator);

    physicsFPSErrorLabel = new QLabel("", settingsGroup);
    physicsFPSErrorLabel->setObjectName("error");
    physicsFPSErrorLabel->setVisible(false);

    // Image Size input
    QLabel *imageSizeLabel = new QLabel("Image Size:", settingsGroup);
    imageSizeLabel->setToolTip("Size in pixels (e.g., 100px, 250px)");

    imageSizeEdit = new QLineEdit(settingsGroup);
    imageSizeEdit->setText(MainWindow::scenarioconfig->getSavedImageSize());
    imageSizeEdit->setPlaceholderText("e.g., 100px");
    imageSizeEdit->setToolTip("Enter size in pixels (1px to 10000px)");

    QRegularExpression pxRegEx("^\\d+\\s*px$", QRegularExpression::CaseInsensitiveOption);
    QValidator *pxValidator = new QRegularExpressionValidator(pxRegEx, this);
    imageSizeEdit->setValidator(pxValidator);

    imageSizeErrorLabel = new QLabel("", settingsGroup);
    imageSizeErrorLabel->setObjectName("error");
    imageSizeErrorLabel->setVisible(false);

    // Add to form layout
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

    // Create buttons
    okButton = new QPushButton("OK", this);
    okButton->setObjectName("okButton");
    okButton->setDefault(true);
    okButton->setCursor(Qt::PointingHandCursor);

    cancelButton = new QPushButton("Cancel", this);
    cancelButton->setObjectName("cancelButton");
    cancelButton->setCursor(Qt::PointingHandCursor);

    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->addWidget(settingsGroup);
    mainLayout->addLayout(buttonLayout);

    // Set focus to first input
    fpsEdit->setFocus();

    // Initial validation
    validateInputs();
}

void ApplicationDialog::setupConnections()
{
    connect(okButton, &QPushButton::clicked, this, &ApplicationDialog::onOkClicked);
    connect(cancelButton, &QPushButton::clicked, this, &ApplicationDialog::onCancelClicked);

    // Connect text changes to validation
    connect(fpsEdit, &QLineEdit::textChanged, this, &ApplicationDialog::validateInputs);
    connect(guiFPSEdit, &QLineEdit::textChanged, this, &ApplicationDialog::validateInputs);
    connect(simulationFPSEdit, &QLineEdit::textChanged, this, &ApplicationDialog::validateInputs);
    connect(physicsFPSEdit, &QLineEdit::textChanged, this, &ApplicationDialog::validateInputs);
    connect(imageSizeEdit, &QLineEdit::textChanged, this, &ApplicationDialog::validateInputs);

    // Connect Enter key to OK button
    connect(fpsEdit, &QLineEdit::returnPressed, this, &ApplicationDialog::onOkClicked);
    connect(guiFPSEdit, &QLineEdit::returnPressed, this, &ApplicationDialog::onOkClicked);
    connect(simulationFPSEdit, &QLineEdit::returnPressed, this, &ApplicationDialog::onOkClicked);
    connect(physicsFPSEdit, &QLineEdit::returnPressed, this, &ApplicationDialog::onOkClicked);
    connect(imageSizeEdit, &QLineEdit::returnPressed, this, &ApplicationDialog::onOkClicked);
}

void ApplicationDialog::validateInputs()
{
    bool isValid = true;

    // Validate Main FPS
    if (fpsEdit->text().isEmpty()) {
        fpsEdit->setProperty("class", "error");
        fpsErrorLabel->setText("Please enter Main FPS value");
        fpsErrorLabel->setVisible(true);
        isValid = false;
    } else {
        int fpsValue = fpsEdit->text().toInt();
        if (fpsValue < 1 || fpsValue > 1000) {
            fpsEdit->setProperty("class", "error");
            fpsErrorLabel->setText("Main FPS must be between 1 and 1000");
            fpsErrorLabel->setVisible(true);
            isValid = false;
        } else {
            fpsEdit->setProperty("class", "");
            fpsErrorLabel->setVisible(false);
        }
    }

    // Validate GUI FPS
    if (guiFPSEdit->text().isEmpty()) {
        guiFPSEdit->setProperty("class", "error");
        guiFPSErrorLabel->setText("Please enter GUI FPS value");
        guiFPSErrorLabel->setVisible(true);
        isValid = false;
    } else {
        int guiFPSValue = guiFPSEdit->text().toInt();
        if (guiFPSValue < 1 || guiFPSValue > 1000) {
            guiFPSEdit->setProperty("class", "error");
            guiFPSErrorLabel->setText("GUI FPS must be between 1 and 1000");
            guiFPSErrorLabel->setVisible(true);
            isValid = false;
        } else {
            guiFPSEdit->setProperty("class", "");
            guiFPSErrorLabel->setVisible(false);
        }
    }

    // Validate Simulation FPS
    if (simulationFPSEdit->text().isEmpty()) {
        simulationFPSEdit->setProperty("class", "error");
        simulationFPSErrorLabel->setText("Please enter Simulation FPS value");
        simulationFPSErrorLabel->setVisible(true);
        isValid = false;
    } else {
        int simFPSValue = simulationFPSEdit->text().toInt();
        if (simFPSValue < 1 || simFPSValue > 1000) {
            simulationFPSEdit->setProperty("class", "error");
            simulationFPSErrorLabel->setText("Simulation FPS must be between 1 and 1000");
            simulationFPSErrorLabel->setVisible(true);
            isValid = false;
        } else {
            simulationFPSEdit->setProperty("class", "");
            simulationFPSErrorLabel->setVisible(false);
        }
    }

    // Validate Physics FPS (NEW)
    if (physicsFPSEdit->text().isEmpty()) {
        physicsFPSEdit->setProperty("class", "error");
        physicsFPSErrorLabel->setText("Please enter Physics FPS value");
        physicsFPSErrorLabel->setVisible(true);
        isValid = false;
    } else {
        int physicsFPSValue = physicsFPSEdit->text().toInt();
        if (physicsFPSValue < 1 || physicsFPSValue > 1000) {
            physicsFPSEdit->setProperty("class", "error");
            physicsFPSErrorLabel->setText("Physics FPS must be between 1 and 1000");
            physicsFPSErrorLabel->setVisible(true);
            isValid = false;
        } else {
            physicsFPSEdit->setProperty("class", "");
            physicsFPSErrorLabel->setVisible(false);
        }
    }

    // Validate Image Size
    if (imageSizeEdit->text().isEmpty()) {
        imageSizeEdit->setProperty("class", "error");
        imageSizeErrorLabel->setText("Please enter image size");
        imageSizeErrorLabel->setVisible(true);
        isValid = false;
    } else {
        QString imageSize = imageSizeEdit->text().trimmed();
        QRegularExpression re("^(\\d+)\\s*px$", QRegularExpression::CaseInsensitiveOption);

        if (!re.match(imageSize).hasMatch()) {
            imageSizeEdit->setProperty("class", "error");
            imageSizeErrorLabel->setText("Format: number followed by 'px' (e.g., 100px)");
            imageSizeErrorLabel->setVisible(true);
            isValid = false;
        } else {
            // Extract and validate pixel value
            int pixelValue = 0;
            QRegularExpressionMatch match = re.match(imageSize);
            if (match.hasMatch()) {
                pixelValue = match.captured(1).toInt();
            }

            if (pixelValue < 1 || pixelValue > 10000) {
                imageSizeEdit->setProperty("class", "error");
                imageSizeErrorLabel->setText("Size must be between 1px and 10000px");
                imageSizeErrorLabel->setVisible(true);
                isValid = false;
            } else {
                imageSizeEdit->setProperty("class", "");
                imageSizeErrorLabel->setVisible(false);
            }
        }
    }

    // Enable/disable OK button based on validation
    okButton->setEnabled(isValid);

    // Force style update
    fpsEdit->style()->polish(fpsEdit);
    guiFPSEdit->style()->polish(guiFPSEdit);
    simulationFPSEdit->style()->polish(simulationFPSEdit);
    physicsFPSEdit->style()->polish(physicsFPSEdit);
    imageSizeEdit->style()->polish(imageSizeEdit);
}

int ApplicationDialog::getFPS() const
{
    return fpsEdit->text().toInt();
}

int ApplicationDialog::getGUIFPS() const
{
    return guiFPSEdit->text().toInt();
}

int ApplicationDialog::getSimulationFPS() const
{
    return simulationFPSEdit->text().toInt();
}

int ApplicationDialog::getPhysicsFPS() const
{
    return physicsFPSEdit->text().toInt();
}

QString ApplicationDialog::getImageSize() const
{
    return imageSizeEdit->text();
}

int ApplicationDialog::getGlobalFPS()
{
    return MainWindow::scenarioconfig->getSavedFPS();
}

int ApplicationDialog::getGlobalGUIFPS()
{
    return MainWindow::scenarioconfig->getSavedGUIFPS();
}

int ApplicationDialog::getGlobalSimulationFPS()
{
    return MainWindow::scenarioconfig->getSavedSimulationFPS();
}

int ApplicationDialog::getGlobalPhysicsFPS()
{
    return MainWindow::scenarioconfig->getSavedPhysicsFPS();
}

QString ApplicationDialog::getGlobalImageSize()
{
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

void ApplicationDialog::onOkClicked()
{
    // Final validation
    validateInputs();

    if (!okButton->isEnabled()) {
        // If still invalid, don't proceed
        return;
    }

    // Save to global variables AND to config file
    setGlobalFPS(getFPS());
    setGlobalGUIFPS(getGUIFPS());
    setGlobalSimulationFPS(getSimulationFPS());
    setGlobalPhysicsFPS(getPhysicsFPS());
    setGlobalImageSize(getImageSize());

    // Close dialog
    accept();
}

void ApplicationDialog::onCancelClicked()
{
    reject();
}
