/* ========================================================================= */
/* File: menubar.h                                                          */
/* Purpose: Defines menu bar for application interface                       */
/* ========================================================================= */

#ifndef MENUBAR_H
#define MENUBAR_H

#include <QMenuBar>                               // For menu bar base class
#include <QMenu>                                  // For menu widget
#include <QAction>                                // For action items
#include <GUI/Feedback/feedback.h>                // For feedback window

// %%% Class Definition %%%
/* Menu bar for application UI */
class MenuBar : public QMenuBar
{
    Q_OBJECT

public:
    // Initialize menu bar
    explicit MenuBar(QWidget *parent = nullptr);
    // Get file menu
    QMenu* getFileMenu();
    // Get edit menu
    QMenu* getEditMenu();
    // Get view menu
    QMenu* getViewMenu();
    // Get load action
    QAction* getLoadAction();
    // Get load to library action
    QAction* getLoadToLibraryAction();
    // Get save same action
    QAction* getSameSaveAction();
    // Get save action
    QAction* getSaveAction();
    // Get feedback action
    QAction* getFeedbackAction();
    // Get new file action
    QAction* getNewFileAction();
    // Get recent project action
    QAction* getRecentProjectAction();
    // Get run action (commented)
    // QAction* getRunAction();
    // Get exit action
    QAction* getExitAction();
    // Get undo action
    QAction* getUndoAction();
    // Get redo action
    QAction* getRedoAction();
    // Get select all action
    QAction* getSelectAllAction();
    // Get deselect all action
    QAction* getDeselectAllAction();
    // Get cut action
    QAction* getCutAction();
    // Get copy action
    QAction* getCopyAction();
    // Get paste action
    QAction* getPasteAction();
    // Get duplicate action
    QAction* getDuplicateAction();
    // Get rename action
    QAction* getRenameAction();
    // Get delete action
    QAction* getDeleteAction();
    // Get play action
    QAction* getPlayAction();
    // Get pause action
    QAction* getPauseAction();
    // Get add 3D view action
    QAction* getAdd3DViewAction();
    // Get remove 3D view action
    QAction* getRemove3DViewAction();

signals:
    // Signal feedback action triggered
    void feedbackTriggered();
    // Signal new file action triggered
    void newFileTriggered();
    // Signal recent project action triggered
    void recentProjectTriggered();
    // Signal load action triggered
    void loadTriggered();
    // Signal load to library action triggered
    void loadToLibraryTriggered();
    // Signal save same action triggered
    void sameSaveTriggered();
    // Signal save action triggered
    void saveTriggered();
    // Signal run action triggered (commented)
    // void runTriggered();
    // Signal exit action triggered
    void exitTriggered();
    // Signal undo action triggered
    void undoTriggered();
    // Signal redo action triggered
    void redoTriggered();
    // Signal select all action triggered
    void selectAllTriggered();
    // Signal deselect all action triggered
    void deselectAllTriggered();
    // Signal cut action triggered
    void cutTriggered();
    // Signal copy action triggered
    void copyTriggered();
    // Signal paste action triggered
    void pasteTriggered();
    // Signal duplicate action triggered
    void duplicateTriggered();
    // Signal rename action triggered
    void renameTriggered();
    // Signal delete action triggered
    void deleteTriggered();
    // Signal play action triggered
    void playTriggered();
    // Signal pause action triggered
    void pauseTriggered();
    // Signal add 3D view action triggered
    void add3DViewTriggered();
    // Signal remove 3D view action triggered
    void remove3DViewTriggered();

private:
    // %%% Menu Components %%%
    // File menu
    QMenu* fileMenu;
    // Edit menu
    QMenu* editMenu;
    // View menu
    QMenu* viewMenu;
    // New file action
    QAction* newFileAction;
    // Recent project action
    QAction* recentProjectAction;
    // Load JSON action
    QAction* loadJsonAction;
    // Load to library action
    QAction* loadToLibraryAction;
    // Save same action
    QAction* sameSaveAction;
    // Save JSON action
    QAction* saveJsonAction;
    // Run action (commented)
    // QAction* runAction;
    // Exit action
    QAction* exitAction;
    // Undo action
    QAction* undoAction;
    // Redo action
    QAction* redoAction;
    // Select all action
    QAction* selectAllAction;
    // Deselect all action
    QAction* deselectAllAction;
    // Cut action
    QAction* cutAction;
    // Copy action
    QAction* copyAction;
    // Paste action
    QAction* pasteAction;
    // Duplicate action
    QAction* duplicateAction;
    // Rename action
    QAction* renameAction;
    // Delete action
    QAction* deleteAction;
    // Play action
    QAction* playAction;
    // Pause action
    QAction* pauseAction;
    // Add 3D view action
    QAction* add3DViewAction;
    // Remove 3D view action
    QAction* remove3DViewAction;
    // Feedback menu
    QMenu* feedbackMenu;
    // Feedback action
    QAction* feedbackAction;
};

#endif // MENUBAR_H
