#ifndef CONSOLEVIEW_H
#define CONSOLEVIEW_H

// =============================================================================
// FILE:         consoleview.h
// MODULE:       Console View / Logging UI
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen 2 Innovation (O2I).
// STANDARD:     Qt Coding Standards / ISO 26262 ASIL B (adapted)
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Declares the ConsoleView class which provides a widget containing
//               multiple QTextEdit consoles (general, error, debug, warning, log)
//               with control buttons for clearing and saving logs. Supports
//               coloured timestamped messages and tab switching. Designed to be
//               embedded in a QDockWidget for flexible UI placement.
//
// REQUIREMENTS: REQ-CONSOLE-010  Display general console messages
//               REQ-CONSOLE-011  Separate error console with distinct styling
//               REQ-CONSOLE-012  Separate debug console
//               REQ-CONSOLE-013  Separate warning console
//               REQ-CONSOLE-014  Separate log console
//               REQ-CONSOLE-015  Clear currently active console
//               REQ-CONSOLE-016  Save currently active console to text file
//               REQ-CONSOLE-017  Timestamp and colour-coded message appending
//
// AUTHOR:       Arti Rajpoot
// REVIEWED BY:  N/A - Initial draft
//
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

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
    // static void runUnitTestsOnce();


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

#endif
