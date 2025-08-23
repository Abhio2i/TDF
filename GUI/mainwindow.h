#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "GUI/Editors/databaseeditor.h"
#include "GUI/Editors/scenarioeditor.h"
#include "GUI/Editors/runtimeeditor.h"
#include "GUI/Navigation/navigationpage.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void switchEditor(const QString &editorKey);

private:
    void setupUI();

    QStackedWidget *stackedWidget;
    DatabaseEditor *databaseEditor;
    ScenarioEditor *scenarioEditor;
    RuntimeEditor *runtimeEditor;
    NavigationPage *navigationPage;
};

#endif // MAINWINDOW_H
