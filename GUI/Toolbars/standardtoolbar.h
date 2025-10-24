/* ========================================================================= */
/* File: standardtoolbar.h                                                  */
/* Purpose: Defines toolbar for standard editing actions                     */
/* ========================================================================= */

#ifndef STANDARDTOOLBAR_H
#define STANDARDTOOLBAR_H

#include <QToolBar>                               // For toolbar base class
#include <QAction>                                // For action items
#include <QPixmap>                                // For pixmap handling

// %%% Class Definition %%%
/* Toolbar for standard editing tools */
class StandardToolBar : public QToolBar
{
    Q_OBJECT

public:
    // Initialize toolbar
    explicit StandardToolBar(QWidget *parent = nullptr);
    // Get add trajectory action
    QAction* getAddTrajectoryAction() const { return addTrajectoryAction; }
    // Get save action
    QAction* getSaveAction() const { return saveAction; }
    // Get test script action
    QAction* getTestScriptAction() const { return testScriptAction; }

private slots:
    // Handle test script trigger
    void onTestScriptTriggered();

private:
    // %%% UI Components %%%
    // New action
    QAction *newAction;
    // Save action
    QAction *saveAction;
    // Save all action
    QAction *saveAllAction;
    // Cut action
    QAction *cutAction;
    // Copy action
    QAction *copyAction;
    // Paste action
    QAction *pasteAction;
    // Undo action
    QAction *undoAction;
    // Redo action
    QAction *redoAction;
    // Add trajectory action
    QAction *addTrajectoryAction;
    // Test script action
    QAction *testScriptAction;

    // %%% Utility Methods %%%
    // Create toolbar actions
    void createActions();
    // Create pixmap with white background
    QPixmap withWhiteBg(const QString &iconPath);
};

#endif // STANDARDTOOLBAR_H
