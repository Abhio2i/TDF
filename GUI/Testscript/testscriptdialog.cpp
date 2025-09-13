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
#include <QDir>
#include <QMessageBox>
#include <QCoreApplication>

TestScriptDialog::TestScriptDialog(QWidget *parent, bool editMode, const QString &filePath)
    : QWidget(parent), isEditMode(editMode), editFilePath(filePath)
{
    Console::log("Creating TestScriptDialog with parent: " + std::string(parent ? parent->objectName().toStdString() : "null") +
                 ", editMode: " + std::string(editMode ? "true" : "false"));
    setWindowTitle(tr("Test Script Dialog"));
    setMinimumSize(600, 400);
    setWindowFlags(Qt::Window);
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    Console::log("Main layout created for window");
    QVBoxLayout *leftLayout = new QVBoxLayout();

    // Script Name ComboBox
    scriptNameCombo = new QComboBox(this);
    scriptNameCombo->setObjectName("scriptNameCombo");
    scriptNameCombo->setEditable(true);
    scriptNameLabel = new QLabel(tr("Script Name:"), this);
    scriptNameCombo->setPlaceholderText(tr("Enter script name or select..."));
    Console::log("Using native styling for scriptNameCombo to match scriptTypeCombo");

    QString projectDir = QCoreApplication::applicationDirPath() + "/../..";
    QString testScriptPath = QDir(projectDir).absoluteFilePath("Testscript");
    QDir testScriptDir(testScriptPath);
    if (!testScriptDir.exists()) {
        if (testScriptDir.mkpath(testScriptPath)) {
            Console::log("Created Testscript folder at: " + testScriptPath.toStdString());
        } else {
            Console::error("Failed to create Testscript folder at: " + testScriptPath.toStdString());
            QMessageBox::warning(this, tr("Folder Error"),
                                 tr("Testscript folder not found and could not be created at: %1").arg(testScriptPath));
        }
    }

    // Populate with .as files
    if (testScriptDir.exists()) {
        QStringList filters;
        filters << "*.as";
        testScriptDir.setNameFilters(filters);
        QStringList fileList = testScriptDir.entryList(QDir::Files, QDir::Name);
        scriptNameCombo->addItems(fileList);
        Console::log("Populated scriptNameCombo with " + std::to_string(fileList.size()) + " .as files from " + testScriptPath.toStdString());
    } else {
        Console::error("Testscript folder not found at: " + testScriptPath.toStdString());
    }

    leftLayout->addWidget(scriptNameLabel);
    leftLayout->addWidget(scriptNameCombo);
    Console::log("Script name widgets created");

    // Script Type ComboBox
    scriptTypeCombo = new QComboBox(this);
    scriptTypeCombo->setObjectName("scriptTypeCombo");
    scriptTypeLabel = new QLabel(tr("Script Type:"), this);
    scriptTypeCombo->addItems({"Entity Behaviour", "Sensors Script", "Radar Script", "Editor Script"});
    Console::log("Script type combo created with native styling");
    leftLayout->addWidget(scriptTypeLabel);
    leftLayout->addWidget(scriptTypeCombo);
    Console::log("Script type widgets created");

    // Script Path
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

    // Run Button
    runButton = new QPushButton(QIcon(":/icons/images/run.png"), tr("Run"), this);
    runButton->setFixedWidth(100);
    leftLayout->addStretch();
    leftLayout->addWidget(runButton);
    Console::log("Run button created");

    // Right Layout
    QVBoxLayout *rightLayout = new QVBoxLayout();
    codeEditor = new QTextEdit(this);
    codeEditor->setObjectName("codeEditor");
    codeEditor->setPlaceholderText(tr("Enter your script here..."));
    codeEditor->setFont(QFont("Courier New", 10));
    codeEditor->setText(
        "void main(ScriptEngine@ e) {\n"
        " Print('Before');\n"
        " ProfileCategaory@ d = e.NewProfile('hello');\n"
        " Platform@ m = e.addEntity(d.id, 'entity',true);\n"
        " for(int i=0;i<1000;i++){\n"
        " m.transform.position.x +=0.1;\n"
        " e.sleep(10);\n"
        " e.render();\n"
        " } \n"
        " Print('After'); \n"
        "}\n");
    rightLayout->addWidget(codeEditor);
    Console::log("Code editor created");

    // OK and Cancel Buttons
    okButton = new QPushButton(tr("OK"), this);
    okButton->setEnabled(!editMode);
    cancelButton = new QPushButton(tr("Cancel"), this);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    rightLayout->addLayout(buttonLayout);
    Console::log("OK and Cancel buttons created");

    // Combine layouts
    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addLayout(rightLayout, 3);

    // Connect signals
    connect(runButton, &QPushButton::clicked, this, &TestScriptDialog::onRunButtonClicked);
    connect(browseButton, &QPushButton::clicked, this, &TestScriptDialog::onBrowseButtonClicked);
    connect(okButton, &QPushButton::clicked, this, &TestScriptDialog::onOkButtonClicked);
    connect(cancelButton, &QPushButton::clicked, this, &TestScriptDialog::onCancelButtonClicked);
    connect(scriptPathEdit, &QLineEdit::textChanged, this, &TestScriptDialog::onScriptPathChanged);
    connect(scriptNameCombo, &QComboBox::currentTextChanged, this, &TestScriptDialog::onScriptNameChanged);
    Console::log("Signals connected for window, including scriptNameCombo currentTextChanged");

    // Icon checks
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

    // Edit mode handling
    if (isEditMode) {
        scriptNameLabel->hide();
        scriptNameCombo->hide();
        scriptTypeLabel->hide();
        scriptTypeCombo->hide();
        scriptPathLabel->hide();
        scriptPathEdit->hide();
        browseButton->hide();
        Console::log("Hiding script name, type, and path fields for edit mode");
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            Console::error("Failed to open file for editing: " + filePath.toStdString());
            okButton->setEnabled(false);
        } else {
            QTextStream in(&file);
            QString scriptContent = in.readAll();
            file.close();
            codeEditor->setPlainText(scriptContent);
            okButton->setEnabled(true);
            Console::log("Loaded script content from: " + filePath.toStdString());
        }
    } else {
        // Set default path to /Testscript
        scriptPathEdit->setText(testScriptPath);
        onScriptPathChanged();
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
    if (!codeEditor || !scriptNameCombo || !scriptTypeCombo || !scriptPathEdit) {
        Console::error("Invalid widget pointer in onRunButtonClicked");
        return;
    }
    QString code = codeEditor->toPlainText();
    QString scriptName = scriptNameCombo->currentText();
    QString scriptType = scriptTypeCombo->currentText();
    QString scriptPath = scriptPathEdit->text();
    Console::log("Run button clicked. Script Name: " + scriptName.toStdString() +
                 ", Type: " + scriptType.toStdString() +
                 ", Path: " + scriptPath.toStdString() +
                 ", Content:\n" + code.toStdString());
    emit runScriptstring(code);
}

void TestScriptDialog::onBrowseButtonClicked()
{
    if (!scriptPathEdit || !okButton) {
        Console::error("Invalid widget pointer in onBrowseButtonClicked");
        return;
    }
    QString projectDir = QCoreApplication::applicationDirPath() + "/../..";
    QString folderPath = QFileDialog::getExistingDirectory(this, tr("Select Folder for Script"),
                                                           projectDir);
    if (!folderPath.isEmpty()) {
        scriptPathEdit->setText(folderPath);
        Console::log("Selected folder path: " + folderPath.toStdString());
        QFileInfo folderInfo(folderPath);
        okButton->setEnabled(folderInfo.exists() && folderInfo.isDir() && folderInfo.isWritable());

        // Update scriptNameCombo with .as files
        scriptNameCombo->clear();
        QDir selectedDir(folderPath);
        if (selectedDir.exists()) {
            QStringList filters;
            filters << "*.as";
            selectedDir.setNameFilters(filters);
            QStringList fileList = selectedDir.entryList(QDir::Files, QDir::Name);
            scriptNameCombo->addItems(fileList);
            Console::log("Populated scriptNameCombo with " + std::to_string(fileList.size()) + " .as files from " + folderPath.toStdString());
        } else {
            Console::error("Selected folder not found: " + folderPath.toStdString());
        }
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

void TestScriptDialog::onScriptNameChanged(const QString &text)
{
    if (!codeEditor || !scriptPathEdit) {
        Console::error("Invalid widget pointer in onScriptNameChanged");
        return;
    }
    if (text.isEmpty() || !text.endsWith(".as")) {
        // Clear codeEditor if no valid .as file is selected
        codeEditor->clear();
        Console::log("Cleared codeEditor due to empty or non-.as selection");
        return;
    }
    QString filePath = QDir(scriptPathEdit->text()).filePath(text);
    QFile file(filePath);
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString scriptContent = in.readAll();
            file.close();
            codeEditor->setPlainText(scriptContent);
            Console::log("Loaded script content into codeEditor from: " + filePath.toStdString());
        } else {
            Console::error("Failed to open file for reading: " + filePath.toStdString());
            QMessageBox::warning(this, tr("File Error"),
                                 tr("Failed to open file: %1").arg(filePath));
            codeEditor->clear();
        }
    } else {
        // Clear codeEditor if file doesn't exist (e.g., new script name)
        codeEditor->clear();
        Console::log("Cleared codeEditor as file does not exist: " + filePath.toStdString());
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
        if (!scriptNameCombo || !scriptPathEdit || !scriptTypeCombo) {
            Console::error("Invalid widget pointer in onOkButtonClicked");
            return;
        }
        QString scriptName = scriptNameCombo->currentText();
        QString scriptType = scriptTypeCombo->currentText();
        QString folderPath = scriptPathEdit->text();
        if (scriptName.isEmpty() || folderPath.isEmpty()) {
            Console::error("Script name or folder path is empty");
            return;
        }
        if (!scriptName.endsWith(".as")) {
            scriptName += ".as";
        }
        filePath = QDir(folderPath).filePath(scriptName);
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
        QString scriptName = scriptNameCombo->currentText();
        QString scriptType = scriptTypeCombo->currentText();
        Console::log("OK button clicked. Saved script: " + scriptName.toStdString() +
                     ", Type: " + scriptType.toStdString() +
                     ", Path: " + filePath.toStdString());
    } else {
        Console::log("OK button clicked. Updated script at: " + filePath.toStdString());
    }
    emit closed();
    close();
}

void TestScriptDialog::onCancelButtonClicked()
{
    emit closed();
    close();
}
