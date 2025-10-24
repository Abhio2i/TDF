

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "GUI/Navigation/navigationpage.h"
#include "GUI/Editors/databaseeditor.h"
#include "GUI/Editors/scenarioeditor.h"
#include "GUI/Editors/runtimeeditor.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void switchEditor(const QString &editorKey);

private:
    void setupUI();
    QMainWindow* getCurrentEditor() const;
    bool handleUnsavedChanges();

    QStackedWidget *stackedWidget;
    NavigationPage *navigationPage;
    DatabaseEditor *databaseEditor;
    ScenarioEditor *scenarioEditor;
    RuntimeEditor *runtimeEditor;
};

#endif // MAINWINDOW_H
