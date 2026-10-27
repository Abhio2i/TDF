#include "mainwindow.h"
#include "core/core_test.h"
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
#include "GUI/Tacticaldisplay/tooltiphelper.h"
#include "GUI/Tacticaldisplay/Layer/layerpanel.h"
#include "GUI/plugins/pluginmanagerdialog.h"
#include <QtConcurrent/QtConcurrent>
#include <GIS_API/backend/api/shape_api.h>
#include <QFuture>
#include <QFutureWatcher>
#include <QProcess>
#include <QStandardPaths>
#include "GUI/Canvas/canvas_layer/dynamiccanvas.h"
#include <GIS_API/backend/api/shape_api.h>
#include <GIS_API/backend/api/layer_api.h>


ScenarioConfig* MainWindow::scenarioconfig = nullptr;
MainWindow* MainWindow::s_instance = nullptr;

const QString MainWindow::kScenarioInstanceFolder = "ScenarioInstance";
const QString MainWindow::kScenarioPrefix = "Scenario_";
const QString MainWindow::kRuntimePrefix  = "Runtime_";
const QString MainWindow::kInstancePrefix = "Instance_";


static const QString kHelpButtonStyle =
    "QPushButton#helpButton {"
    "  background-color: transparent;"
    "  border: none;"
    "  border-radius: 0px;"
    "}"
    "QPushButton#helpButton:hover {"
    "  background-color: rgba(0, 191, 255, 0.12);"
    "  border-radius: 4px;"
    "}";

// ---------------------------------------------------------------------------
// EditorKey helpers (replaces hardcoded "database"/"scenario"/... strings)
// ---------------------------------------------------------------------------
QString MainWindow::editorKeyToString(EditorKey key)
{
    switch (key) {
    case EditorKey::Database: return "database";
    case EditorKey::Scenario: return "scenario";
    case EditorKey::Mission:  return "mission";
    case EditorKey::Runtime:  return "runtime";
    case EditorKey::Analysis: return "analysis";
    }
    return QString();
}

MainWindow::EditorKey MainWindow::editorKeyFromString(const QString& key)
{
    if (key == "database") return EditorKey::Database;
    if (key == "scenario") return EditorKey::Scenario;
    if (key == "mission")  return EditorKey::Mission;
    if (key == "runtime")  return EditorKey::Runtime;
    return EditorKey::Analysis;
}

// ---------------------------------------------------------------------------
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


MainWindow::~MainWindow()
{
    std::cout << "Mainwindow delete";
    if (databaseEditor) delete databaseEditor;
    if (scenarioEditor) delete scenarioEditor;
    if (missionEditor)  delete missionEditor;
    if (runtimeEditor)  delete runtimeEditor;
    if (analysisEditor) delete analysisEditor;
    if (scenarioconfig) { delete scenarioconfig; scenarioconfig = nullptr; }
}

// ---------------------------------------------------------------------------
// setupUI() — split into smaller pieces (createTopBar/createHelpButton)
// ---------------------------------------------------------------------------
void MainWindow::setupUI()
{
    QWidget *topBarWidget = createTopBar();

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

    m_statusBar = new StatusBar(this);
    setStatusBar(m_statusBar);
    connect(m_statusBar, &StatusBar::saveRequested, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (currentEditor) saveToSameFileWithTDFSupport(currentEditor);
    });

    connect(navigationPage, &NavigationPage::editorRequested, this, &MainWindow::switchEditor);
    connect(databaseEditor, &DatabaseEditor::unsavedChangesChanged,
            this, &MainWindow::onUnsavedChangesChanged);
    connect(databaseEditor, &DatabaseEditor::databaseFileLoaded,
            this, &MainWindow::onDatabaseFileChanged);

    stackedWidget->setCurrentWidget(databaseEditor);
    setupMenuBarConnections();
}
QWidget* MainWindow::createTopBar()
{
    QWidget *topBarWidget = new QWidget(this);
    topBarWidget->setObjectName("topBarWidget");
    topBarWidget->setFixedHeight(50);
    topBarWidget->setStyleSheet(MainWindowStyles::TopBarWidget);
    QHBoxLayout *topBarLayout = new QHBoxLayout(topBarWidget);
    topBarLayout->setContentsMargins(0, 0, 0, 0);
    topBarLayout->setSpacing(0);

    mainMenuBar = new MenuBar(this);
    mainMenuBar->setStyleSheet(MainWindowStyles::MenuBar);
    mainMenuBar->updateFileMenuForEditor("database");
    mainMenuBar->setLibraryActionsVisible(false);
    setMenuBar(nullptr);
    topBarLayout->addWidget(mainMenuBar);

    navigationPage = new NavigationPage(this);
    navigationPage->setStyleSheet(MainWindowStyles::NavigationPage);
    topBarLayout->addWidget(navigationPage);
    topBarLayout->addStretch();

    QPushButton *helpButton = createHelpButton();
    topBarLayout->addWidget(helpButton);
    topBarLayout->setContentsMargins(0, 0, 10, 0);

    return topBarWidget;
}

QPushButton* MainWindow::createHelpButton()
{
    QPushButton *helpButton = new QPushButton(this);
    helpButton->setObjectName("helpButton");
    helpButton->setFixedSize(24, 24);
    helpButton->setToolTip("Help");
    helpButton->setCursor(Qt::PointingHandCursor);
    helpButton->setIcon(QIcon(":/icons/images/help.png"));
    helpButton->setIconSize(QSize(22, 22));
    helpButton->setText("");
    helpButton->setStyleSheet(kHelpButtonStyle);
    connect(helpButton, &QPushButton::clicked, this, &MainWindow::onHelpButtonClicked);
    return helpButton;
}


void MainWindow::copyDirectoryRecursive(const QString& src, const QString& dst)
{
    QDir srcDir(src);
    QDir dstDir(dst);
    if (!dstDir.exists()) dstDir.mkpath(dst);
    for (const QString& file : srcDir.entryList(QDir::Files)) {
        QFile::copy(src + "/" + file, dst + "/" + file);
    }
    for (const QString& subDir : srcDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        copyDirectoryRecursive(src + "/" + subDir, dst + "/" + subDir);
    }
}

void MainWindow::onHelpButtonClicked()
{
    QString appDirPath = QApplication::applicationDirPath();
    QStringList searchPaths = {
        appDirPath + "/../../DB",
        appDirPath + "/../../../DB",
        appDirPath + "/../DB",
        appDirPath + "/DB",
    };
    QString helpPath;
    for (const QString& path : searchPaths) {
        QDir d(path);
        if (d.exists()) {
            helpPath = d.absolutePath();
            break;
        }
    }
    if (helpPath.isEmpty()) {
        QMessageBox::warning(this, "Error", "DB directory not found!");
        return;
    }
    QDir dbDir(helpPath);
    QStringList helpDirs = dbDir.entryList(
        QStringList() << "TDF_HELP_V_*",
        QDir::Dirs | QDir::NoDotAndDotDot
        );
    if (helpDirs.isEmpty()) {
        QMessageBox::warning(this, "Error", "TDF_HELP_V_* directory not found!");
        return;
    }
    QString srcHelpDir = helpPath + "/" + helpDirs.first() + "/build/html";
    QDir srcDir(srcHelpDir);
    if (!srcDir.exists()) {
        QMessageBox::warning(this, "Error",
                             "Help docs not built: \"" + srcHelpDir +
                                 "\" not found.\nRun 'make html' inside " + helpDirs.first() + "/build first.");
        return;
    }

    QString destHelpDir = QDir::homePath() + "/TDF/Help/" + helpDirs.first() + "/build/html";
    QString htmlFilePath = destHelpDir + "/index.html";
    if (!QFileInfo::exists(htmlFilePath)) {
        QDir destDir(destHelpDir);
        destDir.mkpath(destHelpDir);
        copyDirectoryRecursive(srcHelpDir, destHelpDir);
    }
    if (!QFileInfo::exists(htmlFilePath)) {
        QMessageBox::warning(this, "Error", "index.html not found after copy!");
        return;
    }

    QString browser = QStandardPaths::findExecutable("xdg-open");
    if (browser.isEmpty()) {
        QStringList browsers = {
            "/usr/bin/google-chrome",
            "/usr/bin/chromium-browser",
            "/usr/bin/chromium",
            "/usr/bin/firefox",
            "/usr/bin/xdg-open"
        };
        for (const QString& b : browsers) {
            if (QFileInfo::exists(b)) {
                browser = b;
                break;
            }
        }
    }
    if (browser.isEmpty()) {
        QMessageBox::warning(this, "Error", "No browser found!");
        return;
    }


    QString browserBinary = QFileInfo(browser).fileName();
    bool isChromeFamily = browserBinary.contains("chrome", Qt::CaseInsensitive) ||
                          browserBinary.contains("chromium", Qt::CaseInsensitive);
    QStringList args;
    if (isChromeFamily) {
        args << "--no-sandbox";
    }
    args << htmlFilePath;

    if (!QProcess::startDetached(browser, args)) {
        QMessageBox::warning(this, "Error", "Failed to launch browser for Help.");
    }
}
// ---------------------------------------------------------------------------
// setupMenuBarConnections() — split into focused connect* methods
// ---------------------------------------------------------------------------
void MainWindow::setupMenuBarConnections()
{
    connectFileMenuActions();
    connectLoadActions();
    connectLibraryActions();
    connectSettingsAndInfoActions();
    connectPluginActions();
    connect(mainMenuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
}

void MainWindow::connectFileMenuActions()
{
    connect(mainMenuBar, &MenuBar::newFileTriggered, this, [=]() {
        pauseSimulationIfRunning();
        QMainWindow *currentEditor = getCurrentEditor();
        if (currentEditor) {
            if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(currentEditor)) {
                dbEditor->hierarchy->fromJson(QJsonObject());
                HierarchyConnector::instance()->initializeDummyData(dbEditor->hierarchy);
                dbEditor->lastSavedFilePath = QString();
                dbEditor->clearUnsavedChanges();
                if (m_statusBar)
                    m_statusBar->setFileName(QString(), false);
                updateWindowTitleForCurrentEditor();
            } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(currentEditor)) {
                scEditor->hierarchy->fromJson(QJsonObject());
                HierarchyConnector::instance()->initializeDummyData(scEditor->hierarchy);
                if (scEditor->tacticalDisplay && scEditor->tacticalDisplay->canvas) {
                    scEditor->tacticalDisplay->canvas->fromJson(QJsonObject());
                    scEditor->tacticalDisplay->canvas->resetEntityInfoDialog();
                    scEditor->tacticalDisplay->canvas->fullCanvasReset();
                }
                if (LayerPanel* layerPanel = scEditor->findChild<LayerPanel*>()) {
                    layerPanel->fromJson(QJsonObject());
                }
                ShapeApi::instance()->deleteAllShapes();
                LayerApi::instance()->fromJson(QJsonObject());
                scEditor->lastSavedFilePath = QString();
                scEditor->clearUnsavedChanges();
                if (m_statusBar)
                    m_statusBar->setFileName(QString(), false);
                updateWindowTitleForCurrentEditor();
            } else if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(currentEditor)) {
                msEditor->hierarchy->fromJson(QJsonObject());
                HierarchyConnector::instance()->initializeDummyData(msEditor->hierarchy);
                msEditor->lastSavedFilePath = QString();
                msEditor->clearUnsavedChanges();
                if (m_statusBar)
                    m_statusBar->setFileName(QString(), false);
                updateWindowTitleForCurrentEditor();
            } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor)) {
                rtEditor->hierarchy->fromJson(QJsonObject());
                if (rtEditor->tacticalDisplay && rtEditor->tacticalDisplay->canvas) {
                    rtEditor->tacticalDisplay->canvas->fromJson(QJsonObject());
                    rtEditor->tacticalDisplay->canvas->resetEntityInfoDialog();
                    rtEditor->tacticalDisplay->canvas->fullCanvasReset();
                }
                for (EntityInfoDialog* dlg : rtEditor->findChildren<EntityInfoDialog*>()) {
                    if (dlg)
                        dlg->onHierarchyReset();
                }
                if (LayerPanel* layerPanel = rtEditor->findChild<LayerPanel*>()) {
                    layerPanel->fromJson(QJsonObject());
                }
                HierarchyConnector::instance()->initializeDummyData(rtEditor->hierarchy);
                ShapeApi::instance()->deleteAllShapes();
                LayerApi::instance()->fromJson(QJsonObject());
                rtEditor->lastSavedFilePath = QString();
                rtEditor->clearUnsavedChanges();
                if (m_statusBar)
                    m_statusBar->setFileName(QString(), false);
                updateWindowTitleForCurrentEditor();
            }
        }
    });

    // Recent Project
    connect(mainMenuBar, &MenuBar::recentProjectTriggered, this, [=]() {
        pauseSimulationIfRunning();
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
    }, Qt::UniqueConnection);

    // Save As
    connect(mainMenuBar->getSaveAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;
        saveFileWithTDFSupport(currentEditor);
    }, Qt::UniqueConnection);

    connect(mainMenuBar->getSameSaveAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;
        saveToSameFileWithTDFSupport(currentEditor);
    }, Qt::UniqueConnection);


    if (QAction* sameSave = mainMenuBar->getSameSaveAction()) {
        sameSave->setShortcut(QKeySequence::Save);
        sameSave->setShortcutContext(Qt::ApplicationShortcut);
        addAction(sameSave);
    }
    if (QAction* saveAs = mainMenuBar->getSaveAction()) {
        saveAs->setShortcutContext(Qt::ApplicationShortcut);
        addAction(saveAs);
    }
}

void MainWindow::connectLoadActions()
{
    // Load File
    connect(mainMenuBar->getLoadAction(), &QAction::triggered, this, [=]() {
        pauseSimulationIfRunning();
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
    }, Qt::UniqueConnection);

    // Load to Library
    connect(mainMenuBar->getLoadToLibraryAction(), &QAction::triggered, this, [=]() {
        pauseSimulationIfRunning();
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;
        HierarchyConnector::instance()->loadToLibrary(currentEditor);
        QString loadedPath;
        if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(currentEditor)) {
            if (scEditor->libTreeView)
                loadedPath = scEditor->libTreeView->libraryFilePath();
            if (!loadedPath.isEmpty())
                scEditor->setLoadedDatabaseFile(loadedPath);
        } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor)) {
            if (rtEditor->libTreeView)
                loadedPath = rtEditor->libTreeView->libraryFilePath();
            if (!loadedPath.isEmpty())
                rtEditor->setLoadedDatabaseFile(loadedPath);
        }
        if (!loadedPath.isEmpty())
            updateWindowTitleForCurrentEditor();
    }, Qt::UniqueConnection);

    connect(mainMenuBar->getOpenRuntimeInstanceAction(), &QAction::triggered, this, [=]() {
        pauseSimulationIfRunning();
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;

        QString activeDb = resolveActiveDatabasePath();
        QString startPath = ensureScenarioFolderForDb(activeDb);
        if (startPath.isEmpty() || !QDir(startPath).exists())
            startPath = tdfRootPath() + "/Database";
        if (!QDir(startPath).exists())
            startPath = QDir::homePath();

        QFileDialog fileDialog(this);
        fileDialog.setStyleSheet(MainWindowStyles::FileDialog);
        fileDialog.setWindowTitle("Open Runtime Instance File");
        fileDialog.setDirectory(startPath);
        fileDialog.setNameFilter("Scenario Instance Files (*.sc);;All Files (*.*)");
        fileDialog.setFileMode(QFileDialog::ExistingFile);

        QString filePath;
        if (fileDialog.exec() == QDialog::Accepted && !fileDialog.selectedFiles().isEmpty())
            filePath = fileDialog.selectedFiles().first();
        if (filePath.isEmpty()) return;

        if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor)) {
            rtEditor->loadFromJsonFile(filePath);
            RecentProjectsManager::instance()->addToRecentProjects(filePath, RecentProjectsManager::RuntimeEditor);
        } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(currentEditor)) {
            scEditor->loadFromJsonFile(filePath);
            RecentProjectsManager::instance()->addToRecentProjects(filePath, RecentProjectsManager::ScenarioEditor);
        }
    }, Qt::UniqueConnection);
    connect(mainMenuBar->getOpenMissionFileAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!qobject_cast<RuntimeEditor*>(currentEditor)) return;
        QString startPath = ensureTDFSubfolder("Mission");
        QFileDialog fileDialog(this);
        fileDialog.setStyleSheet(MainWindowStyles::FileDialog);
        fileDialog.setWindowTitle("Open Mission File");
        fileDialog.setDirectory(startPath);
        fileDialog.setNameFilter("Mission Files (*.ms);;All Files (*.*)");
        fileDialog.setFileMode(QFileDialog::ExistingFile);
        QString filePath;
        if (fileDialog.exec() == QDialog::Accepted && !fileDialog.selectedFiles().isEmpty())
            filePath = fileDialog.selectedFiles().first();
        if (filePath.isEmpty()) return;
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
        if (missionEditor) {
            missionEditor->loadFromJsonFile(filePath);
        }
        if (m_statusBar)
            m_statusBar->setFileName(filePath, false);
        RecentProjectsManager::instance()->addToRecentProjects(
            filePath, RecentProjectsManager::MissionEditor);
    }, Qt::UniqueConnection);


    connect(mainMenuBar->getLoadXmlAction(), &QAction::triggered, this, [=]() {
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;
        QFileDialog fileDialog(this);
        fileDialog.setStyleSheet(MainWindowStyles::FileDialog);
        fileDialog.setWindowTitle("Open XML");
        fileDialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        fileDialog.setNameFilter("XML Files (*.xml)");
        fileDialog.setFileMode(QFileDialog::ExistingFile);
        QString filePath;
        if (fileDialog.exec() == QDialog::Accepted && !fileDialog.selectedFiles().isEmpty())
            filePath = fileDialog.selectedFiles().first();
        if (filePath.isEmpty()) return;

        if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(currentEditor)) {
            HierarchyConnector::instance()->openXmlFile(dbEditor->hierarchy, filePath);
        } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(currentEditor)) {
            HierarchyConnector::instance()->openXmlFile(scEditor->hierarchy, filePath);
        } else if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(currentEditor)) {
            HierarchyConnector::instance()->openXmlFile(msEditor->hierarchy, filePath);
        } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor)) {
            HierarchyConnector::instance()->openXmlFile(rtEditor->hierarchy, filePath);
        }
    }, Qt::UniqueConnection);
}


void MainWindow::loadLibraryFileForEditor(const QString& filePath, QMainWindow* editor)
{
    if (filePath.isEmpty()) return;

    RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor);
    ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor);
    if (!rtEditor && !scEditor) return;

    if (rtEditor && rtEditor->tacticalDisplay && rtEditor->tacticalDisplay->canvas) {
        rtEditor->tacticalDisplay->canvas->fullCanvasReset();
    }

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
        Hierarchy* library = getDatabaseHierarchy();
        HierarchyTree* libTreeView = scEditor ? scEditor->libTreeView : rtEditor->libTreeView;
        if (m_loadingLabel) { m_loadingLabel->setText("Loading Database..."); QCoreApplication::processEvents(); }
        if (library) {
            library->clear();
            library->fromJson(obj["hierarchy"].toObject());
        }
        if (m_loadingLabel) { m_loadingLabel->setText("Updating Tree View..."); QCoreApplication::processEvents(); }
        if (libTreeView) {
            libTreeView->setLibraryFileName(filePath);
            libTreeView->getTreeWidget()->update();
        }
        RecentProjectsManager::instance()->addToRecentProjects(filePath, RecentProjectsManager::LibraryData);
    }

    if (scEditor) scEditor->setLoadedDatabaseFile(filePath);
    else if (rtEditor) rtEditor->setLoadedDatabaseFile(filePath);
    hideLoadingOverlay();
}


void MainWindow::connectLibraryActions()
{
    connect(RecentProjectsManager::instance(), &RecentProjectsManager::projectSelected,
            this, &MainWindow::handleLibraryProjectSelected, Qt::UniqueConnection);

    connect(mainMenuBar, &MenuBar::recentProjectLibraryTriggered, this, [=]() {
        pauseSimulationIfRunning();
        QMainWindow *currentEditor = getCurrentEditor();
        if (!currentEditor) return;

        if (qobject_cast<ScenarioEditor*>(currentEditor) || qobject_cast<RuntimeEditor*>(currentEditor)) {
            RecentProjectsManager::instance()->showRecentLibraryMenu(this);
        } else {
            QMessageBox::information(this, "Recent Library",
                                     "Recent Library is only available in Scenario and Runtime editors.");
        }
    }, Qt::UniqueConnection);
}

void MainWindow::handleLibraryProjectSelected(const QString& filePath, RecentProjectsManager::EditorType type)
{
    if (type != RecentProjectsManager::LibraryData || filePath.isEmpty()) return;
    QMainWindow* currentEditor = getCurrentEditor();
    if (!currentEditor) return;
    if (!qobject_cast<ScenarioEditor*>(currentEditor) && !qobject_cast<RuntimeEditor*>(currentEditor)) return;
    loadLibraryFileForEditor(filePath, currentEditor);
}

void MainWindow::connectSettingsAndInfoActions()
{
    connect(mainMenuBar, &MenuBar::feedbackTriggered, this, [=]() {
        Feedback *feedbackWindow = new Feedback(this);
        feedbackWindow->show();
    }, Qt::UniqueConnection);

    connect(mainMenuBar, &MenuBar::profileTriggered, this, [=]() {
        ProfileInfoDialog::showProfileInfo(this);
    }, Qt::UniqueConnection);

    connect(mainMenuBar, &MenuBar::applicationTriggered, this, [=]() {
        ApplicationDialog dialog(this);
        QMainWindow *currentEditor = getCurrentEditor();
        if (currentEditor) {
            if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(currentEditor)) {
                // tactical canvas
                if (scEditor->tacticalDisplay && scEditor->tacticalDisplay->canvas) {
                    connect(&dialog, &ApplicationDialog::canvasIconState,
                            scEditor->tacticalDisplay->canvas, &CanvasWidget::setImageScale);
                }
                // ===== NEW MAP  =====
                if (scEditor->m_apiMapWidget && scEditor->m_apiMapWidget->dynamicCanvas()) {
                    connect(&dialog, &ApplicationDialog::canvasIconState,
                            scEditor->m_apiMapWidget->dynamicCanvas(),
                            &DynamicCanvas::setImageScale);
                }
                // ==============================
            } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(currentEditor)) {
                if (rtEditor->tacticalDisplay && rtEditor->tacticalDisplay->canvas) {
                    connect(&dialog, &ApplicationDialog::canvasIconState,
                            rtEditor->tacticalDisplay->canvas, &CanvasWidget::setImageScale);
                }
                // ===== NEW MAP — =====
                if (rtEditor->m_apiMapWidget && rtEditor->m_apiMapWidget->dynamicCanvas()) {
                    connect(&dialog, &ApplicationDialog::canvasIconState,
                            rtEditor->m_apiMapWidget->dynamicCanvas(),
                            &DynamicCanvas::setImageScale);
                }
                // ==============================
                if (rtEditor->simulation) {
                    connect(&dialog, &ApplicationDialog::fpsState,
                            rtEditor->simulation, &Simulation::setFps);
                }
            }
        }
        connect(&dialog, &ApplicationDialog::databaseSettingsChanged,
                this, [=](bool enabled, const QString& path) {
                    if (scenarioconfig) {
                        scenarioconfig->saveDatabaseSettings(enabled, path);
                    }
                });
        dialog.exec();
    }, Qt::UniqueConnection);
}
void MainWindow::connectPluginActions()
{
    connect(mainMenuBar, &MenuBar::pluginsTriggered, this, [=]() {
        for (QDialog *dlg : findChildren<QDialog*>()) {
            if (dlg->windowTitle() == "Plugin Manager") {
                dlg->raise();
                dlg->activateWindow();
                return;
            }
        }
        PluginManagerDialog *dlg = new PluginManagerDialog(this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        RuntimeEditor *rtEditor = qobject_cast<RuntimeEditor*>(getCurrentEditor());

        connect(dlg, &PluginManagerDialog::pluginInstallRequested,
                this, [=](int pluginId, const QString &pluginName) {
                    if (rtEditor && rtEditor->pluginManager)
                        rtEditor->pluginManager->loadPlugin(pluginName.toStdString());
                });
        connect(dlg, &PluginManagerDialog::pluginUpdateRequested,
                this, [=](int pluginId, const QString &pluginName) {
                    if (rtEditor && rtEditor->pluginManager) {
                        rtEditor->pluginManager->unloadPlugin(pluginName.toStdString());
                        rtEditor->pluginManager->loadPlugin(pluginName.toStdString());
                    }
                });
        connect(dlg, &PluginManagerDialog::pluginUninstallRequested,
                this, [=](int pluginId, const QString &pluginName) {
                    if (rtEditor && rtEditor->pluginManager)
                        rtEditor->pluginManager->unloadPlugin(pluginName.toStdString());
                });
        connect(dlg, &PluginManagerDialog::pluginRemoveRequested,
                this, [=](int pluginId, const QString &pluginName) {
                    if (rtEditor && rtEditor->pluginManager)
                        rtEditor->pluginManager->unloadPlugin(pluginName.toStdString());
                });
        connect(dlg, &PluginManagerDialog::pluginAddRequested,
                this, [=](const PluginInfo &plugin) {
                });

        dlg->show();
        dlg->raise();
    }, Qt::UniqueConnection);
}


void MainWindow::loadFileWithTDFSupport(QMainWindow* editor,
                                        RecentProjectsManager::EditorType editorType,
                                        const QString& extension,
                                        const QString& filter)
{
    pauseSimulationIfRunning();
    if (editorType != RecentProjectsManager::DatabaseEditor
        && editorType != RecentProjectsManager::ScenarioEditor
        && editorType != RecentProjectsManager::MissionEditor
        && editorType != RecentProjectsManager::RuntimeEditor) {
        qWarning() << "loadFileWithTDFSupport: unrecognized editorType, aborting";
        return;
    }
    QString startPath = startPathForEditorType(editorType);
    QString filePath = QFileDialog::getOpenFileName(this, "Open File", startPath, filter);
    if (filePath.isEmpty()) return;

    showLoadingOverlay("Loading file...");
    QCoreApplication::processEvents();

    auto finishLoad = [&](const QString& label) {
        if (m_loadingLabel) { m_loadingLabel->setText(label); QCoreApplication::processEvents(); }
    };

    if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor)) {
        finishLoad("Loading Database...");
        dbEditor->loadFromJsonFile(filePath);
        QString dbName = databaseProjectNameFromPath(filePath);
        if (!dbName.isEmpty())
            ensureDatabaseProjectFolder(dbName);
    } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor)) {
        finishLoad("Loading Scenario...");
        scEditor->loadFromJsonFile(filePath);
    } else if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(editor)) {
        finishLoad("Loading Mission...");
        msEditor->loadFromJsonFile(filePath);
    } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor)) {
        finishLoad("Loading Runtime...");
        if (rtEditor->tacticalDisplay && rtEditor->tacticalDisplay->canvas) {
            rtEditor->tacticalDisplay->canvas->fullCanvasReset();
        }
        rtEditor->loadFromJsonFile(filePath);
    } else {
        hideLoadingOverlay();
        return;
    }
    RecentProjectsManager::instance()->addToRecentProjects(filePath, editorType);
    hideLoadingOverlay();
}

QJsonObject MainWindow::buildSaveJson(QMainWindow* editor) const
{
    QJsonObject obj;
    if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(editor)) {
        if (msEditor->doctrinePanel)
            obj["doctrine"] = msEditor->doctrinePanel->toJson();
        if (msEditor->tacticalPanel)
            obj["tactical"] = msEditor->tacticalPanel->toJsonBothTeams();
        return obj;
    }

    Hierarchy* hierarchy = nullptr;
    TacticalDisplay* tacticalDisplay = nullptr;
    if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor)) {
        hierarchy = dbEditor->hierarchy;
    } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor)) {
        hierarchy = scEditor->hierarchy;
        tacticalDisplay = scEditor->tacticalDisplay;
    } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor)) {
        hierarchy = rtEditor->hierarchy;
        tacticalDisplay = rtEditor->tacticalDisplay;
    }
    if (hierarchy) {
        obj["hierarchy"] = hierarchy->toJson();
    } else {
        qWarning() << "buildSaveJson: hierarchy is null, skipping hierarchy save";
    }
    if (tacticalDisplay && tacticalDisplay->canvas)
        obj["tactical"] = tacticalDisplay->canvas->toJson();
    if (qobject_cast<ScenarioEditor*>(editor) || qobject_cast<RuntimeEditor*>(editor)) {
        obj[QStringLiteral("apiGis")] = ShapeApi::instance()->toJson();
    }
    return obj;
}
void MainWindow::saveFileWithTDFSupport(QMainWindow* editor)
{
    pauseSimulationIfRunning();
    RecentProjectsManager::EditorType editorType;
    QString extension;
    QString defaultPrefix;
    QString lastFilePath;

    if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor)) {
        editorType = RecentProjectsManager::DatabaseEditor;
        extension = "db"; defaultPrefix = "Database";
        lastFilePath = dbEditor->lastSavedFilePath;
    } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor)) {
        editorType = RecentProjectsManager::ScenarioEditor;
        extension = "sc"; defaultPrefix = "Scenario";
        lastFilePath = scEditor->lastSavedFilePath;
    } else if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(editor)) {
        editorType = RecentProjectsManager::MissionEditor;
        extension = "ms"; defaultPrefix = "Mission";
        lastFilePath = msEditor->lastSavedFilePath;
    } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor)) {
        editorType = RecentProjectsManager::RuntimeEditor;
        extension = "rn"; defaultPrefix = "Runtime";
        lastFilePath = rtEditor->lastSavedFilePath;
    } else {
        return;
    }
    QString startPath = startPathForEditorType(editorType);
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
    if (fileInfo.suffix().isEmpty())
        filePath += "." + extension;

    if (editorType == RecentProjectsManager::DatabaseEditor) {
        QString dbName = QFileInfo(filePath).completeBaseName();
        QString projectRoot = ensureDatabaseProjectFolder(dbName);
        filePath = projectRoot + "/" + dbName + ".db";
    } else if (editorType == RecentProjectsManager::ScenarioEditor) {
        QString activeDb = resolveActiveDatabasePath();
        QString scenarioRoot = ensureScenarioFolderForDb(activeDb);

        QString chosenBase = QFileInfo(filePath).completeBaseName();
        QString scName = chosenBase;
        scName.remove(QRegularExpression(QStringLiteral("_\\d{4}-\\d{2}-\\d{2}$")));
        if (scName.isEmpty())
            scName = QStringLiteral("Untitled");

        QString scFolder = scenarioRoot + "/" + scName;
        QDir().mkpath(scFolder);

        QString scFile = QFileInfo(filePath).fileName();
        if (scFile.isEmpty() || QFileInfo(scFile).suffix().isEmpty())
            scFile = chosenBase + QLatin1String(".sc");
        if (!scFile.endsWith(QLatin1String(".sc"), Qt::CaseInsensitive))
            scFile += QLatin1String(".sc");

        filePath = scFolder + "/" + scFile;
    } else if (editorType == RecentProjectsManager::RuntimeEditor) {
        QString activeDb = resolveActiveDatabasePath();
        QString scName;
        // 1) Open scenario .sc → stable base name
        if (scenarioEditor && !scenarioEditor->lastSavedFilePath.isEmpty()) {
            scName = QFileInfo(scenarioEditor->lastSavedFilePath).completeBaseName();
            scName.remove(QRegularExpression(QStringLiteral("_\\d{4}-\\d{2}-\\d{2}$")));
        }
        // 2) Already under .../Scenario/<scName>/Runtime/
        if (scName.isEmpty() && runtimeEditor && !runtimeEditor->lastSavedFilePath.isEmpty()) {
            QDir rnDir = QFileInfo(runtimeEditor->lastSavedFilePath).dir();
            if (rnDir.dirName().compare(QLatin1String("Runtime"), Qt::CaseInsensitive) == 0
                && rnDir.cdUp()) {
                QDir probe = rnDir;
                if (probe.cdUp()
                    && probe.dirName().compare(QLatin1String("Scenario"), Qt::CaseInsensitive) == 0) {
                    scName = rnDir.dirName();
                }
            }
        }
        // 3) Chosen / previous file base name (strip date)
        if (scName.isEmpty()) {
            scName = QFileInfo(filePath).completeBaseName();
            scName.remove(QRegularExpression(QStringLiteral("_\\d{4}-\\d{2}-\\d{2}$")));
        }
        if (scName.isEmpty() && runtimeEditor && !runtimeEditor->lastSavedFilePath.isEmpty()) {
            scName = QFileInfo(runtimeEditor->lastSavedFilePath).completeBaseName();
            scName.remove(QRegularExpression(QStringLiteral("_\\d{4}-\\d{2}-\\d{2}$")));
        }
        if (scName.isEmpty())
            scName = QStringLiteral("Untitled");

        QString runtimeFolder = ensureRuntimeFolderForDb(activeDb, scName);
        QString rnFile = QFileInfo(filePath).fileName();
        if (!rnFile.endsWith(QLatin1String(".rn"), Qt::CaseInsensitive))
            rnFile += QLatin1String(".rn");
        filePath = runtimeFolder + "/" + rnFile;
    }

    QJsonObject obj = buildSaveJson(editor);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(obj);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor)) {
            dbEditor->lastSavedFilePath = filePath;
            dbEditor->clearUnsavedChanges();
            onDatabaseFileChanged(filePath);
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
        updateWindowTitleForCurrentEditor();
        if (m_statusBar)
            m_statusBar->setFileName(filePath, false);
    } else {
        qWarning() << "saveFileWithTDFSupport: failed to open file for writing:" << filePath;
    }
}
void MainWindow::saveToSameFileWithTDFSupport(QMainWindow* editor)
{
    pauseSimulationIfRunning();
    RecentProjectsManager::EditorType editorType;
    QString expectedExtension;
    QString filePath;

    if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor)) {
        editorType = RecentProjectsManager::DatabaseEditor;
        expectedExtension = "db";
        filePath = dbEditor->lastSavedFilePath;
    } else if (ScenarioEditor* scEditor = qobject_cast<ScenarioEditor*>(editor)) {
        editorType = RecentProjectsManager::ScenarioEditor;
        expectedExtension = "sc";
        filePath = scEditor->lastSavedFilePath;
    } else if (MissionEditor* msEditor = qobject_cast<MissionEditor*>(editor)) {
        editorType = RecentProjectsManager::MissionEditor;
        expectedExtension = "ms";
        filePath = msEditor->lastSavedFilePath;
    } else if (RuntimeEditor* rtEditor = qobject_cast<RuntimeEditor*>(editor)) {
        editorType = RecentProjectsManager::RuntimeEditor;
        expectedExtension = "rn";
        filePath = rtEditor->lastSavedFilePath;
    } else {
        return;
    }
    if (filePath.isEmpty()) {
        saveFileWithTDFSupport(editor);
        return;
    }
    QFileInfo fileInfo(filePath);
    const QString tdfRoot = tdfRootPath();
    const bool underTdf = filePath.startsWith(tdfRoot);
    if (!underTdf || fileInfo.suffix().toLower() != expectedExtension) {
        saveFileWithTDFSupport(editor);
        return;
    }

    QJsonObject obj = buildSaveJson(editor);

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QJsonDocument doc(obj);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        if (DatabaseEditor* dbEditor = qobject_cast<DatabaseEditor*>(editor)) {
            dbEditor->clearUnsavedChanges();
            QString dbName = databaseProjectNameFromPath(filePath);
            if (!dbName.isEmpty())
                ensureDatabaseProjectFolder(dbName);
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
        updateWindowTitleForCurrentEditor();
        if (m_statusBar)
            m_statusBar->setFileName(filePath, false);
    } else {
        qWarning() << "saveToSameFileWithTDFSupport: failed to open file for writing:" << filePath;
    }
}
void MainWindow::createScenarioInstanceCopy(const QString& runtimeFilePath, const QJsonObject& data)
{
    QFileInfo runtimeFileInfo(runtimeFilePath);
    if (runtimeFileInfo.suffix().toLower() != "rn") return;

    QString activeDb = resolveActiveDatabasePath();
    QString scenarioRoot = ensureScenarioFolderForDb(activeDb);

    QString scName;
    QDir rnDir = runtimeFileInfo.dir();
    if (rnDir.dirName().compare(QLatin1String("Runtime"), Qt::CaseInsensitive) != 0)
        scName = rnDir.dirName();
    if (scName.isEmpty() && scenarioEditor && !scenarioEditor->lastSavedFilePath.isEmpty())
        scName = QFileInfo(scenarioEditor->lastSavedFilePath).completeBaseName();
    if (scName.isEmpty())
        scName = runtimeFileInfo.completeBaseName();

    QString instanceFolderPath = scenarioRoot + "/" + scName + "/" + kScenarioInstanceFolder;
    QDir().mkpath(instanceFolderPath);

    QString runtimeFileName = runtimeFileInfo.completeBaseName();
    QString scenarioFileName;
    if (runtimeFileName.startsWith(kScenarioPrefix)) {
        scenarioFileName = kInstancePrefix + runtimeFileName + ".sc";
    } else if (runtimeFileName.startsWith(kRuntimePrefix)) {
        scenarioFileName = kInstancePrefix + runtimeFileName.replace(kRuntimePrefix, kScenarioPrefix) + ".sc";
    } else {
        scenarioFileName = kInstancePrefix + kScenarioPrefix + runtimeFileName + ".sc";
    }
    QString scenarioFilePath = instanceFolderPath + "/" + scenarioFileName;
    QFile scenarioFile(scenarioFilePath);
    if (scenarioFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(data);
        scenarioFile.write(doc.toJson(QJsonDocument::Indented));
        scenarioFile.close();
    } else {
        qWarning() << "createScenarioInstanceCopy: failed to write instance copy at" << scenarioFilePath;
    }
}
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

void MainWindow::getEditorUnsavedState(QMainWindow* editor, bool& hasUnsaved,
                                       QString& editorName, QString& lastSavedFilePath) const
{
    hasUnsaved = false;
    editorName.clear();
    lastSavedFilePath.clear();
    if (editor == databaseEditor) {
        hasUnsaved = databaseEditor->hasUnsavedChanges;
        editorName = "Database Editor";
        lastSavedFilePath = databaseEditor->lastSavedFilePath;
    } else if (scenarioEditor && editor == scenarioEditor) {
        hasUnsaved = scenarioEditor->hasUnsavedChanges;
        editorName = "Scenario Editor";
        lastSavedFilePath = scenarioEditor->lastSavedFilePath;
    } else if (missionEditor && editor == missionEditor) {
        hasUnsaved  = missionEditor->hasUnsavedChanges;
        editorName  = "Mission Editor";
        lastSavedFilePath = missionEditor->lastSavedFilePath;
    } else if (runtimeEditor && editor == runtimeEditor) {
        hasUnsaved = runtimeEditor->hasUnsavedChanges;
        editorName = "Runtime Editor";
        lastSavedFilePath = runtimeEditor->lastSavedFilePath;
    } else if (analysisEditor && editor == analysisEditor) {
        hasUnsaved = analysisEditor->hasUnsavedChanges;
        editorName = "Analysis Editor";
        lastSavedFilePath = analysisEditor->lastSavedFilePath;
    }
}

void MainWindow::clearUnsavedForEditor(QMainWindow* editor)
{
    if (editor == databaseEditor) databaseEditor->clearUnsavedChanges();
    else if (scenarioEditor && editor == scenarioEditor) scenarioEditor->clearUnsavedChanges();
    else if (missionEditor && editor == missionEditor) missionEditor->clearUnsavedChanges();
    else if (runtimeEditor && editor == runtimeEditor) runtimeEditor->clearUnsavedChanges();
    else if (analysisEditor && editor == analysisEditor) analysisEditor->clearUnsavedChanges();
}

bool MainWindow::showUnsavedChangesPrompt(QMainWindow* currentEditor,
                                          const QString& editorName,
                                          const QString& lastSavedFilePath)
{
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

        bool stillHasUnsaved = false;
        QString unusedName, unusedPath;
        getEditorUnsavedState(currentEditor, stillHasUnsaved, unusedName, unusedPath);
        if (stillHasUnsaved) {
            QMessageBox::warning(this, "Save Failed",
                                 "Failed to save changes. Please try again or choose Discard.");
            return false;
        }
        updateWindowTitleForCurrentEditor();
        return true;
    } else if (reply == QMessageBox::Discard) {
        clearUnsavedForEditor(currentEditor);
        updateWindowTitleForCurrentEditor();
        return true;
    }
    return false;
}

bool MainWindow::handleUnsavedChanges()
{
    QMainWindow *currentEditor = getCurrentEditor();
    if (!currentEditor) return true;

    bool hasUnsavedChanges = false;
    QString editorName, lastSavedFilePath;
    getEditorUnsavedState(currentEditor, hasUnsavedChanges, editorName, lastSavedFilePath);

    if (!hasUnsavedChanges) return true;
    return showUnsavedChangesPrompt(currentEditor, editorName, lastSavedFilePath);
}

MainWindow* MainWindow::instance()
{
    return MainWindow::s_instance;
}

// ---------------------------------------------------------------------------
// switchEditor()
// ---------------------------------------------------------------------------
void MainWindow::switchEditor(const QString &editorKeyStr)
{
    if (m_loadingOverlay && m_loadingOverlay->isVisible()) {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(MainWindowStyles::MessageBox);
        msgBox.setWindowTitle("Please Wait");
        msgBox.setText("Data is still loading. Please wait until loading is complete.");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        navigationPage->restorePreviousButton();
        return;
    }
    if (!handleUnsavedChanges()) {
        navigationPage->restorePreviousButton();
        return;
    }

    EditorKey key = editorKeyFromString(editorKeyStr);

    QWidget* leavingWidget = stackedWidget->currentWidget();
    captureGisSnapshotFor(leavingWidget);

    if (key == EditorKey::Database) {
        showLoadingOverlay("Loading Database Editor...");
        mainMenuBar->updateFileMenuForEditor(editorKeyStr);
        mainMenuBar->setLibraryActionsVisible(false);
        stackedWidget->setCurrentWidget(databaseEditor);
        QCoreApplication::processEvents();
        updateWindowTitleForCurrentEditor();
        if (m_statusBar)
            m_statusBar->setFileName(databaseEditor->lastSavedFilePath,
                                     databaseEditor->hasUnsavedChanges);
        emit databaseEditor->Activated();
        hideLoadingOverlay();
    }
    else if (key == EditorKey::Scenario) {
        showLoadingOverlay("Loading Scenario Editor...");
        mainMenuBar->updateFileMenuForEditor(editorKeyStr);
        mainMenuBar->setLibraryActionsVisible(true);

        if (!scenarioEditor) {
            m_loadingLabel->setText("Creating Scenario Editor...");
            QCoreApplication::processEvents();
            scenarioEditor = new ScenarioEditor(centralWidget());
            stackedWidget->addWidget(scenarioEditor);
            connect(scenarioEditor, &ScenarioEditor::unsavedChangesChanged,
                    this, &MainWindow::onUnsavedChangesChanged);
            connect(scenarioEditor, &ScenarioEditor::scenarioFileLoaded,
                    this, &MainWindow::onScenarioFileLoaded);
        }

        m_loadingLabel->setText("Switching to Scenario Editor...");
        QCoreApplication::processEvents();
        stackedWidget->setCurrentWidget(scenarioEditor);
        restoreGisSnapshotFor(scenarioEditor);   // ARTI CHANGE: bring back this editor's own shapes
        updateWindowTitleForCurrentEditor();
        if (m_statusBar)
            m_statusBar->setFileName(scenarioEditor->lastSavedFilePath,
                                     scenarioEditor->hasUnsavedChanges);

        syncDatabaseEntityChanges();
        emit scenarioEditor->Activated();
        hideLoadingOverlay();
        QString dbPath = resolveLibraryDbPath();
        QTimer::singleShot(50, this, [this, dbPath]() {
            loadLibraryForEditor(getDatabaseHierarchy(),
                                 scenarioEditor->libTreeView,
                                 dbPath);
            if (!dbPath.isEmpty() && scenarioEditor->lastSavedFilePath.isEmpty()) {
                scenarioEditor->setLoadedDatabaseFile(dbPath);
            }
        });
    }
    else if (key == EditorKey::Mission) {
        showLoadingOverlay("Loading Mission Editor...");
        mainMenuBar->updateFileMenuForEditor(editorKeyStr);
        mainMenuBar->setLibraryActionsVisible(false);
        bool justCreated = false;
        if (!missionEditor) {
            m_loadingLabel->setText("Creating Mission Editor...");
            QCoreApplication::processEvents();
            missionEditor = new MissionEditor(centralWidget());
            stackedWidget->addWidget(missionEditor);
            connect(missionEditor, &MissionEditor::unsavedChangesChanged,
                    this, &MainWindow::onUnsavedChangesChanged);
            justCreated = true;
        }

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

        if (justCreated && !missionEditor->lastSavedFilePath.isEmpty()) {
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

    else if (key == EditorKey::Runtime) {
        showLoadingOverlay("Loading Runtime Editor...");
        mainMenuBar->updateFileMenuForEditor(editorKeyStr);
        mainMenuBar->setLibraryActionsVisible(true);

        bool justCreated = false;
        if (!runtimeEditor) {
            m_loadingLabel->setText("Creating Runtime Editor...");
            QCoreApplication::processEvents();
            runtimeEditor = new RuntimeEditor(centralWidget());
            stackedWidget->addWidget(runtimeEditor);
            connect(runtimeEditor, &RuntimeEditor::pluginMenuActionRequested,
                    this, [this](const QString& menuName, QAction* action, const QString& pluginId) {
                        if (!mainMenuBar || !action) return;
                        QMenu* menu = nullptr;
                        QString target = menuName;
                        target.remove('&');
                        for (QAction* act : mainMenuBar->actions()) {
                            if (!act->menu()) continue;
                            QString existing = act->text();
                            existing.remove('&');
                            if (existing.compare(target, Qt::CaseInsensitive) == 0) {
                                menu = act->menu();
                                break;
                            }
                        }

                        if (!menu) {
                            menu = mainMenuBar->addMenu(menuName);
                            qDebug() << "[MainWindow] Created new menu:" << menuName;
                        }

                        menu->addAction(action);
                        qDebug() << "[MainWindow] Menu action added:" << menuName << "→" << action->text();
                    });

            connect(runtimeEditor, &RuntimeEditor::unsavedChangesChanged,
                    this, &MainWindow::onUnsavedChangesChanged);
            connect(runtimeEditor, &RuntimeEditor::runtimeFileLoaded,
                    this, &MainWindow::onRuntimeFileLoaded);
            for (const PluginInfo &p : PluginManagerDialog::loadSavedPlugins()) {
                if (p.status == "installed" && !p.filePath.isEmpty()) {
                    runtimeEditor->pluginManager->loadPlugin(p.filePath.toStdString());
                }
            }
            justCreated = true;
        }

        QWidget* prevWidget = stackedWidget->currentWidget();
        bool comingFromAnalysis = (analysisEditor && prevWidget == analysisEditor);
        bool runtimeIsEmpty = justCreated || (!runtimeEditor->hasUnsavedChanges &&
                                              runtimeEditor->lastSavedFilePath.isEmpty());

        bool scenarioUpdatedSincePush = false;
        if (scenarioEditor && !scenarioEditor->lastSavedFilePath.isEmpty()) {
            const QFileInfo scInfo(scenarioEditor->lastSavedFilePath);
            const QString scAbsPath = scInfo.absoluteFilePath();
            const QDateTime scMTime = scInfo.lastModified();
            scenarioUpdatedSincePush =
                (scAbsPath != m_scenarioPathPushedToRuntime) ||
                (m_scenarioMTimePushedToRuntime.isNull() ||
                 scMTime > m_scenarioMTimePushedToRuntime);
        }

        bool pulledScenarioIntoRuntime = false;   // ARTI CHANGE
        if (!comingFromAnalysis && !runtimeEditor->hasUnsavedChanges &&
            (runtimeIsEmpty || scenarioUpdatedSincePush) &&
            scenarioEditor && !scenarioEditor->lastSavedFilePath.isEmpty()) {
            m_loadingLabel->setText("Loading Scenario Data...");
            QCoreApplication::processEvents();
            runtimeEditor->loadFromJsonFile(scenarioEditor->lastSavedFilePath);
            pulledScenarioIntoRuntime = true;

            const QFileInfo pushedInfo(scenarioEditor->lastSavedFilePath);
            m_scenarioPathPushedToRuntime = pushedInfo.absoluteFilePath();
            m_scenarioMTimePushedToRuntime = pushedInfo.lastModified();
        }

        m_loadingLabel->setText("Switching to Runtime Editor...");
        QCoreApplication::processEvents();
        stackedWidget->setCurrentWidget(runtimeEditor);
        if (!pulledScenarioIntoRuntime)
            restoreGisSnapshotFor(runtimeEditor);
        updateWindowTitleForCurrentEditor();
        if (m_statusBar)
            m_statusBar->setFileName(runtimeEditor->lastSavedFilePath,
                                     runtimeEditor->hasUnsavedChanges);

        syncDatabaseEntityChanges();
        emit runtimeEditor->Activated();
        hideLoadingOverlay();
        QString dbPath = resolveLibraryDbPath();
        QTimer::singleShot(50, this, [this, dbPath]() {
            loadLibraryForEditor(runtimeEditor->library,
                                 runtimeEditor->libTreeView,
                                 dbPath);
            if (!dbPath.isEmpty() && runtimeEditor->lastSavedFilePath.isEmpty()) {
                runtimeEditor->setLoadedDatabaseFile(dbPath);
            }
        });
    }
    else if (key == EditorKey::Analysis) {
        showLoadingOverlay("Loading Analysis Editor...");
        mainMenuBar->updateFileMenuForEditor(editorKeyStr);
        mainMenuBar->setLibraryActionsVisible(false);

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
            QJsonObject analysisJson = runtimeEditor->hierarchy->loadAnalysisJson();
            if (!analysisJson.isEmpty())
                analysisEditor->loadFromHierarchyJson(analysisJson);
        } else if (scenarioEditor && scenarioEditor->hierarchy) {
            QJsonObject analysisJson = scenarioEditor->hierarchy->loadAnalysisJson();
            if (!analysisJson.isEmpty())
                analysisEditor->loadFromHierarchyJson(analysisJson);
        } else if (databaseEditor && databaseEditor->hierarchy) {
            QJsonObject analysisJson = databaseEditor->hierarchy->loadAnalysisJson();
            if (!analysisJson.isEmpty())
                analysisEditor->loadFromHierarchyJson(analysisJson);
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
}QString MainWindow::buildTitleSuffixForEditor(QMainWindow* currentWidget, const QString& editorName) const
{
    QString suffix;
    if (databaseEditor && currentWidget == databaseEditor) {
        suffix = !databaseEditor->lastSavedFilePath.isEmpty()
                     ? QFileInfo(databaseEditor->lastSavedFilePath).fileName()
                     : editorName;
    } else if (scenarioEditor && currentWidget == scenarioEditor) {
        suffix = !scenarioEditor->lastSavedFilePath.isEmpty()
                     ? QFileInfo(scenarioEditor->lastSavedFilePath).fileName()
                     : editorName;
        if (!scenarioEditor->m_loadedDatabaseFile.isEmpty())
            suffix += " | DB: " + QFileInfo(scenarioEditor->m_loadedDatabaseFile).fileName();
    } else if (runtimeEditor && currentWidget == runtimeEditor) {
        suffix = !runtimeEditor->lastSavedFilePath.isEmpty()
                     ? QFileInfo(runtimeEditor->lastSavedFilePath).fileName()
                     : editorName;
        if (!runtimeEditor->m_loadedDatabaseFile.isEmpty())
            suffix += " | DB: " + QFileInfo(runtimeEditor->m_loadedDatabaseFile).fileName();
    } else {
        suffix = editorName;
    }
    return suffix;
}

void MainWindow::updateWindowTitleForCurrentEditor()
{
    QMainWindow *currentEditor = getCurrentEditor();
    bool hasUnsaved = false;
    QString editorName, filePath;
    getEditorUnsavedState(currentEditor, hasUnsaved, editorName, filePath);

    updateWindowTitle(editorName, hasUnsaved);
    if (m_statusBar) m_statusBar->setFileName(filePath, hasUnsaved);
}

void MainWindow::updateWindowTitle(const QString& editorName, bool hasUnsavedChanges)
{
    QString title = "Indigenous Scenario and Sensor Simulation Toolkit";
    if (scenarioconfig && !scenarioconfig->software_version.isEmpty())
        title += QString(" (V_%1)").arg(scenarioconfig->software_version);

    QWidget* currentWidget = stackedWidget->currentWidget();
    title += " - " + buildTitleSuffixForEditor(qobject_cast<QMainWindow*>(currentWidget), editorName);

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
    else if (anEditor) { hasUnsaved = anEditor->hasUnsavedChanges; filePath = anEditor->lastSavedFilePath; }
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
        if (!saveAction) return false;

        saveAction->trigger();
        QCoreApplication::processEvents();
        if      (dbEditor) return !dbEditor->hasUnsavedChanges;
        else if (scEditor) return !scEditor->hasUnsavedChanges;
        else if (msEditor) return !msEditor->hasUnsavedChanges;
        else if (rtEditor) return !rtEditor->hasUnsavedChanges;
        else if (anEditor) return !anEditor->hasUnsavedChanges;
        return false;
    } else if (reply == QMessageBox::Discard) {
        if      (dbEditor) dbEditor->clearUnsavedChanges();
        else if (scEditor) scEditor->clearUnsavedChanges();
        else if (msEditor) msEditor->clearUnsavedChanges();
        else if (rtEditor) rtEditor->clearUnsavedChanges();
        else if (anEditor) anEditor->clearUnsavedChanges();
        return true;
    }
    return false;
}

void MainWindow::showLoadingOverlay(const QString& message)
{
    if (navigationPage) navigationPage->setEnabled(false);

    if (!m_loadingOverlay) {
        m_loadingOverlay = new QWidget(this);
        m_loadingOverlay->setObjectName("loadingOverlay");
        m_loadingOverlay->setStyleSheet(
            "QWidget#loadingOverlay {"
            "  background-color: rgba(15, 38, 54, 0.85);"
            "}"
            );

        m_loadingCard = new QWidget(m_loadingOverlay);
        m_loadingCard->setFixedSize(280, 110);
        m_loadingCard->setStyleSheet(
            "QWidget {"
            "  background-color: #1A3A4F;"
            "  border-radius: 10px;"
            "  border: 1px solid #00BFFF;"
            "}"
            );
        QVBoxLayout* cardLayout = new QVBoxLayout(m_loadingCard);
        cardLayout->setContentsMargins(20, 18, 20, 18);
        cardLayout->setSpacing(12);

        m_loadingLabel = new QLabel(message, m_loadingCard);
        m_loadingLabel->setAlignment(Qt::AlignCenter);
        m_loadingLabel->setStyleSheet(
            "QLabel { color: white; font-size: 14px; font-weight: bold; background: transparent; border: none; }"
            );
        m_loadingBar = new QProgressBar(m_loadingCard);
        m_loadingBar->setRange(0, 0);
        m_loadingBar->setFixedHeight(6);
        m_loadingBar->setTextVisible(false);
        m_loadingBar->setStyleSheet(
            "QProgressBar { background-color: #0F2636; border-radius: 3px; border: none; }"
            "QProgressBar::chunk { background-color: #00BFFF; border-radius: 3px; }"
            );
        cardLayout->addWidget(m_loadingLabel);
        cardLayout->addWidget(m_loadingBar);

        QVBoxLayout* overlayLayout = new QVBoxLayout(m_loadingOverlay);
        overlayLayout->addStretch();
        QHBoxLayout* hCenter = new QHBoxLayout();
        hCenter->addStretch();
        hCenter->addWidget(m_loadingCard);
        hCenter->addStretch();
        overlayLayout->addLayout(hCenter);
        overlayLayout->addStretch();
    } else {
        if (m_loadingLabel) m_loadingLabel->setText(message);
    }

    m_loadingOverlay->setGeometry(0, 0, width(), height());
    m_loadingOverlay->raise();
    m_loadingOverlay->show();
    QCoreApplication::processEvents();
}

void MainWindow::hideLoadingOverlay()
{
    if (m_loadingOverlay) m_loadingOverlay->hide();
    if (navigationPage) navigationPage->setEnabled(true);
}

void MainWindow::pauseSimulationIfRunning()
{
    m_wasSimulationRunning = false;
    if (!runtimeEditor || stackedWidget->currentWidget() != runtimeEditor)
        return;
    if (!runtimeEditor->simulation || !runtimeEditor->runtimeToolBar)
        return;
    if (runtimeEditor->runtimeToolBar->getSimulationState() == RuntimeToolBar::RUNNING) {
        m_wasSimulationRunning = true;
        runtimeEditor->runtimeToolBar->setSimulationState(RuntimeToolBar::PAUSED);
        runtimeEditor->simulation->pause();
        QCoreApplication::processEvents();
    }
}

QString MainWindow::resolveLibraryDbPath() const
{
    if (databaseEditor && !databaseEditor->lastSavedFilePath.isEmpty())
        return databaseEditor->lastSavedFilePath;
    if (ApplicationDialog::getGlobalDatabaseEnabled()) {
        QString p = ApplicationDialog::getGlobalDatabasePath();
        if (!p.isEmpty() && p != "No path set")
            return p;
    }
    return TDFManager::instance()->getAircraftDbPath();
}
void MainWindow::loadLibraryForEditor(Hierarchy* library,
                                      HierarchyTree* libTreeView,
                                      const QString& dbPath)
{
    if (!library) return;
    const bool isSharedDb = (databaseEditor && library == databaseEditor->hierarchy);
    if (isSharedDb) {
        if (libTreeView) {
            library->disconnect(libTreeView);
            libTreeView->getTreeWidget()->clear();
            if (!libTreeView->Items.isEmpty())
                libTreeView->Items.clear();
            reconnectLibrarySignals(library, libTreeView);
            for (const auto& [profileId, profile] : library->ProfileCategories) {
                if (!profile) continue;
                QString pid = QString::fromStdString(profileId);
                QString pname = QString::fromStdString(profile->Name);
                libTreeView->profileAdded(pid, pname);
                for (const auto& [entityId, entity] : profile->Entities) {
                    if (!entity) continue;
                    libTreeView->entityAdded(pid,
                                             QString::fromStdString(entityId),
                                             QString::fromStdString(entity->Name));
                }
                for (const auto& [folderId, folder] : profile->Folders) {
                    if (!folder) continue;
                    QString fid = QString::fromStdString(folderId);
                    libTreeView->folderAdded(pid, fid, QString::fromStdString(folder->Name));
                    for (const auto& [entityId, entity] : folder->Entities) {
                        if (!entity) continue;
                        libTreeView->entityAdded(fid,
                                                 QString::fromStdString(entityId),
                                                 QString::fromStdString(entity->Name));
                    }
                }
            }
            QString displayPath = dbPath;
            if (displayPath.isEmpty() && databaseEditor)
                displayPath = databaseEditor->lastSavedFilePath;
            libTreeView->setLibraryFileName(displayPath);
            libTreeView->getTreeWidget()->setEnabled(true);
            libTreeView->getTreeWidget()->setUpdatesEnabled(true);
            libTreeView->getTreeWidget()->update();
            libTreeView->getTreeWidget()->collapseAll();
        }
        if (scenarioEditor && stackedWidget->currentWidget() == scenarioEditor
            && scenarioEditor->lastSavedFilePath.isEmpty()) {
            scenarioEditor->setLoadedDatabaseFile(
                dbPath.isEmpty() && databaseEditor ? databaseEditor->lastSavedFilePath : dbPath);
        }
        if (runtimeEditor && stackedWidget->currentWidget() == runtimeEditor
            && runtimeEditor->lastSavedFilePath.isEmpty()) {
            runtimeEditor->setLoadedDatabaseFile(
                dbPath.isEmpty() && databaseEditor ? databaseEditor->lastSavedFilePath : dbPath);
        }
        return;
    }

    if (dbPath.isEmpty()) {
        library->fromJson(QJsonObject());
        if (libTreeView) {
            libTreeView->setLibraryFileName("");
            libTreeView->getTreeWidget()->setEnabled(true);
        }
        return;
    }
    if (navigationPage) navigationPage->setEnabled(false);
    if (libTreeView) {
        libTreeView->getTreeWidget()->setEnabled(false);
        libTreeView->getTreeWidget()->setUpdatesEnabled(false);
        library->disconnect(libTreeView);
    }
    QFuture<QJsonObject> future = QtConcurrent::run([dbPath]() -> QJsonObject {
        QFile f(dbPath);
        if (!f.open(QIODevice::ReadOnly)) return {};
        QByteArray data = f.readAll();
        f.close();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) return {};
        QJsonObject obj = doc.object();
        return obj.contains("hierarchy") ? obj["hierarchy"].toObject() : QJsonObject{};
    });
    auto* watcher = new QFutureWatcher<QJsonObject>(this);
    QPointer<MainWindow> safeThis(this);
    QPointer<Hierarchy> safeLibrary(library);
    QPointer<HierarchyTree> safeLibTreeView(libTreeView);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this,
            [safeThis, watcher, dbPath, safeLibrary, safeLibTreeView]() {
                watcher->deleteLater();
                if (!safeThis || !safeLibrary) return;
                QJsonObject hierJson = watcher->result();
                if (hierJson.isEmpty()) {
                    safeThis->reconnectLibrarySignals(safeLibrary, safeLibTreeView);
                    if (safeLibTreeView) {
                        safeLibTreeView->getTreeWidget()->setEnabled(true);
                        safeLibTreeView->getTreeWidget()->setUpdatesEnabled(true);
                    }
                    if (safeThis->navigationPage) safeThis->navigationPage->setEnabled(true);
                    return;
                }
                if (safeLibTreeView) {
                    safeLibTreeView->getTreeWidget()->clear();
                    if (!safeLibTreeView->Items.isEmpty())
                        safeLibTreeView->Items.clear();
                }
                safeLibrary->clear();
                safeLibrary->fromJson(hierJson);
                safeThis->reconnectLibrarySignals(safeLibrary, safeLibTreeView);
                if (safeLibTreeView) {
                    safeLibTreeView->getTreeWidget()->setUpdatesEnabled(true);
                    safeLibTreeView->getTreeWidget()->setEnabled(true);
                    safeLibTreeView->setLibraryFileName(dbPath);
                    safeLibTreeView->getTreeWidget()->update();
                    safeLibTreeView->getTreeWidget()->collapseAll();
                }
                QWidget* currentWidget = safeThis->stackedWidget->currentWidget();
                if (safeThis->scenarioEditor && currentWidget == safeThis->scenarioEditor
                    && safeThis->scenarioEditor->lastSavedFilePath.isEmpty()) {
                    safeThis->scenarioEditor->setLoadedDatabaseFile(dbPath);
                } else if (safeThis->runtimeEditor && currentWidget == safeThis->runtimeEditor
                           && safeThis->runtimeEditor->lastSavedFilePath.isEmpty()) {
                    safeThis->runtimeEditor->setLoadedDatabaseFile(dbPath);
                }
                if (safeThis->navigationPage) safeThis->navigationPage->setEnabled(true);
            });

    watcher->setFuture(future);
}
void MainWindow::reconnectLibrarySignals(Hierarchy* library,
                                         HierarchyTree* libTreeView)
{
    if (!library || !libTreeView) return;
    QObject::connect(library, &Hierarchy::profileAdded,
                     libTreeView, &HierarchyTree::profileAdded,
                     Qt::UniqueConnection);
    QObject::connect(library, &Hierarchy::folderAdded,
                     libTreeView, &HierarchyTree::folderAdded,
                     Qt::UniqueConnection);
    QObject::connect(library, &Hierarchy::entityAdded,
                     libTreeView,
                     [libTreeView](QString parentID, QString ID, QString name) {
                         libTreeView->entityAdded(parentID, ID, name);
                     });
    QObject::connect(library, &Hierarchy::componentAdded,
                     libTreeView, &HierarchyTree::componentAdded,
                     Qt::UniqueConnection);
    QObject::connect(library, &Hierarchy::subComponentAdded,
                     libTreeView, &HierarchyTree::subComponentAdded,
                     Qt::UniqueConnection);
    QObject::connect(library, &Hierarchy::profileRemoved,
                     libTreeView, &HierarchyTree::profileRemoved,
                     Qt::UniqueConnection);
    QObject::connect(library, &Hierarchy::folderRemoved,
                     libTreeView, &HierarchyTree::folderRemoved,
                     Qt::UniqueConnection);
    QObject::connect(library, &Hierarchy::entityRemoved,
                     libTreeView, &HierarchyTree::entityRemoved,
                     Qt::UniqueConnection);
}

void MainWindow::syncDatabaseEntityChanges()
{
    if (!databaseEditor || !databaseEditor->hierarchy) {
        return;
    }


    if (databaseEditor->hasModifiedEntities()) {
        for (const QString& id : databaseEditor->modifiedEntityIds()) {
            m_pendingEntitySyncToScenario.insert(id);
            m_pendingEntitySyncToRuntime.insert(id);
        }
        databaseEditor->clearModifiedEntityIds();
    }

    Hierarchy* library = databaseEditor->hierarchy;


    if (scenarioEditor && scenarioEditor->hierarchy && !m_pendingEntitySyncToScenario.isEmpty()) {
        for (const QString& id : std::as_const(m_pendingEntitySyncToScenario)) {
            auto libIt = library->Platforms.find(id.toStdString());
            if (libIt == library->Platforms.end() || !libIt->second) continue;
            QJsonObject freshData = libIt->second->toJson();
            if (freshData.isEmpty()) continue;
            for (const auto& pair : scenarioEditor->hierarchy->Platforms) {
                if (pair.second && pair.second->ReferenceID == libIt->second->ID) {
                    pair.second->SyncByJson(freshData);
                }
            }
        }
        m_pendingEntitySyncToScenario.clear();
    }

    if (runtimeEditor && runtimeEditor->hierarchy && !m_pendingEntitySyncToRuntime.isEmpty()) {
        for (const QString& id : std::as_const(m_pendingEntitySyncToRuntime)) {
            auto libIt = library->Platforms.find(id.toStdString());
            if (libIt == library->Platforms.end() || !libIt->second) continue;
            QJsonObject freshData = libIt->second->toJson();
            if (freshData.isEmpty()) continue;
            for (const auto& pair : runtimeEditor->hierarchy->Platforms) {
                if (pair.second && pair.second->ReferenceID == libIt->second->ID) {
                    pair.second->SyncByJson(freshData);
                }
            }
        }
        m_pendingEntitySyncToRuntime.clear();
    }
}
QString MainWindow::tdfRootPath() const
{
    return QDir::homePath() + "/TDF";
}

QString MainWindow::ensureTDFSubfolder(const QString& subfolderName) const
{
    QString tdfPath = tdfRootPath();
    QString targetPath = tdfPath + "/" + subfolderName;
    QDir dir;
    if (!dir.exists(tdfPath))    dir.mkpath(tdfPath);
    if (!dir.exists(targetPath)) dir.mkpath(targetPath);
    return targetPath;
}

QString MainWindow::databaseProjectNameFromPath(const QString& dbFilePath) const
{
    if (dbFilePath.isEmpty())
        return QString();

    QFileInfo fi(dbFilePath);
    QString parentName = fi.dir().dirName();
    QString baseName   = fi.completeBaseName();

    if (parentName == "Database" || parentName == "TDF" || parentName == "Data"
        || parentName == "Scenario" || parentName == "Runtime") {
        return baseName;
    }
    if (!parentName.isEmpty() && QDir(fi.dir().absolutePath()).exists()) {
        if (QFileInfo(fi.dir().absoluteFilePath(baseName + ".db")).exists()
            || parentName.compare(baseName, Qt::CaseInsensitive) == 0) {
            return parentName;
        }
    }
    return baseName;
}

QString MainWindow::ensureDatabaseProjectFolder(const QString& dbName) const
{
    if (dbName.isEmpty())
        return QString();

    QString databaseRoot = tdfRootPath() + "/Database";
    QString projectRoot  = databaseRoot + "/" + dbName;
    QDir dir;
    if (!dir.exists(tdfRootPath())) dir.mkpath(tdfRootPath());
    if (!dir.exists(databaseRoot))  dir.mkpath(databaseRoot);
    if (!dir.exists(projectRoot))   dir.mkpath(projectRoot);
    dir.mkpath(projectRoot + "/Scenario");
    return projectRoot;
}

QString MainWindow::ensureScenarioFolderForDb(const QString& dbFilePath) const
{
    QString dbName = databaseProjectNameFromPath(dbFilePath);
    if (dbName.isEmpty())
        return ensureTDFSubfolder("Scenario");
    return ensureDatabaseProjectFolder(dbName) + "/Scenario";
}

QString MainWindow::ensureRuntimeFolderForDb(const QString& dbFilePath, const QString& scenarioName) const
{
    QString scenarioRoot = ensureScenarioFolderForDb(dbFilePath);
    QString scName = scenarioName.isEmpty() ? QStringLiteral("Untitled") : scenarioName;
    QString scenarioFolder = scenarioRoot + "/" + scName;
    QString runtimeFolder  = scenarioFolder + "/Runtime";
    QDir dir;
    if (!dir.exists(scenarioFolder)) dir.mkpath(scenarioFolder);
    if (!dir.exists(runtimeFolder))  dir.mkpath(runtimeFolder);
    return runtimeFolder;
}
QString MainWindow::resolveActiveDatabasePath() const
{
    if (scenarioEditor && !scenarioEditor->m_loadedDatabaseFile.isEmpty())
        return scenarioEditor->m_loadedDatabaseFile;
    if (runtimeEditor && !runtimeEditor->m_loadedDatabaseFile.isEmpty())
        return runtimeEditor->m_loadedDatabaseFile;
    if (databaseEditor && !databaseEditor->lastSavedFilePath.isEmpty())
        return databaseEditor->lastSavedFilePath;
    return resolveLibraryDbPath();
}
QString MainWindow::startPathForEditorType(RecentProjectsManager::EditorType editorType) const
{
    QString activeDb = resolveActiveDatabasePath();
    switch (editorType) {
    case RecentProjectsManager::DatabaseEditor: {
        if (!activeDb.isEmpty()) {
            QString name = databaseProjectNameFromPath(activeDb);
            if (!name.isEmpty()) {
                QString project = tdfRootPath() + "/Database/" + name;
                if (QDir(project).exists()) return project;
            }
        }
        QString dbRoot = tdfRootPath() + "/Database";
        QDir().mkpath(dbRoot);
        return dbRoot;
    }
    case RecentProjectsManager::ScenarioEditor:
        return ensureScenarioFolderForDb(activeDb);
    case RecentProjectsManager::RuntimeEditor: {
        QString scName;
        if (scenarioEditor && !scenarioEditor->lastSavedFilePath.isEmpty()) {
            scName = QFileInfo(scenarioEditor->lastSavedFilePath).completeBaseName();
            scName.remove(QRegularExpression(QStringLiteral("_\\d{4}-\\d{2}-\\d{2}$")));
        } else if (runtimeEditor && !runtimeEditor->lastSavedFilePath.isEmpty()) {
            QDir rnDir = QFileInfo(runtimeEditor->lastSavedFilePath).dir();
            if (rnDir.dirName().compare(QLatin1String("Runtime"), Qt::CaseInsensitive) == 0
                && rnDir.cdUp()) {
                QDir probe = rnDir;
                if (probe.cdUp()
                    && probe.dirName().compare(QLatin1String("Scenario"), Qt::CaseInsensitive) == 0) {
                    scName = rnDir.dirName();
                }
            }
            if (scName.isEmpty()) {
                scName = QFileInfo(runtimeEditor->lastSavedFilePath).completeBaseName();
                scName.remove(QRegularExpression(QStringLiteral("_\\d{4}-\\d{2}-\\d{2}$")));
            }
        }
        if (scName.isEmpty())
            scName = QStringLiteral("Untitled");
        return ensureRuntimeFolderForDb(activeDb, scName);
    }
    case RecentProjectsManager::MissionEditor:
        return ensureTDFSubfolder("Mission");
    default:
        return tdfRootPath();
    }
}
QString MainWindow::resolveDatabasePathFromProjectFile(const QString& projectFilePath) const
{

    if (projectFilePath.isEmpty())
        return QString();
    QFileInfo fi(projectFilePath);
    QDir dir = fi.dir();

    bool passedRuntimeOrScenario = false;
    for (int i = 0; i < 10; ++i) {
        const QString leaf = dir.dirName();
        if (leaf.compare(QLatin1String("Runtime"), Qt::CaseInsensitive) == 0
            || leaf.compare(QLatin1String("Scenario"), Qt::CaseInsensitive) == 0
            || leaf.compare(QLatin1String("ScenarioInstance"), Qt::CaseInsensitive) == 0) {
            passedRuntimeOrScenario = true;
            if (!dir.cdUp())
                return QString();
            continue;
        }
        if (passedRuntimeOrScenario) {

            QDir probe = dir;
            if (probe.cdUp()
                && probe.dirName().compare(QLatin1String("Scenario"), Qt::CaseInsensitive) == 0) {
                dir = probe;
                if (!dir.cdUp())
                    return QString();
                break;
            }
            break;
        }
        if (!dir.cdUp())
            return QString();
    }

    const QString projectName = dir.dirName();
    const QString preferred = dir.absoluteFilePath(projectName + QLatin1String(".db"));
    if (QFileInfo::exists(preferred))
        return preferred;

    const QStringList dbs = dir.entryList(QStringList() << QLatin1String("*.db"), QDir::Files);
    if (!dbs.isEmpty())
        return dir.absoluteFilePath(dbs.first());
    return QString();
}
QString MainWindow::resolveScenarioPathFromRuntimeFile(const QString& runtimeFilePath) const
{
    if (runtimeFilePath.isEmpty())
        return QString();
    QFileInfo fi(runtimeFilePath);
    QDir runtimeDir = fi.dir();
    if (runtimeDir.dirName().compare(QLatin1String("Runtime"), Qt::CaseInsensitive) != 0)
        return QString();
    if (!runtimeDir.cdUp())
        return QString();

    const QString sameName = runtimeDir.absoluteFilePath(
        fi.completeBaseName() + QLatin1String(".sc"));
    if (QFileInfo::exists(sameName))
        return sameName;

    const QString folderNameSc = runtimeDir.absoluteFilePath(
        runtimeDir.dirName() + QLatin1String(".sc"));
    if (QFileInfo::exists(folderNameSc))
        return folderNameSc;

    const QFileInfoList scFiles = runtimeDir.entryInfoList(
        QStringList() << QLatin1String("*.sc"), QDir::Files, QDir::Time);
    if (!scFiles.isEmpty())
        return scFiles.first().absoluteFilePath();
    return QString();
}

bool MainWindow::isSameDatabaseProject(const QString& projectFilePath, const QString& dbFilePath) const
{
    if (projectFilePath.isEmpty() || dbFilePath.isEmpty())
        return false;
    const QString resolved = resolveDatabasePathFromProjectFile(projectFilePath);
    if (resolved.isEmpty())
        return false;
    return QFileInfo(resolved).absoluteFilePath()
           == QFileInfo(dbFilePath).absoluteFilePath();
}

void MainWindow::unloadScenarioAndRuntimeEditors()
{
    if (scenarioEditor) {
        if (scenarioEditor->hierarchy) {
            scenarioEditor->hierarchy->fromJson(QJsonObject());
            HierarchyConnector::instance()->initializeDummyData(scenarioEditor->hierarchy);
        }
        if (scenarioEditor->tacticalDisplay && scenarioEditor->tacticalDisplay->canvas) {
            scenarioEditor->tacticalDisplay->canvas->fromJson(QJsonObject());
            scenarioEditor->tacticalDisplay->canvas->resetEntityInfoDialog();
            scenarioEditor->tacticalDisplay->canvas->fullCanvasReset();
        }
        if (LayerPanel* layerPanel = scenarioEditor->findChild<LayerPanel*>()) {
            layerPanel->fromJson(QJsonObject());
        }
        ShapeApi::instance()->deleteAllShapes();
        LayerApi::instance()->fromJson(QJsonObject());
        scenarioEditor->lastSavedFilePath = QString();
        scenarioEditor->clearUnsavedChanges();
    }
    if (runtimeEditor) {
        if (runtimeEditor->hierarchy) {
            runtimeEditor->hierarchy->fromJson(QJsonObject());
            HierarchyConnector::instance()->initializeDummyData(runtimeEditor->hierarchy);
        }
        if (runtimeEditor->tacticalDisplay && runtimeEditor->tacticalDisplay->canvas) {
            runtimeEditor->tacticalDisplay->canvas->fromJson(QJsonObject());
            runtimeEditor->tacticalDisplay->canvas->resetEntityInfoDialog();
            runtimeEditor->tacticalDisplay->canvas->fullCanvasReset();
        }
        for (EntityInfoDialog* dlg : runtimeEditor->findChildren<EntityInfoDialog*>()) {
            if (dlg)
                dlg->onHierarchyReset();
        }
        if (LayerPanel* layerPanel = runtimeEditor->findChild<LayerPanel*>()) {
            layerPanel->fromJson(QJsonObject());
        }
        runtimeEditor->lastSavedFilePath = QString();
        runtimeEditor->clearUnsavedChanges();
    }
    m_scenarioPathPushedToRuntime.clear();
    m_scenarioMTimePushedToRuntime = QDateTime();
    m_pendingEntitySyncToScenario.clear();
    m_pendingEntitySyncToRuntime.clear();
}

void MainWindow::onDatabaseFileChanged(const QString& newDbPath)
{
    if (newDbPath.isEmpty() || !databaseEditor)
        return;
    if (scenarioEditor && scenarioEditor->lastSavedFilePath.isEmpty())
        scenarioEditor->setLoadedDatabaseFile(newDbPath);
    if (runtimeEditor && runtimeEditor->lastSavedFilePath.isEmpty())
        runtimeEditor->setLoadedDatabaseFile(newDbPath);

    const QString dbName = databaseProjectNameFromPath(newDbPath);
    if (!dbName.isEmpty())
        ensureDatabaseProjectFolder(dbName);

    updateWindowTitleForCurrentEditor();
}
void MainWindow::onScenarioFileLoaded(const QString& scenarioPath)
{
    if (scenarioPath.isEmpty())
        return;
    const QString dbPath = resolveDatabasePathFromProjectFile(scenarioPath);
    if (!dbPath.isEmpty() && QFileInfo::exists(dbPath)) {
        if (scenarioEditor)
            scenarioEditor->setLoadedDatabaseFile(dbPath);
        if (runtimeEditor)
            runtimeEditor->setLoadedDatabaseFile(dbPath);
    }
    updateWindowTitleForCurrentEditor();
}
void MainWindow::onRuntimeFileLoaded(const QString& runtimePath)
{
    if (runtimePath.isEmpty())
        return;
    const QString dbPath = resolveDatabasePathFromProjectFile(runtimePath);
    const QString scPath = resolveScenarioPathFromRuntimeFile(runtimePath);
    if (!dbPath.isEmpty() && QFileInfo::exists(dbPath)) {
        if (scenarioEditor)
            scenarioEditor->setLoadedDatabaseFile(dbPath);
        if (runtimeEditor)
            runtimeEditor->setLoadedDatabaseFile(dbPath);
    }
    if (!scPath.isEmpty() && QFileInfo::exists(scPath) && scenarioEditor) {
        const QString curSc = scenarioEditor->lastSavedFilePath;
        if (curSc.isEmpty()
            || QFileInfo(curSc).absoluteFilePath() != QFileInfo(scPath).absoluteFilePath()) {
            scenarioEditor->loadFromJsonFile(scPath);
        }
    }
    updateWindowTitleForCurrentEditor();
}
QString MainWindow::associatedDatabasePathForHierarchy(Hierarchy* hier) const
{
    if (!hier)
        return QString();
    if (scenarioEditor && hier == scenarioEditor->hierarchy
        && !scenarioEditor->m_loadedDatabaseFile.isEmpty()) {
        return scenarioEditor->m_loadedDatabaseFile;
    }
    if (runtimeEditor && hier == runtimeEditor->hierarchy
        && !runtimeEditor->m_loadedDatabaseFile.isEmpty()) {
        return runtimeEditor->m_loadedDatabaseFile;
    }
    QString projectFile;
    if (scenarioEditor && hier == scenarioEditor->hierarchy)
        projectFile = scenarioEditor->lastSavedFilePath;
    else if (runtimeEditor && hier == runtimeEditor->hierarchy)
        projectFile = runtimeEditor->lastSavedFilePath;

    if (!projectFile.isEmpty()) {
        QString resolved = resolveDatabasePathFromProjectFile(projectFile);
        if (!resolved.isEmpty())
            return resolved;
        return resolveLibraryDbPath();
    }
    return QString();
}
void MainWindow::captureGisSnapshotFor(QWidget* editorWidget)
{
    if (!editorWidget)
        return;

    if (scenarioEditor && editorWidget == scenarioEditor) {
        m_scenarioGisSnapshot = ShapeApi::instance()->toJson();
        m_hasScenarioGisSnapshot = true;
    } else if (runtimeEditor && editorWidget == runtimeEditor) {
        m_runtimeGisSnapshot = ShapeApi::instance()->toJson();
        m_hasRuntimeGisSnapshot = true;
    }
}

void MainWindow::restoreGisSnapshotFor(QWidget* editorWidget)
{
    if (!editorWidget)
        return;

    if (scenarioEditor && editorWidget == scenarioEditor && m_hasScenarioGisSnapshot) {
        ShapeApi::instance()->fromJson(m_scenarioGisSnapshot);
    } else if (runtimeEditor && editorWidget == runtimeEditor && m_hasRuntimeGisSnapshot) {
        ShapeApi::instance()->fromJson(m_runtimeGisSnapshot);
    }
}
