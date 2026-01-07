

#include "runtimeeditor.h"
#include "GUI/Menubars/menubar.h"
#include "GUI/Panel/radardisplay.h"
#include "GUI/Sidebar/sidebarwidget.h"
#include "GUI/Toolbars/standardtoolbar.h"
#include "GUI/Toolbars/networktoolbar.h"
#include "GUI/Feedback/feedback.h"
#include "qthread.h"
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
#include <QTimer>
#include <GUI/Menubars/profileinfodialog.h>
#include <GUI/Editors/recentprojectsmanager.h>
#include <GUI/Settings/applicationdialog.h>
#include <QProgressDialog>

static QString capitalizeFirstLetter(const QString &str)
{
    if (str.isEmpty()) return str;
    return str[0].toUpper() + str.mid(1);
}

QString RuntimeEditor::getTimingJsonData() const
{
    // return GraphWidgetTime::JSON_DATA;
    return "{}";
}


RuntimeEditor::RuntimeEditor(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Runtime Editor");
    resize(1100, 600);

    // Use enhanced dock widget setup for Linux compatibility - JUST LIKE SCENARIO EDITOR
    setupEnhancedDockWidgets();

    setupMenuBar();
    connect(menuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
    setupToolBars();
    setupStatusBar();
    //connect(navi, &MenuBar::)

    runtime = new Runtime();
    hierarchy = runtime->hierarchy;
    SceneRenderer *renderer = runtime->scenerenderer;
    simulation = runtime->simulation;

    console = runtime->console;
    NetworkManager *networkManager = runtime->networkManager;
    library = runtime->Library;
    lastSavedFilePath = "";

    runtime->scriptengine->setHierarchy(hierarchy, treeView, renderer);
    HierarchyConnector::instance()->setHierarchy(hierarchy);
    HierarchyConnector::instance()->setLibrary(library);
    HierarchyConnector::instance()->setLibTreeView(libTreeView);


    connect(textScriptView, &TextScriptWidget::runScriptstring,
            runtime->scriptengine, &ScriptEngine::loadAndCompileScript);

    connect(this,&RuntimeEditor::Activated,simulation,&Simulation::ReInit);
    connect(this,&RuntimeEditor::Activated,tacticalDisplay->canvas,&CanvasWidget::ReInit);
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

    // Setup display tabs for displayDock
    displayTabs = new QTabWidget(displayDock);
    displayDock->setWidget(displayTabs);

    radarDisplayUI = new RadarDisplay(displayTabs);
    radarDisplayUI->setHierarchy(hierarchy);
    displayTabs->addTab(radarDisplayUI, "Radar");

    // ewDisplayUI = new EWDisplay(displayTabs);
    // ewDisplayUI->setHierarchy(hierarchy);
    // displayTabs->addTab(ewDisplayUI, "EW");

    iffDisplayUI = new IFFDisplay(displayTabs);
    iffDisplayUI->setHierarchy(hierarchy);
    displayTabs->addTab(iffDisplayUI, "IFF");

    radioDisplayUI = new RADIODisplay(displayTabs);
    radioDisplayUI->setHierarchy(hierarchy);
    displayTabs->addTab(radioDisplayUI, "RADIO");

    esmDisplayUI = new ESMDisplay(displayTabs);
    esmDisplayUI->setHierarchy(hierarchy);
    displayTabs->addTab(esmDisplayUI, "ESM");

    csmDisplayUI = new CSMDisplay(displayTabs);
    csmDisplayUI->setHierarchy(hierarchy);
    displayTabs->addTab(csmDisplayUI, "CSM");

    displayTabs->setCurrentIndex(0);

    // Connect simulation updates to displays
    connect(simulation, &Simulation::Update, radarDisplayUI, &RadarDisplay::updateRadar);
    connect(hierarchy, &Hierarchy::entityRemoved, radarDisplayUI, &RadarDisplay::RemoveEntity);

    // connect(simulation, &Simulation::Update, ewDisplayUI, &EWDisplay::updateRadar);
    // connect(hierarchy, &Hierarchy::entityRemoved, ewDisplayUI, &EWDisplay::RemoveEntity);

    connect(simulation, &Simulation::Update, csmDisplayUI, &CSMDisplay::updateRadar);
    connect(hierarchy, &Hierarchy::entityRemoved, csmDisplayUI, &CSMDisplay::RemoveEntity);

    connect(simulation, &Simulation::Update, esmDisplayUI, &ESMDisplay::updateRadar);
    connect(hierarchy, &Hierarchy::entityRemoved, esmDisplayUI, &ESMDisplay::RemoveEntity);

    // IFFDisplay connections
    connect(simulation, &Simulation::Update, iffDisplayUI, &IFFDisplay::updateRadar);
    connect(hierarchy, &Hierarchy::entityRemoved, iffDisplayUI, &IFFDisplay::RemoveEntity);

    // RADIO DISPLAY CONNECTIONS
    connect(simulation, &Simulation::Update, radioDisplayUI, &RADIODisplay::updateRadar);
    connect(hierarchy, &Hierarchy::entityRemoved, radioDisplayUI, &RADIODisplay::RemoveEntity);
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


    connect(runtimeToolBar, &RuntimeToolBar::radarDisplayToggled, this, &RuntimeEditor::toggleRadarDisplay);

    // Connect loggerAction to toggleLoggerDisplay
    connect(runtimeToolBar, &RuntimeToolBar::loggerTriggered, this, &RuntimeEditor::toggleLoggerDisplay);
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
        recordingTimer->start(100);

    });
    connect(runtime->recorder, &Recorder::recordingPaused,
            loggerDialog->getTimelineWidget(), &TimelineWidget::pauseRecording);

    connect(runtime->recorder, &Recorder::recordingResumed,
            loggerDialog->getTimelineWidget(), &TimelineWidget::resumeRecording);


    //End Himan
    connect(loggerDialog, &LoggerDialog::stopRecording, this, [=]() {
        runtime->recorder->stopRecording();
        if (recordingTimer) {
            recordingTimer->stop();
            delete recordingTimer;
            recordingTimer = nullptr;
        }
        recordingStartTime = QDateTime();
        loggerDialog->updateRecordingDuration(0);

    });

    connect(loggerDialog, &LoggerDialog::saveRecording, this, [=](const QString &filePath) {
        if (runtime && runtime->recorder) {
            bool saved = runtime->recorder->saveToFile(filePath);
            if (saved)
                qDebug() << "Recording successfully saved to:" << filePath;
            else
                qWarning() << "Failed to save recording to:" << filePath;
        } else {
            qWarning() << "Recorder instance not available!";
        }
    });
    connect(loggerDialog, &LoggerDialog::loadRecording, this, [=](const QString &filePath) {
        if (runtime && runtime->recorder) {
            if (runtime->recorder->loadReplay(filePath)) {
                qDebug() << "Replay loaded:" << filePath;
            }
        }
    });
    connect(loggerDialog, &LoggerDialog::startReplay, this, [=]() {
        if (runtime && runtime->recorder) {
            runtime->recorder->startReplay();
        }
    });

    connect(loggerDialog, &LoggerDialog::toggleReplayPause, this, [=]() {
        if (runtime && runtime->recorder) {
            runtime->recorder->toggleReplayPause();
        }
    });
    connect(loggerDialog, &LoggerDialog::previousFrame, this, [=]() {
        if (runtime && runtime->recorder) {
            runtime->recorder->goToPreviousFrame();
        }
    });

    connect(loggerDialog, &LoggerDialog::nextFrame, this, [=]() {
        if (runtime && runtime->recorder) {
            runtime->recorder->goToNextFrame();
        }
    });

    connect(loggerDialog, &LoggerDialog::pressPlayAgain, this, [=]() {
        if (runtime && runtime->recorder) {
            runtime->recorder->playAgain();
        }
    });

    connect(loggerDialog, &LoggerDialog::replayRecording, this, [=](const QString &filePath) {
        simulation->stop();
        tacticalDisplay->canvas->Render(0.016f);
        if (!filePath.isEmpty() && runtime->recorder->loadReplay(filePath)) {
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
    });

    connect(loggerDialog, &LoggerDialog::bookmarkAdded, this, [=](const QString &bookmarkNote) {
        qDebug() << "Bookmark button clicked — note:" << bookmarkNote;

        if (!runtime || !runtime->recorder) {
            qWarning() << "Runtime or Recorder not available — cannot save bookmark.";
            return;
        }

        if (recordingStartTime.isValid()) {
            qint64 timestampMs = recordingStartTime.msecsTo(QDateTime::currentDateTime());
            runtime->recorder->recordBookmark(bookmarkNote, timestampMs);
            loggerDialog->addBookmarkWithTimestamp(bookmarkNote, timestampMs);

            qDebug() << "Bookmark added at" << timestampMs << "ms — message:" << bookmarkNote;
        } else {
            qWarning() << "Cannot add bookmark — recording not started.";
        }
    });

    connect(loggerDialog, &LoggerDialog::timestampToggled, this, [=](bool enabled) {
        qDebug() << "Timestamp toggled:" << enabled;
    });

    connect(loggerDialog, &LoggerDialog::requestReplayReset,
            runtime->recorder, &Recorder::resetReplayState);


    connect(loggerDialog, &LoggerDialog::bookmarkClicked,this, [=](const QString &note, qint64 timestampMs) {
        runtime->recorder->bookmarkReplay(note, timestampMs);
    });

    connect(displayWindow, &QObject::destroyed, this, [=]() {
        displayWindow = nullptr;
        radarDisplayUI = nullptr;
        // ewDisplayUI = nullptr;
        iffDisplayUI = nullptr;
        esmDisplayUI = nullptr;
        csmDisplayUI = nullptr;
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
    });

    connect(runtime->recorder, &Recorder::replayBookmark,
            loggerDialog, &LoggerDialog::onReplayBookmarkLoaded);
    connect(runtime->recorder, &Recorder::replayFrameLoaded,
            loggerDialog, &LoggerDialog::updateReplayProgress);
    connect(runtime->recorder, &Recorder::setReplayDuration,
            loggerDialog, &LoggerDialog::setTimelineDuration);


    // Handle RadarDisplay closure to prevent crashes - now for displayWindow
    connect(displayWindow, &QObject::destroyed, this, [=]() {
        displayWindow = nullptr;
        radarDisplayUI = nullptr;
        // ewDisplayUI = nullptr;
        esmDisplayUI = nullptr;
        csmDisplayUI = nullptr;
        if (runtimeToolBar) {
            QAction* radarToggle = runtimeToolBar->findChild<QAction*>("radarToggleAction");
            if (radarToggle) {
                radarToggle->setChecked(false);
                qDebug() << "Display window destroyed, toggle action unchecked";
            }
        }
    });


    connect(simulation, &Simulation::Update, this, [=]() {
        if (iffDisplayUI && iffDisplayUI->entity && iffDisplayUI->iff) {
            // Regular IFF interrogation
            iffDisplayUI->iff->interrogateTargets(iffDisplayUI->entity->transform);
        }
    });



    // Connect tactical display signals
    if (tacticalDisplay && tacticalDisplay->canvas) {

        connect(tacticalDisplay->canvas, &CanvasWidget::trajectoryUpdated, inspector, &Inspector::updateTrajectory);
        connect(tacticalDisplay->canvas, &CanvasWidget::trajectoryUpdated, this, [=](QString entityId, QJsonArray /*waypoints*/) {
            auto it = tacticalDisplay->canvas->Meshes.find(entityId.toStdString());
            if (it != tacticalDisplay->canvas->Meshes.end() && it->second.trajectory) {
                QJsonObject trajData = it->second.trajectory->toJson();
                hierarchy->UpdateComponent(entityId, "Trajectory", trajData);
                Console::log("Trajectory updated for entity: " + entityId.toStdString());
                treeView->getTreeWidget()->update();
            } else {
                Console::error("Failed to update trajectory for entity: " + entityId.toStdString() +
                               " - entity or trajectory not found");
            }
        });
        connect(inspector, &Inspector::trajectoryWaypointsChanged, tacticalDisplay->canvas, &CanvasWidget::updateWaypointsFromInspector);
    }

    connect(renderer, &SceneRenderer::addMesh, tacticalDisplay, &TacticalDisplay::addMesh);
    connect(hierarchy, &Hierarchy::entityRemoved, tacticalDisplay, &TacticalDisplay::removeMesh);

    if (tacticalDisplay && tacticalDisplay->canvas) {
        connect(renderer, &SceneRenderer::Render, tacticalDisplay->canvas, &CanvasWidget::Render);
        connect(renderer, &SceneRenderer::Render, tacticalDisplay->scene3dwidget, &Scene3DWidget::updateEntities);
    }

    connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
    connect(inspector, &Inspector::valueChanged, this, [=]{ renderer->Render(0.01f); markUnsavedChanges(); });


    if (runtimeToolBar && tacticalDisplay && tacticalDisplay->canvas && simulation) {
        connect(simulation, &Simulation::Render, runtimeToolBar, &RuntimeToolBar::onElapsedTime);
        connect(runtimeToolBar, &RuntimeToolBar::startTriggered, [=]() {
            tacticalDisplay->canvas->simulation();
            simulation->start();
            qDebug() << "Simulation started.";
        });
        connect(runtimeToolBar, &RuntimeToolBar::nextStepTriggered, [=]() {
            //tacticalDisplay->canvas->simulation();
            simulation->nextStep();
            qDebug() << "Next Step Simulate.";
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
            if (!filePath.isEmpty() && runtime->recorder->loadReplay(filePath)) {
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
    // connect(RecentProjectsManager::instance(), &RecentProjectsManager::projectSelected,
    //         this, &RuntimeEditor::loadRecentProject);
    connect(RecentProjectsManager::instance(), &RecentProjectsManager::projectSelected,
            this, [=](const QString& filePath, RecentProjectsManager::EditorType type) {
                if (type == RecentProjectsManager::RuntimeEditor) {
                    loadRecentProject(filePath);
                }
            });
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
             QString displayName = capitalizeFirstLetter(name);
            for (Inspector* inspector : inspectors) {
                 if (type == "subcomponent") {
                     QJsonObject componentData = (*hierarchy->Components)[data["parentId"].toString().toStdString()]->getsubComponentData(data["ID"].toString().toStdString());

                     if (!componentData.isEmpty()) {
                         inspector->init(ID, displayName + "_sub", componentData);
                     }
                     //inspector->init(ID, displayName + "", (*hierarchy->Components)[data["ID"].toString().toStdString()]->toJson());
                 }else if (type == "component") {
                     QJsonObject componentData = hierarchy->getComponentData(ID, name);
                     if (!componentData.isEmpty()) {
                         inspector->init(ID, displayName, componentData);
                     }
                     // inspector->init(ID, displayName + "", (*hierarchy->Components)[data["ID"].toString().toStdString()]->toJson());
                 }  else if (type == "profile") {
                    inspector->init(ID, displayName + "_self", (hierarchy->ProfileCategories)[data["ID"].toString().toStdString()]->toJson());
                } else if (type == "folder") {
                    inspector->init(ID, displayName + "_self", (*hierarchy->Folders)[data["ID"].toString().toStdString()]->toJson());
                } else if (type == "entity") {
                    inspector->init(data["ID"].toString(), displayName + "_self", (*hierarchy->Entities)[data["ID"].toString().toStdString()]->toJson());
                    if (radarDisplayUI) {
                        radarDisplayUI->selectEntity((*hierarchy->Entities)[data["ID"].toString().toStdString()]);
                    }
                    // if (ewDisplayUI) {
                    //     ewDisplayUI->selectEntity((*hierarchy->Entities)[data["ID"].toString().toStdString()]);
                    // }
                    if (iffDisplayUI) {
                        iffDisplayUI->selectEntity((*hierarchy->Entities)[data["ID"].toString().toStdString()]);
                    }
                    if (radioDisplayUI) {
                        radioDisplayUI->selectEntity((*hierarchy->Entities)[data["ID"].toString().toStdString()]);
                    }
                    if (csmDisplayUI) {
                        csmDisplayUI->selectEntity((*hierarchy->Entities)[data["ID"].toString().toStdString()]);
                    }
                    if (esmDisplayUI) {
                        esmDisplayUI->selectEntity((*hierarchy->Entities)[data["ID"].toString().toStdString()]);
                    }
                } else {
                    inspector->init(ID, displayName, QJsonObject());
                }
            }
            if (!inspectorDock->isVisible()) {
                addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
                splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
                inspectorDock->show();
                qDebug() << "Inspector dock shown on item selection, geometry:" << inspectorDock->geometry();
            }
            // if (tacticalDisplay && type == "entity") {
            //     tacticalDisplay->selectedMesh(data["ID"].toString());
            //     Console::log("Entity selected: " + data["ID"].toString().toStdString());
            // }
            if (tacticalDisplay && type == "entity") {
                tacticalDisplay->selectedMesh(data["ID"].toString());
                // standardToolBar->getAddTrajectoryAction()->setEnabled(true);
                Console::log("Entity selected: " + data["ID"].toString().toStdString());
            } else {
                // standardToolBar->getAddTrajectoryAction()->setEnabled(false);
                Console::log("Non-entity selected, addTrajectoryAction disabled");
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

void RuntimeEditor::setupEnhancedDockWidgets()
{
    // Full dock features for complete movability - JUST LIKE SCENARIO EDITOR
    QDockWidget::DockWidgetFeatures fullDockFeatures =
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetFloatable;

    // Setup hierarchy dock with enhanced features
    hierarchyDock = new QDockWidget("Editor", this);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                   Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    hierarchyDock->setFeatures(fullDockFeatures);
    treeView = new HierarchyTree(this);
    hierarchyDock->setWidget(treeView);
    hierarchyDock->setMinimumWidth(150);
    hierarchyDock->setTitleBarWidget(nullptr); // Use default title bar
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    // Setup tactical display dock with enhanced features
    tacticalDisplayDock = new QDockWidget("Tactical Display", this);
    tacticalDisplayDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                         Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    tacticalDisplayDock->setFeatures(fullDockFeatures);
    tacticalDisplay = new TacticalDisplay(this);
    tacticalDisplayDock->setWidget(tacticalDisplay);
    tacticalDisplayDock->setMinimumWidth(300);
    tacticalDisplayDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::RightDockWidgetArea, tacticalDisplayDock);

    // Setup console dock with enhanced features - SIRF CONSOLE (NO TIMING GRAPH)
    consoleDock = new QDockWidget("Console", this);
    consoleDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                 Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    consoleDock->setFeatures(fullDockFeatures);
    consoleView = new ConsoleView(this);

    // SIRF CONSOLE VIEW SET KARO - NO TABS, NO TIMING GRAPH
    consoleDock->setWidget(consoleView);
    consoleDock->setMinimumHeight(100);
    consoleDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);
    consoleDock->hide();

    // Setup sidebar dock with enhanced features
    sidebarDock = new QDockWidget("Sidebar", this);
    sidebarDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                 Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    sidebarDock->setFeatures(fullDockFeatures);
    SidebarWidget *sidebar = new SidebarWidget(this);
    sidebarDock->setWidget(sidebar);
    sidebarDock->setMinimumWidth(80);
    sidebarDock->setMinimumHeight(40);
    sidebarDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::RightDockWidgetArea, sidebarDock);

    // Setup inspector dock with enhanced features
    inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                   Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    inspectorDock->setFeatures(fullDockFeatures);
    inspector = new Inspector(this);
    inspectorDock->setWidget(inspector);
    inspectorDock->setMinimumWidth(200);
    inspectorDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);

    // Setup library dock with enhanced features
    libraryDock = new QDockWidget("Library", this);
    libraryDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                 Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    libraryDock->setFeatures(fullDockFeatures);
    libTreeView = new HierarchyTree(this);
    libraryDock->setWidget(libTreeView);
    libraryDock->setMinimumWidth(200);
    libraryDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::RightDockWidgetArea, libraryDock);
    libraryDock->hide();

    // Setup text script dock with enhanced features
    textScriptDock = new QDockWidget("Test Script", this);
    textScriptDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                    Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    textScriptDock->setFeatures(fullDockFeatures);
    textScriptView = new TextScriptWidget(this);
    textScriptDock->setWidget(textScriptView);
    textScriptDock->setMinimumWidth(200);
    textScriptDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::RightDockWidgetArea, textScriptDock);
    textScriptDock->hide();

    // Setup display dock with enhanced features
    displayDock = new QDockWidget("Sensors", this);
    displayDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                 Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    displayDock->setFeatures(fullDockFeatures);
    displayDock->setMinimumWidth(200);
    displayDock->setTitleBarWidget(nullptr);

    // Setup display tabs will be done in constructor
    displayDock->hide();

    // Setup logger dock with enhanced features
    loggerDock = new QDockWidget("Logger", this);
    loggerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    loggerDock->setFeatures(fullDockFeatures);
    loggerDialog = new LoggerDialog(this);
    loggerDock->setWidget(loggerDialog);
    loggerDock->setMinimumWidth(200);
    loggerDock->setTitleBarWidget(nullptr);
    loggerDock->hide();

    // Connect dock visibility signals
    connect(hierarchyDock, &QDockWidget::visibilityChanged, this, &RuntimeEditor::onDockVisibilityChanged);
    connect(tacticalDisplayDock, &QDockWidget::visibilityChanged, this, &RuntimeEditor::onDockVisibilityChanged);
    connect(consoleDock, &QDockWidget::visibilityChanged, this, &RuntimeEditor::onDockVisibilityChanged);
    connect(inspectorDock, &QDockWidget::visibilityChanged, this, &RuntimeEditor::onDockVisibilityChanged);
    connect(libraryDock, &QDockWidget::visibilityChanged, this, &RuntimeEditor::onDockVisibilityChanged);
    connect(sidebarDock, &QDockWidget::visibilityChanged, this, &RuntimeEditor::onDockVisibilityChanged);
    connect(textScriptDock, &QDockWidget::visibilityChanged, this, &RuntimeEditor::onDockVisibilityChanged);
    connect(displayDock, &QDockWidget::visibilityChanged, this, &RuntimeEditor::onDockVisibilityChanged);
    connect(loggerDock, &QDockWidget::visibilityChanged, this, &RuntimeEditor::onDockVisibilityChanged);

    // Set tabified docking to allow tabbed interface when docks are stacked
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

    // Enable docking features
    setDockOptions(QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks |
                   QMainWindow::GroupedDragging |
                   QMainWindow::AnimatedDocks);


    setCentralWidget(nullptr);


    splitDockWidget(hierarchyDock, tacticalDisplayDock, Qt::Horizontal);
    splitDockWidget(tacticalDisplayDock, sidebarDock, Qt::Horizontal);
    splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
    splitDockWidget(tacticalDisplayDock, consoleDock, Qt::Vertical);


    QTimer::singleShot(100, this, [=]() {
        int totalWidth = width();
        int totalHeight = height();


        int hierarchyWidth = static_cast<int>(totalWidth * 0.10);   // 10% for hierarchy
        int tacticalWidth = static_cast<int>(totalWidth * 0.78);    // 78% for tactical display
        int sidebarWidth = static_cast<int>(totalWidth * 0.05);     // 5% for sidebar
        int inspectorWidth = static_cast<int>(totalWidth * 0.07);   // 7% for inspector
        int consoleHeight = static_cast<int>(totalHeight * 0.15);   // 15% for console

        // Resize docks
        resizeDocks({hierarchyDock}, {hierarchyWidth}, Qt::Horizontal);
        resizeDocks({tacticalDisplayDock}, {tacticalWidth}, Qt::Horizontal);
        resizeDocks({sidebarDock}, {sidebarWidth}, Qt::Horizontal);
        resizeDocks({inspectorDock}, {inspectorWidth}, Qt::Horizontal);
        resizeDocks({consoleDock}, {consoleHeight}, Qt::Vertical);
    });


    connect(sidebar, &SidebarWidget::viewSelected, this, [this](const QString &viewName) {
        qDebug() << "Sidebar viewSelected emitted, viewName:" << viewName;

        if (viewName == "Inspector") {
            qDebug() << "Showing Inspector dock";
            libraryDock->hide();
            textScriptDock->hide();
            displayDock->hide();
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
            displayDock->hide();
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
            displayDock->hide();
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
            qDebug() << "Showing Logger dock";
            inspectorDock->hide();
            libraryDock->hide();
            textScriptDock->hide();
            displayDock->hide();
            if (!loggerDock->isVisible()) {
                addDockWidget(Qt::RightDockWidgetArea, loggerDock);
                splitDockWidget(sidebarDock, loggerDock, Qt::Vertical);
                loggerDock->show();
                qDebug() << "Logger dock geometry:" << loggerDock->geometry();
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

void RuntimeEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures)
{
    // This method is replaced by setupEnhancedDockWidgets()
    setupEnhancedDockWidgets();
}

/* Handle dock visibility changes */
void RuntimeEditor::onDockVisibilityChanged(bool visible)
{
    QDockWidget* dock = qobject_cast<QDockWidget*>(sender());
    if (dock) {
        if (visible) {
            dock->raise(); // Bring to front when shown
        }
    }
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

// Logger
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


    QMenu *editMenu = menuBar->getEditMenu();
    if (editMenu) {
        // Create reset layout action
        QAction *resetLayoutAction = new QAction("Reset Layout", this);
        // resetLayoutAction->setShortcut(QKeySequence("Ctrl+R"));
        resetLayoutAction->setStatusTip("Reset all docks to initial positions");

        // Add separator and then reset layout action to Edit menu
        editMenu->addSeparator();
        editMenu->addAction(resetLayoutAction);

        // Connect the action
        connect(resetLayoutAction, &QAction::triggered, this, &RuntimeEditor::resetLayout);
    }
}

void RuntimeEditor::setupToolBars()
{
    designToolBar = new DesignToolBar(this);
    addToolBar(Qt::TopToolBarArea, designToolBar);

    runtimeToolBar = new RuntimeToolBar(this);
    addToolBar(Qt::TopToolBarArea, runtimeToolBar);

    addToolBarBreak(Qt::TopToolBarArea);

    networkToolBar = new NetworkToolbar(this);
    addToolBar(Qt::TopToolBarArea, networkToolBar);

    // Allow toolbars to be movable
    designToolBar->setMovable(true);
    runtimeToolBar->setMovable(true);
    networkToolBar->setMovable(true);
}

void RuntimeEditor::setupToolBarConnections()
{
    DesignToolBar *designToolBar = findChild<DesignToolBar*>();
    if (!designToolBar || !tacticalDisplay || !tacticalDisplay->canvas) {
        qWarning() << "Toolbar connection setup failed - required components missing";
        return;
    }

    // RECENT PROJECT CONNECTION
    connect(menuBar->getRecentProjectAction(), &QAction::triggered,
            this, &RuntimeEditor::onRecentProjectTriggered);

    qDebug() << "MenuBar actions connected successfully";
    qDebug() << "Recent Project Action:" << menuBar->getRecentProjectAction()->text();
    connect(menuBar, &MenuBar::profileTriggered,
            this, &RuntimeEditor::showProfileInfo);

    connect(menuBar, &MenuBar::applicationTriggered, this, &RuntimeEditor::showApplicationDialog);


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
    connect(designToolBar->getAddTrajectoryAction(), &QAction::triggered,
            this, [=]() {
                tacticalDisplay->canvas->setTrajectoryDrawingMode(true);
                Console::log("Add Trajectory action triggered from DesignToolBar");
            });
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
    newInspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                      Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    newInspectorDock->setFeatures(QDockWidget::DockWidgetClosable |
                                  QDockWidget::DockWidgetMovable |
                                  QDockWidget::DockWidgetFloatable);
    Inspector *newInspector = new Inspector(newInspectorDock);
    newInspectorDock->setWidget(newInspector);
    newInspectorDock->setMinimumWidth(200);
    newInspectorDock->setTitleBarWidget(nullptr);

    inspectorDocks.append(newInspectorDock);
    inspectors.append(newInspector);

    connect(newInspector, &Inspector::valueChanged,
            hierarchy, &Hierarchy::UpdateComponent);
    connect(newInspector, &Inspector::addTabRequested,
            this, &RuntimeEditor::addInspectorTab);

    // Connect dock visibility
    connect(newInspectorDock, &QDockWidget::visibilityChanged,
            this, &RuntimeEditor::onDockVisibilityChanged);

    if (inspectorDock->isVisible()) {
        splitDockWidget(inspectorDock, newInspectorDock, Qt::Horizontal);
    } else {
        addDockWidget(Qt::RightDockWidgetArea, newInspectorDock);
    }

    // Handle dock destruction
    connect(newInspectorDock, &QDockWidget::destroyed, this, [=]() {
        inspectorDocks.removeOne(newInspectorDock);
        inspectors.removeOne(newInspector);
    });

    newInspectorDock->show();
    newInspectorDock->raise();
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
    // Create loading dialog with indeterminate progress
    QProgressDialog* loadingDialog = new QProgressDialog("Loading data...", nullptr, 0, 0, this);
    loadingDialog->setWindowTitle("");
    loadingDialog->setWindowModality(Qt::WindowModal);
    loadingDialog->setCancelButton(nullptr); // No cancel button
    loadingDialog->setMinimumDuration(0); // Show immediately
    loadingDialog->setRange(0, 0); // Indeterminate mode
    loadingDialog->setValue(0);
    loadingDialog->show();

    // Force UI update
    QCoreApplication::processEvents();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open JSON file:" << filePath;
        QMessageBox::warning(this, "Error", QString("Failed to open JSON file: %1").arg(filePath));
        loadingDialog->deleteLater();
        return;
    }

    QByteArray data = file.readAll();
    file.close();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse JSON:" << err.errorString();
        QMessageBox::warning(this, "Error", QString("Failed to parse JSON: %1").arg(err.errorString()));
        loadingDialog->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("hierarchy")) {
        loadingDialog->setLabelText("Loading...");
        QCoreApplication::processEvents();

        QJsonObject hier = obj["hierarchy"].toObject();
        hierarchy->fromJson(hier);

        QCoreApplication::processEvents();

        qDebug() << "Hierarchy loaded from file:" << filePath;
        if (treeView && treeView->getTreeWidget()) {
            treeView->getTreeWidget()->update();
            qDebug() << "HierarchyTree updated after loading JSON";
        } else {
            qWarning() << "Failed to update HierarchyTree: treeView or treeWidget is null";
        }

        QCoreApplication::processEvents();
    } else {
        qWarning() << "JSON file does not contain 'hierarchy' key";
    }

    if (tacticalDisplay && obj.contains("tactical")) {
        loadingDialog->setLabelText("Loading tactical display...");
        QCoreApplication::processEvents();

        QJsonObject tac = obj["tactical"].toObject();
        tacticalDisplay->canvas->fromJson(tac);
        qDebug() << "TacticalDisplay loaded from file:" << filePath;
    } else {
        qWarning() << "JSON file does not contain 'tactical' key or tacticalDisplay is null";
    }

    lastSavedFilePath = filePath;
    clearUnsavedChanges();

    // Close loading dialog
    loadingDialog->close();
    loadingDialog->deleteLater();

    updateStatusBar("Project loaded: " + QFileInfo(filePath).fileName());
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


void RuntimeEditor::resetLayout()
{
    // Show message in console
    console->log("Resetting Runtime Editor layout to initial state...");

    // Close all additional inspector docks
    for (int i = inspectorDocks.size() - 1; i >= 0; --i) {
        QDockWidget* dock = inspectorDocks[i];
        if (dock != inspectorDock) { // Keep the main inspector
            inspectors.removeAt(i);
            dock->deleteLater();
        }
    }

    // Keep only main inspector
    if (inspectorDocks.size() > 1) {
        inspectorDocks = QList<QDockWidget*>{inspectorDock};
        inspectors = QList<Inspector*>{inspector};
    }
    inspectorCount = 0;

    // Hide all docks first
    hierarchyDock->hide();
    tacticalDisplayDock->hide();
    consoleDock->hide();
    inspectorDock->hide();
    libraryDock->hide();
    sidebarDock->hide();
    textScriptDock->hide();
    displayDock->hide();
    loggerDock->hide();

    // Remove all docks from main window
    removeDockWidget(hierarchyDock);
    removeDockWidget(tacticalDisplayDock);
    removeDockWidget(consoleDock);
    removeDockWidget(inspectorDock);
    removeDockWidget(libraryDock);
    removeDockWidget(sidebarDock);
    removeDockWidget(textScriptDock);
    removeDockWidget(displayDock);
    removeDockWidget(loggerDock);

    // Add docks back to initial positions
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);
    addDockWidget(Qt::RightDockWidgetArea, tacticalDisplayDock);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);
    addDockWidget(Qt::RightDockWidgetArea, sidebarDock);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    addDockWidget(Qt::RightDockWidgetArea, libraryDock);
    addDockWidget(Qt::RightDockWidgetArea, textScriptDock);
    addDockWidget(Qt::RightDockWidgetArea, displayDock);
    addDockWidget(Qt::RightDockWidgetArea, loggerDock);

    // Hide additional docks by default
    libraryDock->hide();
    textScriptDock->hide();
    displayDock->hide();
    loggerDock->hide();

    // Recreate initial split configuration
    splitDockWidget(hierarchyDock, tacticalDisplayDock, Qt::Horizontal);
    splitDockWidget(tacticalDisplayDock, sidebarDock, Qt::Horizontal);
    splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
    splitDockWidget(tacticalDisplayDock, consoleDock, Qt::Vertical);

    // Show main docks
    hierarchyDock->show();
    tacticalDisplayDock->show();
    consoleDock->show();
    sidebarDock->show();
    inspectorDock->show();

    // Reset to initial sizes with a small delay
    QTimer::singleShot(100, this, [=]() {
        QMainWindow::resize(1100, 600); // Reset window size

        int totalWidth = this->width();
        int totalHeight = this->height();

        // Same distribution as initial setup
        int hierarchyWidth = static_cast<int>(totalWidth * 0.10);   // 10% for hierarchy
        int tacticalWidth = static_cast<int>(totalWidth * 0.78);    // 78% for tactical display
        int sidebarWidth = static_cast<int>(totalWidth * 0.05);     // 5% for sidebar
        int inspectorWidth = static_cast<int>(totalWidth * 0.07);   // 7% for inspector
        int consoleHeight = static_cast<int>(totalHeight * 0.15);   // 15% for console

        // Resize docks
        resizeDocks({hierarchyDock}, {hierarchyWidth}, Qt::Horizontal);
        resizeDocks({tacticalDisplayDock}, {tacticalWidth}, Qt::Horizontal);
        resizeDocks({sidebarDock}, {sidebarWidth}, Qt::Horizontal);
        resizeDocks({inspectorDock}, {inspectorWidth}, Qt::Horizontal);
        resizeDocks({consoleDock}, {consoleHeight}, Qt::Vertical);

        // Bring all docks to front
        hierarchyDock->raise();
        tacticalDisplayDock->raise();
        consoleDock->raise();
        sidebarDock->raise();
        inspectorDock->raise();

        updateStatusBar("Runtime Editor layout reset to initial state");
        console->log("Runtime Editor layout successfully reset to initial configuration");
    });
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


void RuntimeEditor::loadRecentProject(const QString& filePath)
{
    // Create loading dialog with indeterminate progress
    QProgressDialog* loadingDialog = new QProgressDialog("Loading data...", nullptr, 0, 0, this);
    loadingDialog->setWindowTitle("");
    loadingDialog->setWindowModality(Qt::WindowModal);
    loadingDialog->setCancelButton(nullptr); // No cancel button
    loadingDialog->setMinimumDuration(0); // Show immediately
    loadingDialog->setRange(0, 0); // Indeterminate mode
    loadingDialog->setValue(0);
    loadingDialog->show();

    // Force UI update
    QCoreApplication::processEvents();

    if (!QFile::exists(filePath)) {
        QMessageBox::warning(this, "File Not Found",
                             "The runtime project file was not found");
        loadingDialog->deleteLater();
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Failed to open runtime file");
        loadingDialog->deleteLater();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, "Error", "Invalid runtime file format");
        loadingDialog->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("hierarchy")) {
        QMessageBox::warning(this, "Error", "Not a valid runtime file");
        loadingDialog->deleteLater();
        return;
    }

    loadingDialog->setLabelText("Loading...");
    QCoreApplication::processEvents();

    // Load hierarchy
    QJsonObject hier = obj["hierarchy"].toObject();
    hierarchy->fromJson(hier);

    loadingDialog->setLabelText("Loading tactical display...");
    QCoreApplication::processEvents();

    // Load tactical display if present
    if (tacticalDisplay && obj.contains("tactical")) {
        QJsonObject tac = obj["tactical"].toObject();
        tacticalDisplay->canvas->fromJson(tac);
    }

    lastSavedFilePath = filePath;
    clearUnsavedChanges();

    // Add to RuntimeEditor-specific recent projects
    RecentProjectsManager::instance()->addToRecentProjects(filePath,
                                                           RecentProjectsManager::RuntimeEditor);

    // Close loading dialog
    loadingDialog->close();
    loadingDialog->deleteLater();

    updateStatusBar("Runtime loaded: " + QFileInfo(filePath).fileName());
    console->log("Runtime project loaded: " + filePath.toStdString());
}
void RuntimeEditor::onRecentProjectTriggered()
{
    RecentProjectsManager::instance()->showRecentProjectsMenu(this,
                                                              RecentProjectsManager::RuntimeEditor);
}

// void RuntimeEditor::clearRecentProjects()
// {
//     RecentProjectsManager::instance()->clearRecentProjects();
//     updateStatusBar("Recent projects list cleared");
//     console->log("Recent projects list cleared");
// }

void RuntimeEditor::showProfileInfo()
{
    ProfileInfoDialog::showProfileInfo(this);
}

void RuntimeEditor::showApplicationDialog()
{
    ApplicationDialog dialog(this);
    connect(&dialog,&ApplicationDialog::fpsState,simulation,&Simulation::setFps);
    connect(&dialog,&ApplicationDialog::canvasIconState,tacticalDisplay->canvas,&CanvasWidget::setImageScale);
    dialog.exec();


}
