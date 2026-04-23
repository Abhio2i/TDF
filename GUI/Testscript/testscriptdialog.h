/* =============================================================================
 * FILE:         testscriptdialog.h
 * MODULE:       Test Script Editor Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the TestScriptDialog class which provides a widget for
 *               editing, loading, saving, and running AngelScript test scripts.
 *               Includes a code editor with line numbers, syntax highlighting
 *               (via AngelScriptHighlighter), autocompletion, script name/type
 *               selection, and buttons for script operations (new, load, run,
 *               OK, cancel). Supports both edit mode (loading an existing file)
 *               and new script creation. Emits signals to run the script and
 *               notify when closed.
 *
 * REQUIREMENTS: REQ-TESTSCRIPT-010  Test script editor widget
 *               REQ-TESTSCRIPT-011  Code editor with line numbers
 *               REQ-TESTSCRIPT-012  Syntax highlighting for AngelScript
 *               REQ-TESTSCRIPT-013  Autocompletion for keywords
 *               REQ-TESTSCRIPT-014  Load script from file
 *               REQ-TESTSCRIPT-015  Create new script
 *               REQ-TESTSCRIPT-016  Run script (emit signal with code)
 *               REQ-TESTSCRIPT-017  Script name and type selection
 *               REQ-TESTSCRIPT-018  OK/Cancel buttons to close
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-TESTSCRIPT-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef TESTSCRIPTDIALOG_H
#define TESTSCRIPTDIALOG_H

#include "GUI/Testscript/angelscripthighlighter.h" // For AngelScript highlighter
#include <QWidget>                                 // For widget base class
#include <QTextEdit>                               // For text edit widget
#include <QPushButton>                             // For push button widget
#include <QLineEdit>                               // For text input widget
#include <QComboBox>                               // For combo box widget
#include <QHBoxLayout>                             // For horizontal layout
#include <QVBoxLayout>                             // For vertical layout
#include <QDir>                                    // For directory handling
#include <QMessageBox>                             // For message box
#include <QCoreApplication>                        // For core application
#include <QCompleter>                              // For autocompletion
#include <QStringListModel>                        // For string list model
#include <QPainter>                                // For painting operations

// %%% Class Definition %%%
/* Widget for test script editing */
class TestScriptDialog : public QWidget
{
    Q_OBJECT

public:
    // %%% LineNumberArea Class %%%
    /* Widget for displaying line numbers */
    class LineNumberArea : public QWidget
    {
    public:
        // Initialize line number area
        LineNumberArea(QTextEdit *editor);
        // Get line number area width
        int lineNumberAreaWidth() const;

    protected:
        // Handle paint events
        void paintEvent(QPaintEvent *event) override;

    private:
        // Text editor reference
        QTextEdit *codeEditor;
    };

    // Initialize dialog
    explicit TestScriptDialog(QWidget *parent = nullptr, bool editMode = false, const QString &filePath = QString());
    // Clean up resources
    ~TestScriptDialog();
    // static void runUnitTestsOnce();

signals:
    // Signal to run script
    void runScriptstring(QString code);
    // Signal dialog closed
    void closed();

private slots:
    // Handle new script button click
    void onNewScriptButtonClicked();
    // Handle run button click
    void onRunButtonClicked();
    // Handle load script button click
    void onLoadScriptButtonClicked();
    // Handle OK button click
    void onOkButtonClicked();
    // Handle cancel button click
    void onCancelButtonClicked();
    // Handle script path change
    void onScriptPathChanged();
    // Handle script name change
    void onScriptNameChanged(const QString &text);
    // Insert autocompletion
    void insertCompletion(const QString &completion);
    // Handle text change
    void handleTextChanged();
    // Update line number area
    void updateLineNumberArea();

protected:
    // Handle key press events
    void keyPressEvent(QKeyEvent *event) override;

private:
    // %%% UI Components %%%
    // Code editor
    QTextEdit *codeEditor;
    // Line number area
    LineNumberArea *lineNumberArea;
    // Run button
    QPushButton *runButton;
    // Load script button
    QPushButton *loadScriptButton;
    // OK button
    QPushButton *okButton;
    // Cancel button
    QPushButton *cancelButton;
    // New script button
    QPushButton *newScriptButton;
    // Script name combo box
    QComboBox *scriptNameCombo;
    // Script type combo box
    QComboBox *scriptTypeCombo;
    // Script path input
    QLineEdit *scriptPathEdit;
    // Edit file path
    QString editFilePath;
    // Edit mode flag
    bool isEditMode;
    // New script flag
    bool isNewScript;
    // Syntax highlighter
    AngelScriptHighlighter *highlighter;
    // Autocompleter
    QCompleter *completer;
    // Word list model
    QStringListModel *wordListModel;
};

#endif // TESTSCRIPTDIALOG_H
