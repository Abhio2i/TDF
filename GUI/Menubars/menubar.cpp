#include "menubar.h"
#include "menubar-styles.h"
#include <QMenu>
#include <QAction>
#include <QKeySequence>

MenuBar::MenuBar(QWidget *parent)
    : QMenuBar(parent)
{
    setStyleSheet(MenuBarStyles::MenuBar);

    // ── File Menu ────────────────────────────────────────────────────────
    fileMenu = addMenu("File");
    fileMenu->setStyleSheet(MenuBarStyles::Menu);

    newFileAction              = new QAction("New File", this);
    recentProjectAction        = new QAction("Recent Project", this);
    recentProjectLibraryAction = new QAction("Recent Library", this);
    loadJsonAction             = new QAction("Open File", this);
    loadXmlAction              = new QAction("Open XML File", this);
    loadToLibraryAction        = new QAction("Open File to Library", this);
    openRuntimeInstanceAction  = new QAction("Open Runtime Instance", this);
    openMissionFileAction      = new QAction("Open Mission File", this);
    sameSaveAction             = new QAction("Save", this);
    sameSaveAction->setShortcut(QKeySequence("Ctrl+S"));
    saveJsonAction             = new QAction("Save As", this);
    exitAction                 = new QAction("Exit", this);

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

    // ── Settings / Profile ───────────────────────────────────────────────
    profileAction = addAction("Performance");
    profileAction->setMenuRole(QAction::NoRole);

    applicationAction = addAction("Settings");
    applicationAction->setMenuRole(QAction::NoRole);

    pluginsAction = addAction("Plugins");
    pluginsAction->setMenuRole(QAction::NoRole);

    // ── About Menu ───────────────────────────────────────────────────────
    feedbackMenu = addMenu("About");
    feedbackMenu->setStyleSheet(MenuBarStyles::Menu);
    feedbackAction = new QAction("Open About Page", this);
    feedbackMenu->addAction(feedbackAction);

    // ── Signal Connections ───────────────────────────────────────────────
    connect(newFileAction,             &QAction::triggered, this, &MenuBar::newFileTriggered);
    connect(recentProjectAction,       &QAction::triggered, this, &MenuBar::recentProjectTriggered);
    connect(recentProjectLibraryAction,&QAction::triggered, this, &MenuBar::recentProjectLibraryTriggered);
    connect(loadJsonAction,            &QAction::triggered, this, &MenuBar::loadTriggered);
    connect(loadXmlAction,             &QAction::triggered, this, &MenuBar::loadXmlTriggered);
    connect(loadToLibraryAction,       &QAction::triggered, this, &MenuBar::loadToLibraryTriggered);
    connect(openRuntimeInstanceAction, &QAction::triggered, this, &MenuBar::openRuntimeInstanceTriggered);
    connect(openMissionFileAction,     &QAction::triggered, this, &MenuBar::openMissionFileTriggered);
    connect(sameSaveAction,            &QAction::triggered, this, &MenuBar::sameSaveTriggered);
    connect(saveJsonAction,            &QAction::triggered, this, &MenuBar::saveTriggered);
    connect(exitAction,                &QAction::triggered, this, &MenuBar::exitTriggered);
    connect(profileAction,             &QAction::triggered, this, &MenuBar::profileTriggered);
    connect(applicationAction,         &QAction::triggered, this, &MenuBar::applicationTriggered);
    connect(pluginsAction,             &QAction::triggered, this, &MenuBar::pluginsTriggered);
    connect(feedbackAction,            &QAction::triggered, this, &MenuBar::feedbackTriggered);
}

// ── Getters ──────────────────────────────────────────────────────────────
QMenu*   MenuBar::getFileMenu()                   { return fileMenu; }
QAction* MenuBar::getLoadAction()                 { return loadJsonAction; }
QAction* MenuBar::getLoadToLibraryAction()        { return loadToLibraryAction; }
QAction* MenuBar::getSameSaveAction()             { return sameSaveAction; }
QAction* MenuBar::getSaveAction()                 { return saveJsonAction; }
QAction* MenuBar::getFeedbackAction()             { return feedbackAction; }
QAction* MenuBar::getNewFileAction()              { return newFileAction; }
QAction* MenuBar::getRecentProjectAction()        { return recentProjectAction; }
QAction* MenuBar::getrecentProjectLibraryAction() { return recentProjectLibraryAction; }
QAction* MenuBar::getExitAction()                 { return exitAction; }
QAction* MenuBar::getProfileAction()              { return profileAction; }
QAction* MenuBar::getLoadXmlAction()              { return loadXmlAction; }
QAction* MenuBar::getOpenRuntimeInstanceAction()  { return openRuntimeInstanceAction; }
QAction* MenuBar::getOpenMissionFileAction()      { return openMissionFileAction; }
QAction* MenuBar::getPluginsAction()              { return pluginsAction; }

// ── Library Actions Visibility ───────────────────────────────────────────
void MenuBar::setLibraryActionsVisible(bool visible)
{
    recentProjectLibraryAction->setVisible(visible);
    loadToLibraryAction->setVisible(visible);
    openRuntimeInstanceAction->setVisible(visible);
}

// ── Update File Menu Per Editor ──────────────────────────────────────────
void MenuBar::updateFileMenuForEditor(const QString& editorKey)
{
    // Reset all to default
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

    if (editorKey == "database") {
        openMissionFileAction->setVisible(false);
        openRuntimeInstanceAction->setVisible(false);
    }
    else if (editorKey == "scenario") {
        openMissionFileAction->setVisible(false);
    }
    else if (editorKey == "runtime") {
        loadXmlAction->setVisible(true);
        openRuntimeInstanceAction->setVisible(true);
        openMissionFileAction->setVisible(false);
    }
    else if (editorKey == "mission") {
        newFileAction->setVisible(false);
        loadXmlAction->setVisible(false);
        openMissionFileAction->setVisible(false);
    }
    else if (editorKey == "analysis") {
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
