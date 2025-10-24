
#include "mainwindow.h"
#include <QDockWidget>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>

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
    // Set initial editor
    stackedWidget->setCurrentWidget(databaseEditor);
}

QMainWindow* MainWindow::getCurrentEditor() const
{
    QWidget *currentWidget = stackedWidget->currentWidget();
    if (currentWidget == databaseEditor)
        return databaseEditor;
    else if (currentWidget == scenarioEditor)
        return scenarioEditor;
    else if (currentWidget == runtimeEditor)
        return runtimeEditor;
    return nullptr;
}

bool MainWindow::handleUnsavedChanges()
{
    QMainWindow *currentEditor = getCurrentEditor();
    if (!currentEditor)
        return true; // No editor, proceed with switch

    bool hasUnsavedChanges = false;
    QString editorName;
    QString lastSavedFilePath;

    // Check which editor is current and get its unsaved changes status
    if (currentEditor == databaseEditor) {
        hasUnsavedChanges = databaseEditor->hasUnsavedChanges;
        editorName = "Database Editor";
        lastSavedFilePath = databaseEditor->lastSavedFilePath;
    } else if (currentEditor == scenarioEditor) {
        hasUnsavedChanges = scenarioEditor->hasUnsavedChanges;
        editorName = "Scenario Editor";
        lastSavedFilePath = scenarioEditor->lastSavedFilePath;
    } else if (currentEditor == runtimeEditor) {
        hasUnsavedChanges = runtimeEditor->hasUnsavedChanges;
        editorName = "Runtime Editor";
        lastSavedFilePath = runtimeEditor->lastSavedFilePath;
    }

    if (!hasUnsavedChanges)
        return true; // No unsaved changes, proceed with switch

    // Show prompt
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Unsaved Changes",
        QString("There are unsaved changes in the %1. Do you want to save before switching?").arg(editorName),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );

    if (reply == QMessageBox::Save) {
        // Trigger save action
        MenuBar *menuBar = qobject_cast<MenuBar*>(currentEditor->menuBar());
        if (!menuBar) {
            qWarning() << "MenuBar not found in current editor";
            return false;
        }
        QAction *saveAction = lastSavedFilePath.isEmpty() ? menuBar->getSaveAction() : menuBar->getSameSaveAction();
        if (saveAction) {
            saveAction->trigger();
            // Check if save was successful (hasUnsavedChanges should be false)
            if (currentEditor == databaseEditor)
                hasUnsavedChanges = databaseEditor->hasUnsavedChanges;
            else if (currentEditor == scenarioEditor)
                hasUnsavedChanges = scenarioEditor->hasUnsavedChanges;
            else if (currentEditor == runtimeEditor)
                hasUnsavedChanges = runtimeEditor->hasUnsavedChanges;
            return !hasUnsavedChanges; // Proceed only if save was successful
        } else {
            qWarning() << "Save action not found";
            return false;
        }
    } else if (reply == QMessageBox::Discard) {
        // Discard changes and proceed
        if (currentEditor == databaseEditor)
            databaseEditor->clearUnsavedChanges();
        else if (currentEditor == scenarioEditor)
            scenarioEditor->clearUnsavedChanges();
        else if (currentEditor == runtimeEditor)
            runtimeEditor->clearUnsavedChanges();
        return true;
    } else { // Cancel
        return false; // Cancel the switch
    }
}

void MainWindow::switchEditor(const QString &editorKey)
{
    // Check for unsaved changes before switching
    if (!handleUnsavedChanges()) {
        qDebug() << "Editor switch cancelled due to unsaved changes";
        return;
    }

    // Proceed with editor switch
    if (editorKey == "database") {
        stackedWidget->setCurrentWidget(databaseEditor);
        qDebug() << "Switched to Database Editor";
    } else if (editorKey == "scenario") {
        if (!databaseEditor->lastSavedFilePath.isEmpty()) {
            scenarioEditor->loadFromJsonFile(databaseEditor->lastSavedFilePath);
            qDebug() << "Loaded DatabaseEditor data into ScenarioEditor from:" << databaseEditor->lastSavedFilePath;
        }
        stackedWidget->setCurrentWidget(scenarioEditor);
        qDebug() << "Switched to Scenario Editor";
    } else if (editorKey == "runtime") {
        if (!scenarioEditor->lastSavedFilePath.isEmpty()) {
            runtimeEditor->loadFromJsonFile(scenarioEditor->lastSavedFilePath);
            qDebug() << "Loaded ScenarioEditor data into RuntimeEditor from:" << scenarioEditor->lastSavedFilePath;
        }
        stackedWidget->setCurrentWidget(runtimeEditor);
        qDebug() << "Switched to Runtime Editor";
    }
}
