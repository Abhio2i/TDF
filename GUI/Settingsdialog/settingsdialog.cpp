#include "settingsdialog.h"
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QMessageBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Profile Settings");
    setModal(false);  // Non-modal dialog

    setupUI();
    setupConnections();

    // Set window flags to stay on top
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Settings group box
    QGroupBox *settingsGroup = new QGroupBox("Video Settings");
    settingsGroup->setStyleSheet("QGroupBox { font-weight: bold; }");

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(15);
    formLayout->setContentsMargins(15, 20, 15, 20);

    // Frame Rate
    frameRateSpinBox = new QSpinBox();
    frameRateSpinBox->setRange(1, 240);
    frameRateSpinBox->setSuffix(" fps");
    frameRateSpinBox->setToolTip("Set the frame rate for video output (1-240 fps)");
    frameRateSpinBox->setValue(30);  // Default value

    frameRateLabel = new QLabel("Frame Rate:");
    formLayout->addRow(frameRateLabel, frameRateSpinBox);

    // Image Size
    imageSizeLineEdit = new QLineEdit();
    imageSizeLineEdit->setPlaceholderText("e.g., 1920x1080, 1280x720");
    imageSizeLineEdit->setToolTip("Enter image size in WIDTHxHEIGHT format");

    // Add validator for WIDTHxHEIGHT format
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(
        QRegularExpression("\\d+x\\d+"), this);
    imageSizeLineEdit->setValidator(validator);

    imageSizeLabel = new QLabel("Image Size:");
    formLayout->addRow(imageSizeLabel, imageSizeLineEdit);

    settingsGroup->setLayout(formLayout);
    mainLayout->addWidget(settingsGroup);

    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    cancelButton = new QPushButton("Cancel");
    cancelButton->setFixedWidth(80);

    saveButton = new QPushButton("Save");
    saveButton->setFixedWidth(80);
    saveButton->setDefault(true);
    saveButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; }");

    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(saveButton);

    mainLayout->addLayout(buttonLayout);

    // Set dialog properties
    setMinimumSize(400, 250);
    setMaximumSize(500, 300);
}

void SettingsDialog::setupConnections()
{
    connect(saveButton, &QPushButton::clicked, this, &SettingsDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &SettingsDialog::reject);
    connect(imageSizeLineEdit, &QLineEdit::textChanged, this, &SettingsDialog::validateImageSize);
}

int SettingsDialog::getFrameRate() const
{
    return frameRateSpinBox->value();
}

QString SettingsDialog::getImageSize() const
{
    return imageSizeLineEdit->text();
}

void SettingsDialog::setFrameRate(int rate)
{
    frameRateSpinBox->setValue(rate);
}

void SettingsDialog::setImageSize(const QString& size)
{
    imageSizeLineEdit->setText(size);
}

void SettingsDialog::validateImageSize()
{
    QString text = imageSizeLineEdit->text();
    QRegularExpression pattern("^\\d+x\\d+$");

    if (pattern.match(text).hasMatch()) {
        imageSizeLineEdit->setStyleSheet("");
    } else {
        imageSizeLineEdit->setStyleSheet("QLineEdit { border: 1px solid red; }");
    }
}
