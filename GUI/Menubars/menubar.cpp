/* ========================================================================= */
/* File: menubar.cpp                                                      */
/* Purpose: Implements menu bar with file, edit, view, and feedback menus   */
//               Written by Arti Rajpoot
/* ========================================================================= */

#include "menubar.h"                               // For menu bar class
#include "menubar-styles.h"                        // Include separate CSS file
#include <QMenu>                                   // For menu creation
#include <QAction>                                 // For menu actions
#include <QKeySequence>                            // For keyboard shortcuts

// %%% Constructor %%%
/* Initialize menu bar with actions */
MenuBar::MenuBar(QWidget *parent)
    : QMenuBar(parent)
{
    // Apply dark theme to menu bar
    setStyleSheet(MenuBarStyles::MenuBar);

    // Create file menu
    fileMenu = addMenu("File");
    fileMenu->setStyleSheet(MenuBarStyles::Menu);

    newFileAction = new QAction("New File", this);
    // newFileAction->setShortcut(QKeySequence("Ctrl+N"));
    recentProjectAction = new QAction("Recent Project", this);
    recentProjectLibraryAction = new QAction("Recent Library", this);
    loadJsonAction = new QAction("Open File", this);
    loadXmlAction = new QAction("Open XML File", this);
    // loadJsonAction->setShortcut(QKeySequence("Ctrl+O"));
    loadToLibraryAction = new QAction("Open File to Library", this);
    openRuntimeInstanceAction = new QAction("Open Runtime Instance", this);
    sameSaveAction = new QAction("Save", this);
    sameSaveAction->setShortcut(QKeySequence("Ctrl+S"));
    saveJsonAction = new QAction("Save As", this);
    exitAction = new QAction("Exit", this);

    fileMenu->addAction(newFileAction);
    fileMenu->addAction(recentProjectAction);
    fileMenu->addAction(recentProjectLibraryAction);
    fileMenu->addSeparator();
    fileMenu->addAction(loadJsonAction);
    fileMenu->addAction(loadXmlAction);
    fileMenu->addAction(loadToLibraryAction);
    fileMenu->addAction(openRuntimeInstanceAction);
    fileMenu->addAction(sameSaveAction);
    fileMenu->addAction(saveJsonAction);
    fileMenu->addSeparator();
    // fileMenu->addAction(runAction);
    fileMenu->addAction(exitAction);

    // Create edit menu (commented out in original)
    // editMenu = addMenu("Edit");
    // editMenu->setStyleSheet(MenuBarStyles::Menu);

    undoAction = new QAction("Undo", this);
    // undoAction->setShortcut(QKeySequence("Ctrl+Z"));
    redoAction = new QAction("Redo", this);
    // redoAction->setShortcut(QKeySequence("Ctrl+Y"));
    selectAllAction = new QAction("Select All", this);
    // selectAllAction->setShortcut(QKeySequence("Ctrl+A"));
    deselectAllAction = new QAction("Deselect All", this);
    // deselectAllAction->setShortcut(QKeySequence("Ctrl+Shift+A"));
    cutAction = new QAction("Cut", this);
    // cutAction->setShortcut(QKeySequence("Ctrl+X"));
    copyAction = new QAction("Copy", this);
    // copyAction->setShortcut(QKeySequence("Ctrl+C"));
    pasteAction = new QAction("Paste", this);
    // pasteAction->setShortcut(QKeySequence("Ctrl+V"));
    duplicateAction = new QAction("Duplicate", this);
    // duplicateAction->setShortcut(QKeySequence("Ctrl+D"));
    renameAction = new QAction("Rename", this);
    // renameAction->setShortcut(QKeySequence("F2"));
    deleteAction = new QAction("Delete", this);

    // Create feedback menu
    feedbackMenu = addMenu("About");
    feedbackMenu->setStyleSheet(MenuBarStyles::Menu);
    feedbackAction = new QAction("Open About Page", this);
    feedbackMenu->addAction(feedbackAction);

    profileAction = addAction("Performance");
    profileAction->setMenuRole(QAction::NoRole);  // Ensure it's in the menu bar

    applicationAction = addAction("Settings");
    applicationAction->setMenuRole(QAction::NoRole);  // Ensure it's in the menu bar

    // Connect actions to signals
    connect(loadXmlAction, &QAction::triggered, this, &MenuBar::loadXmlTriggered);
    connect(profileAction, &QAction::triggered, this, &MenuBar::profileTriggered);
    connect(applicationAction, &QAction::triggered, this, &MenuBar::applicationTriggered);
    connect(feedbackAction, &QAction::triggered, this, &MenuBar::feedbackTriggered);
    connect(newFileAction, &QAction::triggered, this, &MenuBar::newFileTriggered);
    connect(recentProjectAction, &QAction::triggered, this, &MenuBar::recentProjectTriggered);
    connect(recentProjectLibraryAction, &QAction::triggered, this, &MenuBar::recentProjectLibraryTriggered);
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
    connect(openRuntimeInstanceAction, &QAction::triggered,
            this, &MenuBar::openRuntimeInstanceTriggered);
}

// %%% Getter Methods %%%
/* Get file menu */
QMenu* MenuBar::getFileMenu()
{
    return fileMenu;
}

// /* Get edit menu */
// QMenu* MenuBar::getEditMenu()
// {
//     return editMenu;
// }

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

QAction* MenuBar::getrecentProjectLibraryAction()
{
    return recentProjectLibraryAction;
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

QAction* MenuBar::getLoadXmlAction()
{
    return loadXmlAction;
}

QAction* MenuBar::getOpenRuntimeInstanceAction()
{
    return openRuntimeInstanceAction;
}

// %%% Set Library Actions Visibility %%%
/* Show or hide library-related actions */
void MenuBar::setLibraryActionsVisible(bool visible)
{
    recentProjectLibraryAction->setVisible(visible);
    loadToLibraryAction->setVisible(visible);
    openRuntimeInstanceAction->setVisible(visible);
}
