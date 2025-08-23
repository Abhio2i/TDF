
#include "testscriptdialog.h"
#include "core/Debug/console.h"
#include <QIcon>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <QTextStream>
#include <QFile>
#include <QFileInfo>

TestScriptDialog::TestScriptDialog(QWidget *parent, bool editMode, const QString &filePath)
    : QDialog(parent), isEditMode(editMode), editFilePath(filePath)
{
    Console::log("Creating TestScriptDialog with parent: " + std::string(parent ? parent->objectName().toStdString() : "null") +
                 ", editMode: " + std::string(editMode ? "true" : "false"));
    setWindowTitle(tr("Test Script Dialog"));
    setMinimumSize(600, 400);
    setAttribute(Qt::WA_DeleteOnClose);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    Console::log("Main layout created for dialog box");

    QVBoxLayout *leftLayout = new QVBoxLayout();

    scriptNameEdit = new QLineEdit(this);
    scriptNameEdit->setObjectName("scriptNameEdit");
    scriptNameLabel = new QLabel(tr("Script Name:"), this);
    scriptNameEdit->setPlaceholderText(tr("Enter script name..."));
    leftLayout->addWidget(scriptNameLabel);
    leftLayout->addWidget(scriptNameEdit);
    Console::log("Script name widgets created");

    scriptTypeCombo = new QComboBox(this);
    scriptTypeCombo->setObjectName("scriptTypeCombo");
    scriptTypeLabel = new QLabel(tr("Script Type:"), this);
    scriptTypeCombo->addItems({"Entity Behaviour", "Sensors Script", "Radar Script", "Editor Script"});
    leftLayout->addWidget(scriptTypeLabel);
    leftLayout->addWidget(scriptTypeCombo);
    Console::log("Script type widgets created");

    scriptPathEdit = new QLineEdit(this);
    scriptPathEdit->setObjectName("scriptPathEdit");
    scriptPathLabel = new QLabel(tr("Save Location:"), this);
    scriptPathEdit->setPlaceholderText(tr("Enter or select folder path..."));
    browseButton = new QPushButton(QIcon(":/icons/images/folder.png"), tr("Browse"), this);
    browseButton->setFixedWidth(100);
    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathLayout->addWidget(scriptPathEdit);
    pathLayout->addWidget(browseButton);
    leftLayout->addWidget(scriptPathLabel);
    leftLayout->addLayout(pathLayout);
    Console::log("Browse widgets created");

    runButton = new QPushButton(QIcon(":/icons/images/run.png"), tr("Run"), this);
    runButton->setFixedWidth(100);
    leftLayout->addStretch();
    leftLayout->addWidget(runButton);
    Console::log("Run button created");

    QVBoxLayout *rightLayout = new QVBoxLayout();

    codeEditor = new QTextEdit(this);
    codeEditor->setObjectName("codeEditor");
    codeEditor->setPlaceholderText(tr("Enter your script here..."));
    codeEditor->setFont(QFont("Courier New", 10));
    rightLayout->addWidget(codeEditor);
    Console::log("Code editor created");

    okButton = new QPushButton(tr("OK"), this);
    okButton->setEnabled(!editMode); // Enable OK button by default in add mode
    cancelButton = new QPushButton(tr("Cancel"), this);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    rightLayout->addLayout(buttonLayout);
    Console::log("OK and Cancel buttons created");

    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addLayout(rightLayout, 3);

    connect(runButton, &QPushButton::clicked, this, &TestScriptDialog::onRunButtonClicked);
    connect(browseButton, &QPushButton::clicked, this, &TestScriptDialog::onBrowseButtonClicked);
    connect(okButton, &QPushButton::clicked, this, &TestScriptDialog::onOkButtonClicked);
    connect(cancelButton, &QPushButton::clicked, this, &TestScriptDialog::onCancelButtonClicked);
    connect(scriptPathEdit, &QLineEdit::textChanged, this, &TestScriptDialog::onScriptPathChanged);
    Console::log("Signals connected for dialog box");

    if (runButton->icon().isNull()) {
        Console::error("Failed to load run icon from :/icons/images/run.png");
    } else {
        Console::log("Run icon loaded successfully");
    }
    if (browseButton->icon().isNull()) {
        Console::error("Failed to load browse icon from :/icons/images/folder.png");
    } else {
        Console::log("Browse icon loaded successfully");
    }

    if (isEditMode) {
        // Hide fields for edit mode
        scriptNameLabel->hide();
        scriptNameEdit->hide();
        scriptTypeLabel->hide();
        scriptTypeCombo->hide();
        scriptPathLabel->hide();
        scriptPathEdit->hide();
        browseButton->hide();
        Console::log("Hiding script name, type, and path fields for edit mode");

        // Load existing script content
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            Console::error("Failed to open file for editing: " + filePath.toStdString());
            // QMessageBox::warning(this, "Edit Failed", "Could not open the file for editing.");
            okButton->setEnabled(false);
        } else {
            QTextStream in(&file);
            QString scriptContent = in.readAll();
            file.close();
            codeEditor->setPlainText(scriptContent);
            okButton->setEnabled(true);
            Console::log("Loaded script content from: " + filePath.toStdString());
        }
    }

    setLayout(mainLayout);
    Console::log("TestScriptDialog setup complete");
}

TestScriptDialog::~TestScriptDialog()
{
    Console::log("Destroying TestScriptDialog");
}

void TestScriptDialog::onRunButtonClicked()
{
    if (!codeEditor || !scriptNameEdit || !scriptTypeCombo || !scriptPathEdit) {
        Console::error("Invalid widget pointer in onRunButtonClicked");
        return;
    }
    QString code = codeEditor->toPlainText();
    QString scriptName = scriptNameEdit->text();
    QString scriptType = scriptTypeCombo->currentText();
    QString scriptPath = scriptPathEdit->text();
    Console::log("Run button clicked. Script Name: " + scriptName.toStdString() +
                 ", Type: " + scriptType.toStdString() +
                 ", Path: " + scriptPath.toStdString() +
                 ", Content:\n" + code.toStdString());
}

void TestScriptDialog::onBrowseButtonClicked()
{
    if (!scriptPathEdit || !okButton) {
        Console::error("Invalid widget pointer in onBrowseButtonClicked");
        return;
    }
    QString folderPath = QFileDialog::getExistingDirectory(this, tr("Select Folder for Script"),
                                                           QDir::homePath());
    if (!folderPath.isEmpty()) {
        scriptPathEdit->setText(folderPath);
        Console::log("Selected folder path: " + folderPath.toStdString());
        QFileInfo folderInfo(folderPath);
        okButton->setEnabled(folderInfo.exists() && folderInfo.isDir() && folderInfo.isWritable());
    }
}

void TestScriptDialog::onScriptPathChanged()
{
    if (!scriptPathEdit || !okButton) {
        Console::error("Invalid widget pointer in onScriptPathChanged");
        return;
    }
    QString folderPath = scriptPathEdit->text();
    if (folderPath.isEmpty()) {
        okButton->setEnabled(false);
    } else {
        QFileInfo folderInfo(folderPath);
        okButton->setEnabled(folderInfo.exists() && folderInfo.isDir() && folderInfo.isWritable());
    }
}

void TestScriptDialog::onOkButtonClicked()
{
    if (!codeEditor) {
        Console::error("Invalid widget pointer in onOkButtonClicked");
        return;
    }

    QString filePath;
    if (isEditMode) {
        filePath = editFilePath;
    } else {
        if (!scriptNameEdit || !scriptPathEdit || !scriptTypeCombo) {
            Console::error("Invalid widget pointer in onOkButtonClicked");
            return;
        }
        QString scriptName = scriptNameEdit->text();
        QString scriptType = scriptTypeCombo->currentText();
        QString folderPath = scriptPathEdit->text();

        if (scriptName.isEmpty() || folderPath.isEmpty()) {
            Console::error("Script name or folder path is empty");
            return;
        }

        filePath = QDir(folderPath).filePath(scriptName + ".as");
    }

    QFileInfo fileInfo(filePath);
    if (!isEditMode) {
        QFileInfo folderInfo(fileInfo.absolutePath());
        if (!folderInfo.exists() || !folderInfo.isDir()) {
            Console::error("Folder does not exist: " + fileInfo.absolutePath().toStdString());
            return;
        }
        if (!folderInfo.isWritable()) {
            Console::error("Folder is not writable: " + fileInfo.absolutePath().toStdString());
            return;
        }
    }

    if (fileInfo.exists() && !fileInfo.isWritable()) {
        Console::error("File is not writable: " + filePath.toStdString());
        return;
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << codeEditor->toPlainText();
        file.close();
        Console::log("Script saved successfully to: " + filePath.toStdString());
    } else {
        Console::error("Failed to save script to: " + filePath.toStdString());
        return;
    }

    if (!isEditMode) {
        QString scriptName = scriptNameEdit->text();
        QString scriptType = scriptTypeCombo->currentText();
        Console::log("OK button clicked. Saved script: " + scriptName.toStdString() +
                     ", Type: " + scriptType.toStdString() +
                     ", Path: " + filePath.toStdString());
    } else {
        Console::log("OK button clicked. Updated script at: " + filePath.toStdString());
    }
    accept();
}

void TestScriptDialog::onCancelButtonClicked()
{
    Console::log("Cancel button clicked in dialog box");
    reject();
}
