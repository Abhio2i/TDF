/* ========================================================================= */
/* File: menubar.cpp                                                      */
/* Purpose: Implements menu bar with file, edit, view, and feedback menus   */
/* ========================================================================= */

#include "menubar.h"                               // For menu bar class
#include <QMenu>                                   // For menu creation
#include <QAction>                                 // For menu actions
#include <QKeySequence>                            // For keyboard shortcuts

// %%% Constructor %%%
/* Initialize menu bar with actions */
MenuBar::MenuBar(QWidget *parent)
    : QMenuBar(parent)
{
    // Create file menu
    fileMenu = addMenu("File");
    newFileAction = new QAction("New File", this);
    newFileAction->setShortcut(QKeySequence("Ctrl+N"));
    recentProjectAction = new QAction("Recent Project", this);
    loadJsonAction = new QAction("Open File", this);
    loadJsonAction->setShortcut(QKeySequence("Ctrl+O"));
    loadToLibraryAction = new QAction("Open File to Library", this);
    sameSaveAction = new QAction("Save", this);
    sameSaveAction->setShortcut(QKeySequence("Ctrl+S"));
    saveJsonAction = new QAction("Save As", this);
    saveJsonAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    // runAction = new QAction("Run", this);
    // runAction->setShortcut(QKeySequence("F5"));
    exitAction = new QAction("Exit", this);
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    fileMenu->addAction(newFileAction);
    fileMenu->addAction(recentProjectAction);
    fileMenu->addSeparator();
    fileMenu->addAction(loadJsonAction);
    fileMenu->addAction(loadToLibraryAction);
    fileMenu->addAction(sameSaveAction);
    fileMenu->addAction(saveJsonAction);
    fileMenu->addSeparator();
    // fileMenu->addAction(runAction);
    fileMenu->addAction(exitAction);

    // Create edit menu
    editMenu = addMenu("Edit");
    undoAction = new QAction("Undo", this);
    undoAction->setShortcut(QKeySequence("Ctrl+Z"));
    redoAction = new QAction("Redo", this);
    redoAction->setShortcut(QKeySequence("Ctrl+Y"));
    selectAllAction = new QAction("Select All", this);
    selectAllAction->setShortcut(QKeySequence("Ctrl+A"));
    deselectAllAction = new QAction("Deselect All", this);
    deselectAllAction->setShortcut(QKeySequence("Ctrl+Shift+A"));
    cutAction = new QAction("Cut", this);
    cutAction->setShortcut(QKeySequence("Ctrl+X"));
    copyAction = new QAction("Copy", this);
    copyAction->setShortcut(QKeySequence("Ctrl+C"));
    pasteAction = new QAction("Paste", this);
    pasteAction->setShortcut(QKeySequence("Ctrl+V"));
    duplicateAction = new QAction("Duplicate", this);
    duplicateAction->setShortcut(QKeySequence("Ctrl+D"));
    renameAction = new QAction("Rename", this);
    renameAction->setShortcut(QKeySequence("F2"));
    deleteAction = new QAction("Delete", this);
    deleteAction->setShortcut(QKeySequence("Del"));
    playAction = new QAction("Play", this);
    playAction->setShortcut(QKeySequence("Space"));
    pauseAction = new QAction("Pause", this);
    pauseAction->setShortcut(QKeySequence("Ctrl+P"));
    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);
    editMenu->addSeparator();
    editMenu->addAction(selectAllAction);
    editMenu->addAction(deselectAllAction);
    editMenu->addSeparator();
    editMenu->addAction(cutAction);
    editMenu->addAction(copyAction);
    editMenu->addAction(pasteAction);
    editMenu->addAction(duplicateAction);
    editMenu->addSeparator();
    editMenu->addAction(renameAction);
    editMenu->addAction(deleteAction);
    editMenu->addSeparator();
    editMenu->addAction(playAction);
    editMenu->addAction(pauseAction);

    // Create view menu
    viewMenu = addMenu("View");
    add3DViewAction = new QAction("Add 3D View", this);
    add3DViewAction->setShortcut(QKeySequence("Ctrl+3"));
    remove3DViewAction = new QAction("Remove 3D View", this);
    remove3DViewAction->setShortcut(QKeySequence("Ctrl+Shift+3"));
    viewMenu->addAction(add3DViewAction);
    viewMenu->addAction(remove3DViewAction);

    // Create feedback menu
    feedbackMenu = addMenu("Feedback");
    feedbackAction = new QAction("Open Feedback Page", this);
    feedbackMenu->addAction(feedbackAction);

    // Connect actions to signals
    connect(feedbackAction, &QAction::triggered, this, &MenuBar::feedbackTriggered);
    connect(newFileAction, &QAction::triggered, this, &MenuBar::newFileTriggered);
    connect(recentProjectAction, &QAction::triggered, this, &MenuBar::recentProjectTriggered);
    connect(loadJsonAction, &QAction::triggered, this, &MenuBar::loadTriggered);
    connect(loadToLibraryAction, &QAction::triggered, this, &MenuBar::loadToLibraryTriggered);
    connect(sameSaveAction, &QAction::triggered, this, &MenuBar::sameSaveTriggered);
    connect(saveJsonAction, &QAction::triggered, this, &MenuBar::saveTriggered);
    // connect(runAction, &QAction::triggered, this, &MenuBar::runTriggered);
    connect(exitAction, &QAction::triggered, this, &MenuBar::exitTriggered);
    connect(undoAction, &QAction::triggered, this, &MenuBar::undoTriggered);
    connect(redoAction, &QAction::triggered, this, &MenuBar::redoTriggered);
    connect(selectAllAction, &QAction::triggered, this, &MenuBar::selectAllTriggered);
    connect(deselectAllAction, &QAction::triggered, this, &MenuBar::deselectAllTriggered);
    connect(cutAction, &QAction::triggered, this, &MenuBar::cutTriggered);
    connect(copyAction, &QAction::triggered, this, &MenuBar::copyTriggered);
    connect(pasteAction, &QAction::triggered, this, &MenuBar::pasteTriggered);
    connect(duplicateAction, &QAction::triggered, this, &MenuBar::duplicateTriggered);
    connect(renameAction, &QAction::triggered, this, &MenuBar::renameTriggered);
    connect(deleteAction, &QAction::triggered, this, &MenuBar::deleteTriggered);
    connect(playAction, &QAction::triggered, this, &MenuBar::playTriggered);
    connect(pauseAction, &QAction::triggered, this, &MenuBar::pauseTriggered);
    connect(add3DViewAction, &QAction::triggered, this, &MenuBar::add3DViewTriggered);
    connect(remove3DViewAction, &QAction::triggered, this, &MenuBar::remove3DViewTriggered);
}

// %%% Getter Methods %%%
/* Get file menu */
QMenu* MenuBar::getFileMenu()
{
    return fileMenu;
}

/* Get edit menu */
QMenu* MenuBar::getEditMenu()
{
    return editMenu;
}

/* Get view menu */
QMenu* MenuBar::getViewMenu()
{
    return viewMenu;
}

/* Get load action */
QAction* MenuBar::getLoadAction()
{
    return loadJsonAction;
}

/* Get load to library action */
QAction* MenuBar::getLoadToLibraryAction()
{
    return loadToLibraryAction;
}

/* Get save action */
QAction* MenuBar::getSameSaveAction()
{
    return sameSaveAction;
}

/* Get save as action */
QAction* MenuBar::getSaveAction()
{
    return saveJsonAction;
}

/* Get feedback action */
QAction* MenuBar::getFeedbackAction()
{
    return feedbackAction;
}

/* Get new file action */
QAction* MenuBar::getNewFileAction()
{
    return newFileAction;
}

/* Get recent project action */
QAction* MenuBar::getRecentProjectAction()
{
    return recentProjectAction;
}

/* Get exit action */
QAction* MenuBar::getExitAction()
{
    return exitAction;
}

/* Get undo action */
QAction* MenuBar::getUndoAction()
{
    return undoAction;
}

/* Get redo action */
QAction* MenuBar::getRedoAction()
{
    return redoAction;
}

/* Get select all action */
QAction* MenuBar::getSelectAllAction()
{
    return selectAllAction;
}

/* Get deselect all action */
QAction* MenuBar::getDeselectAllAction()
{
    return deselectAllAction;
}

/* Get cut action */
QAction* MenuBar::getCutAction()
{
    return cutAction;
}

/* Get copy action */
QAction* MenuBar::getCopyAction()
{
    return copyAction;
}

/* Get paste action */
QAction* MenuBar::getPasteAction()
{
    return pasteAction;
}

/* Get duplicate action */
QAction* MenuBar::getDuplicateAction()
{
    return duplicateAction;
}

/* Get rename action */
QAction* MenuBar::getRenameAction()
{
    return renameAction;
}

/* Get delete action */
QAction* MenuBar::getDeleteAction()
{
    return deleteAction;
}

/* Get play action */
QAction* MenuBar::getPlayAction()
{
    return playAction;
}

/* Get pause action */
QAction* MenuBar::getPauseAction()
{
    return pauseAction;
}

/* Get add 3D view action */
QAction* MenuBar::getAdd3DViewAction()
{
    return add3DViewAction;
}
