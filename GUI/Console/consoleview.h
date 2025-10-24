#ifndef CONSOLEVIEW_H
#define CONSOLEVIEW_H

//============================================================================
// File        : consoleview.h
// Description : Header file for ConsoleView class which manages multiple
//               console tabs (general, error, debug, warning, log) and
//               provides functionality to append, clear, and save console logs.
//============================================================================

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

//============================================================================
// CLASS: ConsoleView
//============================================================================

/**
 * @brief ConsoleView class provides a widget containing multiple QTextEdit consoles
 *        along with control buttons for clearing and saving logs.
 *
 * Supports:
 *   - General console
 *   - Error console
 *   - Debug console
 *   - Warning console
 *   - Log console
 *
 * Provides colored timestamped messages and tab switching.
 */
class ConsoleView : public QWidget
{
    Q_OBJECT // Qt macro to enable signals and slots mechanism

public:
    //--------------------------------------------------------------------------
    // Constructor
    //--------------------------------------------------------------------------

    /**
     * @brief Constructs the ConsoleView widget.
     * @param parent Optional pointer to parent QWidget.
     *
     * Initializes UI components, layouts, buttons, and console tabs.
     */
    explicit ConsoleView(QWidget *parent = nullptr);

    //--------------------------------------------------------------------------
    // Public Methods: Append messages to consoles
    //--------------------------------------------------------------------------

    void appendText(const QString &text);       ///< Append text to general console
    void appendError(const QString &text);      ///< Append text to error console
    void appendDebug(const QString &text);      ///< Append text to debug console
    void appendWarning(const QString &text);    ///< Append text to warning console
    void appendLog(const QString &text);        ///< Append text to log console

    //--------------------------------------------------------------------------
    // Public Methods: Docking
    //--------------------------------------------------------------------------

    /**
     * @brief Sets the QDockWidget that contains this console.
     * @param dock Pointer to QDockWidget.
     *
     * Used to manage visibility and tab switching when docked.
     */
    void setConsoleDock(QDockWidget *dock);

private slots:
    //--------------------------------------------------------------------------
    // Slots: Button actions
    //--------------------------------------------------------------------------

    void clearConsole();  ///< Clears the currently active console
    void saveLog();       ///< Saves current console log to a text file

private:
    //--------------------------------------------------------------------------
    // UI Components
    //--------------------------------------------------------------------------

    QTabWidget *tabWidget;         ///< Tab widget containing all console tabs
    QTextEdit *errorConsole;       ///< Error messages console
    QTextEdit *debugConsole;       ///< Debug messages console
    QTextEdit *warningConsole;     ///< Warning messages console
    QTextEdit *logConsole;         ///< Log messages console
    QTextEdit *generalConsole;     ///< General console for standard output
    QPushButton *clearButton;      ///< Button to clear current console
    QPushButton *saveButton;       ///< Button to save current console log
    QDockWidget *consoleDock = nullptr; ///< Optional parent dock widget

    //--------------------------------------------------------------------------
    // Helper Methods
    //--------------------------------------------------------------------------

    void setupConsoleTabs(); ///< Configures all console tabs with style and adds to tabWidget
    void setupButtons();     ///< Styles and connects Clear and Save buttons

    /**
     * @brief Appends text to a specific console with timestamp and color.
     * @param console QTextEdit pointer to the target console.
     * @param text Message to append.
     * @param color QColor for the timestamp.
     */
    void appendTextToConsole(QTextEdit *console, const QString &text, const QColor &color);
};

#endif // CONSOLEVIEW_H
