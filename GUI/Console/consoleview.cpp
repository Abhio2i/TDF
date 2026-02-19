//============================================================================
// File        : consoleview.cpp
// Description : Implements the ConsoleView class for handling multiple
//               console tabs (general, error, debug, warning, log) in Qt.
//               Written by Arti Rajpoot
//============================================================================

#include "consoleview.h"
#include "consoleview-styles.h"  // Include separate CSS file
#include "qtabbar.h"
#include <QFont>
#include <QDateTime>
#include <QTextStream>
#include <QPalette>
#include <QStyleOptionTab>
#include <QStylePainter>

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
    // Apply main widget style
    setStyleSheet(ConsoleViewStyles::MainWidget);

    // Create main vertical layout for entire widget
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    //--------------------------------------------------------------------------
    // STEP 1: Create tab widget for multiple console tabs
    //--------------------------------------------------------------------------

    tabWidget = new QTabWidget(this);
    tabWidget->setDocumentMode(true);
    tabWidget->setStyleSheet(ConsoleViewStyles::TabWidget);
    QWidget *cornerWidget = new QWidget();
    cornerWidget->setStyleSheet("background-color: #0F2636;");
    cornerWidget->setFixedHeight(30); // Match tab height
    tabWidget->setCornerWidget(cornerWidget, Qt::TopRightCorner);

    // Ensure tab widget background is dark
    tabWidget->setAutoFillBackground(true);

    // Set palette to force dark background
    QPalette pal = tabWidget->palette();
    pal.setColor(QPalette::Window, QColor(15, 38, 54));
    pal.setColor(QPalette::Base, QColor(15, 38, 54));
    pal.setColor(QPalette::Button, QColor(15, 38, 54));
    pal.setColor(QPalette::Mid, QColor(15, 38, 54));
    pal.setColor(QPalette::Dark, QColor(15, 38, 54));
    tabWidget->setPalette(pal);

    // Force tab bar to have dark background - COMPREHENSIVE APPROACH
    QTabBar *tabBar = tabWidget->tabBar();
    if (tabBar) {
        // Method 1: Stylesheet with !important
          tabBar->setExpanding(false);
        tabBar->setStyleSheet(
            "QTabBar { background-color: #0F2636 !important; } "
            "QTabBar::tab { background-color: #1A3652; } "
            "QTabBar::tab:selected { background-color: #0F2636; } "
            "QTabBar::tear { background-color: #0F2636 !important; } "
            "QTabBar::scroller { background-color: #0F2636 !important; } "
            "QTabBar QToolButton { background-color: #0F2636; border: none; }"
            );

        // Method 2: Palette
        QPalette tabPal = tabBar->palette();
        tabPal.setColor(QPalette::Window, QColor(15, 38, 54));
        tabPal.setColor(QPalette::Base, QColor(15, 38, 54));
        tabPal.setColor(QPalette::Button, QColor(15, 38, 54));
        tabPal.setColor(QPalette::Mid, QColor(15, 38, 54));
        tabPal.setColor(QPalette::Dark, QColor(15, 38, 54));
        tabBar->setPalette(tabPal);

        // Method 3: AutoFill
        tabBar->setAutoFillBackground(true);

        // Method 4: Force update
        tabBar->update();
    }

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
    // STEP 3: Create layout for buttons - with dark background
    //--------------------------------------------------------------------------

    // Create a container widget for buttons to ensure full background
    QWidget *buttonContainer = new QWidget(this);
    buttonContainer->setStyleSheet(ConsoleViewStyles::ButtonContainer);
    buttonContainer->setAutoFillBackground(true);

    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(5, 2, 5, 2);
    buttonLayout->setSpacing(8);

    // Create clear and save buttons
    clearButton = new QPushButton("Clear");
    saveButton = new QPushButton("Save Log");

    // Configure buttons with compact style
    setupButtons();

    // Add buttons to button layout
    buttonLayout->addWidget(clearButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);

    //--------------------------------------------------------------------------
    // STEP 4: Add widgets to main layout
    //--------------------------------------------------------------------------

    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(buttonContainer);

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
        console->setReadOnly(true);
        console->setFont(QFont("Courier New", 10));
        console->setStyleSheet(ConsoleViewStyles::TextEdit);
        console->document()->setMaximumBlockCount(1000);
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
    QWidget *cornerWidget = new QWidget();
    cornerWidget->setAutoFillBackground(true);
    QPalette cornerPal = cornerWidget->palette();
    cornerPal.setColor(QPalette::Window, QColor(15, 38, 54));
    cornerWidget->setPalette(cornerPal);
    cornerWidget->setStyleSheet("background-color: #0F2636;");
    tabWidget->setCornerWidget(cornerWidget, Qt::TopRightCorner);
    // Set the current tab to general console
    tabWidget->setCurrentWidget(generalConsole);

    // Force repaint of tab bar
    QTabBar *tabBar = tabWidget->tabBar();
    if (tabBar) {
        tabBar->repaint();
    }
}

//============================================================================
// FUNCTION: setupButtons
//============================================================================

/**
 * @brief Styles the Clear and Save buttons and connects their signals.
 */
void ConsoleView::setupButtons()
{
    // Style Clear button with compact style
    clearButton->setStyleSheet(ConsoleViewStyles::PushButton);
    clearButton->setFixedHeight(22);

    // Style Save button with compact style
    saveButton->setStyleSheet(ConsoleViewStyles::PushButton);
    saveButton->setFixedHeight(22);

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
    console->insertPlainText(timestamp);    // Insert timestamp
    console->setTextColor(Qt::white);       // Reset text color to white
    console->insertPlainText(text + "\n");  // Insert message
    console->moveCursor(QTextCursor::End);
}

//============================================================================
// FUNCTIONS: Append text to different console tabs
//============================================================================

void ConsoleView::appendText(const QString &text)
{
    appendTextToConsole(generalConsole, text, Qt::lightGray);

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
    appendTextToConsole(warningConsole, text, QColor(255, 165, 0)); // Orange
    if (consoleDock && consoleDock->isVisible()) {
        tabWidget->setCurrentWidget(warningConsole);
    }
}

void ConsoleView::appendLog(const QString &text)
{
    appendTextToConsole(logConsole, text, Qt::lightGray);
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
