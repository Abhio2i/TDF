
#include "mainwindow.h"
#include <QDockWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Editor Application");
    resize(1900, 1000);

    setupUI();
}

MainWindow::~MainWindow()
{
    // Qt's parent-child relationship handles cleanup
}

void MainWindow::setupUI()
{
    // Create central widget and layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // Initialize stacked widget
    stackedWidget = new QStackedWidget(centralWidget);
    layout->addWidget(stackedWidget);

    // Initialize editors
    databaseEditor = new DatabaseEditor(centralWidget);
    scenarioEditor = new ScenarioEditor(centralWidget);
    runtimeEditor = new RuntimeEditor(centralWidget);

    // Add editors to stacked widget
    stackedWidget->addWidget(databaseEditor);
    stackedWidget->addWidget(scenarioEditor);
    stackedWidget->addWidget(runtimeEditor);

    // Set central widget
    setCentralWidget(centralWidget);

    // Create navigation dock
    QDockWidget *navigationDock = new QDockWidget("", this);
    navigationDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    navigationDock->setFeatures(QDockWidget::DockWidgetClosable |
                                QDockWidget::DockWidgetMovable |
                                QDockWidget::DockWidgetFloatable);
    navigationPage = new NavigationPage(this);
    navigationDock->setWidget(navigationPage);
    navigationDock->setMinimumWidth(50);
    navigationDock->setMaximumWidth(60);
    navigationDock->resize(45, navigationDock->height());
    addDockWidget(Qt::LeftDockWidgetArea, navigationDock);

    // Connect navigation signals
    connect(navigationPage, &NavigationPage::editorRequested, this, &MainWindow::switchEditor);

    // Set initial editor (e.g., DatabaseEditor)
    stackedWidget->setCurrentWidget(databaseEditor);
}

void MainWindow::switchEditor(const QString &editorKey)
{
    if (editorKey == "database") {
        stackedWidget->setCurrentWidget(databaseEditor);
    } else if (editorKey == "scenario") {
        if (!databaseEditor->lastSavedFilePath.isEmpty()) {
            scenarioEditor->loadFromJsonFile(databaseEditor->lastSavedFilePath);
        }
        stackedWidget->setCurrentWidget(scenarioEditor);
    } else if (editorKey == "runtime") {
        if (!scenarioEditor->lastSavedFilePath.isEmpty()) {
            runtimeEditor->loadFromJsonFile(scenarioEditor->lastSavedFilePath);
        }
        stackedWidget->setCurrentWidget(runtimeEditor);
    }
}
