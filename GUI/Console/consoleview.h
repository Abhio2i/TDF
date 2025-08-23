

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
    void appendText(const QString &text);
    void appendError(const QString &text);
    void appendDebug(const QString &text);
    void appendWarning(const QString &text);
    void appendLog(const QString &text);

    // Method to set the parent QDockWidget (used to dock/undock the console)
    void setConsoleDock(QDockWidget *dock);

private slots:
    // Slot to clear all console outputs
    void clearConsole();

    // Slot to save current log contents to a file
    void saveLog();

private:
    // UI components
    QTabWidget *tabWidget;
    QTextEdit *errorConsole;
    QTextEdit *debugConsole;
    QTextEdit *warningConsole;
    QTextEdit *logConsole;
    QTextEdit *generalConsole;
    QPushButton *clearButton;
    QPushButton *saveButton;
    QDockWidget *consoleDock = nullptr;

    // Helper methods
    void setupConsoleTabs();
    void setupButtons();

    // Generic method to append colored text to a given QTextEdit console
    void appendTextToConsole(QTextEdit *console, const QString &text, const QColor &color);
};

#endif // CONSOLEVIEW_H
