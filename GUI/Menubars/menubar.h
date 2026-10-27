#ifndef MENUBAR_H
#define MENUBAR_H

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <GUI/Feedback/projectinformation.h>

class MenuBar : public QMenuBar
{
    Q_OBJECT

public:
    explicit MenuBar(QWidget *parent = nullptr);

    QMenu*   getFileMenu();

    QAction* getLoadAction();
    QAction* getLoadToLibraryAction();
    QAction* getSameSaveAction();
    QAction* getSaveAction();
    QAction* getFeedbackAction();
    QAction* getNewFileAction();
    QAction* getRecentProjectAction();
    QAction* getrecentProjectLibraryAction();
    QAction* getExitAction();
    QAction* getProfileAction();
    QAction* getApplicationAction() { return applicationAction; }
    QAction* getLoadXmlAction();
    QAction* getOpenRuntimeInstanceAction();
    QAction* getOpenMissionFileAction();
    QAction* getPluginsAction();

    void setLibraryActionsVisible(bool visible);
    void updateFileMenuForEditor(const QString& editorKey);

signals:
    void feedbackTriggered();
    void newFileTriggered();
    void recentProjectTriggered();
    void recentProjectLibraryTriggered();
    void loadTriggered();
    void loadToLibraryTriggered();
    void sameSaveTriggered();
    void saveTriggered();
    void exitTriggered();
    void profileTriggered();
    void applicationTriggered();
    void loadXmlTriggered();
    void openRuntimeInstanceTriggered();
    void openMissionFileTriggered();
    void pluginsTriggered();

private:
    QMenu*   fileMenu    = nullptr;
    QMenu*   feedbackMenu= nullptr;

    QAction* newFileAction             = nullptr;
    QAction* recentProjectAction       = nullptr;
    QAction* recentProjectLibraryAction= nullptr;
    QAction* loadJsonAction            = nullptr;
    QAction* loadXmlAction             = nullptr;
    QAction* loadToLibraryAction       = nullptr;
    QAction* openRuntimeInstanceAction = nullptr;
    QAction* openMissionFileAction     = nullptr;
    QAction* sameSaveAction            = nullptr;
    QAction* saveJsonAction            = nullptr;
    QAction* exitAction                = nullptr;
    QAction* feedbackAction            = nullptr;
    QAction* profileAction             = nullptr;
    QAction* applicationAction         = nullptr;
    QAction* pluginsAction             = nullptr;
};

#endif // MENUBAR_H
