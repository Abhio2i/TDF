#include "testscriptdialog.h"
#include "core/Debug/console.h"
#include "qabstractitemview.h"
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
#include <QLineEdit>
#include <QPainter>
#include <QScrollBar>
#include <QPalette>

TestScriptDialog::LineNumberArea::LineNumberArea(QTextEdit *editor) : QWidget(editor), codeEditor(editor)
{
    setFixedWidth(lineNumberAreaWidth());
    Console::log("LineNumberArea created for codeEditor");
}

int TestScriptDialog::LineNumberArea::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, codeEditor->document()->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = 3 + codeEditor->fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void TestScriptDialog::LineNumberArea::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), QColor(30, 30, 30));
    painter.setFont(codeEditor->font());
    painter.setPen(QColor(150, 150, 150));
    int lineHeight = codeEditor->fontMetrics().height();
    int blockCount = codeEditor->document()->blockCount();
    QTextCursor cursor = codeEditor->textCursor();
    int scrollOffset = codeEditor->verticalScrollBar()->value();
    for (int i = 0; i < blockCount; ++i) {
        int y = i * lineHeight - scrollOffset * lineHeight;
        if (y + lineHeight < event->rect().top() || y > event->rect().bottom())
            continue;
        QString number = QString::number(i + 1);
        painter.drawText(0, y, width(), lineHeight, Qt::AlignRight | Qt::AlignVCenter, number);
    }
}

TestScriptDialog::TestScriptDialog(QWidget *parent, bool editMode, const QString &filePath)
    : QWidget(parent), editFilePath(filePath), isEditMode(editMode)
{
    Console::log("Creating TestScriptDialog with parent: " + std::string(parent ? parent->objectName().toStdString() : "null") +
                 ", editMode: " + std::string(editMode ? "true" : "false"));
    Console::log("Qt Version: " + std::string(qVersion()));
    setWindowTitle(tr("Test Script Dialog"));
    setMinimumSize(600, 400);
    setWindowFlags(Qt::Window);
    setStyleSheet("TestScriptDialog { background-color: #1E1E1E; }"); // Consistent dark theme

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    Console::log("Main vertical layout created for window");

    QHBoxLayout *topLayout = new QHBoxLayout();

    scriptNameCombo = new QComboBox(this);
    scriptNameCombo->setObjectName("scriptNameCombo");
    scriptNameCombo->setEditable(true);
    scriptNameLabel = new QLabel(tr("Script Name:"), this);

    // Set text color to white for scriptNameLabel
    QPalette labelPalette = scriptNameLabel->palette();
    labelPalette.setColor(QPalette::WindowText, Qt::white);
    scriptNameLabel->setPalette(labelPalette);

    scriptNameCombo->setPlaceholderText(tr("Enter script name or select..."));
    QLineEdit *comboLineEdit = scriptNameCombo->lineEdit();
    if (comboLineEdit) {
        comboLineEdit->setPlaceholderText(tr("Enter script name or select..."));
        Console::log("Set placeholder text on QComboBox's QLineEdit");
    } else {
        Console::error("Failed to access QLineEdit for scriptNameCombo");
    }
    scriptNameCombo->setCurrentText("");
    scriptNameCombo->setCurrentIndex(-1);
    scriptNameCombo->setStyleSheet("QComboBox#scriptNameCombo QLineEdit { color: black; } QComboBox#scriptNameCombo QLineEdit:placeholder { color: grey; }");
    Console::log("Using native styling for scriptNameCombo with enhanced placeholder fix");
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
    if (testScriptDir.exists()) {
        QStringList filters;
        filters << "*.as";
        testScriptDir.setNameFilters(filters);
        QStringList fileList = testScriptDir.entryList(QDir::Files, QDir::Name);
        scriptNameCombo->addItems(fileList);
        scriptNameCombo->setCurrentIndex(-1);
        scriptNameCombo->clearEditText();
        Console::log("Populated scriptNameCombo with " + std::to_string(fileList.size()) + " .as files from " + testScriptPath.toStdString());
    } else {
        Console::error("Testscript folder not found at: " + testScriptPath.toStdString());
    }
    topLayout->addWidget(scriptNameLabel);
    topLayout->addWidget(scriptNameCombo);
    Console::log("Script name label and combo created");

    scriptTypeCombo = new QComboBox(this);
    scriptTypeCombo->setObjectName("scriptTypeCombo");
    scriptTypeCombo->addItems({"Entity Behaviour", "Sensors Script", "Radar Script", "Editor Script"});
    Console::log("Script type combo created with native styling");
    topLayout->addWidget(scriptTypeCombo);
    Console::log("Script type combo added (no label)");

    scriptPathEdit = new QLineEdit(this);
    scriptPathEdit->setObjectName("scriptPathEdit");
    scriptPathEdit->setPlaceholderText(tr("Enter or select folder path..."));
    scriptPathEdit->setVisible(false);
    browseButton = new QPushButton(QIcon(":/icons/images/folder.png"), "", this);
    browseButton->setFixedSize(30, 30);
    topLayout->addWidget(browseButton);
    topLayout->addWidget(scriptPathEdit);
    Console::log("Browse icon and edit created");

    runButton = new QPushButton(QIcon(":/icons/images/play.png"), "", this);
    runButton->setFixedSize(30, 30);
    topLayout->addWidget(runButton);
    topLayout->addStretch();
    Console::log("Run button with icon created");

    mainLayout->addLayout(topLayout);

    QVBoxLayout *bottomLayout = new QVBoxLayout();
    QHBoxLayout *editorLayout = new QHBoxLayout();
    editorLayout->setContentsMargins(0, 0, 0, 0); // Remove margins
    editorLayout->setSpacing(0); // Remove spacing
    codeEditor = new QTextEdit(this);
    lineNumberArea = new LineNumberArea(codeEditor);
    QPalette p = codeEditor->palette();
    p.setColor(QPalette::Base, QColor(30, 30, 30));
    p.setColor(QPalette::Text, QColor(255, 255, 255));
    codeEditor->setPalette(p);
    codeEditor->setStyleSheet("QTextEdit { background-color: #1E1E1E; }");
    codeEditor->verticalScrollBar()->setStyleSheet("QScrollBar:vertical { background: #1E1E1E; }");
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
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(10);
    font.setWeight(QFont::DemiBold);
    codeEditor->setFont(font);
    QFontMetrics metrics(font);
    int spaceWidth = metrics.horizontalAdvance(' ');
    int tabStopWidth = 4 * spaceWidth;
    codeEditor->setTabStopDistance(tabStopWidth);
    highlighter = new AngelScriptHighlighter(codeEditor->document());
    wordListModel = new QStringListModel(this);
    completer = new QCompleter(this);
    completer->setModel(wordListModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWidget(codeEditor);
    connect(completer, SIGNAL(activated(const QString &)), this, SLOT(insertCompletion(const QString &)));
    connect(codeEditor, SIGNAL(textChanged()), this, SLOT(handleTextChanged()));
    connect(codeEditor, SIGNAL(textChanged()), this, SLOT(updateLineNumberArea()));
    connect(codeEditor, SIGNAL(cursorPositionChanged()), this, SLOT(updateLineNumberArea()));
    connect(codeEditor->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateLineNumberArea()));
    editorLayout->addWidget(lineNumberArea);
    editorLayout->addWidget(codeEditor);
    bottomLayout->addLayout(editorLayout);
    Console::log("Code editor with line numbers created");

    okButton = new QPushButton(tr("OK"), this);
    okButton->setEnabled(!editMode);
    cancelButton = new QPushButton(tr("Cancel"), this);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    bottomLayout->addLayout(buttonLayout);
    Console::log("OK and Cancel buttons created");

    mainLayout->addLayout(bottomLayout);

    connect(runButton, SIGNAL(clicked()), this, SLOT(onRunButtonClicked()));
    connect(browseButton, SIGNAL(clicked()), this, SLOT(onBrowseButtonClicked()));
    connect(okButton, SIGNAL(clicked()), this, SLOT(onOkButtonClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(onCancelButtonClicked()));
    connect(scriptPathEdit, SIGNAL(textChanged(const QString &)), this, SLOT(onScriptPathChanged()));
    connect(scriptNameCombo, SIGNAL(currentTextChanged(const QString &)), this, SLOT(onScriptNameChanged(const QString &)));
    Console::log("Signals connected for window, including scriptNameCombo currentTextChanged");

    if (runButton->icon().isNull()) {
        Console::error("Failed to load run icon from :/icons/images/play.png");
    } else {
        Console::log("Run icon loaded successfully");
    }
    if (browseButton->icon().isNull()) {
        Console::error("Failed to load browse icon from :/icons/images/folder.png");
    } else {
        Console::log("Browse icon loaded successfully");
    }

    if (isEditMode) {
        scriptNameLabel->hide();
        scriptNameCombo->hide();
        scriptTypeCombo->hide();
        browseButton->hide();
        scriptPathEdit->hide();
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
        scriptPathEdit->setText(testScriptPath);
        onScriptPathChanged();
    }

    scriptNameCombo->update();
    updateLineNumberArea();
    Console::log("TestScriptDialog setup complete");
}

TestScriptDialog::~TestScriptDialog()
{
    Console::log("Destroying TestScriptDialog");
}

void TestScriptDialog::updateLineNumberArea()
{
    lineNumberArea->setFixedWidth(lineNumberArea->lineNumberAreaWidth());
    lineNumberArea->update();
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
        scriptNameCombo->clear();
        QDir selectedDir(folderPath);
        if (selectedDir.exists()) {
            QStringList filters;
            filters << "*.as";
            selectedDir.setNameFilters(filters);
            QStringList fileList = selectedDir.entryList(QDir::Files, QDir::Name);
            scriptNameCombo->addItems(fileList);
            scriptNameCombo->setCurrentIndex(-1);
            scriptNameCombo->clearEditText();
            if (scriptNameCombo->lineEdit()) {
                scriptNameCombo->lineEdit()->setPlaceholderText(tr("Enter script name or select..."));
            }
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
    // Allow typing without clearing unless loading an existing file
    if (text.isEmpty()) {
        // Only clear editor if no text is entered
        codeEditor->clear();
        scriptNameCombo->setCurrentIndex(-1);
        scriptNameCombo->clearEditText();
        if (scriptNameCombo->lineEdit()) {
            scriptNameCombo->lineEdit()->setPlaceholderText(tr("Enter script name or select..."));
        }
        Console::log("Cleared codeEditor due to empty script name");
        return;
    }
    // Only attempt to load if the text matches an existing .as file
    QString filePath = QDir(scriptPathEdit->text()).filePath(text);
    QFile file(filePath);
    if (file.exists() && text.endsWith(".as")) {
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
            scriptNameCombo->setCurrentIndex(-1);
            scriptNameCombo->clearEditText();
            if (scriptNameCombo->lineEdit()) {
                scriptNameCombo->lineEdit()->setPlaceholderText(tr("Enter script name or select..."));
            }
        }
    } else {
        // Don't clear the editor for custom names; allow user to keep typing
        Console::log("Typed script name: " + text.toStdString() + ", no file loaded");
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
        QString scriptName = scriptNameCombo->currentText().trimmed();
        QString scriptType = scriptTypeCombo->currentText().trimmed();
        QString folderPath = scriptPathEdit->text();
        if (scriptName.isEmpty() || folderPath.isEmpty()) {
            Console::error("Script name or folder path is empty");
            QMessageBox::warning(this, tr("Input Error"), tr("Please provide a script name and folder path."));
            return;
        }
        if (!scriptName.endsWith(".as")) {
            scriptName += ".as";
            scriptNameCombo->setCurrentText(scriptName); // Update combo box to reflect .as extension
        }
        filePath = QDir(folderPath).filePath(scriptName);
    }
    QFileInfo fileInfo(filePath);
    if (!isEditMode) {
        QFileInfo folderInfo(fileInfo.absolutePath());
        if (!folderInfo.exists() || !folderInfo.isDir()) {
            Console::error("Folder does not exist: " + fileInfo.absolutePath().toStdString());
            QMessageBox::warning(this, tr("Folder Error"), tr("Folder does not exist: %1").arg(fileInfo.absolutePath()));
            return;
        }
        if (!folderInfo.isWritable()) {
            Console::error("Folder is not writable: " + fileInfo.absolutePath().toStdString());
            QMessageBox::warning(this, tr("Permission Error"), tr("Folder is not writable: %1").arg(fileInfo.absolutePath()));
            return;
        }
    }
    if (fileInfo.exists() && !fileInfo.isWritable()) {
        Console::error("File is not writable: " + filePath.toStdString());
        QMessageBox::warning(this, tr("Permission Error"), tr("File is not writable: %1").arg(filePath));
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
        QMessageBox::warning(this, tr("File Error"), tr("Failed to save script to: %1").arg(filePath));
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

void TestScriptDialog::keyPressEvent(QKeyEvent *event)
{
    if (completer && completer->popup()->isVisible()) {
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Tab:
        case Qt::Key_Space:
            event->ignore();
            return;
        case Qt::Key_Escape:
            completer->popup()->hide();
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void TestScriptDialog::insertCompletion(const QString &completion)
{
    if (completer->widget() != codeEditor)
        return;
    QTextCursor tc = codeEditor->textCursor();
    int extra = completion.length() - completer->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    codeEditor->setTextCursor(tc);
}

void TestScriptDialog::handleTextChanged()
{
    QTextCursor tc = codeEditor->textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    QString prefix = tc.selectedText();
    tc = codeEditor->textCursor();
    tc.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
    QString previousChar = tc.selectedText();
    QStringList wordList = {"for", "while", "class", "if", "else", "return", "new", "delete"};
    if (previousChar == ".") {
        QString objectName = codeEditor->document()->findBlock(tc.position()).text();
        objectName = objectName.left(tc.positionInBlock() - 1).trimmed();
        if (objectName.endsWith("myString")) {
            wordList.clear();
            wordList.append({"length", "find", "substring", "replace"});
        } else if (objectName.endsWith("myVector")) {
            wordList.clear();
            wordList.append({"add", "remove", "size", "clear"});
        } else if (objectName.endsWith("Player")) {
            wordList.clear();
            wordList.append({"attack", "move", "jump", "health"});
        } else {
            wordList.clear();
            wordList.append({"size", "type", "name"});
        }
        prefix = "";
    }
    wordListModel->setStringList(wordList);
    completer->setCompletionPrefix(prefix);
    completer->complete();
    if (prefix.length() < 2 && previousChar != ".") {
        completer->popup()->hide();
    }
}
