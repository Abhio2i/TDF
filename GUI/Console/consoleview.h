#ifndef CONSOLEVIEW_H
#define CONSOLEVIEW_H

#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QFile>
#include <QFileDialog>

class ConsoleView : public QWidget
{
    Q_OBJECT

public:
    explicit ConsoleView(QWidget *parent = nullptr);
    void appendText(const QString &text);
    void appendError(const QString &text);
    void appendDebug(const QString &text);
    void appendWarning(const QString &text);
    void appendLog(const QString &text);
private slots:
    void clearConsole();
    void saveLog();
private:
    QTabWidget *tabWidget;
    QTextEdit *errorConsole;
    QTextEdit *debugConsole;
    QTextEdit *warningConsole;
    QTextEdit *logConsole;
    QTextEdit *generalConsole;
    QPushButton *clearButton;
    QPushButton *saveButton;
    void setupConsoleTabs();
    void setupButtons();
    void appendTextToConsole(QTextEdit *console, const QString &text, const QColor &color);

};

#endif
