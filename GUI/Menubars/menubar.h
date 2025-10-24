

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
    QMenu* getEditMenu();
    QMenu* getViewMenu();
    QAction* getLoadAction();
    QAction* getLoadToLibraryAction();
    QAction* getSameSaveAction();
    QAction* getSaveAction();
    QAction* getFeedbackAction();
    QAction* getNewFileAction();
    QAction* getRecentProjectAction();
    // QAction* getRunAction();
    QAction* getExitAction();
    QAction* getUndoAction();
    QAction* getRedoAction();
    QAction* getSelectAllAction();
    QAction* getDeselectAllAction();
    QAction* getCutAction();
    QAction* getCopyAction();
    QAction* getPasteAction();
    QAction* getDuplicateAction();
    QAction* getRenameAction();
    QAction* getDeleteAction();
    QAction* getPlayAction();
    QAction* getPauseAction();
    QAction* getAdd3DViewAction();
    QAction* getRemove3DViewAction();

signals:
    void feedbackTriggered();
    void newFileTriggered();
    void recentProjectTriggered();
    void loadTriggered();
    void loadToLibraryTriggered();
    void sameSaveTriggered();
    void saveTriggered();
    // void runTriggered();
    void exitTriggered();
    void undoTriggered();
    void redoTriggered();
    void selectAllTriggered();
    void deselectAllTriggered();
    void cutTriggered();
    void copyTriggered();
    void pasteTriggered();
    void duplicateTriggered();
    void renameTriggered();
    void deleteTriggered();
    void playTriggered();
    void pauseTriggered();
    void add3DViewTriggered();
    void remove3DViewTriggered();

private:
    QMenu* fileMenu;
    QMenu* editMenu;
    QMenu* viewMenu;
    QAction* newFileAction;
    QAction* recentProjectAction;
    QAction* loadJsonAction;
    QAction* loadToLibraryAction;
     QAction* sameSaveAction;
    QAction* saveJsonAction;
    // QAction* runAction;
    QAction* exitAction;
    QAction* undoAction;
    QAction* redoAction;
    QAction* selectAllAction;
    QAction* deselectAllAction;
    QAction* cutAction;
    QAction* copyAction;
    QAction* pasteAction;
    QAction* duplicateAction;
    QAction* renameAction;
    QAction* deleteAction;
    QAction* playAction;
    QAction* pauseAction;
    QAction* add3DViewAction;
    QAction* remove3DViewAction;
    QMenu* feedbackMenu;
    QAction* feedbackAction;
};

#endif // MENUBAR_H
