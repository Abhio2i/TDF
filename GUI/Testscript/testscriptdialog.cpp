/* ========================================================================= */
/* File: testscriptdialog.cpp                                             */
/* Purpose: Implements dialog for editing and managing AngelScript files   */
/* ========================================================================= */

#include "testscriptdialog.h"                      // For test script dialog class
#include "core/Debug/console.h"                    // For console logging
#include "qabstractitemview.h"                     // For abstract item view
#include <QIcon>                                   // For icon handling
#include <QHBoxLayout>                             // For horizontal layout
#include <QVBoxLayout>                             // For vertical layout
#include <QFileDialog>                             // For file dialog
#include <QLabel>                                  // For label widget
#include <QTextStream>                             // For file text streaming
#include <QFile>                                   // For file operations
#include <QFileInfo>                               // For file information
#include <QDir>                                    // For directory handling
#include <QMessageBox>                             // For message box
#include <QCoreApplication>                        // For application paths
#include <QLineEdit>                               // For line edit widget
#include <QPainter>                                // For painting operations
#include <QScrollBar>                              // For scroll bar
#include <QPalette>                                // For color palette
#include <QAbstractTextDocumentLayout>             // For text document layout

// %%% Line Number Area %%%
/* Initialize line number area for code editor */
TestScriptDialog::LineNumberArea::LineNumberArea(QTextEdit *editor)
    : QWidget(editor), codeEditor(editor)
{
    // Set fixed width for line numbers
    setFixedWidth(lineNumberAreaWidth());
    Console::log("LineNumberArea created for codeEditor");
}

/* Calculate width for line number area */
int TestScriptDialog::LineNumberArea::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, codeEditor->document()->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    // Calculate space based on digit count
    int space = 3 + codeEditor->fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

/* Paint line numbers in the area */
void TestScriptDialog::LineNumberArea::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), QColor(30, 30, 30));
    painter.setFont(codeEditor->font());
    painter.setPen(QColor(150, 150, 150));
    // Get document and viewport bounds
    QTextDocument *doc = codeEditor->document();
    QTextBlock block = doc->begin();
    int top = codeEditor->viewport()->rect().top();
    int bottom = codeEditor->viewport()->rect().bottom();
    int lineNumber = 1;
    // Draw line numbers for visible blocks
    while (block.isValid()) {
        QRectF blockRect = doc->documentLayout()->blockBoundingRect(block);
        QTextCursor cursor(block);
        QRect cursorRect = codeEditor->cursorRect(cursor);
        int blockTop = cursorRect.top();
        if (blockTop + blockRect.height() >= top && blockTop <= bottom) {
            QString number = QString::number(lineNumber);
            painter.drawText(0, blockTop, width(), blockRect.height(),
                             Qt::AlignRight | Qt::AlignVCenter, number);
        }
        block = block.next();
        ++lineNumber;
    }
}

// %%% Constructor %%%
/* Initialize test script dialog */
TestScriptDialog::TestScriptDialog(QWidget *parent, bool editMode, const QString &filePath)
    : QWidget(parent), editFilePath(filePath), isEditMode(editMode), isNewScript(false)
{
    // Log initialization
    Console::log("Creating TestScriptDialog with parent: " +
                 std::string(parent ? parent->objectName().toStdString() : "null") +
                 ", editMode: " + std::string(editMode ? "true" : "false"));
    Console::log("Qt Version: " + std::string(qVersion()));
    // Configure window
    setWindowTitle(tr("Test Script Dialog"));
    setMinimumSize(600, 400);
    setWindowFlags(Qt::Window);
    setStyleSheet("TestScriptDialog { background-color: #1E1E1E; }");
    // Set up main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    Console::log("Main vertical layout created for window");
    // Create top layout
    QHBoxLayout *topLayout = new QHBoxLayout();
    // Create new script button
    newScriptButton = new QPushButton(QIcon(":/icons/images/add.png"), tr("New Script"), this);
    newScriptButton->setFixedWidth(100);
    newScriptButton->setStyleSheet(
        "QPushButton {"
        "   border: 1px solid #707070;"
        "   background-color: #404040;"
        "   padding: 5px;"
        "   color: white;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #505050;"
        "   color: white;"
        "   font-weight: bold;"
        "}");
    topLayout->addWidget(newScriptButton);
    Console::log("New Script button created");
    // Create script name combo
    scriptNameCombo = new QComboBox(this);
    scriptNameCombo->setObjectName("scriptNameCombo");
    scriptNameCombo->setEditable(false);
    Console::log("Using native styling for scriptNameCombo");
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
    scriptNameCombo->addItem(tr("Sample Script"));
    if (testScriptDir.exists()) {
        QStringList filters;
        filters << "*.as";
        testScriptDir.setNameFilters(filters);
        QStringList fileList = testScriptDir.entryList(QDir::Files, QDir::Name);
        scriptNameCombo->addItems(fileList);
        Console::log("Populated scriptNameCombo with " + std::to_string(fileList.size()) +
                     " .as files from " + testScriptPath.toStdString());
    } else {
        Console::error("Testscript folder not found at: " + testScriptPath.toStdString());
    }
    scriptNameCombo->setCurrentIndex(0);
    topLayout->addWidget(scriptNameCombo);
    Console::log("Script name combo created");
    // Create script type combo
    scriptTypeCombo = new QComboBox(this);
    scriptTypeCombo->setObjectName("scriptTypeCombo");
    scriptTypeCombo->addItems({"Entity Behaviour", "Sensors Script", "Radar Script", "Editor Script"});
    Console::log("Script type combo created with native styling");
    topLayout->addWidget(scriptTypeCombo);
    Console::log("Script type combo added");
    // Create script path edit and load button
    scriptPathEdit = new QLineEdit(this);
    scriptPathEdit->setObjectName("scriptPathEdit");
    scriptPathEdit->setPlaceholderText(tr("Enter or select folder path..."));
    scriptPathEdit->setVisible(false);
    loadScriptButton = new QPushButton(tr("Load Script"), this);
    loadScriptButton->setFixedWidth(100);
    loadScriptButton->setStyleSheet(
        "QPushButton {"
        "   border: 1px solid #707070;"
        "   background-color: #404040;"
        "   padding: 5px;"
        "   color: white;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #505050;"
        "   color: white;"
        "   font-weight: bold;"
        "}");
    topLayout->addWidget(loadScriptButton);
    topLayout->addWidget(scriptPathEdit);
    Console::log("Load Script button (text-based) and hidden path edit created");
    // Create run button
    runButton = new QPushButton(QIcon(":/icons/images/play.png"), "", this);
    runButton->setFixedSize(30, 30);
    topLayout->addWidget(runButton);
    topLayout->addStretch();
    Console::log("Run button with icon created");
    mainLayout->addLayout(topLayout);
    // Create bottom layout
    QVBoxLayout *bottomLayout = new QVBoxLayout();
    QHBoxLayout *editorLayout = new QHBoxLayout();
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);
    // Configure code editor
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
    // Set up syntax highlighter and completer
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
    // Create OK and Cancel buttons
    okButton = new QPushButton(tr("Save"), this);
    okButton->setEnabled(!editMode);
    cancelButton = new QPushButton(tr("Cancel"), this);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    bottomLayout->addLayout(buttonLayout);
    Console::log("OK and Cancel buttons created");
    mainLayout->addLayout(bottomLayout);
    // Connect signals
    connect(newScriptButton, SIGNAL(clicked()), this, SLOT(onNewScriptButtonClicked()));
    connect(runButton, SIGNAL(clicked()), this, SLOT(onRunButtonClicked()));
    connect(loadScriptButton, SIGNAL(clicked()), this, SLOT(onLoadScriptButtonClicked()));
    connect(okButton, SIGNAL(clicked()), this, SLOT(onOkButtonClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(onCancelButtonClicked()));
    connect(scriptPathEdit, SIGNAL(textChanged(const QString &)), this, SLOT(onScriptPathChanged()));
    connect(scriptNameCombo, SIGNAL(currentTextChanged(const QString &)), this, SLOT(onScriptNameChanged(const QString &)));
    Console::log("Signals connected for window, including newScriptButton and loadScriptButton");
    // Verify icons
    if (runButton->icon().isNull()) {
        Console::error("Failed to load run icon from :/icons/images/play.png");
    } else {
        Console::log("Run icon loaded successfully");
    }
    if (newScriptButton->icon().isNull()) {
        Console::error("Failed to load add icon from :/icons/images/add.png");
    } else {
        Console::log("Add icon loaded successfully for newScriptButton");
    }
    // Handle edit mode
    if (isEditMode) {
        newScriptButton->hide();
        scriptNameCombo->hide();
        scriptTypeCombo->hide();
        loadScriptButton->hide();
        scriptPathEdit->hide();
        Console::log("Hiding script name, type, path fields, and load script button for edit mode");
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

// %%% Destructor %%%
/* Clean up test script dialog */
TestScriptDialog::~TestScriptDialog()
{
    Console::log("Destroying TestScriptDialog");
}

// %%% Line Number Management %%%
/* Update line number area width and repaint */
void TestScriptDialog::updateLineNumberArea()
{
    lineNumberArea->setFixedWidth(lineNumberArea->lineNumberAreaWidth());
    lineNumberArea->update();
}

// %%% Button Handlers %%%
/* Handle new script button click */
void TestScriptDialog::onNewScriptButtonClicked()
{
    isNewScript = true;
    scriptNameCombo->setCurrentIndex(0);
    codeEditor->clear();
    okButton->setEnabled(true);
    Console::log("New Script button clicked, cleared code editor and reset script selection to 'Sample Script'");
}

/* Handle run button click */
void TestScriptDialog::onRunButtonClicked()
{
    if (!codeEditor || !scriptNameCombo || !scriptTypeCombo || !scriptPathEdit) {
        Console::error("Invalid widget pointer in onRunButtonClicked");
        return;
    }
    QString code = codeEditor->toPlainText();
    QString scriptName = isNewScript || scriptNameCombo->currentText() == tr("Sample Script") ? "NewScript" : scriptNameCombo->currentText();
    QString scriptType = scriptTypeCombo->currentText();
    QString scriptPath = scriptPathEdit->text();
    Console::log("Run button clicked. Script Name: " + scriptName.toStdString() +
                 ", Type: " + scriptType.toStdString() +
                 ", Path: " + scriptPath.toStdString() +
                 ", Content:\n" + code.toStdString());
    emit runScriptstring(code);
}

/* Handle load script button click */
void TestScriptDialog::onLoadScriptButtonClicked()
{
    if (!scriptPathEdit || !codeEditor || !scriptNameCombo || !okButton) {
        Console::error("Invalid widget pointer in onLoadScriptButtonClicked");
        return;
    }
    QString projectDir = QCoreApplication::applicationDirPath() + "/../..";
    QString filePath = QFileDialog::getOpenFileName(this, tr("Load Script"),
                                                    projectDir,
                                                    tr("AngelScript Files (*.as)"));
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString scriptContent = in.readAll();
            file.close();
            codeEditor->setPlainText(scriptContent);
            QFileInfo fileInfo(filePath);
            scriptPathEdit->setText(fileInfo.absolutePath());
            scriptNameCombo->setCurrentIndex(-1);
            QString fileName = fileInfo.fileName();
            int index = scriptNameCombo->findText(fileName);
            if (index == -1) {
                scriptNameCombo->addItem(fileName);
                index = scriptNameCombo->findText(fileName);
            }
            scriptNameCombo->setCurrentIndex(index);
            okButton->setEnabled(fileInfo.isWritable());
            Console::log("Loaded script from: " + filePath.toStdString() +
                         ", updated scriptNameCombo to: " + fileName.toStdString());
            isNewScript = false;
        } else {
            Console::error("Failed to open file for reading: " + filePath.toStdString());
            QMessageBox::warning(this, tr("File Error"),
                                 tr("Failed to open file: %1").arg(filePath));
            scriptNameCombo->setCurrentIndex(0);
        }
    } else {
        Console::log("Load script dialog canceled by user");
    }
}

/* Handle script path changes */
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

/* Handle script name changes */
void TestScriptDialog::onScriptNameChanged(const QString &text)
{
    if (!codeEditor || !scriptPathEdit) {
        Console::error("Invalid widget pointer in onScriptNameChanged");
        return;
    }
    if (text == tr("Sample Script")) {
        isNewScript = false;
        codeEditor->clear();
        scriptNameCombo->setCurrentIndex(0);
        Console::log("Selected 'Sample Script', cleared codeEditor");
        return;
    }
    isNewScript = false;
    QString filePath = QDir(scriptPathEdit->text()).filePath(text);
    QFile file(filePath);
    if (file.exists() && text.endsWith(".as")) {
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString scriptContent = in.readAll();
            file.close();
            codeEditor->setPlainText(scriptContent);
            okButton->setEnabled(QFileInfo(file).isWritable());
            Console::log("Loaded script content into codeEditor from: " + filePath.toStdString());
        } else {
            Console::error("Failed to open file for reading: " + filePath.toStdString());
            QMessageBox::warning(this, tr("File Error"),
                                 tr("Failed to open file: %1").arg(filePath));
            codeEditor->clear();
            scriptNameCombo->setCurrentIndex(0);
        }
    } else {
        Console::log("Selected script name: " + text.toStdString() + ", no file loaded");
        codeEditor->clear();
        scriptNameCombo->setCurrentIndex(0);
    }
}

/* Handle OK button click to save script */
void TestScriptDialog::onOkButtonClicked()
{
    if (!codeEditor) {
        Console::error("Invalid widget pointer in onOkButtonClicked");
        return;
    }
    QString filePath;
    if (isEditMode) {
        filePath = editFilePath;
    } else if (isNewScript) {
        QString projectDir = QCoreApplication::applicationDirPath() + "/../..";
        filePath = QFileDialog::getSaveFileName(this, tr("Save New Script"),
                                                QDir(projectDir).filePath("NewScript.as"),
                                                tr("AngelScript Files (*.as)"));
        if (filePath.isEmpty()) {
            Console::log("Save file dialog canceled by user");
            return;
        }
        scriptPathEdit->setText(QFileInfo(filePath).absolutePath());
    } else {
        if (!scriptNameCombo || !scriptPathEdit || !scriptTypeCombo) {
            Console::error("Invalid widget pointer in onOkButtonClicked");
            return;
        }
        QString scriptName = scriptNameCombo->currentText().trimmed();
        QString scriptType = scriptTypeCombo->currentText().trimmed();
        QString folderPath = scriptPathEdit->text();
        if (scriptName == tr("Sample Script") || scriptName.isEmpty()) {
            Console::error("No valid script name selected");
            QMessageBox::warning(this, tr("Input Error"),
                                 tr("Please select a valid script or create a new one."));
            return;
        }
        if (!scriptName.endsWith(".as")) {
            scriptName += ".as";
            scriptNameCombo->setCurrentText(scriptName);
        }
        if (folderPath.isEmpty()) {
            Console::error("Folder path is empty");
            QMessageBox::warning(this, tr("Input Error"),
                                 tr("Please provide a folder path."));
            return;
        }
        filePath = QDir(folderPath).filePath(scriptName);
    }
    QFileInfo fileInfo(filePath);
    if (!isEditMode) {
        QFileInfo folderInfo(fileInfo.absolutePath());
        if (!folderInfo.exists() || !folderInfo.isDir()) {
            Console::error("Folder does not exist: " + fileInfo.absolutePath().toStdString());
            QMessageBox::warning(this, tr("Folder Error"),
                                 tr("Folder does not exist: %1").arg(fileInfo.absolutePath()));
            return;
        }
        if (!folderInfo.isWritable()) {
            Console::error("Folder is not writable: " + fileInfo.absolutePath().toStdString());
            QMessageBox::warning(this, tr("Permission Error"),
                                 tr("Folder is not writable: %1").arg(fileInfo.absolutePath()));
            return;
        }
    }
    if (fileInfo.exists() && !fileInfo.isWritable()) {
        Console::error("File is not writable: " + filePath.toStdString());
        QMessageBox::warning(this, tr("Permission Error"),
                             tr("File is not writable: %1").arg(filePath));
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
        QMessageBox::warning(this, tr("File Error"),
                             tr("Failed to save script to: %1").arg(filePath));
        return;
    }
    if (!isEditMode) {
        QString scriptName = isNewScript ? QFileInfo(filePath).fileName() : scriptNameCombo->currentText();
        QString scriptType = scriptTypeCombo->currentText();
        Console::log("OK button clicked. Saved script: " + scriptName.toStdString() +
                     ", Type: " + scriptType.toStdString() +
                     ", Path: " + filePath.toStdString());
        if (isNewScript) {
            scriptNameCombo->clear();
            scriptNameCombo->addItem(tr("Sample Script"));
            QDir selectedDir(QFileInfo(filePath).absolutePath());
            QStringList filters;
            filters << "*.as";
            selectedDir.setNameFilters(filters);
            QStringList fileList = selectedDir.entryList(QDir::Files, QDir::Name);
            scriptNameCombo->addItems(fileList);
            scriptNameCombo->setCurrentIndex(0);
            Console::log("Refreshed scriptNameCombo with " + std::to_string(fileList.size()) +
                         " .as files from " + QFileInfo(filePath).absolutePath().toStdString());
            isNewScript = false;
        }
    } else {
        Console::log("OK button clicked. Updated script at: " + filePath.toStdString());
    }
    emit closed();
    close();
}

/* Handle Cancel button click */
void TestScriptDialog::onCancelButtonClicked()
{
    isNewScript = false;
    emit closed();
    close();
}

// %%% Event Handling %%%
/* Handle key press events for completer */
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

/* Insert autocompletion text */
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

/* Handle text changes for autocompletion */
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
