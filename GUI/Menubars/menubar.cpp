

#include "menubar.h"

MenuBar::MenuBar(QWidget *parent) : QMenuBar(parent)
{
    // File menu
    fileMenu = addMenu("File");

    loadJsonAction = new QAction("📂 Load JSON", this);
    saveJsonAction = new QAction("💾 Save JSON", this);

    fileMenu->addAction(loadJsonAction);
    fileMenu->addAction(saveJsonAction);

    // Other menus
    addMenu("Edit");
    addMenu("View");
    addMenu("Database");
    addMenu("Tools");

    // Feedback menu
    feedbackMenu = addMenu("Feedback");
    feedbackAction = new QAction("Open Feedback Page", this);  // Action for Feedback
    feedbackMenu->addAction(feedbackAction);

    // Connect Feedback action to the signal
    connect(feedbackAction, &QAction::triggered, this, &MenuBar::feedbackTriggered);
}

QMenu* MenuBar::getFileMenu()
{
    return fileMenu;
}

QAction* MenuBar::getLoadAction()
{
    return loadJsonAction;
}

QAction* MenuBar::getSaveAction()
{
    return saveJsonAction;
}

QAction* MenuBar::getFeedbackAction()
{
    return feedbackAction;
}
