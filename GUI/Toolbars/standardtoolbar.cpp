
/* =============================================================================
 * FILE:         standardtoolbar.cpp
 * MODULE:       Standard Toolbar
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the StandardToolBar class which provides a toolbar for
 *               standard editing actions (new, save, cut, copy, paste, undo,
 *               redo) along with add trajectory and test script actions.
 *               Integrates with the main application to provide common file
 *               and edit operations in a consistent toolbar interface.
 *
 * REQUIREMENTS: Implements REQ-STDTOOLBAR-010 through REQ-STDTOOLBAR-016
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-STDTOOLBAR-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#include "standardtoolbar.h"
#include "GUI/Testscript/testscriptdialog.h"
#include <QIcon>
#include <QKeySequence>
#include <QPainter>
#include "core/Debug/console.h"

StandardToolBar::StandardToolBar(QWidget *parent)
    : QToolBar(parent)
{
    setObjectName("StandardToolBar");
    setWindowTitle("Standard Tools");
    createActions();
    addAction(newAction);
    addAction(saveAllAction);
    addSeparator();
    addAction(cutAction);
    addAction(copyAction);
    addAction(pasteAction);
    addSeparator();
    addAction(undoAction);
    addAction(redoAction);
    addSeparator();
    addAction(addTrajectoryAction);
    connect(testScriptAction, &QAction::triggered, this, &StandardToolBar::onTestScriptTriggered);
}

void StandardToolBar::createActions()
{
    newAction = new QAction(QIcon(withWhiteBg(":/icons/images/new-document.png")), tr("New"), this);
    saveAllAction = new QAction(QIcon(withWhiteBg(":/icons/images/floppy-disk.png")), tr("Save All"), this);
    cutAction = new QAction(QIcon(withWhiteBg(":/icons/images/cut.png")), tr("Cut"), this);
    copyAction = new QAction(QIcon(withWhiteBg(":/icons/images/copy.png")), tr("Copy"), this);
    pasteAction = new QAction(QIcon(withWhiteBg(":/icons/images/paste.png")), tr("Paste"), this);
    undoAction = new QAction(QIcon(withWhiteBg(":/icons/images/undo.png")), tr("Undo"), this);
    undoAction->setShortcut(QKeySequence::Undo);
    redoAction = new QAction(QIcon(withWhiteBg(":/icons/images/redo.png")), tr("Redo"), this);
    redoAction->setShortcut(QKeySequence::Redo);
    addTrajectoryAction = new QAction(QIcon(withWhiteBg(":/icons/images/trajectory.png")), tr("Add Trajectory"), this);
    addTrajectoryAction->setEnabled(false);
    if (addTrajectoryAction->icon().isNull()) {
        Console::error("Failed to load trajectory icon from :/icons/images/trajectory.png");
    } else {
    }

    testScriptAction = new QAction(QIcon(withWhiteBg(":/icons/images/test.png")), tr("Test Script"), this);
    testScriptAction->setEnabled(true);


    if (testScriptAction->icon().isNull()) {

    } else {

    }
}
void StandardToolBar::onTestScriptTriggered()
{

    TestScriptDialog *window = new TestScriptDialog(this);
    connect(window, &TestScriptDialog::closed, this, [=]() {
        window->deleteLater();
    });
    window->show();
}
QPixmap StandardToolBar::withWhiteBg(const QString &iconPath)
{
    QPixmap pixmap(iconPath);
    if (pixmap.isNull()) {
        Console::error("Failed to load pixmap from: " + iconPath.toStdString());
        return QPixmap();
    }

    QPixmap newPixmap(pixmap.size());
    if (newPixmap.isNull()) {
        Console::error("Failed to create new pixmap for: " + iconPath.toStdString());
        return QPixmap();
    }
    newPixmap.fill(Qt::gray);

    QPainter painter(&newPixmap);
    if (!painter.isActive()) {
        Console::error("Failed to initialize QPainter for: " + iconPath.toStdString());
        return newPixmap;
    }
    painter.drawPixmap(0, 0, pixmap);
    painter.end();

    return newPixmap;
}
