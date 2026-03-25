//============================================================================
// File        : mainwindow.cpp
// Description : Implementation of MainWindow class for the main application
//               window that manages database, scenario, mission, and runtime
//               editors with navigation, unsaved changes handling, and editor
//               switching.
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
#include <QDesktopServices>
#include <GUI/statusbar.h>

ScenarioConfig* MainWindow::scenarioconfig = nullptr;
MainWindow* MainWindow::s_instance = nullptr;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setStyleSheet(MainWindowStyles::MainWindow);
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor("#0F2636"));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    qApp->setPalette(darkPalette);
    qApp->setStyleSheet(qApp->styleSheet() + MainWindowStyles::ToolTip);

    MainWindow::s_instance = this;
    MainWindow::scenarioconfig = new ScenarioConfig();
    ApplicationDialog::setGlobalDatabaseEnabled(scenarioconfig->getSavedDatabaseEnabled());
    ApplicationDialog::setGlobalDatabasePath(scenarioconfig->getSavedDatabasePath());

    setWindowTitle("Indigenous Scenario and Sensor Simulation Toolkit");
    if (scenarioconfig && !scenarioconfig->software_version.isEmpty()) {
        setWindowTitle(windowTitle() + QString(" - V_%1").arg(scenarioconfig->software_version));
    }

    resize(1900, 1000);
    setupUI();
    setAttribute(Qt::WA_DeleteOnClose);
}

// Destructor
MainWindow::~MainWindow()
{
    std::cout << "Mainwindow delete";
    delete databaseEditor;
    if (scenarioEditor) delete scenarioEditor;
    if (missionEditor)  delete missionEditor;
    if (runtimeEditor)  delete runtimeEditor;
     if (analysisEditor)  delete analysisEditor;
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
    mainMenuBar->updateFileMenuForEditor("database");
    mainMenuBar->setLibraryActionsVisible(false);
    setMenuBar(nullptr);
    topBarLayout->addWidget(mainMenuBar);

    // ========== 2. Navigation Section ==========
    navigationPage = new NavigationPage(this);
    navigationPage->setStyleSheet(MainWindowStyles::NavigationPage);
    topBarLayout->addWidget(navigationPage);
    topBarLayout->addStretch();

    QPushButton *helpButton = new QPushButton(this);
    helpButton->setObjectName("helpButton");
    helpButton->setFixedSize(24, 24);
    helpButton->setToolTip("Help");
    helpButton->setCursor(Qt::PointingHandCursor);
    helpButton->setIcon(QIcon(":/icons/images/help.png"));
    helpButton->setIconSize(QSize(22, 22));
    helpButton->setText("");
    helpButton->setStyleSheet(
        "QPushButton#helpButton {"
        "  background-color: transparent;"
        "  border: none;"
        "  border-radius: 0px;"
        "}"
        "QPushButton#helpButton:hover {"
        "  background-color: rgba(0, 191, 255, 0.12);"
        "  border-radius: 4px;"
        "}"
        );
    topBarLayout->addWidget(helpButton);
    topBarLayout->setContentsMargins(0, 0, 10, 0);
    connect(helpButton, &QPushButton::clicked, this, [=]() {
        QString appDirPath = QApplication::applicationDirPath();
        qDebug() << "App dir:" << appDirPath;
        QDir dir(appDirPath);
        QString tdfBasePath;
        while (!dir.isRoot()) {
            if (dir.dirName().contains("TDF", Qt::CaseInsensitive)) {
                tdfBasePath = dir.absolutePath();
                break;
            }
            dir.cdUp();
        }
        if (!tdfBasePath.isEmpty()) {
            QString helpPath = tdfBasePath + "/DB";
            QDir dbDir(helpPath);
            QStringList helpDirs = dbDir.entryList(QStringList() << "TDF_HELP_V_*", QDir::Dirs | QDir::NoDotAndDotDot);

            if (!helpDirs.isEmpty()) {
                QString htmlFilePath = helpPath + "/" + helpDirs.first() + "/build/html/index.html";
                qDebug() << "Help file path:" << htmlFilePath;

                if (QFileInfo::exists(htmlFilePath)) {
                    QUrl url = QUrl::fromLocalFile(htmlFilePath);
                    QProcess::startDetached("google-chrome", QStringList() << "--new-window" << url.toString());
                } else {
                    QMessageBox::warning(this, "Error", "index.html not found!");
                }
            } else {
                QMessageBox::warning(this, "Error", "TDF_HELP_V_* directory not found!");
            }
        } else {
            QMessageBox::warning(this, "Error", "TDF directory not found!");
        }
    });

    // ========== Main Content Area ==========
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet(MainWindowStyles::CentralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(topBarWidget);

    stackedWidget = new QStackedWidget(centralWidget);
    stackedWidget->setStyleSheet(MainWindowStyles::StackedWidget);
    mainLayout->addWidget(stackedWidget);

    databaseEditor = new DatabaseEditor(centralWidget);
    stackedWidget->addWidget(databaseEditor);
    scenarioEditor = nullptr;
    missionEditor  = nullptr;
    runtimeEditor  = nullptr;
      analysisEditor = nullptr;

    setCentralWidget(centralWidget);

    // ========== Status Bar with Save Button ==========
    m_statusBar = new StatusBar(this);
    setStatusBar(m_statusBar);

    connect(m_statusBar, &StatusBar::saveRequested, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (currentEditor) saveToSameFileWithTDFSupport(currentEditor);
    });

    connect(navigationPage, &NavigationPage::editorRequested, this, &MainWindow::switchEditor);
    connect(databaseEditor, &DatabaseEditor::unsavedChangesChanged,
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
            } else if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(currentEditor)) {
                msEditor->hierarchy->fromJson(QJsonObject());
                HierarchyConnector::instance()->initializeDummyData(msEditor->hierarchy);
                msEditor->clearUnsavedChanges();
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
        } else if (qobject_cast<MissionEditor*>(currentEditor)) {
            editorType = RecentProjectsManager::MissionEditor;
        } else {
            return;
        }
        RecentProjectsManager::instance()->showRecentProjectsMenu(this, editorType);
    });

    // Load File
    connect(mainMenuBar->getLoadAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;

        if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(currentEditor)) {
            loadFileWithTDFSupport(dbEditor, RecentProjectsManager::DatabaseEditor, "db", "Database Files (*.db)");
        } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(currentEditor)) {
            loadFileWithTDFSupport(scEditor, RecentProjectsManager::ScenarioEditor, "sc", "Scenario Files (*.sc)");
        } else if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(currentEditor)) {
            loadFileWithTDFSupport(msEditor, RecentProjectsManager::MissionEditor,
                                   "ms", "Mission Files (*.ms)");
        } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor)) {
            loadFileWithTDFSupport(rtEditor, RecentProjectsManager::RuntimeEditor, "rn",
                                   "Runtime & Scenario Files (*.rn *.sc);;Runtime Files (*.rn);;Scenario Files (*.sc);;All Files (*)");
        }
    });

    // Load to Library
    connect(mainMenuBar->getLoadToLibraryAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;
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

        QFileDialog fileDialog(this);
        fileDialog.setStyleSheet(MainWindowStyles::FileDialog);
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Open Runtime Instance File",
            instanceFolderPath,
            "Scenario Instance Files (*.sc);;All Files (*.*)"
            );

        if (filePath.isEmpty()) return;

        if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor)) {
            rtEditor->loadFromJsonFile(filePath);
            RecentProjectsManager::instance()->addToRecentProjects(filePath, RecentProjectsManager::RuntimeEditor);
        } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(currentEditor)) {
            scEditor->loadFromJsonFile(filePath);
            RecentProjectsManager::instance()->addToRecentProjects(filePath, RecentProjectsManager::ScenarioEditor);
        }
    });

    // Save As
    connect(mainMenuBar->getSaveAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;
        saveFileWithTDFSupport(currentEditor);
    });

    // Save (same path)
    connect(mainMenuBar->getSameSaveAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;
        saveToSameFileWithTDFSupport(currentEditor);
    });
    connect(mainMenuBar->getOpenMissionFileAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!qobject_cast<RuntimeEditor*>(currentEditor)) return;

        QString startPath = ensureTDFSubfolder("Mission");
        QFileDialog fileDialog(this);
        fileDialog.setStyleSheet(MainWindowStyles::FileDialog);
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Open Mission File",
            startPath,
            "Mission Files (*.ms);;All Files (*.*)"
            );
        if (filePath.isEmpty()) return;

        RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor);

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "Error", "Failed to open mission file:\n" + filePath);
            return;
        }
        QByteArray data = file.readAll();
        file.close();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            QMessageBox::warning(this, "Error", "Invalid mission file format:\n" + err.errorString());
            return;
        }
        QJsonObject obj = doc.object();
        RuntimeEditor::s_missionData     = obj;
        RuntimeEditor::s_missionFilePath = filePath;

        // If missionEditor already exists, load doctrine data into it
        if (missionEditor) {
            missionEditor->loadFromJsonFile(filePath);
        }

        // Show a status bar message confirming load
        if (m_statusBar)
            m_statusBar->setFileName(filePath, false);

        RecentProjectsManager::instance()->addToRecentProjects(
            filePath, RecentProjectsManager::MissionEditor);
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
            // CHANGE 6 — getLoadXmlAction: MissionEditor branch
        } else if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(currentEditor)) {
            HierarchyConnector::instance()->openXmlFile(msEditor->hierarchy, filePath);
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
        connect(&dialog, &ApplicationDialog::databaseSettingsChanged,
                this, [=](bool enabled, const QString& path) {
                    if (scenarioconfig) {
                        scenarioconfig->saveDatabaseSettings(enabled, path);
                    }
                });

        dialog.exec();
    });

    // Recent Project Library
    connect(mainMenuBar, &MenuBar::recentProjectLibraryTriggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;

        if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(currentEditor)) {
            connect(RecentProjectsManager::instance(), &RecentProjectsManager::projectSelected,
                    this, [=](const QString& filePath, RecentProjectsManager::EditorType type) {
                        if (type != RecentProjectsManager::LibraryData || filePath.isEmpty()) return;

                        showLoadingOverlay("Loading Database...");
                        QCoreApplication::processEvents();

                        QFile file(filePath);
                        if (!file.open(QIODevice::ReadOnly)) {
                            hideLoadingOverlay();
                            QMessageBox::warning(this, "Error", "Failed to open library file:\n" + filePath);
                            return;
                        }

                        if (m_loadingLabel) { m_loadingLabel->setText("Loading Database..."); QCoreApplication::processEvents(); }

                        QByteArray data = file.readAll();
                        file.close();

                        QJsonParseError err;
                        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
                        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
                            hideLoadingOverlay();
                            QMessageBox::warning(this, "Error", "Invalid library file format:\n" + err.errorString());
                            return;
                        }

                        QJsonObject obj = doc.object();
                        if (obj.contains("hierarchy")) {
                            if (m_loadingLabel) { m_loadingLabel->setText("Loading Database..."); QCoreApplication::processEvents(); }
                            scEditor->library->clear();
                            scEditor->library->fromJson(obj["hierarchy"].toObject());
                            if (m_loadingLabel) { m_loadingLabel->setText("Updating Tree View..."); QCoreApplication::processEvents(); }
                            if (scEditor->libTreeView) {
                                scEditor->libTreeView->setLibraryFileName(filePath);
                                scEditor->libTreeView->getTreeWidget()->update();
                            }
                            RecentProjectsManager::instance()->addToRecentProjects(filePath, RecentProjectsManager::LibraryData);
                        }
                        hideLoadingOverlay();
                    }, Qt::UniqueConnection);
            RecentProjectsManager::instance()->showRecentLibraryMenu(this);
        } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor)) {
            connect(RecentProjectsManager::instance(), &RecentProjectsManager::projectSelected,
                    this, [=](const QString& filePath, RecentProjectsManager::EditorType type) {
                        if (type != RecentProjectsManager::LibraryData || filePath.isEmpty()) return;
                        showLoadingOverlay("Loading Database...");
                        QCoreApplication::processEvents();
                        QFile file(filePath);
                        if (!file.open(QIODevice::ReadOnly)) {
                            hideLoadingOverlay();
                            QMessageBox::warning(this, "Error", "Failed to open library file:\n" + filePath);
                            return;
                        }
                        if (m_loadingLabel) { m_loadingLabel->setText("Loading Database..."); QCoreApplication::processEvents(); }
                        QByteArray data = file.readAll();
                        file.close();
                        QJsonParseError err;
                        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
                        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
                            hideLoadingOverlay();
                            QMessageBox::warning(this, "Error", "Invalid library file format:\n" + err.errorString());
                            return;
                        }
                        QJsonObject obj = doc.object();
                        if (obj.contains("hierarchy")) {
                            if (m_loadingLabel) { m_loadingLabel->setText("Loading Database..."); QCoreApplication::processEvents(); }
                            rtEditor->library->clear();
                            rtEditor->library->fromJson(obj["hierarchy"].toObject());
                            if (m_loadingLabel) { m_loadingLabel->setText("Updating Tree View..."); QCoreApplication::processEvents(); }
                            if (rtEditor->libTreeView) {
                                rtEditor->libTreeView->setLibraryFileName(filePath);
                                rtEditor->libTreeView->getTreeWidget()->update();
                            }
                            RecentProjectsManager::instance()->addToRecentProjects(filePath, RecentProjectsManager::LibraryData);
                        }
                        hideLoadingOverlay();
                    }, Qt::UniqueConnection);
            RecentProjectsManager::instance()->showRecentLibraryMenu(this);

        } else {
            QMessageBox::information(this, "Recent Library",
                                     "Recent Library is only available in Scenario and Runtime editors.");
        }
    });
    connect(mainMenuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
}

// Helper function to ensure TDF folder structure exists
QString MainWindow::ensureTDFSubfolder(const QString& subfolderName)
{
    QString homeDir = QDir::homePath();
    QString tdfPath = homeDir + "/TDF";
    QString targetPath = tdfPath + "/" + subfolderName;

    QDir dir;
    if (!dir.exists(tdfPath))    dir.mkpath(tdfPath);
    if (!dir.exists(targetPath)) dir.mkpath(targetPath);

    return targetPath;
}

// loadFileWithTDFSupport: MissionEditor subfolder mapping + loading branch
void MainWindow::loadFileWithTDFSupport(QMainWindow* editor,
                                        RecentProjectsManager::EditorType editorType,
                                        const QString& extension,
                                        const QString& filter)
{
    QString subfolderName;
    if      (editorType == RecentProjectsManager::DatabaseEditor) subfolderName = "Database";
    else if (editorType == RecentProjectsManager::ScenarioEditor) subfolderName = "Scenario";
    else if (editorType == RecentProjectsManager::MissionEditor)  subfolderName = "Mission";
    else if (editorType == RecentProjectsManager::RuntimeEditor)  subfolderName = "Runtime";

    QString startPath = ensureTDFSubfolder(subfolderName);
    QString filePath = QFileDialog::getOpenFileName(this, "Open File", startPath, filter);

    if (filePath.isEmpty()) return;

    showLoadingOverlay("Loading file...");
    QCoreApplication::processEvents();

    if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor)) {
        if (m_loadingLabel) { m_loadingLabel->setText("Loading Database..."); QCoreApplication::processEvents(); }
        dbEditor->loadFromJsonFile(filePath);
        RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);
    } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor)) {
        if (m_loadingLabel) { m_loadingLabel->setText("Loading Scenario..."); QCoreApplication::processEvents(); }
        scEditor->loadFromJsonFile(filePath);
        RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);
    } else if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(editor)) {
        if (m_loadingLabel) { m_loadingLabel->setText("Loading Mission..."); QCoreApplication::processEvents(); }
        msEditor->loadFromJsonFile(filePath);
        RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);
    } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor)) {
        if (m_loadingLabel) { m_loadingLabel->setText("Loading Runtime..."); QCoreApplication::processEvents(); }
        rtEditor->loadFromJsonFile(filePath);
        RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);
    }
    hideLoadingOverlay();
}
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
        extension = "db"; subfolderName = "Database"; defaultPrefix = "Database";
        hierarchy = dbEditor->hierarchy;
        lastFilePath = dbEditor->lastSavedFilePath;
    } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor)) {
        editorType = RecentProjectsManager::ScenarioEditor;
        extension = "sc"; subfolderName = "Scenario"; defaultPrefix = "Scenario";
        hierarchy = scEditor->hierarchy;
        tacticalDisplay = scEditor->tacticalDisplay;
        lastFilePath = scEditor->lastSavedFilePath;
    }  else if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(editor)) {
    editorType = RecentProjectsManager::MissionEditor;
    extension = "ms"; subfolderName = "Mission"; defaultPrefix = "Mission";
    lastFilePath = msEditor->lastSavedFilePath;
} else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor)) {
        editorType = RecentProjectsManager::RuntimeEditor;
        extension = "rn"; subfolderName = "Runtime"; defaultPrefix = "Runtime";
        hierarchy = rtEditor->hierarchy;
        tacticalDisplay = rtEditor->tacticalDisplay;
        lastFilePath = rtEditor->lastSavedFilePath;
    } else {
        return;
    }
    QString startPath = ensureTDFSubfolder(subfolderName);
    QString currentDate = QDate::currentDate().toString("yyyy-MM-dd");
    QString defaultFileName;
    if (!lastFilePath.isEmpty()) {
        QFileInfo fileInfo(lastFilePath);
        QString baseName = fileInfo.completeBaseName();
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
    QFileInfo fileInfo(filePath);
    if (fileInfo.suffix().isEmpty()) filePath += "." + extension;
    // Build JSON object to save
    QJsonObject obj;
    if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(editor)) {
        if (msEditor->doctrinePanel)
            obj["doctrine"]       = msEditor->doctrinePanel->toJson();
        if (msEditor->tacticalPanel)
            obj["tactical"] = msEditor->tacticalPanel->toJsonBothTeams();
    } else {
        obj["hierarchy"] = hierarchy->toJson();
        if (tacticalDisplay != nullptr)
            obj["tactical"] = tacticalDisplay->canvas->toJson();
    }
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(obj);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor)) {
            dbEditor->lastSavedFilePath = filePath;
            dbEditor->clearUnsavedChanges();
        } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor)) {
            scEditor->lastSavedFilePath = filePath;
            scEditor->clearUnsavedChanges();
        } else if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(editor)) {
            msEditor->lastSavedFilePath = filePath;
            msEditor->clearUnsavedChanges();
        } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor)) {
            rtEditor->lastSavedFilePath = filePath;
            rtEditor->clearUnsavedChanges();
            if (editorType == RecentProjectsManager::RuntimeEditor)
                createScenarioInstanceCopy(filePath, obj);
        }
        RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);
    }
}

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
        expectedExtension = "db"; expectedSubfolder = "Database";
        hierarchy = dbEditor->hierarchy;
        filePath = dbEditor->lastSavedFilePath;
    } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor)) {
        editorType = RecentProjectsManager::ScenarioEditor;
        expectedExtension = "sc"; expectedSubfolder = "Scenario";
        hierarchy = scEditor->hierarchy;
        tacticalDisplay = scEditor->tacticalDisplay;
        filePath = scEditor->lastSavedFilePath;
       } else if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(editor)) {
    editorType = RecentProjectsManager::MissionEditor;
    expectedExtension = "ms"; expectedSubfolder = "Mission";
    filePath = msEditor->lastSavedFilePath;
    }else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor)) {
        editorType = RecentProjectsManager::RuntimeEditor;
        expectedExtension = "rn"; expectedSubfolder = "Runtime";
        hierarchy = rtEditor->hierarchy;
        tacticalDisplay = rtEditor->tacticalDisplay;
        filePath = rtEditor->lastSavedFilePath;
    } else {
        return;
    }
    if (filePath.isEmpty()) {
        saveFileWithTDFSupport(editor);
        return;
    }
    QString tdfPath = QDir::homePath() + "/TDF/" + expectedSubfolder;
    QFileInfo fileInfo(filePath);
    if (!filePath.startsWith(tdfPath) || fileInfo.suffix().toLower() != expectedExtension) {
        saveFileWithTDFSupport(editor);
        return;
    }
    // Build JSON object to save
    QJsonObject obj;
    if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(editor)) {
        // .ms files save ONLY doctrine panel data, NOT hierarchy
        if (msEditor->doctrinePanel)
            obj["doctrine"]       = msEditor->doctrinePanel->toJson();
        if (msEditor->tacticalPanel)
            obj["tactical"] = msEditor->tacticalPanel->toJsonBothTeams();
    } else {
        obj["hierarchy"] = hierarchy->toJson();
        if (tacticalDisplay != nullptr)
            obj["tactical"] = tacticalDisplay->canvas->toJson();
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QJsonDocument doc(obj);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor)) {
            dbEditor->clearUnsavedChanges();
        } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor)) {
            scEditor->clearUnsavedChanges();
        } else if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(editor)) {
            msEditor->clearUnsavedChanges();
        } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor)) {
            rtEditor->clearUnsavedChanges();
            if (editorType == RecentProjectsManager::RuntimeEditor)
                createScenarioInstanceCopy(filePath, obj);
        }

        RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);
    }
}
// Helper function to create scenario instance copy
void MainWindow::createScenarioInstanceCopy(const QString& runtimeFilePath, const QJsonObject& data)
{
    QFileInfo runtimeFileInfo(runtimeFilePath);
    if (runtimeFileInfo.suffix().toLower() != "rn") return;
    QString homeDir = QDir::homePath();
    QString tdfPath = homeDir + "/TDF";
    QString scenarioPath = tdfPath + "/Scenario";
    QString instanceFolderPath = scenarioPath + "/Scerioinstance";
    QDir dir;
    if (!dir.exists(scenarioPath))       dir.mkpath(scenarioPath);
    if (!dir.exists(instanceFolderPath)) dir.mkpath(instanceFolderPath);
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
    QFile scenarioFile(scenarioFilePath);
    if (scenarioFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(data);
        scenarioFile.write(doc.toJson(QJsonDocument::Indented));
        scenarioFile.close();
    }
}

//getCurrentEditor: MissionEditor check
QMainWindow* MainWindow::getCurrentEditor() const
{
    QWidget *currentWidget = stackedWidget->currentWidget();
    if (currentWidget == databaseEditor)
        return databaseEditor;
    else if (scenarioEditor && currentWidget == scenarioEditor)
        return scenarioEditor;
    else if (missionEditor && currentWidget == missionEditor)
        return missionEditor;
    else if (runtimeEditor && currentWidget == runtimeEditor)
        return runtimeEditor;
    else if (analysisEditor  && currentWidget == analysisEditor)
        return analysisEditor;
    return nullptr;
}

//handleUnsavedChanges: MissionEditor blocks
bool MainWindow::handleUnsavedChanges()
{
    QMainWindow *currentEditor = getCurrentEditor();
    if (!currentEditor) return true;
    bool hasUnsavedChanges = false;
    QString editorName;
    QString lastSavedFilePath;
    if (currentEditor == databaseEditor) {
        hasUnsavedChanges = databaseEditor->hasUnsavedChanges;
        editorName = "Database Editor";
        lastSavedFilePath = databaseEditor->lastSavedFilePath;
    } else if (scenarioEditor && currentEditor == scenarioEditor) {
        hasUnsavedChanges = scenarioEditor->hasUnsavedChanges;
        editorName = "Scenario Editor";
        lastSavedFilePath = scenarioEditor->lastSavedFilePath;
    } else if (missionEditor && currentEditor == missionEditor) {
        hasUnsavedChanges  = missionEditor->hasUnsavedChanges;
        editorName         = "Mission Editor";
        lastSavedFilePath  = missionEditor->lastSavedFilePath;
    } else if (runtimeEditor && currentEditor == runtimeEditor) {
        hasUnsavedChanges = runtimeEditor->hasUnsavedChanges;
        editorName = "Runtime Editor";
        lastSavedFilePath = runtimeEditor->lastSavedFilePath;
    }else if (analysisEditor && currentEditor == analysisEditor) {
        hasUnsavedChanges = analysisEditor->hasUnsavedChanges;
        editorName = "Analysis Editor";
        lastSavedFilePath = analysisEditor->lastSavedFilePath;
    }
    if (!hasUnsavedChanges) return true;
    QMessageBox msgBox(this);
    msgBox.setStyleSheet(MainWindowStyles::MessageBox);
    msgBox.setWindowTitle("Unsaved Changes");
    msgBox.setText(QString("There are unsaved changes in the %1. Do you want to save before switching?").arg(editorName));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    QMessageBox::StandardButton reply = (QMessageBox::StandardButton)msgBox.exec();
    if (reply == QMessageBox::Save) {
        if (!mainMenuBar) { qWarning() << "Main menu bar not found"; return false; }
        QAction *saveAction = lastSavedFilePath.isEmpty()
                                  ? mainMenuBar->getSaveAction()
                                  : mainMenuBar->getSameSaveAction();
        if (!saveAction) { qWarning() << "Save action not found"; return false; }
        saveAction->trigger();
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        QThread::msleep(100);
        QCoreApplication::processEvents();
        bool stillHasUnsaved = false;
        if (currentEditor == databaseEditor) {
            stillHasUnsaved = databaseEditor->hasUnsavedChanges;
        } else if (scenarioEditor && currentEditor == scenarioEditor) {
            stillHasUnsaved = scenarioEditor->hasUnsavedChanges;
        } else if (missionEditor && currentEditor == missionEditor) {
            stillHasUnsaved = missionEditor->hasUnsavedChanges;
        } else if (runtimeEditor && currentEditor == runtimeEditor) {
            stillHasUnsaved = runtimeEditor->hasUnsavedChanges;
        }else if (analysisEditor && currentEditor == analysisEditor) {
            stillHasUnsaved = analysisEditor->hasUnsavedChanges;
        }
        if (stillHasUnsaved) {
            QMessageBox::warning(this, "Save Failed",
                                 "Failed to save changes. Please try again or choose Discard.");
            return false;
        }
        updateWindowTitleForCurrentEditor();
        return true;

    } else if (reply == QMessageBox::Discard) {
        if (currentEditor == databaseEditor) {
            databaseEditor->clearUnsavedChanges();
        } else if (scenarioEditor && currentEditor == scenarioEditor) {
            scenarioEditor->clearUnsavedChanges();
        } else if (missionEditor && currentEditor == missionEditor) {
            missionEditor->clearUnsavedChanges();
        } else if (runtimeEditor && currentEditor == runtimeEditor) {
            runtimeEditor->clearUnsavedChanges();
        }else if (analysisEditor && currentEditor == analysisEditor) {
            analysisEditor->clearUnsavedChanges();
        }
        updateWindowTitleForCurrentEditor();
        return true;
    } else {
        return false;
    }
}
//========MainWindow--instance=====
MainWindow* MainWindow::instance()
{
    return MainWindow::s_instance;
}

//switchEditor: new "mission" branch added between "scenario" and "runtime"
// ===================================================================
// Replace the entire switchEditor() function in mainwindow.cpp
// with this corrected version.
//
// KEY FIXES:
//   1. updateFileMenuForEditor() called in EVERY branch
//   2. updateFileMenuForEditor() always called BEFORE setLibraryActionsVisible()
//      so library-action visibility is applied last and wins.
// ===================================================================

void MainWindow::switchEditor(const QString &editorKey)
{
    if (!handleUnsavedChanges()) {
        navigationPage->restorePreviousButton();
        return;
    }

    // ── DATABASE ─────────────────────────────────────────────────────────
    if (editorKey == "database") {
        showLoadingOverlay("Loading Database Editor...");
        mainMenuBar->updateFileMenuForEditor(editorKey);      // 1st

        mainMenuBar->setLibraryActionsVisible(false);         // 2nd (wins over reset)
        stackedWidget->setCurrentWidget(databaseEditor);
        QCoreApplication::processEvents();
        updateWindowTitleForCurrentEditor();
        if (m_statusBar)
            m_statusBar->setFileName(databaseEditor->lastSavedFilePath, databaseEditor->hasUnsavedChanges);
        emit databaseEditor->Activated();
        hideLoadingOverlay();
    }

    // ── SCENARIO ─────────────────────────────────────────────────────────
    else if (editorKey == "scenario") {
        showLoadingOverlay("Loading Scenario Editor...");
        mainMenuBar->updateFileMenuForEditor(editorKey);      // 1st
        mainMenuBar->setLibraryActionsVisible(true);          // 2nd
        if (!scenarioEditor) {
            m_loadingLabel->setText("Creating Scenario Editor...");
            QCoreApplication::processEvents();
            scenarioEditor = new ScenarioEditor(centralWidget());
            stackedWidget->addWidget(scenarioEditor);
            connect(scenarioEditor, &ScenarioEditor::unsavedChangesChanged,
                    this, &MainWindow::onUnsavedChangesChanged);
        }
        // PRIORITY 1: Database Editor se library load karo
        if (!databaseEditor->lastSavedFilePath.isEmpty()) {
            m_loadingLabel->setText("Loading Database...");
            QCoreApplication::processEvents();
            QFile file(databaseEditor->lastSavedFilePath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                file.close();
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(data, &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.contains("hierarchy")) {
                        m_loadingLabel->setText("Loading Database...");
                        QCoreApplication::processEvents();
                        scenarioEditor->library->fromJson(obj["hierarchy"].toObject());
                        if (scenarioEditor->libTreeView) {
                            scenarioEditor->libTreeView->setLibraryFileName(
                                QFileInfo(databaseEditor->lastSavedFilePath).fileName());
                            scenarioEditor->libTreeView->getTreeWidget()->update();
                            scenarioEditor->libTreeView->getTreeWidget()->collapseAll();
                        }
                    }
                }
            }
        }
        // PRIORITY 2: Application Settings mein database path se load karo
        else if (ApplicationDialog::getGlobalDatabaseEnabled() &&
                 !ApplicationDialog::getGlobalDatabasePath().isEmpty() &&
                 ApplicationDialog::getGlobalDatabasePath() != "No path set") {
            m_loadingLabel->setText("Loading Database...");
            QCoreApplication::processEvents();
            QString dbPath = ApplicationDialog::getGlobalDatabasePath();
            QFile file(dbPath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                file.close();
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(data, &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.contains("hierarchy")) {
                        m_loadingLabel->setText("Loading Database...");
                        QCoreApplication::processEvents();
                        scenarioEditor->library->fromJson(obj["hierarchy"].toObject());
                        if (scenarioEditor->libTreeView) {
                            scenarioEditor->libTreeView->setLibraryFileName(QFileInfo(dbPath).fileName());
                            scenarioEditor->libTreeView->getTreeWidget()->update();
                            scenarioEditor->libTreeView->getTreeWidget()->collapseAll();
                        }
                    }
                }
            }
        }
        // PRIORITY 3: Default Aircraft.db load karo
        else if (ApplicationDialog::getGlobalDatabaseEnabled()) {
            m_loadingLabel->setText("Loading Database...");
            QCoreApplication::processEvents();
            QString aircraftDbPath = TDFManager::instance()->getAircraftDbPath();
            QFile jsonFile(aircraftDbPath);
            if (jsonFile.open(QIODevice::ReadOnly)) {
                QByteArray data = jsonFile.readAll();
                jsonFile.close();
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
                if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.contains("hierarchy")) {
                        m_loadingLabel->setText("Loading Database...");
                        QCoreApplication::processEvents();
                        scenarioEditor->library->fromJson(obj["hierarchy"].toObject());
                        if (scenarioEditor->libTreeView) {
                            scenarioEditor->libTreeView->setLibraryFileName("Aircraft.db (Default)");
                            scenarioEditor->libTreeView->getTreeWidget()->update();
                            scenarioEditor->libTreeView->getTreeWidget()->collapseAll();
                        }
                    }
                }
            }
        }
        // PRIORITY 4: Empty library
        else {
            m_loadingLabel->setText("Initializing Empty Library...");
            QCoreApplication::processEvents();
            scenarioEditor->library->fromJson(QJsonObject());
            if (scenarioEditor->libTreeView) {
                scenarioEditor->libTreeView->setLibraryFileName("");
                scenarioEditor->libTreeView->getTreeWidget()->update();
            }
        }
        m_loadingLabel->setText("Updating Tree View...");
        QCoreApplication::processEvents();
        if (scenarioEditor->libTreeView) {
            scenarioEditor->libTreeView->getTreeWidget()->update();
            scenarioEditor->libTreeView->getTreeWidget()->collapseAll();
        }
        m_loadingLabel->setText("Switching to Scenario Editor...");
        QCoreApplication::processEvents();
        stackedWidget->setCurrentWidget(scenarioEditor);
        updateWindowTitleForCurrentEditor();
        if (m_statusBar)
            m_statusBar->setFileName(scenarioEditor->lastSavedFilePath, scenarioEditor->hasUnsavedChanges);
        emit scenarioEditor->Activated();
        hideLoadingOverlay();
    }

    // ── MISSION ──────────────────────────────────────────────────────────
    else if (editorKey == "mission") {
        showLoadingOverlay("Loading Mission Editor...");
        mainMenuBar->updateFileMenuForEditor(editorKey);      // 1st
        mainMenuBar->setLibraryActionsVisible(false);         // 2nd
        if (!missionEditor) {
            m_loadingLabel->setText("Creating Mission Editor...");
            QCoreApplication::processEvents();
            missionEditor = new MissionEditor(centralWidget());
            stackedWidget->addWidget(missionEditor);
            connect(missionEditor, &MissionEditor::unsavedChangesChanged,
                    this, &MainWindow::onUnsavedChangesChanged);
        }
        // Always sync hierarchy from ScenarioEditor into MissionEditor
        if (scenarioEditor) {
            m_loadingLabel->setText("Syncing Scenario Hierarchy to Mission Editor...");
            QCoreApplication::processEvents();
            QJsonObject sceneJson = scenarioEditor->hierarchy->toJson();
            if (!sceneJson.isEmpty()) {
                missionEditor->hierarchy->fromJson(sceneJson);
                if (missionEditor->treeView && missionEditor->treeView->getTreeWidget()) {
                    missionEditor->treeView->getTreeWidget()->update();
                    missionEditor->treeView->getTreeWidget()->collapseAll();
                }
            }
        }
        // If a .ms file was previously saved/loaded, reload its doctrine data
        if (!missionEditor->lastSavedFilePath.isEmpty()) {
            m_loadingLabel->setText("Restoring Mission Doctrine Data...");
            QCoreApplication::processEvents();
            missionEditor->loadFromJsonFile(missionEditor->lastSavedFilePath);
        }
        m_loadingLabel->setText("Switching to Mission Editor...");
        QCoreApplication::processEvents();
        stackedWidget->setCurrentWidget(missionEditor);
        updateWindowTitleForCurrentEditor();
        if (m_statusBar)
            m_statusBar->setFileName(missionEditor->lastSavedFilePath,
                                     missionEditor->hasUnsavedChanges);
        emit missionEditor->Activated();
        hideLoadingOverlay();
    }

    // ── RUNTIME ──────────────────────────────────────────────────────────
    else if (editorKey == "runtime") {
        showLoadingOverlay("Loading Runtime Editor...");
        mainMenuBar->updateFileMenuForEditor(editorKey);      // 1st
        mainMenuBar->setLibraryActionsVisible(true);          // 2nd
        if (!runtimeEditor) {
            m_loadingLabel->setText("Creating Runtime Editor...");
            QCoreApplication::processEvents();
            runtimeEditor = new RuntimeEditor(centralWidget());
            stackedWidget->addWidget(runtimeEditor);
            connect(runtimeEditor, &RuntimeEditor::unsavedChangesChanged,
                    this, &MainWindow::onUnsavedChangesChanged);
        }
        if (!databaseEditor->lastSavedFilePath.isEmpty()) {
            m_loadingLabel->setText("Loading Database...");
            QCoreApplication::processEvents();
            QFile file(databaseEditor->lastSavedFilePath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                file.close();
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(data, &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.contains("hierarchy")) {
                        m_loadingLabel->setText("Loading Database...");
                        QCoreApplication::processEvents();
                        runtimeEditor->library->fromJson(obj["hierarchy"].toObject());
                        if (runtimeEditor->libTreeView) {
                            runtimeEditor->libTreeView->setLibraryFileName(
                                QFileInfo(databaseEditor->lastSavedFilePath).fileName());
                            runtimeEditor->libTreeView->getTreeWidget()->update();
                            runtimeEditor->libTreeView->getTreeWidget()->collapseAll();
                        }
                    }
                }
            }
        }
        else if (ApplicationDialog::getGlobalDatabaseEnabled() &&
                 !ApplicationDialog::getGlobalDatabasePath().isEmpty() &&
                 ApplicationDialog::getGlobalDatabasePath() != "No path set") {
            m_loadingLabel->setText("Loading Database...");
            QCoreApplication::processEvents();
            QString dbPath = ApplicationDialog::getGlobalDatabasePath();
            QFile file(dbPath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                file.close();
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(data, &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.contains("hierarchy")) {
                        m_loadingLabel->setText("Loading Database...");
                        QCoreApplication::processEvents();
                        runtimeEditor->library->fromJson(obj["hierarchy"].toObject());
                        if (runtimeEditor->libTreeView) {
                            runtimeEditor->libTreeView->setLibraryFileName(QFileInfo(dbPath).fileName());
                            runtimeEditor->libTreeView->getTreeWidget()->update();
                            runtimeEditor->libTreeView->getTreeWidget()->collapseAll();
                        }
                    }
                }
            }
        }
        // PRIORITY 3: Default Aircraft.db load karo
        else if (ApplicationDialog::getGlobalDatabaseEnabled()) {
            m_loadingLabel->setText("Loading Database...");
            QCoreApplication::processEvents();
            QString aircraftDbPath = TDFManager::instance()->getAircraftDbPath();
            QFile jsonFile(aircraftDbPath);
            if (jsonFile.open(QIODevice::ReadOnly)) {
                QByteArray data = jsonFile.readAll();
                jsonFile.close();
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
                if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.contains("hierarchy")) {
                        m_loadingLabel->setText("Loading Database...");
                        QCoreApplication::processEvents();
                        runtimeEditor->library->fromJson(obj["hierarchy"].toObject());
                        if (runtimeEditor->libTreeView) {
                            runtimeEditor->libTreeView->setLibraryFileName("Aircraft.db (Default)");
                            runtimeEditor->libTreeView->getTreeWidget()->update();
                            runtimeEditor->libTreeView->getTreeWidget()->collapseAll();
                        }
                    }
                }
            }
        }
        // PRIORITY 4: Empty library
        else {
            m_loadingLabel->setText("Initializing Empty Library...");
            QCoreApplication::processEvents();
            runtimeEditor->library->fromJson(QJsonObject());
            if (runtimeEditor->libTreeView) {
                runtimeEditor->libTreeView->setLibraryFileName("");
                runtimeEditor->libTreeView->getTreeWidget()->update();
            }
        }
        QWidget* prevWidget = stackedWidget->currentWidget();
        bool comingFromAnalysis = (analysisEditor && prevWidget == analysisEditor);
        if (!comingFromAnalysis &&
            scenarioEditor && !scenarioEditor->lastSavedFilePath.isEmpty()) {
            m_loadingLabel->setText("Loading Scenario Data...");
            QCoreApplication::processEvents();
            runtimeEditor->loadFromJsonFile(scenarioEditor->lastSavedFilePath);
        }
        m_loadingLabel->setText("Updating Tree View...");
        QCoreApplication::processEvents();
        if (runtimeEditor->libTreeView) {
            runtimeEditor->libTreeView->getTreeWidget()->update();
            runtimeEditor->libTreeView->getTreeWidget()->collapseAll();
        }
        m_loadingLabel->setText("Switching to Runtime Editor...");
        QCoreApplication::processEvents();
        stackedWidget->setCurrentWidget(runtimeEditor);
        updateWindowTitleForCurrentEditor();
        if (m_statusBar)
            m_statusBar->setFileName(runtimeEditor->lastSavedFilePath, runtimeEditor->hasUnsavedChanges);
        emit runtimeEditor->Activated();
        hideLoadingOverlay();
    }

    // ── ANALYSIS ─────────────────────────────────────────────────────────
    else if (editorKey == "analysis") {
        showLoadingOverlay("Loading Analysis Editor...");
        mainMenuBar->updateFileMenuForEditor(editorKey);      // 1st  ← was MISSING
        mainMenuBar->setLibraryActionsVisible(false);         // 2nd
        if (!analysisEditor) {
            m_loadingLabel->setText("Creating Analysis Editor...");
            QCoreApplication::processEvents();
            analysisEditor = new AnalysisEditor(centralWidget());
            stackedWidget->addWidget(analysisEditor);
            connect(analysisEditor, &AnalysisEditor::unsavedChangesChanged,
                    this, &MainWindow::onUnsavedChangesChanged);
        }
        m_loadingLabel->setText("Loading Analysis Data...");
        QCoreApplication::processEvents();
        if (runtimeEditor && runtimeEditor->hierarchy) {
            if (runtimeEditor->hierarchy->Platforms) {
                int i = 0;
                for (const auto& [key, entity] : *runtimeEditor->hierarchy->Platforms) {
                    if (entity) { } else { }
                    i++;
                }
            }
            QJsonObject analysisJson = runtimeEditor->hierarchy->loadAnalysisJson();
            if (!analysisJson.isEmpty()) {
                analysisEditor->loadFromHierarchyJson(analysisJson);
            }
        }
        else if (scenarioEditor && scenarioEditor->hierarchy) {
            QJsonObject analysisJson = scenarioEditor->hierarchy->loadAnalysisJson();
            if (!analysisJson.isEmpty()) {
                analysisEditor->loadFromHierarchyJson(analysisJson);
            }
        }
        else if (databaseEditor && databaseEditor->hierarchy) {
            QJsonObject analysisJson = databaseEditor->hierarchy->loadAnalysisJson();
            if (!analysisJson.isEmpty()) {
                analysisEditor->loadFromHierarchyJson(analysisJson);
            }
        }
        m_loadingLabel->setText("Switching to Analysis Editor...");
        QCoreApplication::processEvents();
        stackedWidget->setCurrentWidget(analysisEditor);
        updateWindowTitleForCurrentEditor();
        if (m_statusBar)
            m_statusBar->setFileName(analysisEditor->lastSavedFilePath,
                                     analysisEditor->hasUnsavedChanges);
        emit analysisEditor->Activated();
        hideLoadingOverlay();
    }
}
// updateWindowTitleForCurrentEditor: MissionEditor block
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
    } else if (scenarioEditor && currentWidget == scenarioEditor) {
        editorName = "Scenario Editor";
        filePath = scenarioEditor->lastSavedFilePath;
        hasUnsaved = scenarioEditor->hasUnsavedChanges;
    } else if (missionEditor && currentWidget == missionEditor) {
        editorName  = "Mission Editor";
        filePath    = missionEditor->lastSavedFilePath;
        hasUnsaved  = missionEditor->hasUnsavedChanges;
    } else if (runtimeEditor && currentWidget == runtimeEditor) {
        editorName = "Runtime Editor";
        filePath = runtimeEditor->lastSavedFilePath;
        hasUnsaved = runtimeEditor->hasUnsavedChanges;
    }else if (analysisEditor && currentWidget == analysisEditor) {
        editorName = "Analysis Editor";
        filePath   = analysisEditor->lastSavedFilePath;
        hasUnsaved = analysisEditor->hasUnsavedChanges;
    }
    updateWindowTitle(editorName, hasUnsaved);
    if (m_statusBar) m_statusBar->setFileName(filePath, hasUnsaved);
}

void MainWindow::updateWindowTitle(const QString& editorName, bool hasUnsavedChanges)
{
    QString title = "Indigenous Scenario and Sensor Simulation Toolkit";
    if (scenarioconfig && !scenarioconfig->software_version.isEmpty())
        title += QString(" (V_%1)").arg(scenarioconfig->software_version);
    title += " - " + editorName;
    if (hasUnsavedChanges) title = "*" + title;
    setWindowTitle(title);
}

void MainWindow::onUnsavedChangesChanged(bool hasUnsaved)
{
    updateWindowTitleForCurrentEditor();
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    bool proceedWithClose = true;

    if (databaseEditor && databaseEditor->hasUnsavedChanges)
        proceedWithClose &= promptForSave(databaseEditor, "Database Editor");
    if (scenarioEditor && scenarioEditor->hasUnsavedChanges)
        proceedWithClose &= promptForSave(scenarioEditor, "Scenario Editor");
    if (missionEditor && missionEditor->hasUnsavedChanges)
        proceedWithClose &= promptForSave(missionEditor, "Mission Editor");
    if (runtimeEditor && runtimeEditor->hasUnsavedChanges)
        proceedWithClose &= promptForSave(runtimeEditor, "Runtime Editor");
    if (analysisEditor && analysisEditor->hasUnsavedChanges)
        proceedWithClose &= promptForSave(analysisEditor, "Analysis Editor");
    proceedWithClose ? event->accept() : event->ignore();
}
bool MainWindow::promptForSave(QMainWindow* editor, const QString& editorName)
{
    DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor);
    ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor);
    MissionEditor*  msEditor = qobject_cast<MissionEditor*>(editor);
    RuntimeEditor*  rtEditor = qobject_cast<RuntimeEditor*>(editor);
    AnalysisEditor* anEditor = qobject_cast<AnalysisEditor*>(editor);
    bool hasUnsaved = false;
    QString filePath;
    if      (dbEditor) { hasUnsaved = dbEditor->hasUnsavedChanges; filePath = dbEditor->lastSavedFilePath; }
    else if (scEditor) { hasUnsaved = scEditor->hasUnsavedChanges; filePath = scEditor->lastSavedFilePath; }
    else if (msEditor) { hasUnsaved = msEditor->hasUnsavedChanges; filePath = msEditor->lastSavedFilePath; }
    else if (rtEditor) { hasUnsaved = rtEditor->hasUnsavedChanges; filePath = rtEditor->lastSavedFilePath; }
    if (!hasUnsaved) return true;
    QMessageBox msgBox(this);
    msgBox.setStyleSheet(MainWindowStyles::MessageBox);
    msgBox.setWindowTitle("Unsaved Changes");
    msgBox.setText(QString("There are unsaved changes in %1. Do you want to save?").arg(editorName));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    QMessageBox::StandardButton reply = (QMessageBox::StandardButton)msgBox.exec();

    if (reply == QMessageBox::Save) {
        if (!mainMenuBar) { qWarning() << "Main menu bar not found"; return false; }

        QAction *saveAction = filePath.isEmpty()
                                  ? mainMenuBar->getSaveAction()
                                  : mainMenuBar->getSameSaveAction();
        if (saveAction) {
            saveAction->trigger();
            QCoreApplication::processEvents();
            QCoreApplication::processEvents();
            QThread::msleep(100);
            QCoreApplication::processEvents();
            if      (dbEditor) return !dbEditor->hasUnsavedChanges;
            else if (scEditor) return !scEditor->hasUnsavedChanges;
            else if (msEditor) return !msEditor->hasUnsavedChanges;
            else if (rtEditor) return !rtEditor->hasUnsavedChanges;
        }
        return false;

    } else if (reply == QMessageBox::Discard) {
        if      (dbEditor) dbEditor->clearUnsavedChanges();
        else if (scEditor) scEditor->clearUnsavedChanges();
        else if (msEditor) msEditor->clearUnsavedChanges();
        else if (rtEditor) rtEditor->clearUnsavedChanges();
        return true;
    } else {
        return false;
    }
}

void MainWindow::showLoadingOverlay(const QString& message)
{
    if (!m_loadingOverlay) {
        m_loadingOverlay = new QWidget(this);
        m_loadingOverlay->setStyleSheet(
            "QWidget#loadingOverlay {"
            "  background-color: rgba(15, 38, 54, 0.85);"
            "}"
            );
        m_loadingOverlay->setObjectName("loadingOverlay");

        QWidget* card = new QWidget(m_loadingOverlay);
        card->setFixedSize(280, 110);
        card->setStyleSheet(
            "QWidget {"
            "  background-color: #1A3A4F;"
            "  border-radius: 10px;"
            "  border: 1px solid #00BFFF;"
            "}"
            );

        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20, 18, 20, 18);
        cardLayout->setSpacing(12);

        m_loadingLabel = new QLabel(message, card);
        m_loadingLabel->setAlignment(Qt::AlignCenter);
        m_loadingLabel->setStyleSheet(
            "QLabel {"
            "  color: white;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "  background: transparent;"
            "  border: none;"
            "}"
            );
        m_loadingBar = new QProgressBar(card);
        m_loadingBar->setRange(0, 0);
        m_loadingBar->setFixedHeight(6);
        m_loadingBar->setTextVisible(false);
        m_loadingBar->setStyleSheet(
            "QProgressBar { background-color: #0F2636; border-radius: 3px; border: none; }"
            "QProgressBar::chunk { background-color: #00BFFF; border-radius: 3px; }"
            );
        cardLayout->addWidget(m_loadingLabel);
        cardLayout->addWidget(m_loadingBar);
        card->setProperty("isCard", true);
    } else {
        if (m_loadingLabel) m_loadingLabel->setText(message);
    }
    m_loadingOverlay->setGeometry(0, 0, width(), height());
    m_loadingOverlay->raise();
    m_loadingOverlay->show();
    for (QObject* child : m_loadingOverlay->children()) {
        QWidget* w = qobject_cast<QWidget*>(child);
        if (w && w->property("isCard").toBool()) {
            w->move((m_loadingOverlay->width()  - w->width())  / 2,
                    (m_loadingOverlay->height() - w->height()) / 2);
            break;
        }
    }
    QCoreApplication::processEvents();
}
void MainWindow::hideLoadingOverlay()
{
    if (m_loadingOverlay) m_loadingOverlay->hide();
}
