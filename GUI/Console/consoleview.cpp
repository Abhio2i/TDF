


//============================================================================
// File        : consoleview.cpp
// Description : Implements the ConsoleView class for handling multiple
//               console tabs (general, error, debug, warning, log) in Qt.
//============================================================================

#include "consoleview.h"
#include <QFont>
#include <QDateTime>
#include <QTextStream>

//============================================================================
// CLASS: ConsoleView
//============================================================================

/**
 * @brief Constructs the ConsoleView widget.
 * @param parent Pointer to parent QWidget.
 *
 * Initializes all console tabs, layouts, and buttons.
 */
ConsoleView::ConsoleView(QWidget *parent) : QWidget(parent)
{
    // Create main vertical layout for entire widget
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    //--------------------------------------------------------------------------
    // STEP 1: Create tab widget for multiple console tabs
    //--------------------------------------------------------------------------

    tabWidget = new QTabWidget(this);

    //--------------------------------------------------------------------------
    // STEP 2: Create individual console QTextEdit widgets
    //--------------------------------------------------------------------------

    errorConsole = new QTextEdit();
    debugConsole = new QTextEdit();
    warningConsole = new QTextEdit();
    logConsole = new QTextEdit();
    generalConsole = new QTextEdit();

    // Setup common properties for all console tabs
    setupConsoleTabs();

    //--------------------------------------------------------------------------
    // STEP 3: Create layout for buttons
    //--------------------------------------------------------------------------

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    // Create clear and save buttons
    clearButton = new QPushButton("Clear");
    saveButton = new QPushButton("Save Log");

    // Configure buttons with style and signals
    setupButtons();

    // Add buttons to button layout with stretch for alignment
    buttonLayout->addWidget(clearButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);

    //--------------------------------------------------------------------------
    // STEP 4: Add widgets to main layout
    //--------------------------------------------------------------------------

    mainLayout->addWidget(tabWidget);
    mainLayout->addLayout(buttonLayout);

    // Set main layout for this widget
    setLayout(mainLayout);
}

//============================================================================
// FUNCTION: setupConsoleTabs
//============================================================================

/**
 * @brief Configures each console tab with common styles and adds them to tabWidget.
 */
void ConsoleView::setupConsoleTabs()
{
    // Lambda function for setting up each QTextEdit console
    auto setupConsole = [](QTextEdit *console) {
        console->setReadOnly(true);                     // Make console read-only
        console->setFont(QFont("Courier", 10));        // Monospace font for readability
        console->setStyleSheet(
            "QTextEdit { "
            "background-color: #1E1E1E; "
            "color: black; "
            "border: none; "
            "}"
            );
    };

    // Apply setup to all consoles
    setupConsole(errorConsole);
    setupConsole(debugConsole);
    setupConsole(warningConsole);
    setupConsole(logConsole);
    setupConsole(generalConsole);

    // Add consoles as tabs
    tabWidget->addTab(generalConsole, "Console");
    tabWidget->addTab(errorConsole, "Error");
    tabWidget->addTab(debugConsole, "Debug");
    tabWidget->addTab(warningConsole, "Warning");
    tabWidget->addTab(logConsole, "Log");

    // Set tab bar colors and styles
    tabWidget->setStyleSheet(
        "QTabBar::tab { color: black; background: #1E1E1E; } "
        "QTabBar::tab:selected { background: #2E2E2E; } "
        "QTabWidget::pane { border: none; } "
        );
}

//============================================================================
// FUNCTION: setupButtons
//============================================================================

/**
 * @brief Styles the Clear and Save buttons and connects their signals.
 */
void ConsoleView::setupButtons()
{
    // Style Clear button
    clearButton->setStyleSheet(
        "QPushButton { "
        "color: black; "
        "border: 1px solid #5A5A5A; "
        "}"
        "QPushButton:hover { background-color: #4A4A4A; }"
        );

    // Style Save button
    saveButton->setStyleSheet(
        "QPushButton { "
        "color: black; "
        "border: 1px solid #5A5A5A; "
        "}"
        "QPushButton:hover { background-color: #4A4A4A; }"
        );

    // Connect buttons to respective slots
    connect(clearButton, &QPushButton::clicked, this, &ConsoleView::clearConsole);
    connect(saveButton, &QPushButton::clicked, this, &ConsoleView::saveLog);
}

//============================================================================
// FUNCTION: appendTextToConsole
//============================================================================

/**
 * @brief Appends a timestamped text message to a specific console with color.
 * @param console QTextEdit to append text to.
 * @param text    Message text to append.
 * @param color   QColor to use for the timestamp.
 */
void ConsoleView::appendTextToConsole(QTextEdit *console, const QString &text, const QColor &color)
{
    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");

    console->moveCursor(QTextCursor::End);
    console->setTextColor(color);           // Set timestamp color
    console->insertPlainText(timestamp);   // Insert timestamp
    console->setTextColor(Qt::white);      // Reset text color
    console->insertPlainText(text + "\n"); // Insert message
    console->moveCursor(QTextCursor::End);
}

//============================================================================
// FUNCTIONS: Append text to different console tabs
//============================================================================

void ConsoleView::appendText(const QString &text)
{
    appendTextToConsole(generalConsole, text, Qt::gray);

    // Switch to general console tab if visible and not current
    if (consoleDock && consoleDock->isVisible() && tabWidget->currentWidget() != generalConsole) {
        tabWidget->setCurrentWidget(generalConsole);
    }
}

void ConsoleView::appendError(const QString &text)
{
    appendTextToConsole(errorConsole, text, Qt::red);
    if (consoleDock && consoleDock->isVisible()) {
        tabWidget->setCurrentWidget(errorConsole);
    }
}

void ConsoleView::appendDebug(const QString &text)
{
    appendTextToConsole(debugConsole, text, Qt::cyan);
    if (consoleDock && consoleDock->isVisible()) {
        tabWidget->setCurrentWidget(debugConsole);
    }
}

void ConsoleView::appendWarning(const QString &text)
{
    appendTextToConsole(warningConsole, text, QColor(255, 165, 0));
    if (consoleDock && consoleDock->isVisible()) {
        tabWidget->setCurrentWidget(warningConsole);
    }
}

void ConsoleView::appendLog(const QString &text)
{
    appendTextToConsole(logConsole, text, Qt::gray);
    if (consoleDock && consoleDock->isVisible()) {
        tabWidget->setCurrentWidget(logConsole);
    }
}

//============================================================================
// FUNCTION: setConsoleDock
//============================================================================

void ConsoleView::setConsoleDock(QDockWidget *dock)
{
    consoleDock = dock;
}

//============================================================================
// FUNCTION: clearConsole
//============================================================================

void ConsoleView::clearConsole()
{
    QTextEdit *currentConsole = qobject_cast<QTextEdit*>(tabWidget->currentWidget());
    if (currentConsole) {
        currentConsole->clear();
    }
}

//============================================================================
// FUNCTION: saveLog
//============================================================================

void ConsoleView::saveLog()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Log", "", "Text Files (*.txt);;All Files (*)");
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);
    QTextEdit *currentConsole = qobject_cast<QTextEdit*>(tabWidget->currentWidget());
    if (currentConsole) {
        out << currentConsole->toPlainText();
    }

    file.close();
}
