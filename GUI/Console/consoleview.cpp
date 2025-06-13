
#include "consoleview.h"
#include <QFont>
#include <QDateTime>

ConsoleView::ConsoleView(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Create tab widget
    tabWidget = new QTabWidget(this);

    // Create consoles
    errorConsole = new QTextEdit();
    debugConsole = new QTextEdit();
    warningConsole = new QTextEdit();
    logConsole = new QTextEdit();
    generalConsole = new QTextEdit(); // For backward compatibility

    // Setup consoles
    setupConsoleTabs();

    // Create button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    // Create buttons
    clearButton = new QPushButton("Clear");
    saveButton = new QPushButton("Save Log");

    // Setup buttons
    setupButtons();

    // Add buttons to layout
    buttonLayout->addWidget(clearButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);

    // Add widgets to main layout
    mainLayout->addWidget(tabWidget);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void ConsoleView::setupConsoleTabs()
{
    // Common console settings
    auto setupConsole = [](QTextEdit *console) {
        console->setReadOnly(true);
        console->setFont(QFont("Courier", 10));
        console->setStyleSheet(
            "QTextEdit { "
            "background-color: #1E1E1E; "
            "color: white; "
            "border: none; "
            "}"
            );
    };

    setupConsole(errorConsole);
    setupConsole(debugConsole);
    setupConsole(warningConsole);
    setupConsole(logConsole);
    setupConsole(generalConsole);

    // Add tabs
    tabWidget->addTab(generalConsole, "Console");   // Default tab for backward compatibility
    tabWidget->addTab(errorConsole, "Error");
    tabWidget->addTab(debugConsole, "Debug");
    tabWidget->addTab(warningConsole, "Warning");
    tabWidget->addTab(logConsole, "Log");

    // Set tab colors
    tabWidget->setStyleSheet(
        "QTabBar::tab { color: white; background: #1E1E1E; } "
        "QTabBar::tab:selected { background: #2E2E2E; } "
        "QTabWidget::pane { border: none; } "
        );
}

void ConsoleView::setupButtons()
{
    clearButton->setStyleSheet(
        "QPushButton { "
        "color: white; "
        "border: 1px solid #5A5A5A; "
        "}"
        "QPushButton:hover { background-color: #4A4A4A; }"
        );

    saveButton->setStyleSheet(
        "QPushButton { "
        "color: white; "
        "border: 1px solid #5A5A5A; "
        "}"
        "QPushButton:hover { background-color: #4A4A4A; }"
        );

    connect(clearButton, &QPushButton::clicked, this, &ConsoleView::clearConsole);
    connect(saveButton, &QPushButton::clicked, this, &ConsoleView::saveLog);
}

void ConsoleView::appendTextToConsole(QTextEdit *console, const QString &text, const QColor &color)
{
    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");

    console->moveCursor(QTextCursor::End);
    console->setTextColor(color);
    console->insertPlainText(timestamp);
    console->setTextColor(Qt::white);
    console->insertPlainText(text + "\n");
    console->moveCursor(QTextCursor::End);
}

void ConsoleView::appendText(const QString &text)
{
    appendTextToConsole(generalConsole, text, Qt::gray);
    // Only switch to Console tab if it's explicitly requested or no other tab is active
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

void ConsoleView::setConsoleDock(QDockWidget *dock)
{
    consoleDock = dock;
}

void ConsoleView::clearConsole()
{
    QTextEdit *currentConsole = qobject_cast<QTextEdit*>(tabWidget->currentWidget());
    if (currentConsole) {
        currentConsole->clear();
    }
}

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
