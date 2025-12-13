#ifndef BASEEDITOR_H
#define BASEEDITOR_H

#include <QMainWindow>
#include <QStatusBar>
#include "GUI/Hierarchytree/hierarchyconnector.h"
#include "GUI/Menubars/menubar.h"

class BaseEditor : public QMainWindow
{
    Q_OBJECT

public:
    BaseEditor(QWidget *parent = nullptr);
    virtual ~BaseEditor();

    // Common recent project functionality
    void onRecentProjectTriggered();
    void loadRecentProject(const QString &filePath);
    void clearRecentProjects();
    void setupRecentProjectsMenu();

protected:
    MenuBar *menuBar = nullptr;
    Hierarchy *hierarchy = nullptr;
    // Console *console = nullptr;
    QStatusBar *statusBar = nullptr;
    QString lastSavedFilePath;
    bool hasUnsavedChanges = false;

    virtual void updateUIAfterLoad() = 0;
    virtual void markUnsavedChanges() = 0;
    virtual void clearUnsavedChanges() = 0;
    virtual void updateStatusBar(const QString &message);

private slots:
    void showProfileInfo();
};

#endif // BASEEDITOR_H
