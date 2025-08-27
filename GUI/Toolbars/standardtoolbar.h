

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
    QAction* getSaveAction() const { return saveAction; }
    QAction* getTestScriptAction() const { return testScriptAction; }

private slots:
    void onTestScriptTriggered(); // Slot to open the dialog
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
    QAction *testScriptAction;

    void createActions();
    QPixmap withWhiteBg(const QString &iconPath);
};

#endif // STANDARDTOOLBAR_H
