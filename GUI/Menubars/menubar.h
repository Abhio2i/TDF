// #ifndef MENUBAR_H
// #define MENUBAR_H

// #include <QMenuBar>
// #include <QMenu>
// #include <QAction>

// class MenuBar : public QMenuBar
// {
//     Q_OBJECT

// public:
//     explicit MenuBar(QWidget *parent = nullptr);
//     QMenu* getFileMenu();

//     QAction* getLoadAction();
//     QAction* getSaveAction();

// private:
//     QMenu* fileMenu;
//     QAction* loadJsonAction;
//     QAction* saveJsonAction;
// };

// #endif // MENUBAR_H


#ifndef MENUBAR_H
#define MENUBAR_H

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <GUI/Feedback/feedback.h>

class MenuBar : public QMenuBar
{
    Q_OBJECT

public:
    explicit MenuBar(QWidget *parent = nullptr);
    QMenu* getFileMenu();
    QAction* getLoadAction();
    QAction* getSaveAction();
    QAction* getFeedbackAction();  // Added getter for Feedback action

signals:
    void feedbackTriggered();  // Signal to emit when Feedback is clicked

private:
    QMenu* fileMenu;
    QAction* loadJsonAction;
    QAction* saveJsonAction;
    QMenu* feedbackMenu;       // Added for Feedback menu
    QAction* feedbackAction;   // Added for Feedback action
};

#endif // MENUBAR_H
