#include "menubar.h"
MenuBar::MenuBar(QWidget *parent) : QMenuBar(parent)
{
    // File menu
    fileMenu = addMenu("File");

    newFileAction = new QAction("New File", this);
    recentProjectAction = new QAction("Recent Project", this);
    loadJsonAction = new QAction("Open File", this);
  loadToLibraryAction = new QAction("Open File to Library", this);
    saveJsonAction = new QAction("Save As", this);
    runAction = new QAction("Run", this);
    exitAction = new QAction("Exit", this);

    fileMenu->addAction(newFileAction);
    fileMenu->addAction(recentProjectAction);
    fileMenu->addSeparator();
    fileMenu->addAction(loadJsonAction);
    fileMenu->addAction(loadToLibraryAction);
    fileMenu->addAction(saveJsonAction);
    fileMenu->addSeparator();
    fileMenu->addAction(runAction);
    fileMenu->addAction(exitAction);

    // Edit menu
    editMenu = addMenu("Edit");

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
    playAction = new QAction("Play", this);
    pauseAction = new QAction("Pause", this);

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

    // View menu
    viewMenu = addMenu("View");
    add3DViewAction = new QAction("Add 3D View", this);
    remove3DViewAction = new QAction("Remove 3D View", this);
    viewMenu->addAction(add3DViewAction);
    viewMenu->addAction(remove3DViewAction);

    // Feedback menu
    feedbackMenu = addMenu("Feedback");
    feedbackAction = new QAction("Open Feedback Page", this);
    feedbackMenu->addAction(feedbackAction);

    // Connect actions to signals
    connect(feedbackAction, &QAction::triggered, this, &MenuBar::feedbackTriggered);
    connect(newFileAction, &QAction::triggered, this, &MenuBar::newFileTriggered);
    connect(recentProjectAction, &QAction::triggered, this, &MenuBar::recentProjectTriggered);
    connect(loadJsonAction, &QAction::triggered, this, &MenuBar::loadTriggered);
    connect(loadToLibraryAction, &QAction::triggered, this, &MenuBar::loadToLibraryTriggered);
    connect(saveJsonAction, &QAction::triggered, this, &MenuBar::saveTriggered);
    connect(runAction, &QAction::triggered, this, &MenuBar::runTriggered);
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

QMenu* MenuBar::getFileMenu() { return fileMenu; }
QMenu* MenuBar::getEditMenu() { return editMenu; }
QMenu* MenuBar::getViewMenu() { return viewMenu; }
QAction* MenuBar::getLoadAction() { return loadJsonAction; }
QAction* MenuBar::getLoadToLibraryAction() { return loadToLibraryAction; }
QAction* MenuBar::getSaveAction() { return saveJsonAction; }
QAction* MenuBar::getFeedbackAction() { return feedbackAction; }
QAction* MenuBar::getNewFileAction() { return newFileAction; }
QAction* MenuBar::getRecentProjectAction() { return recentProjectAction; }
QAction* MenuBar::getRunAction() { return runAction; }
QAction* MenuBar::getExitAction() { return exitAction; }
QAction* MenuBar::getUndoAction() { return undoAction; }
QAction* MenuBar::getRedoAction() { return redoAction; }
QAction* MenuBar::getSelectAllAction() { return selectAllAction; }
QAction* MenuBar::getDeselectAllAction() { return deselectAllAction; }
QAction* MenuBar::getCutAction() { return cutAction; }
QAction* MenuBar::getCopyAction() { return copyAction; }
QAction* MenuBar::getPasteAction() { return pasteAction; }
QAction* MenuBar::getDuplicateAction() { return duplicateAction; }
QAction* MenuBar::getRenameAction() { return renameAction; }
QAction* MenuBar::getDeleteAction() { return deleteAction; }
QAction* MenuBar::getPlayAction() { return playAction; }
QAction* MenuBar::getPauseAction() { return pauseAction; }
QAction* MenuBar::getAdd3DViewAction() { return add3DViewAction; }
QAction* MenuBar::getRemove3DViewAction() { return remove3DViewAction; }
