

#ifndef CONSOLEVIEW_H
#define CONSOLEVIEW_H

// Include necessary Qt header files
#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QFile>
#include <QFileDialog>
#include <QDockWidget>

// ConsoleView class definition - inherits from QWidget
class ConsoleView : public QWidget
{
    Q_OBJECT // Qt macro to enable signals and slots mechanism

public:
    // Constructor
    explicit ConsoleView(QWidget *parent = nullptr);

    // Public methods to append different types of text messages to the console
    void appendText(const QString &text);          // Append general text
    void appendError(const QString &text);         // Append error message
    void appendDebug(const QString &text);         // Append debug message
    void appendWarning(const QString &text);       // Append warning message
    void appendLog(const QString &text);           // Append log message

    // Method to set the parent QDockWidget (used to dock/undock the console)
    void setConsoleDock(QDockWidget *dock);

private slots:
    // Slot to clear all console outputs
    void clearConsole();

    // Slot to save current log contents to a file
    void saveLog();

private:
    // UI components
    QTabWidget *tabWidget;         // Holds multiple tabs for different message types
    QTextEdit *errorConsole;       // Displays error messages
    QTextEdit *debugConsole;       // Displays debug messages
    QTextEdit *warningConsole;     // Displays warning messages
    QTextEdit *logConsole;         // Displays log messages
    QTextEdit *generalConsole;     // Displays general messages
    QPushButton *clearButton;      // Button to clear console
    QPushButton *saveButton;       // Button to save console log to file
    QDockWidget *consoleDock = nullptr; // Associated dock widget (optional, used for docking)

    // Helper methods
    void setupConsoleTabs();       // Setup and initialize the tab widget and its tabs
    void setupButtons();           // Setup and initialize the clear and save buttons

    // Generic method to append colored text to a given QTextEdit console
    void appendTextToConsole(QTextEdit *console, const QString &text, const QColor &color);
};

#endif // CONSOLEVIEW_H
