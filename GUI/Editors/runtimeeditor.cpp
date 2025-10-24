

#include "runtimeeditor.h"
#include "GUI/Menubars/menubar.h"
#include "GUI/Panel/radardisplay.h"
#include "GUI/Sidebar/sidebarwidget.h"
#include "GUI/Toolbars/standardtoolbar.h"
#include "GUI/Toolbars/networktoolbar.h"
#include "GUI/Feedback/feedback.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QListWidget>
#include <QDockWidget>
#include <core/structure/runtime.h>
#include <core/structure/scenario.h>
#include <core/Hierarchy/Components/transform.h>
#include <core/Hierarchy/Components/mesh.h>
#include <QSplitter>
#include <QFileDialog>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMessageBox>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QVBoxLayout>

QString RuntimeEditor::getTimingJsonData() const
{
    return GraphWidgetTime::JSON_DATA;
}

RuntimeEditor::RuntimeEditor(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Runtime Editor");
    resize(1100, 600);
    QDockWidget::DockWidgetFeatures dockFeatures = QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable;

// Initialize LoggerDock and LoggerDialog
loggerDock = new QDockWidget("Logger", this);
loggerDock->setFeatures(dockFeatures);
loggerDock->setAllowedAreas(Qt::RightDockWidgetArea);
loggerDialog = new LoggerDialog(this);
loggerDock->setWidget(loggerDialog);
loggerDock->setMinimumWidth(200);
loggerDock->hide(); // Initially hidden


    textScriptDock = new QDockWidget("Text Script", this);
    textScriptView = new TextScriptWidget(this);
    textScriptDock->setWidget(textScriptView);
    setupMenuBar();
    connect(menuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
    setupToolBars();
    setupDockWidgets(dockFeatures);
    setupStatusBar();
    runtime = new Runtime();
    hierarchy = runtime->hierarchy;
    SceneRenderer *renderer = runtime->scenerenderer;
    simulation = runtime->simulation;
    Simulation *simulation = runtime->simulation;
    console = runtime->console;
    NetworkManager *networkManager = runtime->networkManager;
    library = runtime->Library;
    lastSavedFilePath = "";

    runtime->scriptengine->setHierarchy(hierarchy, treeView, renderer);
    HierarchyConnector::instance()->setHierarchy(hierarchy);
    HierarchyConnector::instance()->setLibrary(library);
    HierarchyConnector::instance()->setLibTreeView(libTreeView);
    // 🔥 FIX: let Run in RuntimeEditor actually trigger the ScriptEngine
    connect(textScriptView, &TextScriptWidget::runScriptstring,
            runtime->scriptengine, &ScriptEngine::loadAndCompileScript);

    // Initialize Display Window with QTabWidget
    displayWindow = new QWidget(this);
    displayWindow->setWindowTitle("Display Window");
    displayWindow->setWindowFlags(Qt::Dialog |
                                  Qt::WindowTitleHint |
                                  Qt::WindowCloseButtonHint |
                                  Qt::WindowMinMaxButtonsHint |
                                  Qt::WindowSystemMenuHint |
                                  Qt::WindowStaysOnTopHint);
    displayWindow->setAttribute(Qt::WA_DeleteOnClose, true);
    displayWindow->resize(800, 600);
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    displayWindow->move(screenGeometry.center() - displayWindow->rect().center());


    displayDock = new QDockWidget("Sensors", this);
    displayDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    displayDock->setAllowedAreas(Qt::RightDockWidgetArea);
    displayDock->setMinimumWidth(200);


    displayTabs = new QTabWidget(displayDock);
    displayDock->setWidget(displayTabs);


    radarDisplayUI = new RadarDisplay(displayTabs);
    radarDisplayUI->setHierarchy(hierarchy);
    displayTabs->addTab(radarDisplayUI, "Radar");


    ewDisplayUI = new EWDisplay(displayTabs);
    ewDisplayUI->setHierarchy(hierarchy);
    displayTabs->addTab(ewDisplayUI, "EW");


    displayTabs->setCurrentIndex(0);


    connect(displayTabs, &QTabWidget::currentChanged, this, [=](int index) {
        qDebug() << "Display tab changed to index:" << index;
    });


    connect(simulation, &Simulation::Update, ewDisplayUI, &EWDisplay::updateRadar);
    connect(hierarchy, &Hierarchy::entityRemoved, ewDisplayUI, &EWDisplay::RemoveEntity);


    connect(displayDock, &QDockWidget::visibilityChanged, this, [=](bool visible) {
        if (!visible) {
            if (runtimeToolBar) {
                QAction* radarToggle = runtimeToolBar->findChild<QAction*>("radarToggleAction");
                if (radarToggle) {
                    radarToggle->setChecked(false);
                    qDebug() << "Display dock hidden, toggle action unchecked";
                }
            }
        }
    });


    displayDock->hide();



    connect(simulation, &Simulation::Update, radarDisplayUI, &RadarDisplay::updateRadar);
    connect(hierarchy, &Hierarchy::entityRemoved, radarDisplayUI, &RadarDisplay::RemoveEntity);
    connect(runtimeToolBar, &RuntimeToolBar::radarDisplayToggled, this, &RuntimeEditor::toggleRadarDisplay);
// Connect loggerAction to toggleLoggerDisplay
connect(runtimeToolBar, &RuntimeToolBar::loggerTriggered, this, &RuntimeEditor::toggleLoggerDisplay);
// Handle loggerDock visibility changes
connect(loggerDock, &QDockWidget::visibilityChanged, this, [=](bool visible) {
    qDebug() << "Logger dock visibility changed, visible:" << visible;
    if (!visible) {
        SidebarWidget *sidebar = sidebarDock->widget()->findChild<SidebarWidget*>();
        if (sidebar) {
            sidebar->setActiveButton("");
        }
        QAction *loggerAction = runtimeToolBar->findChild<QAction*>("loggerAction");
        if (loggerAction) {
            loggerAction->setChecked(false);
        }
    }
});

//Logger connections
connect(loggerDialog, &LoggerDialog::startRecording, this, [=]() {
    recordingStartTime = QDateTime::currentDateTime();
    runtime->recorder->startRecording();
    recordingTimer = new QTimer(this);
    connect(recordingTimer, &QTimer::timeout, this, [=]() {
        if (recordingStartTime.isValid()) {
            qint64 durationMs = recordingStartTime.msecsTo(QDateTime::currentDateTime());
            loggerDialog->updateRecordingDuration(durationMs);
        }
    });
    recordingTimer->start(100); // Update every 100ms
    qDebug() << "Recording started from LoggerDialog";
});

connect(loggerDialog, &LoggerDialog::stopRecording, this, [=]() {
    runtime->recorder->stopRecording();
    if (recordingTimer) {
        recordingTimer->stop();
        delete recordingTimer;
        recordingTimer = nullptr;
    }
    recordingStartTime = QDateTime();
    loggerDialog->updateRecordingDuration(0);
    qDebug() << "Recording stopped from LoggerDialog";
});
connect(loggerDialog, &LoggerDialog::replayRecording, this, [=](const QString &filePath) {
    simulation->stop();
    tacticalDisplay->canvas->Render(0.016f);
    if (!filePath.isEmpty() && runtime->recorder->loadFromFile(filePath)) {
        QVector<QJsonObject> frames = runtime->recorder->getRecordedFrames();
        if (!frames.isEmpty()) {
            simulation->replay(frames);
            qDebug() << "Replay started using file:" << filePath;
        } else {
            qWarning() << "Replay file loaded but contains no frames.";
        }
    } else {
        qWarning() << "Replay cancelled or file failed to load.";
    }
});

connect(loggerDialog, &LoggerDialog::eventTypesSelected, this, [=](const QStringList &eventTypes) {
    qDebug() << "Event types selected:" << eventTypes;
    // Pass to runtime->recorder if needed
});

connect(loggerDialog, &LoggerDialog::bookmarkAdded, this, [=](const QString &bookmarkNote) {
    qDebug() << "Bookmark added:" << bookmarkNote;
    if (recordingStartTime.isValid()) {
        qint64 timestampMs = recordingStartTime.msecsTo(QDateTime::currentDateTime());
        loggerDialog->addBookmarkWithTimestamp(bookmarkNote, timestampMs);
    }
});

connect(loggerDialog, &LoggerDialog::timestampToggled, this, [=](bool enabled) {
    qDebug() << "Timestamp toggled:" << enabled;
    // Update runtime->recorder to enable/disable timestamps
});

connect(displayWindow, &QObject::destroyed, this, [=]() {
    displayWindow = nullptr;
    radarDisplayUI = nullptr;
    ewDisplayUI = nullptr;
    if (runtimeToolBar) {
        QAction* radarToggle = runtimeToolBar->findChild<QAction*>("radarToggleAction");
        if (radarToggle) {
            radarToggle->setChecked(false);
            qDebug() << "Display window destroyed, toggle action unchecked";
        }
    }
});

connect(loggerDialog, &LoggerDialog::eventTypesSelected, this, [=](const QStringList &eventTypes) {
    qDebug() << "Event types selected:" << eventTypes;
    // Pass to runtime->recorder if needed
});
    // Handle RadarDisplay closure to prevent crashes - now for displayWindow
    connect(displayWindow, &QObject::destroyed, this, [=]() {
        displayWindow = nullptr;
        radarDisplayUI = nullptr;
        ewDisplayUI = nullptr;
        if (runtimeToolBar) {
            QAction* radarToggle = runtimeToolBar->findChild<QAction*>("radarToggleAction");
            if (radarToggle) {
                radarToggle->setChecked(false);
                qDebug() << "Display window destroyed, toggle action unchecked";
            }
        }
    });

    connect(renderer, &SceneRenderer::addMesh, tacticalDisplay, &TacticalDisplay::addMesh);
    connect(hierarchy, &Hierarchy::entityRemoved, tacticalDisplay, &TacticalDisplay::removeMesh);
    if (tacticalDisplay && tacticalDisplay->canvas) {
        connect(renderer, &SceneRenderer::Render, tacticalDisplay->canvas, &CanvasWidget::Render);
        connect(renderer, &SceneRenderer::Render, tacticalDisplay->scene3dwidget, &Scene3DWidget::updateEntities);
    }
    connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
    if (runtimeToolBar && tacticalDisplay && tacticalDisplay->canvas && simulation) {
        connect(simulation, &Simulation::Render, runtimeToolBar, &RuntimeToolBar::onElapsedTime);
        connect(runtimeToolBar, &RuntimeToolBar::startTriggered, [=]() {
            tacticalDisplay->canvas->simulation();
            simulation->start();
            runtime->recorder->startRecording();
            qDebug() << "Simulation started and recording started.";
        });
        connect(runtimeToolBar, &RuntimeToolBar::pauseTriggered, this, [=]() {
            simulation->pause();
            qDebug() << "Simulation paused.";
        });
        connect(runtimeToolBar, &RuntimeToolBar::replayTriggered, this, [=]() {
            simulation->stop();
            tacticalDisplay->canvas->Render(0.016f);
            QString filePath = QFileDialog::getOpenFileName(
                this,
                "Select Replay File",
                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/recordings",
                "*.json"
                );
            if (!filePath.isEmpty() && runtime->recorder->loadFromFile(filePath)) {
                QVector<QJsonObject> frames = runtime->recorder->getRecordedFrames();
                if (!frames.isEmpty()) {
                    simulation->replay(frames);
                    qDebug() << "Replay started using file:" << filePath;
                } else {
                    qWarning() << "Replay file loaded but contains no frames.";
                }
            } else {
                qWarning() << "Replay cancelled or file failed to load.";
            }
        });
        connect(runtimeToolBar, &RuntimeToolBar::speedChanged, simulation, &Simulation::setSpeed);
        connect(runtimeToolBar, &RuntimeToolBar::speedChanged, this, [=](int speed) {
            float moveSpeed = static_cast<float>(speed);
            for (auto& [id, comp] : simulation->physicsComponent) {
                if (comp.dynamicModel) {
                    comp.dynamicModel->setMoveSpeed(moveSpeed);
                }
            }
            qDebug() << "Simulation speed set to:" << speed << "(moveSpeed:" << moveSpeed << ")";
        });
        networkToolBar->setNetworkManager(networkManager);
    } else {
        qWarning() << "Failed to connect RuntimeToolBar signals - nullptr detected";
    }
    consoleView->setConsoleDock(consoleDock);
    connect(console, &Console::logUpdate, this, [=](std::string log) {
        if (consoleView) {
            consoleView->appendLog(QString::fromStdString(log));
            consoleView->appendText(QString::fromStdString(log));
        }
    });
    connect(console, &Console::errorUpdate, this, [=](std::string error) {
        if (consoleView) {
            consoleView->appendError(QString::fromStdString(error));
            consoleView->appendText(QString::fromStdString(error));
        }
    });
    connect(console, &Console::warningUpdate, this, [=](std::string warning) {
        if (consoleView) {
            consoleView->appendWarning(QString::fromStdString(warning));
            consoleView->appendText(QString::fromStdString(warning));
        }
    });
    connect(console, &Console::debugUpdate, this, [=](std::string debug) {
        if (consoleView) {
            consoleView->appendDebug(QString::fromStdString(debug));
            consoleView->appendText(QString::fromStdString(debug));
        }
    });
    HierarchyConnector::instance()->connectSignals(hierarchy, treeView, tacticalDisplay, inspector);
    HierarchyConnector::instance()->connectLibrarySignals(library, libTreeView);
    HierarchyConnector::instance()->initializeDummyData(hierarchy);
    HierarchyConnector::instance()->initializeLibraryData(library);
    HierarchyConnector::instance()->setupFileOperations(this, hierarchy, tacticalDisplay);
    if (tacticalDisplay && tacticalDisplay->canvas) {
        connect(tacticalDisplay->canvas, &CanvasWidget::selectEntitybyCursor,
                treeView, &HierarchyTree::selectEntityById);
    }
    connect(treeView, &HierarchyTree::itemSelected, this, [=](QVariantMap data) {
        QString type;
        if (data["type"].type() == QVariant::Map) {
            QVariantMap typeData = data["type"].toMap();
            if (typeData.contains("type") && typeData["type"].toString() == "option") {
                type = "profile";
            } else {
                qWarning() << "Invalid nested type structure in itemSelected:" << data["type"];
                return;
            }
        } else {
            type = data["type"].toString();
        }
        QString name = data["name"].toString();
        QString ID = data["parentId"].toString();
        for (Inspector* inspector : inspectors) {
            // if (inspector->isLocked()) {
            //     continue;
            // }
            if (type == "component") {
                QJsonObject componentData = hierarchy->getComponentData(ID, name);
                if (!componentData.isEmpty()) {
                    inspector->init(ID, name, componentData);
                }
            } else if (type == "profile") {
                inspector->init(ID, name + "_self", (hierarchy->ProfileCategories)[data["ID"].toString().toStdString()]->toJson());
            } else if (type == "folder") {
                inspector->init(ID, name + "_self", (*hierarchy->Folders)[data["ID"].toString().toStdString()]->toJson());
            } else if (type == "entity") {
                inspector->init(data["ID"].toString(), name + "_self", (*hierarchy->Entities)[data["ID"].toString().toStdString()]->toJson());
                if (radarDisplayUI) {
                    radarDisplayUI->selectEntity((*hierarchy->Entities)[data["ID"].toString().toStdString()]);
                }
                if (ewDisplayUI) {
                    ewDisplayUI->selectEntity((*hierarchy->Entities)[data["ID"].toString().toStdString()]);
                }
            } else {
                inspector->init(ID, name, QJsonObject());
            }
        }
        if (!inspectorDock->isVisible()) {
            addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
            splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
            inspectorDock->show();
            qDebug() << "Inspector dock shown on item selection, geometry:" << inspectorDock->geometry();
        }
        if (tacticalDisplay && type == "entity") {
            tacticalDisplay->selectedMesh(data["ID"].toString());
            Console::log("Entity selected: " + data["ID"].toString().toStdString());
        }
    });
    connect(treeView, &HierarchyTree::itemSelected, this, [=](QVariantMap data) {
        QString type;
        if (data["type"].type() == QVariant::Map) {
            QVariantMap typeData = data["type"].toMap();
            if (typeData.contains("type") && typeData["type"].toString() == "option") {
                type = "profile";
            } else {
                qWarning() << "Invalid nested type structure in itemSelected:" << data["type"];
                return;
            }
        } else {
            type = data["type"].toString();
        }
        QString name = data["name"].toString();
        QString ID = data["parentId"].toString();
        for (Inspector* inspector : inspectors) {
            // if (inspector->isLocked()) {
            //     continue;
            // }
            if (type == "component") {
                QJsonObject componentData = hierarchy->getComponentData(ID, name);
                if (!componentData.isEmpty()) {
                    inspector->init(ID, name, componentData);
                }
            } else if (type == "profile") {
                inspector->init(ID, name + "_self", (hierarchy->ProfileCategories)[data["ID"].toString().toStdString()]->toJson());
            } else if (type == "folder") {
                inspector->init(ID, name + "_self", (*hierarchy->Folders)[data["ID"].toString().toStdString()]->toJson());
            } else if (type == "entity") {
                inspector->init(data["ID"].toString(), name + "_self", (*hierarchy->Entities)[data["ID"].toString().toStdString()]->toJson());
            } else {
                inspector->init(ID, name, QJsonObject());
            }
        }
        if (!inspectorDock->isVisible()) {
            addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
            splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
            inspectorDock->show();
            qDebug() << "Inspector dock shown on item selection, geometry:" << inspectorDock->geometry();
        }
        if (tacticalDisplay && type == "entity") {
            tacticalDisplay->selectedMesh(data["ID"].toString());
            Console::log("Entity selected: " + data["ID"].toString().toStdString());
        }
    });
    connect(libTreeView, &HierarchyTree::itemSelected, this, &RuntimeEditor::onLibraryItemSelected);
    connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
    connect(inspector, &Inspector::addTabRequested, this, &RuntimeEditor::addInspectorTab);
    inspectorDocks.append(inspectorDock);
    inspectors.append(inspector);
    inspector->setHierarchy(hierarchy);
    setupToolBarConnections();
}

void RuntimeEditor::toggleRadarDisplay() {
    if (!displayDock->isVisible()) {

        inspectorDock->hide();
        libraryDock->hide();
        textScriptDock->hide();
       loggerDock->hide();
        addDockWidget(Qt::RightDockWidgetArea, displayDock);
        splitDockWidget(sidebarDock, displayDock, Qt::Vertical);
        displayDock->show();
        qDebug() << "Display dock shown, geometry:" << displayDock->geometry();

        SidebarWidget *sidebar = sidebarDock->widget()->findChild<SidebarWidget*>();
        if (sidebar) {
            sidebar->setActiveButton("Sensors");
        }
    } else {

        displayDock->hide();
        qDebug() << "Display dock hidden";

        SidebarWidget *sidebar = sidebarDock->widget()->findChild<SidebarWidget*>();
        if (sidebar) {
            sidebar->setActiveButton("");
        }
    }
    runtimeToolBar->findChild<QAction*>("radarToggleAction")->setChecked(displayDock->isVisible());
}
//   Logger
void RuntimeEditor::toggleLoggerDisplay(bool checked)
{
    qDebug() << "toggleLoggerDisplay called, checked:" << checked << ", loggerDock visible:" << loggerDock->isVisible();
    if (checked && !loggerDock->isVisible()) {
        inspectorDock->hide();
        libraryDock->hide();
        textScriptDock->hide();
        displayDock->hide();
        addDockWidget(Qt::RightDockWidgetArea, loggerDock);
        splitDockWidget(sidebarDock, loggerDock, Qt::Vertical);
        loggerDock->show();
        qDebug() << "Logger dock shown, geometry:" << loggerDock->geometry();
        SidebarWidget *sidebar = sidebarDock->widget()->findChild<SidebarWidget*>();
        if (sidebar) {
            sidebar->setActiveButton("Logger");
        }
    } else if (!checked && loggerDock->isVisible()) {
        loggerDock->hide();
        qDebug() << "Logger dock hidden";
        SidebarWidget *sidebar = sidebarDock->widget()->findChild<SidebarWidget*>();
        if (sidebar) {
            sidebar->setActiveButton("");
        }
    }
    QAction *loggerAction = runtimeToolBar->findChild<QAction*>("loggerAction");
    if (loggerAction) {
        bool isDockVisible = loggerDock->isVisible();
        loggerAction->setChecked(isDockVisible);
        qDebug() << "loggerAction checked state updated to:" << isDockVisible;
    }
}

void RuntimeEditor::setupMenuBar()
{
    menuBar = new MenuBar(this);
    setMenuBar(menuBar);
    connect(menuBar, &MenuBar::feedbackTriggered, this, &RuntimeEditor::showFeedbackWindow);
}

void RuntimeEditor::setupToolBars()
{
    standardToolBar = new StandardToolBar(this);
    addToolBar(Qt::TopToolBarArea, standardToolBar);
    addToolBarBreak(Qt::TopToolBarArea);
    designToolBar = new DesignToolBar(this);
    addToolBar(Qt::TopToolBarArea, designToolBar);
    runtimeToolBar = new RuntimeToolBar(this);
    addToolBar(Qt::TopToolBarArea, runtimeToolBar);
    addToolBarBreak(Qt::TopToolBarArea);
    networkToolBar = new NetworkToolbar(this);
    addToolBar(Qt::TopToolBarArea, networkToolBar);
    if (menuBar && standardToolBar) {
        connect(standardToolBar->getSaveAction(), &QAction::triggered,
                menuBar->getSaveAction(), &QAction::trigger);
    }
}

void RuntimeEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures)
{
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);
    hierarchyDock = new QDockWidget("Editor", this);
    hierarchyDock->setFeatures(dockFeatures);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    treeView = new HierarchyTree(this);
    hierarchyDock->setWidget(treeView);
    hierarchyDock->setMinimumWidth(100);
    QSplitter *rightSplitter = new QSplitter(Qt::Horizontal, this);
    QSplitter *tacticalSplitter = new QSplitter(Qt::Vertical, this);
    tacticalDisplayDock = new QDockWidget("Tactical Display", this);
    tacticalDisplayDock->setFeatures(dockFeatures);
    tacticalDisplayDock->setAllowedAreas(Qt::RightDockWidgetArea);
    tacticalDisplay = new TacticalDisplay(this);
    tacticalDisplayDock->setWidget(tacticalDisplay);
    tacticalDisplayDock->setMinimumWidth(200);
    consoleDock = new QDockWidget("", this);
    consoleDock->setFeatures(dockFeatures);
    consoleDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    consoleView = new ConsoleView(this);
    timingGraphWidget = new GraphWidgetTime(this);
    QTabWidget *bottomTabs = new QTabWidget(this);
    bottomTabs->addTab(consoleView, "Console");
    bottomTabs->addTab(timingGraphWidget, "Timing Graph");
    bottomTabs->setCurrentIndex(0);
    consoleDock->setWidget(bottomTabs);
    consoleDock->setMinimumHeight(100);
    // Initialize timing graph data
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(GraphWidgetTime::JSON_DATA.toUtf8(), &parseError);
    if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
        QJsonArray jsonArray = doc.array();
        QList<QMap<QString, QString>> timingDataList;
        QMap<QString, QString> teamMap; // Create team map from JSON
        for (const QJsonValue &value : jsonArray) {
            QJsonObject obj = value.toObject();
            QMap<QString, QString> data;
            data["entity"] = obj["entity"].toString();
            data["startTime"] = obj["startTime"].toString();
            data["endTime"] = obj["endTime"].toString();
            data["active"] = obj["active"].toString();
            timingDataList.append(data);
            // Extract team information from JSON
            if (obj.contains("team")) {
                teamMap[obj["entity"].toString()] = obj["team"].toString();
            }
        }
        // Remove the hardcoded teamMap assignment and use the one from JSON
        timingGraphWidget->setTeamDivision(teamMap);
        timingGraphWidget->setTimingData(timingDataList, "both");
    }
    tacticalSplitter->addWidget(tacticalDisplayDock);
    tacticalSplitter->addWidget(consoleDock);
    sidebarDock = new QDockWidget("", this);
    sidebarDock->setFeatures(dockFeatures);
    sidebarDock->setAllowedAreas(Qt::RightDockWidgetArea);
    SidebarWidget *sidebar = new SidebarWidget(this);
    sidebarDock->setTitleBarWidget(new QWidget());
    sidebarDock->setWidget(sidebar);
    sidebarDock->setMinimumWidth(80);
    sidebarDock->setMinimumHeight(40);
    rightSplitter->addWidget(tacticalSplitter);
    rightSplitter->addWidget(sidebarDock);
    mainSplitter->addWidget(hierarchyDock);
    mainSplitter->addWidget(rightSplitter);
    QTimer::singleShot(0, this, [=]() {
        int totalWidth = width();
        mainSplitter->setSizes(QList<int>() << totalWidth * 0.15 << totalWidth * 0.85);
        rightSplitter->setSizes(QList<int>() << totalWidth * 0.70 << totalWidth * 0.15);
    });
    tacticalSplitter->setSizes(QList<int>() << height() * 0.9 << height() * 0.1);
    libraryDock = new QDockWidget("Library", this);
    libraryDock->setFeatures(dockFeatures);
    libraryDock->setAllowedAreas(Qt::RightDockWidgetArea);
    libTreeView = new HierarchyTree(this);
    libraryDock->setWidget(libTreeView);
    libraryDock->setMinimumWidth(200);
    libraryDock->hide();
    inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setFeatures(dockFeatures);
    inspectorDock->setAllowedAreas(Qt::RightDockWidgetArea);
    inspector = new Inspector(this);
    inspectorDock->setWidget(inspector);
    inspectorDock->setMinimumWidth(200);
    addDockWidget(Qt::RightDockWidgetArea, sidebarDock);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
    inspectorDock->show();
    textScriptDock->setFeatures(dockFeatures);
    textScriptDock->setAllowedAreas(Qt::RightDockWidgetArea);
    textScriptDock->setMinimumWidth(200);
    addDockWidget(Qt::RightDockWidgetArea, textScriptDock);
    textScriptDock->hide();
// Added   lOGER
addDockWidget(Qt::RightDockWidgetArea, loggerDock);
loggerDock->hide();

    connect(sidebar, &SidebarWidget::viewSelected, this, [this](const QString &viewName) {
        qDebug() << "Sidebar viewSelected emitted, viewName:" << viewName;
        if (viewName == "Inspector") {
            qDebug() << "Showing Inspector dock";
            libraryDock->hide();
            textScriptDock->hide();
            displayDock->hide(); // Hide display dock
      loggerDock->hide();
            if (!inspectorDock->isVisible()) {
                addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
                splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
                inspectorDock->show();
                qDebug() << "Inspector dock geometry:" << inspectorDock->geometry();
            }
        } else if (viewName == "Library") {
            qDebug() << "Showing Library dock";
            inspectorDock->hide();
            textScriptDock->hide();
            displayDock->hide(); // Hide display dock
     loggerDock->hide();
            if (!libraryDock->isVisible()) {
                addDockWidget(Qt::RightDockWidgetArea, libraryDock);
                splitDockWidget(sidebarDock, libraryDock, Qt::Vertical);
                libraryDock->show();
                qDebug() << "Library dock geometry:" << libraryDock->geometry();
            }
        } else if (viewName == "TextScript") {
            qDebug() << "Showing TextScript dock";
            inspectorDock->hide();
            libraryDock->hide();
            displayDock->hide(); // Hide display dock
       loggerDock->hide();
            if (!textScriptDock->isVisible()) {
                addDockWidget(Qt::RightDockWidgetArea, textScriptDock);
                splitDockWidget(sidebarDock, textScriptDock, Qt::Vertical);
                textScriptDock->show();
                qDebug() << "TextScript dock geometry:" << textScriptDock->geometry();
            }
        } else if (viewName == "Sensors") {
            qDebug() << "Showing Sensors dock";
            inspectorDock->hide();
            libraryDock->hide();
            textScriptDock->hide();
 loggerDock->hide();
            if (!displayDock->isVisible()) {
                addDockWidget(Qt::RightDockWidgetArea, displayDock);
                splitDockWidget(sidebarDock, displayDock, Qt::Vertical);
                displayDock->show();
                qDebug() << "Display dock geometry:" << displayDock->geometry();
            }


} else if (viewName == "Logger") {
    qDebug() << "Toggling Logger dock, current visibility:" << loggerDock->isVisible();
    if (!loggerDock->isVisible()) {
        inspectorDock->hide();
        libraryDock->hide();
        textScriptDock->hide();
        displayDock->hide();
        addDockWidget(Qt::RightDockWidgetArea, loggerDock);
        splitDockWidget(sidebarDock, loggerDock, Qt::Vertical);
        loggerDock->show();
        qDebug() << "Logger dock shown, geometry:" << loggerDock->geometry();
        runtimeToolBar->findChild<QAction*>("loggerAction")->setChecked(true);
        SidebarWidget *sidebar = sidebarDock->widget()->findChild<SidebarWidget*>();
        if (sidebar) {
            sidebar->setActiveButton("Logger");
        }
    } else {
        loggerDock->hide();
        qDebug() << "Logger dock hidden";
        runtimeToolBar->findChild<QAction*>("loggerAction")->setChecked(false);
        SidebarWidget *sidebar = sidebarDock->widget()->findChild<SidebarWidget*>();
        if (sidebar) {
            sidebar->setActiveButton("");
        }
    }


        } else if (viewName == "Console") {
            qDebug() << "Toggling Console dock, current visibility:" << consoleDock->isVisible();
            consoleDock->setVisible(!consoleDock->isVisible());
            qDebug() << "Console dock geometry:" << consoleDock->geometry();
        } else {
            qDebug() << "Unknown viewName received:" << viewName;
        }
    });
}

void RuntimeEditor::setupToolBarConnections()
{
    DesignToolBar *designToolBar = findChild<DesignToolBar*>();
    if (!designToolBar || !tacticalDisplay || !tacticalDisplay->canvas) {
        qWarning() << "Toolbar connection setup failed - required components missing";
        return;
    }
    if (menuBar) {
        connect(standardToolBar->getSaveAction(), &QAction::triggered,
                menuBar->getSaveAction(), &QAction::trigger);
    }
    connect(designToolBar, &DesignToolBar::modeChanged,
            this, [=](int mode) {
                tacticalDisplay->canvas->setTransformMode(static_cast<TransformMode>(mode));
            });
    connect(designToolBar, &DesignToolBar::shapeSelected,
            this, [=](const QString &shape) {
                tacticalDisplay->canvas->setShapeDrawingMode(true, shape);
                Console::log("Shape selected: " + shape.toStdString());
            });
    connect(designToolBar, &DesignToolBar::gridPlaneXToggled,
            tacticalDisplay->canvas, &CanvasWidget::setXGridVisible);
    connect(designToolBar, &DesignToolBar::gridPlaneYToggled,
            tacticalDisplay->canvas, &CanvasWidget::setYGridVisible);
    connect(designToolBar, &DesignToolBar::gridOpacityChanged,
            this, [=](int opacity) {
                tacticalDisplay->canvas->setGridOpacity(opacity);
                Console::log("Grid opacity changed to: " + std::to_string(opacity));
            });
    connect(designToolBar, &DesignToolBar::layerOptionToggled,
            tacticalDisplay->canvas, &CanvasWidget::toggleLayerVisibility);
    connect(designToolBar, &DesignToolBar::bitmapImageSelected,
            tacticalDisplay->canvas, &CanvasWidget::onBitmapImageSelected);
    connect(designToolBar, &DesignToolBar::bitmapSelected,
            this, [=](const QString &fileName) {
                tacticalDisplay->canvas->onBitmapSelected(fileName);
                Console::log("Bitmap selected: " + fileName.toStdString());
            });
    if (tacticalDisplay && tacticalDisplay->mapWidget) {
        connect(designToolBar, &DesignToolBar::mapLayerChanged,
                this, [=](const QString &layers) {
                    tacticalDisplay->setMapLayers(layers.split(",", Qt::SkipEmptyParts));
                    Console::log("Map layers updated: " + layers.toStdString());
                });
        connect(designToolBar, &DesignToolBar::customMapAdded,
                tacticalDisplay, &TacticalDisplay::addCustomMap);
        connect(designToolBar, &DesignToolBar::customMapAdded,
                this, [=](const QString &name, int zoomMin, int zoomMax, const QString &url) {
                    qDebug() << "RuntimeEditor received customMapAdded: name =" << name
                             << ", zoomMin =" << zoomMin << ", zoomMax =" << zoomMax
                             << ", url =" << url;
                });
        connect(designToolBar, &DesignToolBar::searchPlaceTriggered,
                tacticalDisplay->mapWidget, &GISlib::serachPlace);
        connect(designToolBar->zoomInAction, &QAction::triggered,
                tacticalDisplay, &TacticalDisplay::zoomIn);
        connect(designToolBar->zoomOutAction, &QAction::triggered,
                tacticalDisplay, &TacticalDisplay::zoomOut);
        connect(designToolBar->selectCenterAction, &QAction::triggered, this, [=]() {
            if (tacticalDisplay && tacticalDisplay->mapWidget) {
                tacticalDisplay->mapWidget->setCenter(0, 0);
                Console::log("Map centered at (0, 0)");
            }
        });
    } else {
        qCritical() << "Map widget not available for layer connections";
    }
    connect(designToolBar->getMeasureDistanceAction(), &QAction::triggered,
            this, [=]() {
                bool isChecked = designToolBar->getMeasureDistanceAction()->isChecked();
                tacticalDisplay->canvas->setTransformMode(isChecked ? MeasureDistance : Translate);
                Console::log(isChecked ? "Measure Distance mode enabled" : "Measure Distance mode disabled");
            });
    connect(designToolBar, &DesignToolBar::bitmapImageSelected,
            tacticalDisplay->canvas, &CanvasWidget::onBitmapImageSelected);
    // for preset
    connect(designToolBar, &DesignToolBar::presetLayerSelected,
            tacticalDisplay->canvas, &CanvasWidget::onPresetLayerSelected);
    // gor geojson function
    connect(designToolBar, &DesignToolBar::importGeoJsonTriggered,
            tacticalDisplay->canvas, &CanvasWidget::importGeoJsonLayer);

    // NEW: Connections for GeoJSON layers menu
    connect(tacticalDisplay->canvas, &CanvasWidget::geoJsonLayerAdded,
            designToolBar, &DesignToolBar::onGeoJsonLayerAdded);
    connect(designToolBar, &DesignToolBar::geoJsonLayerToggled,
            tacticalDisplay->canvas, &CanvasWidget::onGeoJsonLayerToggled);
    connect(designToolBar, &DesignToolBar::bitmapSelected,
            this, [=](const QString &fileName) {
                tacticalDisplay->canvas->onBitmapSelected(fileName);
                Console::log("Bitmap selected: " + fileName.toStdString());
            });
    connect(designToolBar, &DesignToolBar::shapeSelected,
            this, [=](const QString &shape) {
                tacticalDisplay->canvas->setShapeDrawingMode(true, shape);
                Console::log("Shape selected: " + shape.toStdString());
            });
}

void RuntimeEditor::onItemSelected(QVariantMap data)
{
    QString type = data["type"].toString();
    QString name = data["name"].toString();
    QString ID = data["parentId"].toString();
    for (Inspector* inspector : inspectors) {
        if (type == "component") {
            QJsonObject componentData = hierarchy->getComponentData(ID, name);
            if (!componentData.isEmpty()) {
                inspector->init(ID, name, componentData);
            }
        } else if (type == "profile") {
            inspector->init(ID, name + "_self", (hierarchy->ProfileCategories)[data["ID"].toString().toStdString()]->toJson());
        } else if (type == "folder") {
            inspector->init(ID, name + "_self", (*hierarchy->Folders)[data["ID"].toString().toStdString()]->toJson());
        } else if (type == "entity") {
            inspector->init(data["ID"].toString(), name + "_self", (*hierarchy->Entities)[data["ID"].toString().toStdString()]->toJson());
        } else {
            inspector->init(ID, name, QJsonObject());
        }
    }
    if (tacticalDisplay && type == "entity") {
        tacticalDisplay->selectedMesh(data["ID"].toString());
    }
}

void RuntimeEditor::onLibraryItemSelected(QVariantMap data)
{
    qDebug() << "Library item selected:" << data;
    // TODO: Implement library item selection functionality
}

void RuntimeEditor::addInspectorTab()
{
    QDockWidget *newInspectorDock = new QDockWidget("Inspector " + QString::number(++inspectorCount), this);
    newInspectorDock->setAllowedAreas(Qt::RightDockWidgetArea);
    newInspectorDock->setFeatures(QDockWidget::DockWidgetClosable |
                                  QDockWidget::DockWidgetMovable |
                                  QDockWidget::DockWidgetFloatable);
    Inspector *newInspector = new Inspector(newInspectorDock);
    newInspectorDock->setWidget(newInspector);
    inspectorDocks.append(newInspectorDock);
    inspectors.append(newInspector);
    connect(newInspector, &Inspector::valueChanged,
            hierarchy, &Hierarchy::UpdateComponent);
    connect(newInspector, &Inspector::addTabRequested,
            this, &RuntimeEditor::addInspectorTab);
    if (inspectorDock->isVisible()) {
        splitDockWidget(inspectorDock, newInspectorDock, Qt::Horizontal);
    } else {
        addDockWidget(Qt::RightDockWidgetArea, newInspectorDock);
    }
    connect(newInspectorDock, &QDockWidget::destroyed, this, [=]() {
        inspectorDocks.removeOne(newInspectorDock);
        inspectors.removeOne(newInspector);
    });
}

void RuntimeEditor::showFeedbackWindow()
{
    Feedback *feedbackWindow = new Feedback(this);
    feedbackWindow->h = hierarchy;
    feedbackWindow->loadDashboardData("{}");
    feedbackWindow->show();
}

void RuntimeEditor::loadFromJsonFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open JSON file:" << filePath;
        QMessageBox::warning(this, "Error", QString("Failed to open JSON file: %1").arg(filePath));
        return;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse JSON:" << err.errorString();
        QMessageBox::warning(this, "Error", QString("Failed to parse JSON: %1").arg(err.errorString()));
        return;
    }
    QJsonObject obj = doc.object();
    if (obj.contains("hierarchy")) {
        QJsonObject hier = obj["hierarchy"].toObject();
        hierarchy->fromJson(hier);
        qDebug() << "Hierarchy loaded from file:" << filePath;
        if (treeView && treeView->getTreeWidget()) {
            treeView->getTreeWidget()->update();
            qDebug() << "HierarchyTree updated after loading JSON";
        } else {
            qWarning() << "Failed to update HierarchyTree: treeView or treeWidget is null";
        }
    } else {
        qWarning() << "JSON file does not contain 'hierarchy' key";
    }
    if (tacticalDisplay && obj.contains("tactical")) {
        QJsonObject tac = obj["tactical"].toObject();
        tacticalDisplay->canvas->fromJson(tac);
        qDebug() << "TacticalDisplay loaded from file:" << filePath;
    } else {
        qWarning() << "JSON file does not contain 'tactical' key or tacticalDisplay is null";
    }
    lastSavedFilePath = filePath;
    clearUnsavedChanges();
}

void RuntimeEditor::markUnsavedChanges()
{
    if (!hasUnsavedChanges) {
        hasUnsavedChanges = true;
        emit unsavedChangesChanged(true);
        setWindowTitle("Runtime Editor *");
    }
}

void RuntimeEditor::clearUnsavedChanges()
{
    if (hasUnsavedChanges) {
        hasUnsavedChanges = false;
        emit unsavedChangesChanged(false);
        setWindowTitle("Runtime Editor");
    }
}

RuntimeEditor::~RuntimeEditor()
{
    // Cleanup managed by Qt's parent-child relationships
}

void RuntimeEditor::setupStatusBar() {
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Ready");
}
void RuntimeEditor::updateStatusBar(const QString &message) {
    if (statusBar) {
        statusBar->showMessage(message);
    }
}
