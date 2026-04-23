/* =============================================================================
 * FILE:         menubar.cpp
 * MODULE:       Application Menu Bar
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the MenuBar class which provides the main application
 *               menu bar. It contains File, Edit, View, and Feedback menus with
 *               actions for file operations (new, open, save, save as, load to
 *               library, recent projects, exit), edit operations (undo, redo,
 *               select/deselect all, cut, copy, paste, duplicate, rename, delete),
 *               view operations (play/pause simulation), feedback, profile info,
 *               application info, XML loading, and runtime instance opening.
 *               Supports dynamic visibility of library actions and editor‑specific
 *               menu updates. Emits signals for all triggerable actions.
 *
 * REQUIREMENTS: Implements REQ-MENU-010 through REQ-MENU-016
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-MENU-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================*/

#include "menubar.h"                               // For menu bar class
#include "menubar-styles.h"                        // Include separate CSS file
#include <QMenu>                                   // For menu creation
#include <QAction>                                 // For menu actions
#include <QKeySequence>                            // For keyboard shortcuts

#include <QTimer>
#include <QDebug>



// %%% Constructor %%%
/* Initialize menu bar with actions */
MenuBar::MenuBar(QWidget *parent)
    : QMenuBar(parent)
{

    setStyleSheet(MenuBarStyles::MenuBar);
    fileMenu = addMenu("File");
    fileMenu->setStyleSheet(MenuBarStyles::Menu);
    newFileAction = new QAction("New File", this);
    recentProjectAction = new QAction("Recent Project", this);
    recentProjectLibraryAction = new QAction("Recent Library", this);
    loadJsonAction = new QAction("Open File", this);
    loadXmlAction = new QAction("Open XML File", this);

    loadToLibraryAction = new QAction("Open File to Library", this);
    openRuntimeInstanceAction = new QAction("Open Runtime Instance", this);
    openMissionFileAction = new QAction("Open Mission File", this);
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
    fileMenu->addAction(openMissionFileAction);
    fileMenu->addAction(sameSaveAction);
    fileMenu->addAction(saveJsonAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    undoAction = new QAction("Undo", this);
    redoAction = new QAction("Redo", this);
    selectAllAction = new QAction("Select All", this);
    deselectAllAction = new QAction("Deselect All", this);
    cutAction = new QAction("Cut", this);
    copyAction = new QAction("Copy", this);
    pasteAction = new QAction("Paste", this);
    duplicateAction = new QAction("Duplicate", this);
    renameAction = new QAction("Rename", this);
    deleteAction = new QAction("Delete", this);

    // Create feedback menu
    feedbackMenu = addMenu("About");
    feedbackMenu->setStyleSheet(MenuBarStyles::Menu);
    feedbackAction = new QAction("Open About Page", this);
    feedbackMenu->addAction(feedbackAction);

    profileAction = addAction("Performance");
    profileAction->setMenuRole(QAction::NoRole);

    applicationAction = addAction("Settings");
    applicationAction->setMenuRole(QAction::NoRole);

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
    connect(openMissionFileAction, &QAction::triggered, this, &MenuBar::openMissionFileTriggered);
    connect(sameSaveAction, &QAction::triggered, this, &MenuBar::sameSaveTriggered);
    connect(saveJsonAction, &QAction::triggered, this, &MenuBar::saveTriggered);
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
QMenu* MenuBar::getFileMenu()        { return fileMenu; }
QMenu* MenuBar::getViewMenu()        { return viewMenu; }
QAction* MenuBar::getLoadAction()    { return loadJsonAction; }
QAction* MenuBar::getLoadToLibraryAction() { return loadToLibraryAction; }
QAction* MenuBar::getSameSaveAction(){ return sameSaveAction; }
QAction* MenuBar::getSaveAction()    { return saveJsonAction; }
QAction* MenuBar::getFeedbackAction(){ return feedbackAction; }
QAction* MenuBar::getNewFileAction() { return newFileAction; }
QAction* MenuBar::getRecentProjectAction()  { return recentProjectAction; }
QAction* MenuBar::getrecentProjectLibraryAction() { return recentProjectLibraryAction; }
QAction* MenuBar::getExitAction()    { return exitAction; }
QAction* MenuBar::getUndoAction()    { return undoAction; }
QAction* MenuBar::getRedoAction()    { return redoAction; }
QAction* MenuBar::getSelectAllAction()   { return selectAllAction; }
QAction* MenuBar::getDeselectAllAction() { return deselectAllAction; }
QAction* MenuBar::getCutAction()     { return cutAction; }
QAction* MenuBar::getCopyAction()    { return copyAction; }
QAction* MenuBar::getPasteAction()   { return pasteAction; }
QAction* MenuBar::getDuplicateAction(){ return duplicateAction; }
QAction* MenuBar::getRenameAction()  { return renameAction; }
QAction* MenuBar::getDeleteAction()  { return deleteAction; }
QAction* MenuBar::getLoadXmlAction() { return loadXmlAction; }
QAction* MenuBar::getOpenRuntimeInstanceAction() { return openRuntimeInstanceAction; }
QAction* MenuBar::getOpenMissionFileAction()     { return openMissionFileAction; }

// %%% Set Library Actions Visibility %%%
/* Show or hide library-related actions */
void MenuBar::setLibraryActionsVisible(bool visible)
{
    recentProjectLibraryAction->setVisible(visible);
    loadToLibraryAction->setVisible(visible);
    openRuntimeInstanceAction->setVisible(visible);
}

// %%% Update File Menu Per Editor %%%

void MenuBar::updateFileMenuForEditor(const QString& editorKey)
{

    newFileAction->setVisible(true);
    recentProjectAction->setVisible(true);
    recentProjectLibraryAction->setVisible(true);
    loadJsonAction->setVisible(true);
    loadXmlAction->setVisible(false);
    loadToLibraryAction->setVisible(true);
    openRuntimeInstanceAction->setVisible(true);
    openMissionFileAction->setVisible(false);
    sameSaveAction->setVisible(true);
    saveJsonAction->setVisible(true);
    exitAction->setVisible(true);


    if (editorKey == "database" || editorKey == "scenario")
    {

        openMissionFileAction->setVisible(false);
    }
    else if (editorKey == "mission")
    {

        newFileAction->setVisible(false);
        loadXmlAction->setVisible(false);
        openMissionFileAction->setVisible(false);
    }
    else if (editorKey == "analysis")
    {

        newFileAction->setVisible(false);
        recentProjectAction->setVisible(false);
        recentProjectLibraryAction->setVisible(false);
        loadJsonAction->setVisible(false);
        loadXmlAction->setVisible(false);
        loadToLibraryAction->setVisible(false);
        openRuntimeInstanceAction->setVisible(false);
        openMissionFileAction->setVisible(false);
        sameSaveAction->setVisible(false);
        saveJsonAction->setVisible(false);
    }
}
QAction* MenuBar::getProfileAction()
{
    return profileAction;
}
