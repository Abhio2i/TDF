

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

    // Add actions to the toolbar
    addAction(newAction);
    addAction(saveAction);
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
    // addAction(testScriptAction);

    // Connect testScriptAction to open the dialog
    connect(testScriptAction, &QAction::triggered, this, &StandardToolBar::onTestScriptTriggered);

    Console::log("StandardToolBar initialized with addTrajectoryAction and testScriptAction");
}

void StandardToolBar::createActions()
{
    newAction = new QAction(QIcon(withWhiteBg(":/icons/images/new-document.png")), tr("New"), this);
    newAction->setShortcut(QKeySequence::New);

    saveAction = new QAction(QIcon(withWhiteBg(":/icons/images/floppy-disk.png")), tr("SaveAction"), this);
    saveAction->setShortcut(QKeySequence::Save);

    saveAllAction = new QAction(QIcon(withWhiteBg(":/icons/images/floppy-disk.png")), tr("Save All"), this);

    cutAction = new QAction(QIcon(withWhiteBg(":/icons/images/cut.png")), tr("Cut"), this);
    cutAction->setShortcut(QKeySequence::Cut);

    copyAction = new QAction(QIcon(withWhiteBg(":/icons/images/copy.png")), tr("Copy"), this);
    copyAction->setShortcut(QKeySequence::Copy);

    pasteAction = new QAction(QIcon(withWhiteBg(":/icons/images/paste.png")), tr("Paste"), this);
    pasteAction->setShortcut(QKeySequence::Paste);

    undoAction = new QAction(QIcon(withWhiteBg(":/icons/images/undo.png")), tr("Undo"), this);
    undoAction->setShortcut(QKeySequence::Undo);

    redoAction = new QAction(QIcon(withWhiteBg(":/icons/images/redo.png")), tr("Redo"), this);
    redoAction->setShortcut(QKeySequence::Redo);

    addTrajectoryAction = new QAction(QIcon(withWhiteBg(":/icons/images/trajectory.png")), tr("Add Trajectory"), this);
    addTrajectoryAction->setEnabled(false);

    // Verify trajectory icon
    if (addTrajectoryAction->icon().isNull()) {
        Console::error("Failed to load trajectory icon from :/icons/images/trajectory.png");
    } else {
        Console::log("Trajectory icon loaded successfully");
    }

    testScriptAction = new QAction(QIcon(withWhiteBg(":/icons/images/test.png")), tr("Test Script"), this);
    testScriptAction->setEnabled(true);

    // Verify test script icon
    if (testScriptAction->icon().isNull()) {
        Console::error("Failed to load test script icon from :/icons/images/test.png");
    } else {
        Console::log("Test script icon loaded successfully");
    }
}

void StandardToolBar::onTestScriptTriggered()
{
    Console::log("Opening TestScriptDialog");
    TestScriptDialog *dialog = new TestScriptDialog(this);
    dialog->exec(); // Changed from show() to exec() for modal dialog
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
