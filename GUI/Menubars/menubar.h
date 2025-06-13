/* Header guard section */
#ifndef MENUBAR_H
#define MENUBAR_H

/* Includes section */
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <GUI/Feedback/feedback.h>

/* Class declaration section */
class MenuBar : public QMenuBar
{
    /* Qt meta-object section */
    Q_OBJECT

public:
    /* Constructor section */
    explicit MenuBar(QWidget *parent = nullptr);

    /* Menu and action accessor section */
    QMenu* getFileMenu();
    QAction* getLoadAction();
    QAction* getSaveAction();
    QAction* getFeedbackAction();

signals:
    /* Signals section */
    void feedbackTriggered();

private:
    /* Private members section */
    QMenu* fileMenu;
    QAction* loadJsonAction;
    QAction* saveJsonAction;
    QMenu* feedbackMenu;
    /* Feedback members section */
    QAction* feedbackAction;
};

/* End of header guard section */
#endif // MENUBAR_H
