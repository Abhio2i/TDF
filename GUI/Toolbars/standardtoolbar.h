
// #ifndef STANDARDTOOLBAR_H
// #define STANDARDTOOLBAR_H

// #include <QToolBar>
// #include <QAction>
// #include <QPixmap>

// class StandardToolBar : public QToolBar
// {
//     Q_OBJECT

// public:
//     explicit StandardToolBar(QWidget *parent = nullptr);

// private:
//     QAction *newAction;
//     QAction *saveAction;
//     QAction *saveAllAction;
//     QAction *cutAction;
//     QAction *copyAction;
//     QAction *pasteAction;
//     QAction *undoAction;
//     QAction *redoAction;

//     void createActions();
//     QPixmap withWhiteBg(const QString &iconPath);
// };

// #endif // STANDARDTOOLBAR_H



#ifndef STANDARDTOOLBAR_H
#define STANDARDTOOLBAR_H

#include <QToolBar>
#include <QAction>
#include <QPixmap>

class StandardToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit StandardToolBar(QWidget *parent = nullptr);
    QAction* getAddTrajectoryAction() const { return addTrajectoryAction; }
    QAction* getSaveAction() const { return saveAction; } // Added getter for saveAction

private:
    QAction *newAction;
    QAction *saveAction;
    QAction *saveAllAction;
    QAction *cutAction;
    QAction *copyAction;
    QAction *pasteAction;
    QAction *undoAction;
    QAction *redoAction;
    QAction *addTrajectoryAction;

    void createActions();
    QPixmap withWhiteBg(const QString &iconPath);
};

#endif // STANDARDTOOLBAR_H
