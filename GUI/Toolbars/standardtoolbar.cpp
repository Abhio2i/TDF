
#include "standardtoolbar.h"
#include <QIcon>
#include <QKeySequence>
#include <QPainter>

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
    addTrajectoryAction->setEnabled(false); // Initially disabled
}

QPixmap StandardToolBar::withWhiteBg(const QString &iconPath)
{
    QPixmap pixmap(iconPath);
    if (pixmap.isNull()) return QPixmap();  // Safety check

    QPixmap newPixmap(pixmap.size());
    newPixmap.fill(Qt::gray);

    QPainter painter(&newPixmap);
    painter.drawPixmap(0, 0, pixmap);
    painter.end();

    return newPixmap;
}
