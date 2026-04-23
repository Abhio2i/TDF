/* =============================================================================
 * FILE:         standardtoolbar.h
 * MODULE:       Standard Toolbar
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the StandardToolBar class which provides a toolbar for
 *               standard editing actions (new, save, cut, copy, paste, undo,
 *               redo) along with add trajectory and test script actions.
 *               Integrates with the main application to provide common file
 *               and edit operations in a consistent toolbar interface.
 *
 * REQUIREMENTS: REQ-STDTOOLBAR-010  Standard editing toolbar
 *               REQ-STDTOOLBAR-011  New file action
 *               REQ-STDTOOLBAR-012  Save file action
 *               REQ-STDTOOLBAR-013  Cut, copy, paste actions
 *               REQ-STDTOOLBAR-014  Undo, redo actions
 *               REQ-STDTOOLBAR-015  Add trajectory action
 *               REQ-STDTOOLBAR-016  Test script action
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-STDTOOLBAR-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef STANDARDTOOLBAR_H
#define STANDARDTOOLBAR_H

#include <QToolBar>
#include <QAction>
#include <QPixmap>

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
    QPixmap withWhiteBg(const QString &iconPath);
};

#endif // STANDARDTOOLBAR_H
