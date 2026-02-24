//============================================================================
// File        : mainwindow.cpp
// Description : Implementation of MainWindow class for the main application
//               window that manages database, scenario, and runtime editors
//               with navigation, unsaved changes handling, and editor switching.
//               Written by Arti Rajpoot
//============================================================================
#include "mainwindow.h"
#include "mainwindow-styles.h"
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QApplication>
#include <QStyleFactory>
#include <QMessageBox>
#include <QDebug>
#include <QFileInfo>
#include <QCloseEvent>
#include "Setup.h"
#include <QProxyStyle>
#include <QStyleFactory>
ScenarioConfig* MainWindow::scenarioconfig = nullptr;
MainWindow* MainWindow::s_instance = nullptr;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{

    setStyleSheet(MainWindowStyles::MainWindow);
        qApp->setStyle(QStyleFactory::create("Fusion"));


        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor("#0F2636"));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        // darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
        // darkPalette.setColor(QPalette::AlternateBase, QColor(45, 45, 45));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        // darkPalette.setColor(QPalette::Button, QColor(45, 45, 45));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        // darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        qApp->setPalette(darkPalette);
        qApp->setStyleSheet(qApp->styleSheet() + MainWindowStyles::ToolTip);
    MainWindow::s_instance = this;
    MainWindow::scenarioconfig = new ScenarioConfig();
    setWindowTitle("Indigenous Scenario and Sensor Simulation Toolkit");
    if (scenarioconfig && !scenarioconfig->software_version.isEmpty()) {
        setWindowTitle(windowTitle() + QString(" - V_%1").arg(scenarioconfig->software_version));
    }
    resize(1900, 1000);
    setupUI();
    setAttribute(Qt::WA_DeleteOnClose);
}

MainWindow::~MainWindow()
{
    qDebug()<<"Mainwindow delete";
    //Console::log("Mainwindow delete");
    std::cout << "Mainwindow delete";
    delete databaseEditor;
    delete scenarioEditor;
    delete runtimeEditor;
    // Qt's parent-child relationship handles cleanup
}

void MainWindow::setupUI()
{
    QWidget *topBarWidget = new QWidget(this);
    topBarWidget->setObjectName("topBarWidget");
    topBarWidget->setFixedHeight(50);
    topBarWidget->setStyleSheet(MainWindowStyles::TopBarWidget);

    QHBoxLayout *topBarLayout = new QHBoxLayout(topBarWidget);
    topBarLayout->setContentsMargins(0, 0, 0, 0);
    topBarLayout->setSpacing(0);

    // ========== 1. Menu Bar Section ==========
    mainMenuBar = new MenuBar(this);
    mainMenuBar->setStyleSheet(MainWindowStyles::MenuBar);
    mainMenuBar->setLibraryActionsVisible(false);
    setMenuBar(nullptr);
    topBarLayout->addWidget(mainMenuBar);

    // ========== 2. Navigation Section ==========
    navigationPage = new NavigationPage(this);
    navigationPage->setStyleSheet(MainWindowStyles::NavigationPage);
    topBarLayout->addWidget(navigationPage);
    topBarLayout->addStretch();

    // ========== Main Content Area ==========
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet(MainWindowStyles::CentralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Add top bar first
    mainLayout->addWidget(topBarWidget);

    // Add stacked widget for editors
    stackedWidget = new QStackedWidget(centralWidget);
    stackedWidget->setStyleSheet(MainWindowStyles::StackedWidget);
    mainLayout->addWidget(stackedWidget);

    // Create editors
    databaseEditor = new DatabaseEditor(centralWidget);
    scenarioEditor = new ScenarioEditor(centralWidget);
    runtimeEditor = new RuntimeEditor(centralWidget);

    // Add editors to stacked widget
    stackedWidget->addWidget(databaseEditor);
    stackedWidget->addWidget(scenarioEditor);
    stackedWidget->addWidget(runtimeEditor);

    setCentralWidget(centralWidget);

    connect(navigationPage, &NavigationPage::editorRequested, this, &MainWindow::switchEditor);

    connect(databaseEditor, &DatabaseEditor::unsavedChangesChanged,
            this, &MainWindow::onUnsavedChangesChanged);
    connect(scenarioEditor, &ScenarioEditor::unsavedChangesChanged,
            this, &MainWindow::onUnsavedChangesChanged);
    connect(runtimeEditor, &RuntimeEditor::unsavedChangesChanged,
            this, &MainWindow::onUnsavedChangesChanged);

    stackedWidget->setCurrentWidget(databaseEditor);
    setupMenuBarConnections();
}

void MainWindow::setupMenuBarConnections()
{


    // File Menu Connections
    connect(mainMenuBar, &MenuBar::newFileTriggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (currentEditor) {
            if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(currentEditor)) {
                dbEditor->hierarchy->fromJson(QJsonObject());
                HierarchyConnector::instance()->initializeDummyData(dbEditor->hierarchy);
                dbEditor->clearUnsavedChanges();
            } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(currentEditor)) {
                scEditor->hierarchy->fromJson(QJsonObject());
                HierarchyConnector::instance()->initializeDummyData(scEditor->hierarchy);
                scEditor->clearUnsavedChanges();
            } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor)) {
                rtEditor->hierarchy->fromJson(QJsonObject());
                HierarchyConnector::instance()->initializeDummyData(rtEditor->hierarchy);
                rtEditor->clearUnsavedChanges();
            }
        }
    });

    // Recent Project
    connect(mainMenuBar, &MenuBar::recentProjectTriggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;

        RecentProjectsManager::EditorType editorType;
        if (qobject_cast<DatabaseEditor*>(currentEditor)) {
            editorType = RecentProjectsManager::DatabaseEditor;
        } else if (qobject_cast<ScenarioEditor*>(currentEditor)) {
            editorType = RecentProjectsManager::ScenarioEditor;
        } else if (qobject_cast<RuntimeEditor*>(currentEditor)) {
            editorType = RecentProjectsManager::RuntimeEditor;
        } else {
            return;
        }

        RecentProjectsManager::instance()->showRecentProjectsMenu(this, editorType);
    });

    // Load File - Use HierarchyConnector's file operations
    connect(mainMenuBar->getLoadAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;

        // Delegate to the current editor's file operations
        if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(currentEditor)) {
            // Call the same file operations as scenario editor
            loadFileWithTDFSupport(dbEditor, RecentProjectsManager::DatabaseEditor, "db", "Database Files (*.db)");
        } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(currentEditor)) {
            loadFileWithTDFSupport(scEditor, RecentProjectsManager::ScenarioEditor, "sc", "Scenario Files (*.sc)");
        } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor)) {
            loadFileWithTDFSupport(rtEditor, RecentProjectsManager::RuntimeEditor, "rn",
                                   "Runtime & Scenario Files (*.rn *.sc);;Runtime Files (*.rn);;Scenario Files (*.sc);;All Files (*)");
        }
    });

    // Load to Library - Use HierarchyConnector
    connect(mainMenuBar->getLoadToLibraryAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;

        // Use HierarchyConnector's loadToLibrary function
        HierarchyConnector::instance()->loadToLibrary(currentEditor);
    });

    // Open Runtime Instance
    connect(mainMenuBar->getOpenRuntimeInstanceAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;

        QString homeDir = QDir::homePath();
        QString tdfPath = homeDir + "/TDF";
        QString scenarioPath = tdfPath + "/Scenario";
        QString instanceFolderPath = scenarioPath + "/Scerioinstance";

        // Check if Scerioinstance folder exists
        QDir instanceDir(instanceFolderPath);
        if (!instanceDir.exists()) {
            QMessageBox msgBox(this);
            msgBox.setStyleSheet(MainWindowStyles::MessageBox);
            msgBox.setWindowTitle("Folder Not Found");
            msgBox.setText("Scerioinstance folder not found at:\n" + instanceFolderPath +
                           "\n\nPlease save a runtime file first to create the folder.");
            msgBox.setIcon(QMessageBox::Information);
            msgBox.exec();
            return;
        }

        // Get list of .sc files in the instance folder
        QStringList filters;
        filters << "*.sc";
        instanceDir.setNameFilters(filters);
        QStringList scFiles = instanceDir.entryList(QDir::Files);

        if (scFiles.isEmpty()) {
            QMessageBox msgBox(this);
            msgBox.setStyleSheet(MainWindowStyles::MessageBox);
            msgBox.setWindowTitle("No Files Found");
            msgBox.setText("No .sc files found in Scerioinstance folder.");
            msgBox.setIcon(QMessageBox::Information);
            msgBox.exec();
            return;
        }

        // Show file selection dialog
        QFileDialog fileDialog(this);
        fileDialog.setStyleSheet(MainWindowStyles::FileDialog);
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Open Runtime Instance File",
            instanceFolderPath,
            "Scenario Instance Files (*.sc);;All Files (*.*)"
            );

        if (filePath.isEmpty()) {
            return;
        }

        // Load the selected file in appropriate editor
        if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor)) {
            rtEditor->loadFromJsonFile(filePath);
            RecentProjectsManager::instance()->addToRecentProjects(filePath,
                                                                   RecentProjectsManager::RuntimeEditor);
        }
        else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(currentEditor)) {
            scEditor->loadFromJsonFile(filePath);
            RecentProjectsManager::instance()->addToRecentProjects(filePath,
                                                                   RecentProjectsManager::ScenarioEditor);
        }
    });

    // Save As - Use TDF folder structure
    connect(mainMenuBar->getSaveAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;

        saveFileWithTDFSupport(currentEditor);
    });

    // Save (same path) - Use TDF folder structure
    connect(mainMenuBar->getSameSaveAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;

        saveToSameFileWithTDFSupport(currentEditor);
    });

    // Load XML
    connect(mainMenuBar->getLoadXmlAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;

        QFileDialog fileDialog(this);
        fileDialog.setStyleSheet(MainWindowStyles::FileDialog);
        QString filePath = QFileDialog::getOpenFileName(this, "Open XML",
                                                        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                        "XML Files (*.xml)");

        if (filePath.isEmpty()) return;

        if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(currentEditor)) {
            HierarchyConnector::instance()->openXmlFile(dbEditor->hierarchy, filePath);
        } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(currentEditor)) {
            HierarchyConnector::instance()->openXmlFile(scEditor->hierarchy, filePath);
        } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor)) {
            HierarchyConnector::instance()->openXmlFile(rtEditor->hierarchy, filePath);
        }
    });

    // About / Feedback
    connect(mainMenuBar, &MenuBar::feedbackTriggered, this, [=]() {
        Feedback *feedbackWindow = new Feedback(this);
        feedbackWindow->show();
    });

    // Profile/Performance
    connect(mainMenuBar, &MenuBar::profileTriggered, this, [=]() {
        ProfileInfoDialog::showProfileInfo(this);
    });

    // Settings
    connect(mainMenuBar, &MenuBar::applicationTriggered, this, [=]() {
        ApplicationDialog dialog(this);

        QMainWindow *currentEditor = getCurrentEditor();
        if (currentEditor) {
            if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(currentEditor)) {
                connect(&dialog, &ApplicationDialog::canvasIconState,
                        scEditor->tacticalDisplay->canvas, &CanvasWidget::setImageScale);
            } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor)) {
                connect(&dialog, &ApplicationDialog::canvasIconState,
                        rtEditor->tacticalDisplay->canvas, &CanvasWidget::setImageScale);
                connect(&dialog, &ApplicationDialog::fpsState,
                        rtEditor->simulation, &Simulation::setFps);
            }
        }

        dialog.exec();
    });

    // Exit
    connect(mainMenuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
}

// Helper function to ensure TDF folder structure exists
QString MainWindow::ensureTDFSubfolder(const QString& subfolderName)
{
    QString homeDir = QDir::homePath();
    QString tdfPath = homeDir + "/TDF";
    QString targetPath = tdfPath + "/" + subfolderName;

    QDir dir;
    // Create TDF folder if it doesn't exist
    if (!dir.exists(tdfPath)) {
        dir.mkpath(tdfPath);
    }

    // Create subfolder (Database, Scenario, or Runtime) if it doesn't exist
    if (!dir.exists(targetPath)) {
        dir.mkpath(targetPath);
    }

    return targetPath;
}

// Helper function to load file with TDF support
void MainWindow::loadFileWithTDFSupport(QMainWindow* editor,
                                        RecentProjectsManager::EditorType editorType,
                                        const QString& extension,
                                        const QString& filter)
{
    QString subfolderName;
    if (editorType == RecentProjectsManager::DatabaseEditor) {
        subfolderName = "Database";
    } else if (editorType == RecentProjectsManager::ScenarioEditor) {
        subfolderName = "Scenario";
    } else if (editorType == RecentProjectsManager::RuntimeEditor) {
        subfolderName = "Runtime";
    }

    QString startPath = ensureTDFSubfolder(subfolderName);

    QFileDialog fileDialog(this);
    fileDialog.setStyleSheet(MainWindowStyles::FileDialog);
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    "Open File",
                                                    startPath,
                                                    filter);

    if (filePath.isEmpty()) return;

    if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor)) {
        dbEditor->loadFromJsonFile(filePath);
        RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);
    } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor)) {
        scEditor->loadFromJsonFile(filePath);
        RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);
    } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor)) {
        rtEditor->loadFromJsonFile(filePath);
        RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);
    }
}

// Helper function to save file with TDF support
void MainWindow::saveFileWithTDFSupport(QMainWindow* editor)
{
    RecentProjectsManager::EditorType editorType;
    QString extension;
    QString subfolderName;
    QString defaultPrefix;
    Hierarchy* hierarchy = nullptr;
    TacticalDisplay* tacticalDisplay = nullptr;
    QString lastFilePath;

    if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor)) {
        editorType = RecentProjectsManager::DatabaseEditor;
        extension = "db";
        subfolderName = "Database";
        defaultPrefix = "Database";
        hierarchy = dbEditor->hierarchy;
        lastFilePath = dbEditor->lastSavedFilePath;
    } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor)) {
        editorType = RecentProjectsManager::ScenarioEditor;
        extension = "sc";
        subfolderName = "Scenario";
        defaultPrefix = "Scenario";
        hierarchy = scEditor->hierarchy;
        tacticalDisplay = scEditor->tacticalDisplay;
        lastFilePath = scEditor->lastSavedFilePath;
    } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor)) {
        editorType = RecentProjectsManager::RuntimeEditor;
        extension = "rn";
        subfolderName = "Runtime";
        defaultPrefix = "Runtime";
        hierarchy = rtEditor->hierarchy;
        tacticalDisplay = rtEditor->tacticalDisplay;
        lastFilePath = rtEditor->lastSavedFilePath;
    } else {
        return;
    }

    QString startPath = ensureTDFSubfolder(subfolderName);
    QString currentDate = QDate::currentDate().toString("yyyy-MM-dd");
    QString defaultFileName;

    // Generate default filename
    if (!lastFilePath.isEmpty()) {
        QFileInfo fileInfo(lastFilePath);
        QString baseName = fileInfo.completeBaseName();
        // Remove date pattern if present
        QRegularExpression datePattern("_\\d{4}-\\d{2}-\\d{2}$");
        baseName.remove(datePattern);
        defaultFileName = baseName + "_" + currentDate + "." + extension;
    } else {
        defaultFileName = defaultPrefix + "_" + currentDate + "." + extension;
    }

    QString filter = QString("%1 Files (*.%2);;JSON Files (*.json);;All Files (*.*)")
                         .arg(defaultPrefix).arg(extension);

    QFileDialog fileDialog(this);
    fileDialog.setStyleSheet(MainWindowStyles::FileDialog);
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    "Save File",
                                                    startPath + "/" + defaultFileName,
                                                    filter);

    if (filePath.isEmpty()) return;

    // Ensure correct extension
    QFileInfo fileInfo(filePath);
    if (fileInfo.suffix().isEmpty()) {
        filePath += "." + extension;
    }

    // Prepare JSON data
    QJsonObject obj;
    obj["hierarchy"] = hierarchy->toJson();
    if (tacticalDisplay != nullptr) {
        obj["tactical"] = tacticalDisplay->canvas->toJson();
    }

    // Save file
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(obj);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        // Update last saved path
        if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor)) {
            dbEditor->lastSavedFilePath = filePath;
            dbEditor->clearUnsavedChanges();
        } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor)) {
            scEditor->lastSavedFilePath = filePath;
            scEditor->clearUnsavedChanges();
        } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor)) {
            rtEditor->lastSavedFilePath = filePath;
            rtEditor->clearUnsavedChanges();

            // For Runtime Editor, create scenario instance copy
            if (editorType == RecentProjectsManager::RuntimeEditor) {
                createScenarioInstanceCopy(filePath, obj);
            }
        }

        RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);
    }
}

// Helper function to save to same file
void MainWindow::saveToSameFileWithTDFSupport(QMainWindow* editor)
{
    RecentProjectsManager::EditorType editorType;
    QString expectedExtension;
    QString expectedSubfolder;
    Hierarchy* hierarchy = nullptr;
    TacticalDisplay* tacticalDisplay = nullptr;
    QString filePath;

    if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor)) {
        editorType = RecentProjectsManager::DatabaseEditor;
        expectedExtension = "db";
        expectedSubfolder = "Database";
        hierarchy = dbEditor->hierarchy;
        filePath = dbEditor->lastSavedFilePath;
    } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor)) {
        editorType = RecentProjectsManager::ScenarioEditor;
        expectedExtension = "sc";
        expectedSubfolder = "Scenario";
        hierarchy = scEditor->hierarchy;
        tacticalDisplay = scEditor->tacticalDisplay;
        filePath = scEditor->lastSavedFilePath;
    } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor)) {
        editorType = RecentProjectsManager::RuntimeEditor;
        expectedExtension = "rn";
        expectedSubfolder = "Runtime";
        hierarchy = rtEditor->hierarchy;
        tacticalDisplay = rtEditor->tacticalDisplay;
        filePath = rtEditor->lastSavedFilePath;
    } else {
        return;
    }

    // If no previous save, trigger Save As
    if (filePath.isEmpty()) {
        saveFileWithTDFSupport(editor);
        return;
    }

    // Check if file is in correct TDF folder
    QString tdfPath = QDir::homePath() + "/TDF/" + expectedSubfolder;
    QFileInfo fileInfo(filePath);

    if (!filePath.startsWith(tdfPath) || fileInfo.suffix().toLower() != expectedExtension) {
        // File is not in the correct location, trigger Save As
        saveFileWithTDFSupport(editor);
        return;
    }

    // Save to the same file
    QJsonObject obj;
    obj["hierarchy"] = hierarchy->toJson();
    if (tacticalDisplay != nullptr) {
        obj["tactical"] = tacticalDisplay->canvas->toJson();
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QJsonDocument doc(obj);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        // Clear unsaved changes
        if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor)) {
            dbEditor->clearUnsavedChanges();
        } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor)) {
            scEditor->clearUnsavedChanges();
        } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor)) {
            rtEditor->clearUnsavedChanges();

            // For Runtime Editor, create scenario instance copy
            if (editorType == RecentProjectsManager::RuntimeEditor) {
                createScenarioInstanceCopy(filePath, obj);
            }
        }

        RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);
    }
}

// Helper function to create scenario instance copy
void MainWindow::createScenarioInstanceCopy(const QString& runtimeFilePath, const QJsonObject& data)
{
    QFileInfo runtimeFileInfo(runtimeFilePath);
    if (runtimeFileInfo.suffix().toLower() != "rn") {
        return;
    }

    QString homeDir = QDir::homePath();
    QString tdfPath = homeDir + "/TDF";
    QString scenarioPath = tdfPath + "/Scenario";
    QString instanceFolderPath = scenarioPath + "/Scerioinstance";

    // Create scenario instance folder if it doesn't exist
    QDir dir;
    if (!dir.exists(scenarioPath)) {
        dir.mkpath(scenarioPath);
    }
    if (!dir.exists(instanceFolderPath)) {
        dir.mkpath(instanceFolderPath);
    }

    // Create scenario instance file name
    QString runtimeFileName = runtimeFileInfo.completeBaseName();
    QString scenarioFileName;

    if (runtimeFileName.startsWith("Scenario_")) {
        scenarioFileName = "Instance_" + runtimeFileName + ".sc";
    } else if (runtimeFileName.startsWith("Runtime_")) {
        scenarioFileName = "Instance_" + runtimeFileName.replace("Runtime_", "Scenario_") + ".sc";
    } else {
        scenarioFileName = "Instance_Scenario_" + runtimeFileName + ".sc";
    }

    QString scenarioFilePath = instanceFolderPath + "/" + scenarioFileName;

    // Save the copy with .sc extension
    QFile scenarioFile(scenarioFilePath);
    if (scenarioFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(data);
        scenarioFile.write(doc.toJson(QJsonDocument::Indented));
        scenarioFile.close();
    }
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
        return true;

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
        return true;

    QMessageBox msgBox(this);
    msgBox.setStyleSheet(MainWindowStyles::MessageBox);
    msgBox.setWindowTitle("Unsaved Changes");
    msgBox.setText(QString("There are unsaved changes in the %1. Do you want to save before switching?").arg(editorName));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    QMessageBox::StandardButton reply = (QMessageBox::StandardButton)msgBox.exec();

    if (reply == QMessageBox::Save) {

        if (!mainMenuBar) {
            qWarning() << "Main menu bar not found";
            return false;
        }

        // Determine which save action to use
        QAction *saveAction = nullptr;
        if (lastSavedFilePath.isEmpty()) {
            saveAction = mainMenuBar->getSaveAction();
        } else {
            saveAction = mainMenuBar->getSameSaveAction();
        }

        if (!saveAction) {
            qWarning() << "Save action not found in main menu bar";
            return false;
        }

        // Trigger the save action
        saveAction->trigger();

        // Process events to ensure save completes
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();

        // Small delay to ensure file write completes
        QThread::msleep(100);
        QCoreApplication::processEvents();

        // Check if changes were actually saved
        bool stillHasUnsaved = false;
        if (currentEditor == databaseEditor) {
            stillHasUnsaved = databaseEditor->hasUnsavedChanges;
        } else if (currentEditor == scenarioEditor) {
            stillHasUnsaved = scenarioEditor->hasUnsavedChanges;
        } else if (currentEditor == runtimeEditor) {
            stillHasUnsaved = runtimeEditor->hasUnsavedChanges;
        }

        if (stillHasUnsaved) {
            // Save failed or was cancelled
            QMessageBox::warning(this, "Save Failed",
                                 "Failed to save changes. Please try again or choose Discard.");
            return false;
        }

        // Update window title
        updateWindowTitleForCurrentEditor();

        // Return true to allow switching
        return true;

    } else if (reply == QMessageBox::Discard) {
        if (currentEditor == databaseEditor) {
            databaseEditor->clearUnsavedChanges();
        } else if (currentEditor == scenarioEditor) {
            scenarioEditor->clearUnsavedChanges();
        } else if (currentEditor == runtimeEditor) {
            runtimeEditor->clearUnsavedChanges();
        }
        updateWindowTitleForCurrentEditor();
        return true;
    } else {
        return false;
    }
}

MainWindow* MainWindow::instance()
{
    if (!MainWindow::s_instance) {
    }
    return MainWindow::s_instance;
}

void MainWindow::switchEditor(const QString &editorKey)
{

    if (!handleUnsavedChanges()) {
        return;
    }
    if (editorKey == "database") {

        mainMenuBar->setLibraryActionsVisible(false);
        stackedWidget->setCurrentWidget(databaseEditor);
        updateWindowTitleForCurrentEditor();
        emit databaseEditor->Activated();
    }
    else if (editorKey == "scenario") {

        mainMenuBar->setLibraryActionsVisible(true);

        // Load library data if needed
        if (!databaseEditor->lastSavedFilePath.isEmpty()) {
            QFile file(databaseEditor->lastSavedFilePath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                file.close();
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(data, &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.contains("hierarchy")) {
                        QJsonObject hierarchyData = obj["hierarchy"].toObject();
                        scenarioEditor->library->fromJson(hierarchyData);
                        if (scenarioEditor->libTreeView) {
                            scenarioEditor->libTreeView->setLibraryFileName(databaseEditor->lastSavedFilePath);
                        }
                    }
                }
            }
        } else {
            QString resourcePath = TDFManager::instance()->getAircraftDbPath();
            QFile jsonFile(resourcePath);
            if (jsonFile.open(QIODevice::ReadOnly)) {
                QByteArray data = jsonFile.readAll();
                jsonFile.close();
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
                if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.contains("hierarchy")) {
                        QJsonObject hierarchyObj = obj["hierarchy"].toObject();
                        scenarioEditor->library->fromJson(hierarchyObj);
                        if (scenarioEditor->libTreeView) {
                            scenarioEditor->libTreeView->setLibraryFileName("Aircraft.db");
                        }
                    }
                }
            }
        }

        if (scenarioEditor->libTreeView) {
            scenarioEditor->libTreeView->getTreeWidget()->update();
            scenarioEditor->libTreeView->getTreeWidget()->collapseAll();
        }
        stackedWidget->setCurrentWidget(scenarioEditor);
        updateWindowTitleForCurrentEditor();
        emit scenarioEditor->Activated();
    }
    else if (editorKey == "runtime") {
        mainMenuBar->setLibraryActionsVisible(true);
        if (!databaseEditor->lastSavedFilePath.isEmpty()) {
            QFile file(databaseEditor->lastSavedFilePath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                file.close();

                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(data, &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.contains("hierarchy")) {
                        QJsonObject hierarchyData = obj["hierarchy"].toObject();
                        runtimeEditor->library->fromJson(hierarchyData);
                        if (runtimeEditor->libTreeView) {
                            runtimeEditor->libTreeView->setLibraryFileName(databaseEditor->lastSavedFilePath);
                        }
                    }
                }
            }
        } else {
            QString resourcePath = TDFManager::instance()->getAircraftDbPath();
            QFile jsonFile(resourcePath);
            if (jsonFile.open(QIODevice::ReadOnly)) {
                QByteArray data = jsonFile.readAll();
                jsonFile.close();
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
                if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.contains("hierarchy")) {
                        QJsonObject hierarchyObj = obj["hierarchy"].toObject();
                        runtimeEditor->library->fromJson(hierarchyObj);
                        if (runtimeEditor->libTreeView) {
                            runtimeEditor->libTreeView->setLibraryFileName("Aircraft.db");
                        }
                    }
                }
            }
        }
        if (!scenarioEditor->lastSavedFilePath.isEmpty()) {
            runtimeEditor->loadFromJsonFile(scenarioEditor->lastSavedFilePath);
        }

        if (runtimeEditor->libTreeView) {
            runtimeEditor->libTreeView->getTreeWidget()->update();
            runtimeEditor->libTreeView->getTreeWidget()->collapseAll();
        }
        stackedWidget->setCurrentWidget(runtimeEditor);
        updateWindowTitleForCurrentEditor();
        emit runtimeEditor->Activated();
    }
}

void MainWindow::updateWindowTitleForCurrentEditor()
{
    QWidget *currentWidget = stackedWidget->currentWidget();
    QString editorName;
    QString filePath;
    bool hasUnsaved = false;
    if (currentWidget == databaseEditor) {
        editorName = "Database Editor";
        filePath = databaseEditor->lastSavedFilePath;
        hasUnsaved = databaseEditor->hasUnsavedChanges;
    } else if (currentWidget == scenarioEditor) {
        editorName = "Scenario Editor";
        filePath = scenarioEditor->lastSavedFilePath;
        hasUnsaved = scenarioEditor->hasUnsavedChanges;
    } else if (currentWidget == runtimeEditor) {
        editorName = "Runtime Editor";
        filePath = runtimeEditor->lastSavedFilePath;
        hasUnsaved = runtimeEditor->hasUnsavedChanges;
    }

    updateWindowTitle(editorName, filePath, hasUnsaved);
}

void MainWindow::updateWindowTitle(const QString& editorName, const QString& filePath, bool hasUnsavedChanges)
{
    QString title = "Indigenous Scenario and Sensor Simulation Toolkit";

    if (scenarioconfig && !scenarioconfig->software_version.isEmpty()) {
        title += QString(" (V_%1)").arg(scenarioconfig->software_version);
    }

    title += " - " + editorName;

    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        title += " - " + fileInfo.fileName();
    }

    if (hasUnsavedChanges) {
        title = "*" + title;
    }

    setWindowTitle(title);
}

void MainWindow::onUnsavedChangesChanged(bool hasUnsaved)
{
    // Update title when unsaved changes status changes
    updateWindowTitleForCurrentEditor();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    bool proceedWithClose = true;

    // Check each editor individually
    if (databaseEditor->hasUnsavedChanges) {
        proceedWithClose &= promptForSave(databaseEditor, "Database Editor");
    }

    if (scenarioEditor->hasUnsavedChanges) {
        proceedWithClose &= promptForSave(scenarioEditor, "Scenario Editor");
    }

    if (runtimeEditor->hasUnsavedChanges) {
        proceedWithClose &= promptForSave(runtimeEditor, "Runtime Editor");
    }

    if (proceedWithClose) {
        event->accept();
    } else {
        event->ignore();
    }
}

bool MainWindow::promptForSave(QMainWindow* editor, const QString& editorName)
{
    DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor);
    ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor);
    RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor);

    bool hasUnsaved = false;
    QString filePath;

    if (dbEditor) {
        hasUnsaved = dbEditor->hasUnsavedChanges;
        filePath = dbEditor->lastSavedFilePath;
    } else if (scEditor) {
        hasUnsaved = scEditor->hasUnsavedChanges;
        filePath = scEditor->lastSavedFilePath;
    } else if (rtEditor) {
        hasUnsaved = rtEditor->hasUnsavedChanges;
        filePath = rtEditor->lastSavedFilePath;
    }

    if (!hasUnsaved) return true;

    QMessageBox msgBox(this);
    msgBox.setStyleSheet(MainWindowStyles::MessageBox);
    msgBox.setWindowTitle("Unsaved Changes");
    msgBox.setText(QString("There are unsaved changes in %1. Do you want to save?").arg(editorName));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    QMessageBox::StandardButton reply = (QMessageBox::StandardButton)msgBox.exec();

    if (reply == QMessageBox::Save) {
        if (!mainMenuBar) {
            qWarning() << "Main menu bar not found";
            return false;
        }

        QAction *saveAction = filePath.isEmpty()
                                  ? mainMenuBar->getSaveAction()
                                  : mainMenuBar->getSameSaveAction();
        if (saveAction) {
            saveAction->trigger();

            // Process events to ensure save completes
            QCoreApplication::processEvents();
            QCoreApplication::processEvents();
            QThread::msleep(100);
            QCoreApplication::processEvents();

            if (dbEditor) {
                return !dbEditor->hasUnsavedChanges;
            } else if (scEditor) {
                return !scEditor->hasUnsavedChanges;
            } else if (rtEditor) {
                return !rtEditor->hasUnsavedChanges;
            }
        }
        return false;
    } else if (reply == QMessageBox::Discard) {
        if (dbEditor) dbEditor->clearUnsavedChanges();
        else if (scEditor) scEditor->clearUnsavedChanges();
        else if (rtEditor) rtEditor->clearUnsavedChanges();
        return true;
    } else {
        return false;
    }
}
