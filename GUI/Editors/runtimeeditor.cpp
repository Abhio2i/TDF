/* ========================================================================= */
/* File: runtimeeditor.cpp                                                   */
/* Purpose: Implements runtime editor for simulation and visualization       */
/* ========================================================================= */

#include "runtimeeditor.h"                      // For runtime editor class
#include "GUI/Menubars/menubar.h"               // For menu bar
#include "GUI/Panel/radardisplay.h"             // For radar display
#include "GUI/Sidebar/sidebarwidget.h"          // For sidebar widget
#include "GUI/Timing/graphwidget.h"             // For timing graph widget
#include "GUI/Toolbars/standardtoolbar.h"       // For standard toolbar
#include "GUI/Toolbars/networktoolbar.h"        // For network toolbar
#include "GUI/Feedback/projectinformation.h"    // For feedback window
#include "qthread.h"                            // For thread operations
#include <QFile>                                // For file operations
#include <QTextStream>                          // For text streaming
#include <QDebug>                               // For debug output
#include <QListWidget>                          // For list widget
#include <QDockWidget>                          // For dock widget
#include <core/structure/runtime.h>             // For runtime structure
#include <core/structure/scenario.h>            // For scenario structure
#include <core/Hierarchy/Components/transform.h> // For transform component
#include <core/Hierarchy/Components/mesh.h>     // For mesh component
#include <QSplitter>                            // For splitter widget
#include <QFileDialog>                          // For file dialogs
#include <QStandardPaths>                       // For standard paths
#include <QJsonDocument>                        // For JSON document handling
#include <QJsonParseError>                      // For JSON parse errors
#include <QMessageBox>                          // For message box
#include <QApplication>                         // For application instance
#include <QGuiApplication>                      // For GUI application
#include <QScreen>                              // For screen information
#include <QVBoxLayout>                          // For vertical layout
#include <QTimer>                               // For timer operations
#include <GUI/Menubars/profileinfodialog.h>     // For profile info dialog
#include <GUI/Editors/recentprojectsmanager.h>  // For recent projects manager
#include <GUI/Settings/applicationdialog.h>     // For application settings dialog
#include <QProgressDialog>                      // For progress indication
#include <GUI/Editors/customresizableoverlaydock.h>
#include "core/Hierarchy/EntityProfiles/SensorProfiles/sonar.h"

QJsonObject RuntimeEditor::s_missionData;
QString     RuntimeEditor::s_missionFilePath;
// %%% String Utility Function %%%
/* Capitalize the first letter of a string */
static QString capitalizeFirstLetter(const QString &str)
{
    if (str.isEmpty()) return str;
    return str[0].toUpper() + str.mid(1);
}
// %%% Timing Data Getter %%%
/* Get timing JSON data */
QString RuntimeEditor::getTimingJsonData() const
{
    return "{}";
}
// %%% Constructor %%%
/* Initialize runtime editor */
RuntimeEditor::RuntimeEditor(QWidget *parent)
    : QMainWindow(parent)
{
    m_scenarioConfig = new ScenarioConfig(this);
    setWindowTitle("Runtime Editor");
    resize(1100, 600);
    setupEnhancedDockWidgets();
    // setupMenuBar();
    // connect(menuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
    setupToolBars();
    runtime = new Runtime();
    hierarchy = runtime->hierarchy;
    SceneRenderer *renderer = runtime->scenerenderer;
    simulation = runtime->simulation;
    scriptengine = runtime->scriptengine;
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
    connect(scriptengine, &ScriptEngine::requestSidebarView, this, &RuntimeEditor::triggerSidebarView);
    connect(scriptengine, &ScriptEngine::requestDisplayTab, this, &RuntimeEditor::triggerDisplayTab);
    connect(scriptengine, &ScriptEngine::requestSelectEntity, this, [=](const QString &entityId) {
        // Select entity in tree (visual highlight)
        treeView->selectEntityById(entityId);
        if (hierarchy->Entities->count(entityId.toStdString())) {
            Entity* entity = (*hierarchy->Entities)[entityId.toStdString()];
            if (radarDisplayUI) {
                radarDisplayUI->selectEntity(entity);
            }
            if (iffDisplayUI) {
                iffDisplayUI->selectEntity(entity);
            }
            if (radioDisplayUI) {
                radioDisplayUI->selectEntity(entity);
            }
            if (csmDisplayUI) {
                csmDisplayUI->selectEntity(entity);
            }
            if (esmDisplayUI) {
                esmDisplayUI->selectEntity(entity);
            }
        }
    });
    static bool canvasScreenshotTaken = false;
    static int radarScreenshotCount = 0;
    connect(textScriptView, &TextScriptWidget::runScriptFile,
            this, &RuntimeEditor::onRunScriptFileRequested);
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
        // Setup display tabs
    displayTabs = new QTabWidget(displayDock);
    displayDock->setWidget(displayTabs);
    radarDisplayUI = new RadarDisplay(displayTabs);
    radarDisplayUI->setHierarchy(hierarchy);
    displayTabs->addTab(radarDisplayUI, "Radar");
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

    sonarDisplayUI = new SonarDisplay(displayTabs);
    displayTabs->addTab(sonarDisplayUI, "SONAR");

    displayTabs->setCurrentIndex(0);
    // Connect simulation updates to displays
    connect(simulation, &Simulation::Update, radarDisplayUI, &RadarDisplay::updateRadar);
    connect(hierarchy, &Hierarchy::entityRemoved, radarDisplayUI, &RadarDisplay::RemoveEntity);
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
    connect(simulation, &Simulation::Update, sonarDisplayUI, &SonarDisplay::updateRadar);  // add by amjad
    // ── Sonar scan loop ──
    connect(simulation, &Simulation::Update, this, [=]() {

        // qDebug() << "Sonar Update triggered";

        // if (!sonarDisplayUI) return;

        // // Timer initialize
        // static bool timerStarted = false;
        // if (!timerStarted) {
        //     m_sonarTimer.start();

        //     timerStarted = true;
        // }

        // float simTime = m_sonarTimer.elapsed() / 1000.0f;

        // // Ping interval check
        // if (!m_activeSonar.canPing(simTime)) return;

        // // ── Entity position set karo ──
        // // selected entity se lat/lon baad me aayega
        // // ── Sonar config ──
        // double sonarLat = -9.23;
        // double sonarLon =  77.99;
        // m_activeSonar.setEntityPosition(sonarLat, sonarLon);
        // m_activeSonar.setHeading(0.0f);
        // m_activeSonar.setMaxRange(100000.0f);   // 100 km — test ke liye
        // m_activeSonar.setBeamWidth(360.0f);     // 360° — omnidirectional test
        // m_activeSonar.setPingInterval(5.0f);    // 3 sec — fast ping test
        // m_activeSonar.setFalseDetectionRate(0.0f); //  false contacts band
        // m_activeSonar.setSoundSpeed(1531.0f);
        // m_activeSonar.setMaxDepth(500.0f);

        // if (sonarDisplayUI)
        // {
        //     sonarDisplayUI->setHeading(0.0f);
        //     sonarDisplayUI->setBeamWidth(360.0f); // ya jo bhi set hai
        // }

        // // ── Hierarchy se underwater targets collect karo ──
        // // ── Platforms se targets collect karo (directly) ──
        // std::vector<SonarTarget> targets;

        // if (hierarchy && hierarchy->Entities)
        // {
        //     std::string sonarEntityId = "sonar_entity_id";

        //     for (auto& [id, entity] : *hierarchy->Entities)
        //     {
        //         if (!entity) continue;

        //         // ── Sonar entity khud ko skip kare — by ID ──
        //         if (id == sonarEntityId) continue;

        //         QJsonObject transformJson = entity->getComponent("transform");
        //         if (transformJson.isEmpty()) continue;

        //         QJsonObject geocord = transformJson["geocord"].toObject();
        //         if (geocord.isEmpty()) continue;

        //         double lat      = geocord["latitude"].toDouble();
        //         double lon      = geocord["longitude"].toDouble();
        //         double altitude = geocord["altitude"].toDouble();  // ← sirf ek baar

        //         if (lat == 0.0 && lon == 0.0) continue;

        //         QJsonObject entityJson = entity->toJson();
        //         QString     entityType = entityJson["Type"].toString();

        //         if (entityType.contains("Aircraft",   Qt::CaseInsensitive) ||
        //             entityType.contains("Helicopter", Qt::CaseInsensitive) ||
        //             entityType.contains("UAV",        Qt::CaseInsensitive) ||
        //             entityType.contains("Airplane",   Qt::CaseInsensitive))
        //             continue;

        //         SonarTarget t;
        //         t.name           = entity->Name;
        //         t.lat            = lat;
        //         t.lon            = lon;
        //         t.depth          = (altitude < 0) ? (float)(-altitude) : 0.0f; // ← geocord se
        //         t.targetStrength = 20.0f;

        //         if (entityType.contains("Submarine",  Qt::CaseInsensitive))
        //             t.targetStrength = 25.0f;
        //         if (entityType.contains("Ship",       Qt::CaseInsensitive) ||
        //             entityType.contains("Destroyer",  Qt::CaseInsensitive) ||
        //             entityType.contains("Frigate",    Qt::CaseInsensitive))
        //             t.targetStrength = 15.0f;

        //         targets.push_back(t);
        //     }
        // }

        // qDebug() << "Total targets:" << targets.size();

        // if (targets.empty()) return;

        // // ── Acoustic config ──
        // SonarInput input;
        // input.sourceLevel        = 220.0f;
        // input.noiseLevel         = 30.0f;
        // input.detectionThreshold = 10.0f;
        // input.absorption         = 0.0001f;
        // input.targetStrength     = 0.0f;

        // // ── ONE ping → scan all → UI feed ──
        // std::vector<DetectionResult> results =
        //     m_activeSonar.scan(targets, input);

        // sonarDisplayUI->updateContacts(results);

        if (!sonarDisplayUI || !hierarchy) return;
        if (!hierarchy->Sensors) return;

        for (auto& [id, sensor] : *hierarchy->Sensors)
        {
            Sonar* sonar = dynamic_cast<Sonar*>(sensor);
            if (!sonar) continue;

            sonar->scan();

            sonarDisplayUI->updateContacts(sonar->getLastResults());
        }
    });
    connect(hierarchy, &Hierarchy::entityRemoved, sonarDisplayUI, &SonarDisplay::RemoveEntity);     // add by amjad

    connect(displayDock, &QDockWidget::visibilityChanged, this, [=](bool visible) {
        if (!visible) {
            if (runtimeToolBar) {
                QAction* radarToggle = runtimeToolBar->findChild<QAction*>("radarToggleAction");
                if (radarToggle) {
                    radarToggle->setChecked(false);
                }
            }
        }
    });
    displayDock->hide();
    connect(hierarchy, &Hierarchy::Init, runtimeToolBar, &RuntimeToolBar::Init);
    connect(runtimeToolBar, &RuntimeToolBar::radarDisplayToggled, this, &RuntimeEditor::toggleRadarDisplay);
    connect(loggerDialog, &LoggerDialog::eventTypesSelected, this, [=](const QStringList &eventTypes) {
    });
    connect(displayWindow, &QObject::destroyed, this, [=]() {
        displayWindow = nullptr;
        radarDisplayUI = nullptr;
        iffDisplayUI = nullptr;
        esmDisplayUI = nullptr;
        csmDisplayUI = nullptr;
        if (runtimeToolBar) {
            QAction* radarToggle = runtimeToolBar->findChild<QAction*>("radarToggleAction");
            if (radarToggle) {
                radarToggle->setChecked(false);
            }
        }
    });
    connect(loggerDialog, &LoggerDialog::eventTypesSelected, this, [=](const QStringList &eventTypes) {

    });
    connect(displayWindow, &QObject::destroyed, this, [=]() {
        displayWindow = nullptr;
        radarDisplayUI = nullptr;
        esmDisplayUI = nullptr;
        csmDisplayUI = nullptr;
        if (runtimeToolBar) {
            QAction* radarToggle = runtimeToolBar->findChild<QAction*>("radarToggleAction");
            if (radarToggle) {
                radarToggle->setChecked(false);
            }
        }
    });

    connect(simulation, &Simulation::Update, this, [=]() {
        if (iffDisplayUI && iffDisplayUI->entity && iffDisplayUI->iff) {
            iffDisplayUI->iff->interrogateTargets(iffDisplayUI->entity->transform);
        }
    });
    if (tacticalDisplay && tacticalDisplay->canvas && tacticalDisplay->mapWidget) {
        scriptengine->setCanvas(tacticalDisplay->canvas);
        scriptengine->setGIS(tacticalDisplay->mapWidget);
        connect(tacticalDisplay->canvas, &CanvasWidget::trajectoryUpdated,
                inspector, [=](QString entityId, QJsonArray waypoints) {
                    if (inspector->getName() == "trajectory" &&
                        inspector->getConnectedID() == entityId) {
                        inspector->updateTrajectory(entityId, waypoints);
                    }
                });
        connect(tacticalDisplay->canvas, &CanvasWidget::trajectoryUpdated, this, [=](QString entityId, QJsonArray /*waypoints*/) {
            auto it = tacticalDisplay->canvas->Meshes.find(entityId.toStdString());
            if (it != tacticalDisplay->canvas->Meshes.end() && it->second.trajectory) {
                QJsonObject trajData = it->second.trajectory->toJson();
                hierarchy->UpdateComponent(entityId, "Trajectory", trajData);
                treeView->getTreeWidget()->update();
                markUnsavedChanges();
            } else {
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
        // Wire canvas to simulation so weapon blast effects are rendered
        simulation->setCanvas(tacticalDisplay->canvas);
        connect(simulation, &Simulation::Render, runtimeToolBar, &RuntimeToolBar::onElapsedTime);
        connect(runtimeToolBar, &RuntimeToolBar::startTriggered, [=]() {
            if (runtimeToolBar->getSnapshot().isEmpty()) {
                QJsonObject currentState = hierarchy->toJson();
                runtimeToolBar->storeSnapshot(currentState);
            }
            tacticalDisplay->canvas->simulation();
            simulation->start();
        });
        connect(runtimeToolBar, &RuntimeToolBar::nextStepTriggered, [=]() {
            simulation->nextStep();
        });
        connect(runtimeToolBar, &RuntimeToolBar::pauseTriggered, this, [=]() {
            tacticalDisplay->canvas->editor();
            simulation->pause();
        });
        connect(runtimeToolBar, &RuntimeToolBar::resetTriggered, this, [=]() {
            simulation->stop();
            tacticalDisplay->canvas->editor();
            if (!runtimeToolBar->m_initialSnapshot.isEmpty()) {
                hierarchy->fromJson(runtimeToolBar->m_initialSnapshot);
                if (tacticalDisplay && tacticalDisplay->canvas) {
                    tacticalDisplay->canvas->ReInit();
                }
                if (treeView && treeView->getTreeWidget()) {
                    treeView->getTreeWidget()->update();
                }
            }
        });
        connect(simulation, &Simulation::sendMode, this, [=](SimulationStateNS::State state) {
            if (runtimeToolBar) {
                switch(state) {
                case SimulationStateNS::START:
                    runtimeToolBar->highlightAction(runtimeToolBar->startAction);
                    runtimeToolBar->setSimulationState(RuntimeToolBar::RUNNING);
                    break;
                case SimulationStateNS::PAUSE:
                    runtimeToolBar->highlightAction(runtimeToolBar->pauseAction);
                    runtimeToolBar->setSimulationState(RuntimeToolBar::PAUSED);
                    break;
                case SimulationStateNS::STOP:
                    runtimeToolBar->highlightAction(runtimeToolBar->stopAction);
                    runtimeToolBar->setSimulationState(RuntimeToolBar::STOPPED);
                    break;
                default:
                    break;
                }
            }
        });
        connect(runtimeToolBar, &RuntimeToolBar::speedChanged, simulation, &Simulation::setSpeed);
        connect(runtimeToolBar, &RuntimeToolBar::timeChanged, simulation, &Simulation::timeJump);
        connect(runtimeToolBar, &RuntimeToolBar::speedChanged, this, [=](int speed) {
            float moveSpeed = static_cast<float>(speed);
            for (auto& [id, comp] : simulation->physicsComponent) {
                if (comp.dynamicModel) {
                    comp.dynamicModel->setMoveSpeed(moveSpeed);
                }
            }

        });
        networkToolBar->setNetworkManager(networkManager);
    } else {

    }
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

    HierarchyConnector::instance()->connectSignals(hierarchy,library, treeView, tacticalDisplay, inspector);
    HierarchyConnector::instance()->connectLibrarySignals(library, libTreeView);
    HierarchyConnector::instance()->initializeDummyData(hierarchy);
    HierarchyConnector::instance()->initializeLibraryData(library);
    HierarchyConnector::instance()->setupFileOperations(this, hierarchy, tacticalDisplay);
    if (tacticalDisplay && tacticalDisplay->canvas) {
        connect(tacticalDisplay->canvas, &CanvasWidget::selectEntitybyCursor,
                treeView, &HierarchyTree::selectEntityById);

        connect(tacticalDisplay->canvas, &CanvasWidget::selectEntitybyCursor,
                this, [=](const QString& entityId) {
                    if (!hierarchy->Entities) return;
                    auto it = hierarchy->Entities->find(entityId.toStdString());
                    if (it == hierarchy->Entities->end()) return;

                    Entity* entity = it->second;
                    QString entityName = QString::fromStdString(entity->Name);
                    QString displayName = capitalizeFirstLetter(entityName);
                    if (displayDock) displayDock->setWindowTitle("Sensors - " + displayName);
                    if (!inspectorDock->isLocked()) {
                        for (Inspector* insp : inspectors) {
                            insp->init(entityId, displayName + "_self",
                                       (*hierarchy->Entities)[entityId.toStdString()]->toJson());
                        }
                    }
                    if (!displayDock || !displayDock->isLocked()) {
                        if (displayDock) displayDock->setWindowTitle("Sensors - " + displayName);
                        if (radarDisplayUI) radarDisplayUI->selectEntity(entity);
                        if (iffDisplayUI)   iffDisplayUI->selectEntity(entity);
                        if (radioDisplayUI) radioDisplayUI->selectEntity(entity);
                        if (csmDisplayUI)   csmDisplayUI->selectEntity(entity);
                        if (esmDisplayUI)   esmDisplayUI->selectEntity(entity);
                    }
                });
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
        // ===== FORMATION MULTI-SELECT LOGIC =====
        if (type == "entity") {
            QString entityID = data["ID"].toString();

            // Check if this entity is a Formation
            if (hierarchy && hierarchy->Entities &&
                hierarchy->Entities->find(entityID.toStdString()) != hierarchy->Entities->end()) {

                Entity* entity = (*hierarchy->Entities)[entityID.toStdString()];
                Formation* formation = dynamic_cast<Formation*>(entity);

                if (formation) {
                    QList<QString> formationEntityIds;
                    formationEntityIds.append(entityID);

                    // Add mothership
                    if (formation->mothership && formation->mothership->entity) {
                        QString mothershipId = QString::fromStdString(formation->mothership->entity->ID);
                        if (mothershipId != "dummy" && !mothershipId.isEmpty()) {
                            formationEntityIds.append(mothershipId);
                        }
                    }

                    // Add all allies
                    if (formation->formationPositions) {
                        for (const auto& pair : *formation->formationPositions) {
                            FormationPosition* pos = pair.second;
                            if (pos && pos->entity) {
                                QString allyId = QString::fromStdString(pos->entity->ID);
                                if (allyId != "dummy" && !allyId.isEmpty()) {
                                    formationEntityIds.append(allyId);
                                }
                            }
                        }
                    }

                    Console::log("Formation selected with " +
                                 std::to_string(formationEntityIds.size()) + " entities");

                    // Use HierarchyTree's method to select all
                    treeView->selectMultipleEntitiesInTree(formationEntityIds);

                    // Also select in tactical display
                    if (tacticalDisplay && tacticalDisplay->canvas) {
                        tacticalDisplay->canvas->selectMultipleEntities(formationEntityIds);
                    }

                    return; // Don't proceed with single selection
                }
            }
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
            }else if (type == "component") {
                QJsonObject componentData = hierarchy->getComponentData(ID, name);
                if (!componentData.isEmpty()) {
                    inspector->init(ID, displayName, componentData);
                }
            }  else if (type == "profile") {
                inspector->init(ID, displayName + "_self", (hierarchy->ProfileCategories)[data["ID"].toString().toStdString()]->toJson());
            } else if (type == "folder") {
                inspector->init(ID, displayName + "_self", (*hierarchy->Folders)[data["ID"].toString().toStdString()]->toJson());
            } else if (type == "entity") {
                inspector->init(data["ID"].toString(), displayName + "_self", (*hierarchy->Entities)[data["ID"].toString().toStdString()]->toJson());

                if (!displayDock || !displayDock->isLocked()) {
                    if (displayDock) displayDock->setWindowTitle("Sensors - " + displayName);

                    if (radarDisplayUI)
                        radarDisplayUI->selectEntity((*hierarchy->Entities)[data["ID"].toString().toStdString()]);
                    if (iffDisplayUI)
                        iffDisplayUI->selectEntity((*hierarchy->Entities)[data["ID"].toString().toStdString()]);
                    if (radioDisplayUI)
                        radioDisplayUI->selectEntity((*hierarchy->Entities)[data["ID"].toString().toStdString()]);
                    if (csmDisplayUI)
                        csmDisplayUI->selectEntity((*hierarchy->Entities)[data["ID"].toString().toStdString()]);
                    if (esmDisplayUI)
                        esmDisplayUI->selectEntity((*hierarchy->Entities)[data["ID"].toString().toStdString()]);
                }
            } else {
                inspector->init(ID, displayName, QJsonObject());
            }
        }

        if (!inspectorDock->isVisible()) {
            // Hide other right panel docks (but NOT sensors)
            if (libraryDock && libraryDock->isVisible()) {
                libraryDock->hide();
            }
            if (textScriptDock && textScriptDock->isVisible()) {
                textScriptDock->hide();
            }
            if (loggerDock && loggerDock->isVisible()) {
                loggerDock->hide();
            }

            QRect sGeo = sidebarDock->geometry();
            if (displayDock && displayDock->isVisible()) {
                QRect displayGeo = displayDock->geometry();
                inspectorDock->setGeometry(displayGeo);
                inspectorDock->show();
            } else {
                inspectorDock->setGeometry(sGeo.x(), sGeo.y() + sGeo.height() + 5,
                                           sGeo.width(), height() - sGeo.y() - sGeo.height() - 150);
                inspectorDock->show();
                inspectorDock->raise();
            }
        } else {
            if (displayDock && displayDock->isVisible()) {
                displayDock->raise();
            } else {
                inspectorDock->raise();
            }
        }
        if (tacticalDisplay && type == "entity") {
            tacticalDisplay->selectedMesh(data["ID"].toString());
        } else {
        }
    });
    connect(treeView, &HierarchyTree::itemsSelected, this, [=](QList<QVariantMap> dataList) {
        QList<QString> entityIds;

        for (const QVariantMap& data : dataList) {
            QString type;
            if (data["type"].type() == QVariant::Map) {
                QVariantMap typeData = data["type"].toMap();
                if (typeData.contains("type") && typeData["type"].toString() == "option") {
                    type = "profile";
                }
            } else {
                type = data["type"].toString();
            }
            if (type == "entity") {
                entityIds.append(data["ID"].toString());
            }
        }
        if (tacticalDisplay && tacticalDisplay->canvas) {
            if (entityIds.isEmpty()) {
                tacticalDisplay->canvas->clearSelection();
            } else {
                tacticalDisplay->canvas->selectMultipleEntities(entityIds);
            }
        }
    });
    connect(libTreeView, &HierarchyTree::itemSelected, this, &RuntimeEditor::onLibraryItemSelected);
    connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
    connect(inspector, &Inspector::addTabRequested, this, &RuntimeEditor::addInspectorTab);
    inspectorDocks.append(inspectorDock);
    inspectors.append(inspector);
    inspector->setHierarchy(hierarchy);
    setupToolBarConnections();
    connect(hierarchy, &Hierarchy::profileAdded, this, &RuntimeEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderAdded, this, &RuntimeEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityAdded, this, &RuntimeEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::componentAdded, this, &RuntimeEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::subComponentAdded, this, &RuntimeEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::profileRemoved, this, &RuntimeEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderRemoved, this, &RuntimeEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityRemoved, this, &RuntimeEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::componentRemoved, this, &RuntimeEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::subComponentRemoved, this, &RuntimeEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::profileRenamed, this, &RuntimeEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderRenamed, this, &RuntimeEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityRenamed, this, &RuntimeEditor::markUnsavedChanges);


    //==============================================scriptengine(Raj &Amjad)========================================
    connect(scriptengine, &ScriptEngine::requestSensorScreenshot, this, [=](const QString &filePath) mutable {
        if (!displayTabs) {
            qWarning() << "displayTabs not available for screenshot";
            return;
        }
        if (!displayDock || !displayDock->isVisible()) {
            qWarning() << "Sensor display dock is not visible";
            return;
        }
        QWidget *currentDisplay = displayTabs->currentWidget();
        if (!currentDisplay || !currentDisplay->isVisible()) {
            qWarning() << "No sensor display is currently visible";
            return;
        }

        // ✅ CREATE PROPER DIRECTORY PATHS
        QString sensorDir = QDir::tempPath() + "/sensor_screenshots";
        QString canvasDir = QDir::tempPath() + "/gis_reports";
        QDir().mkpath(sensorDir);
        QDir().mkpath(canvasDir);

        QString tabName = displayTabs->tabText(displayTabs->currentIndex());
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");


        QString sensorPath = sensorDir + "/" + timestamp + "_" + tabName + "_" + QString::number(radarScreenshotCount) + ".png";
        QPixmap sensorScreenshot = currentDisplay->grab();

        if (sensorScreenshot.save(sensorPath)) {
            radarScreenshotCount++;
            qDebug() << "✅ Captured" << tabName << "display screenshot #" << radarScreenshotCount << ":" << sensorPath;

            // LOG SENSOR SCREENSHOT TO PDF
            scriptengine->logEvent(
                ReportCategory::SIMULATION,
                "Sensor Display Captured",
                tabName + " Display",
                "",
                "Display Screenshot #" + QString::number(radarScreenshotCount),
                "Screenshot Saved",
                "SUCCESS",
                "",
                sensorPath
                );
        }

        if (!canvasScreenshotTaken && filePath.contains("detection", Qt::CaseInsensitive)) {
            if (tacticalDisplay && tacticalDisplay->canvas) {
                QWidget* canvasContainer = tacticalDisplay->canvas->parentWidget();
                if (canvasContainer) {
                    // Force render
                    tacticalDisplay->canvas->update();
                    if (tacticalDisplay->canvas->gislib) {
                        tacticalDisplay->canvas->gislib->update();
                    }
                    QCoreApplication::processEvents();
                    QThread::msleep(150);
                    QCoreApplication::processEvents();
                    // Capture canvas + map
                    QPixmap canvasScreenshot = canvasContainer->grab();
                    QString canvasPath = canvasDir + "/" + timestamp + "_tactical_canvas_CONDITION_REACHED.png";
                    if (canvasScreenshot.save(canvasPath)) {
                        canvasScreenshotTaken = true;
                        // LOG CANVAS SCREENSHOT TO PDF
                        scriptengine->logEvent(
                            ReportCategory::SIMULATION,
                            "Tactical Canvas Captured",
                            "Tactical Display",
                            "",
                            "Canvas Screenshot (Condition Reached)",
                            "Screenshot Saved",
                            "SUCCESS",
                            "85% Detection Threshold Reached",
                            canvasPath
                            );
                    }
                }
            }
        }
    });

    //=================================================================================================
    //===================Logger (Himanshu)==========================================
    connect(runtimeToolBar, &RuntimeToolBar::loggerTriggered, this, &RuntimeEditor::toggleLoggerDisplay);
    connect(loggerDock, &QDockWidget::visibilityChanged, this, [=](bool visible) {
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


    // Logger connections
    connect(loggerDialog, &LoggerDialog::getRecorder, this, [this]{
        loggerDialog->recorder = runtime->recorder;
    });
    //Logger mode changing Command
    connect(loggerDialog, &LoggerDialog::loggerModeSend,
            runtime->recorder, &Recorder::loggerModeCheck);


    //Recorder Information Connection Start
    //Logger Update Recorder Information Recorder =>
    //M
    connect(runtime->recorder, &Recorder::recorderInfoSendOnce,
            loggerDialog, &LoggerDialog::recorderInfoReceiveOnce);

    connect(runtime->recorder, &Recorder::recorderInfoSend,
            loggerDialog, &LoggerDialog::recorderInfoReceive);

    // connect(runtime->recorder, &Recorder::recorderInfoSendOnce,
    //         loggerDialog, &LoggerDialog::recorderInfoReceiveOnce);

    //2nd one for Whole without Date and
    connect(runtime->recorder, &Recorder::recorderInfoSendUsual,
            loggerDialog, &LoggerDialog::recorderInfoReceiveUsual);

    //3rd one for Only for
    connect(runtime->recorder, &Recorder::recorderInfoSendDuration,
            loggerDialog, &LoggerDialog::recorderInfoReceiveDuration);
    //Recorder Information Connection End


    // Recorder: Recording Start

    // Recorder: Recording
    // START
    // START
    connect(loggerDialog, &LoggerDialog::recordingStart,
            runtime->recorder->m_recording,&Recording::start);
    // connect(runtime->recorder->m_recording,&Recording::sendRecorder,
    //         loggerDialog, &LoggerDialog::receiveRecorder);

    connect(runtime->recorder,&Recorder::sendRecorder, this, [this]{
        loggerDialog->recorder = runtime->recorder;
    });
    connect(runtime->recorder->m_recording,&Recording::updateUiDuration,
            loggerDialog, &LoggerDialog::updateDuration);
    connect(runtime->recorder->m_replay,&Replay::updateUiDuration,
            loggerDialog, &LoggerDialog::updateDuration);
    // PAUSE
    connect(loggerDialog, &LoggerDialog::recordingPause, this, [this]() {
        // Pause engine
        runtime->recorder->getRecording()->pause();

        // Stop UI timer
        if (recordingTimer && recordingTimer->isActive())
            recordingTimer->stop();

        // accumulate elapsed time since last start/resume
        if (recordingStartTime.isValid()) {
            pausedTimeMs += recordingStartTime.msecsTo(QDateTime::currentDateTime());
        }

        // keep recordingStartTime valid? we can invalidate to signal pause state
        recordingStartTime = QDateTime(); // optional: mark as paused

        // show paused duration
        loggerDialog->updateRecordingDuration(pausedTimeMs);
    });

    // RESUME
    connect(loggerDialog, &LoggerDialog::recordingResume, this, [this]() {
        // Resume engine
        runtime->recorder->getRecording()->resume();

        // reset start reference to now; pausedTimeMs already contains past time
        recordingStartTime = QDateTime::currentDateTime();

        // restart UI timer
        if (recordingTimer && !recordingTimer->isActive())
            recordingTimer->start(100);
    });

    // STOP
    connect(loggerDialog, &LoggerDialog::recordingStop, this, [this]() {
        // Stop engine
        runtime->recorder->getRecording()->stop();

        // stop UI timer
        if (recordingTimer && recordingTimer->isActive())
            recordingTimer->stop();

        // reset state
        loggerDialog->updateRecordingDuration(0);
        loggerDialog->getTimelineWidget()->clearBookmarks();
        pausedTimeMs = 0;
        recordingStartTime = QDateTime();
    });
    connect(loggerDialog, &LoggerDialog::bookmarkAdded, this, [this](const QString &bookmarkNote) {
        if (!runtime || !runtime->recorder->getRecording()) {
            qWarning() << "Runtime or Recorder not available — cannot save bookmark.";
            return;
        }

        // compute total time = accumulated paused time + current running segment
        qint64 currentSegmentMs = 0;
        if (recordingStartTime.isValid()) {
            currentSegmentMs = recordingStartTime.msecsTo(QDateTime::currentDateTime());
        }
        qint64 timestampMs = pausedTimeMs + currentSegmentMs;

        runtime->recorder->getRecording()->recordingBookmark(bookmarkNote, timestampMs);
        loggerDialog->addBookmarkWithTimestamp(bookmarkNote, timestampMs);
    });



    //Recorder: Recording End

    //Recorder: Replay Start
    connect(loggerDialog, &LoggerDialog::replayStart,
            runtime->recorder->getReplay(), &Replay::start);
    connect(loggerDialog, &LoggerDialog::replayPause,
            runtime->recorder->getReplay(), &Replay::pause);
    connect(loggerDialog, &LoggerDialog::replayResume,
            runtime->recorder->getReplay(), &Replay::resume);
    connect(loggerDialog, &LoggerDialog::replayStop,
            runtime->recorder->getReplay(), &Replay::stop);

    // connect(loggerDialog, &LoggerDialog::replayFileLoaded,
    //         runtime->recorder->getReplay(), &Replay::fileLoaded);
    connect(loggerDialog, &LoggerDialog::loadRecording, this, [=](const QString &filePath) {
        if (runtime && runtime->recorder) {
            if (runtime->recorder->getReplay()->replayLoaded(filePath)) {
            }
        }
    });


    connect(runtime->recorder->getReplay(), &Replay::replayBookmark,
            loggerDialog, &LoggerDialog::onReplayBookmarkLoaded);

    connect(runtime->recorder->getReplay(), &Replay::setReplayDuration,
            loggerDialog, &LoggerDialog::setTimelineDuration);

    connect(loggerDialog, &LoggerDialog::replayFileUnloaded,
            runtime->recorder->getReplay(), &Replay::fileUnloaded);

    connect(runtime->recorder->getReplay(), &Replay::replayFrameLoaded,
            loggerDialog, &LoggerDialog::updateReplayProgress);


    connect(loggerDialog, &LoggerDialog::nextFrame, this, [=]() {
        if (runtime && runtime->recorder) {
            runtime->recorder->getReplay()->goToNextFrame();
        }
    });
    connect(loggerDialog, &LoggerDialog::previousFrame, this, [=]() {
        if (runtime && runtime->recorder) {
            runtime->recorder->getReplay()->goToPreviousFrame();
        }
    });
    connect(loggerDialog, &LoggerDialog::pressPlayAgain, this, [=]() {
        if (runtime && runtime->recorder) {
            runtime->recorder->getReplay()->playAgain();
        }
    });
    connect(loggerDialog, &LoggerDialog::bookmarkClicked,this, [=](const QString &note, qint64 timestampMs) {
        runtime->recorder->getReplay()->bookmarkReplay(note, timestampMs);
    });
    //Recorder: Replay End



    //Logger Recoding command

    //Start Himan
    // Logger Pause/Resume connection



    connect(loggerDialog, &LoggerDialog::dbInit, this, [=]() {
        if (runtime && runtime->sqlite) {
            runtime->sqlite->dbInit();
        }
    });

    connect(loggerDialog, &LoggerDialog::dbConnect, this, [=]() {
        if (runtime && runtime->sqlite) {
            runtime->sqlite->dbConnect();
            // runtime->replay->entitiesMap = runtime->sqlite->getEntities();
            // runtime->replay->frameMap    = runtime->sqlite->getFrameMap();
        }
    });

    connect(loggerDialog, &LoggerDialog::getDBStatus, this, [=]() {
        if (runtime && runtime->sqlite) {
            loggerDialog->dbStatusPtr = &runtime->sqlite->dbStatus;
        }
    });

    //    connect(runtime->recording, &Recording::mapFrame, this, [=](const qint64 &s_duration) {
    //        if (runtime && runtime->sqlite) {
    //            runtime->sqlite->insertFrame(s_duration);
    //        }
    //    });


    connect(runtime->replay, &Replay::getMaxFrameIndexNDuration,
            this, [=](int* maxFrameIndex, qint64* maxDuration){

                if (runtime && runtime->sqlite) {
                    runtime->sqlite->
                        setFrameIndexNDuration(*maxFrameIndex, *maxDuration);
                }
            });
    connect(runtime->replay, &Replay::getPayLoad,
            this, [=](PayLoad* payload) {

                if (runtime && runtime->sqlite) {
                    runtime->sqlite->setPayLoad(*payload);
                }
            });
    connect(hierarchy, &Hierarchy::profileAdded, this, [this](QString ID, QString profileName){
        if(runtime && runtime->recording && runtime->recorder->modeOfLogger == Recorder::RECORDING){
            runtime->recording->profileCategoriesUpdate(ID, profileName, Operation::CREATE);
        }
    });
    connect(hierarchy, &Hierarchy::profileRemoved, this, [this](QString ID){
        if(runtime && runtime->recording && runtime->recorder->modeOfLogger == Recorder::RECORDING){
            runtime->recording->profileCategoriesDeleted(ID);
        }
    });


    connect(hierarchy, &Hierarchy::entityAdded, this, [this](QString parentID, QString ID, QString entityName){
        if(runtime && runtime->recording && runtime->recorder->modeOfLogger == Recorder::RECORDING){
            /* To Add Entity */
            runtime->recording->entityAddedInBetween( parentID, ID, entityName);

            // /* To Add Mesh of Entity*/
            // if(hierarchy->Platforms->at(ID.toStdString())->meshRenderer2d){
            //     //runtime->recording->meshRenderer2DCRUD(ID,hierarchy->Platforms->at(ID.toStdString())->meshRenderer2d);
            // }else{
            //     qDebug()<<"Mesh not exist";
            // }
        }
    });
    connect(hierarchy, &Hierarchy::meshRenderer2DisAdded, this, [this](const QString &ID, MeshRenderer2D* meshRenderer2D){
        if(runtime && runtime->recording && runtime->recorder->modeOfLogger == Recorder::RECORDING){
            /* To Add Entity */
            //qDebug()<<"Mesh is added "<<ID<<" and "<<entity->ID.c_str();
            runtime->recording->meshRenderer2DCRUD(ID,meshRenderer2D);
        }
    });
    connect(hierarchy, &Hierarchy::trajectoryisAdded, this, [this](const QString &ID, Trajectory* trajectory){
        if(runtime && runtime->recording && runtime->recorder->modeOfLogger == Recorder::RECORDING){
            /* To Add Entity */
            //qDebug()<<"Mesh is added "<<ID<<" and "<<entity->ID.c_str();
            runtime->recording->trajectoryCRUD(ID , trajectory->Trajectories,Operation::CREATE);
        }
    });
    connect(hierarchy, &Hierarchy::entityRemovedfull, this, [this](QString parentId, QString ID, bool Profile){
        if(runtime && runtime->recording && runtime->recorder->modeOfLogger == Recorder::RECORDING){
            if(runtime->recording->entitiesIDIndex.contains(ID)){
                runtime->recording->meshRenderer2DCRUD(ID,nullptr,Operation::DELETE);
                runtime->recording->entityRemovedInBetween(ID);
                /* To Add Mesh of Entity*/
            }
        }
    });
    connect(tacticalDisplay->canvas, &CanvasWidget::trajectoryUpdatedforLogger, this, [=]
            (QString entityId, std::vector<Waypoints *> Trajectories) {
                if(runtime && runtime->recording &&
                    (runtime->recorder->modeOfLogger == Recorder::RECORDING)){
                    if(runtime->recorder->loggerStatus == Recorder::S_RECORDING_PAUSED ||
                        runtime->recorder->loggerStatus == Recorder::S_RECORDING){
                        if(Trajectories.size() == 0){
                            runtime->recording->trajectoryCRUD(entityId,Trajectories,Operation::DELETE);
                        }else{
                            runtime->recording->trajectoryCRUD(entityId,Trajectories,Operation::UPDATE);
                        }
                    }

                }
            });
    /**********************************************************
 *                   Logger Start                         *
 **********************************************************/

    /*                       Alert !!                         */
    connect(runtime->recorder,&Recorder::alertViaStr,
            loggerDialog,&LoggerDialog::alertViaStr);

    /*                    SQLite Connection                   */
    connect(loggerDialog, &LoggerDialog::getDBStatusOfRecording, this,
            [=](SQLite::DBStatuses &dbStatusOfRecording) {
                if (runtime && runtime->recording) {
                    loggerDialog->dbStatusPtr = &runtime->sqlite->dbStatus;
                }
            });
    /*              Send SQLite DB File Path                  */
    connect(loggerDialog, &LoggerDialog::getFilePath, this,
            [=](QString path) {
                if (runtime && runtime->recorder) {
                    runtime->recorder->loadFile(path);
                }
            });
    connect(loggerDialog, &LoggerDialog::savedFilePath, this,
            [=](QString path) {
                if (runtime && runtime->recorder) {
                    runtime->recorder->saveFile(path);
                }
            });
    /*            Freeze and Defreeze the Button             */
    connect(runtime->recorder,&Recorder::freezeButtonOperation,
            loggerDialog,&LoggerDialog::freezeButtonOperation);
    /**********************************************************
 *                   Logger End                           *
 **********************************************************/

    /**********************************************************
 *              Timeline Widget Start                     *
 **********************************************************/
    connect(runtime->replay, &Replay::setMaxDuration,
            this, [=](qint64* maxDuration) {
                if (loggerDialog) {
                    loggerDialog->getTimelineWidget()->
                        maxDurationPtr = maxDuration;
                }
            });
    connect(loggerDialog, &LoggerDialog::sendClickedTimestamp, this,
            [=](qint64 clickedTimestamp) {
                if (runtime && runtime->replay) {
                    runtime->replay->
                        jumpInBetween(clickedTimestamp);
                }
            });
    /**********************************************************
*                Timeline Widget End                      *
 **********************************************************/

    /**********************************************************
 *       Recording Send  to SQLite PayLoad Start          *
 **********************************************************/

    /*                     SQLite Instance                    */
    // connect(runtime->recorder, &Recorder::getSQLite,
    //         this, [=](SQLite *m_sqlite) {
    //             if (runtime && runtime->sqlite) {
    //                 runtime->sqlite->setSQLite(m_sqlite);
    //             }
    //         });

    /*                         PayLoad                        */
    connect(runtime->recording, &Recording::sendPayLoad,
            this, [=](PayLoad m_payLoad) {

                if (runtime && runtime->sqlite) {
                    runtime->sqlite->receivePayLoad(m_payLoad);
                }
            });
    connect(runtime->replay, &Replay::getPayLoadFromIndex,
            this, [=](PayLoad* payload,int frameIndex) {
                if (runtime && runtime->sqlite) {
                    runtime->sqlite->setPayLoadFromIndex(*payload,frameIndex);
                }
            });

    /*                  Tranform                              */
    // connect(hierarchy->Components->at(), &Recording::sendPayLoad,
    //         this, [=](PayLoad m_payLoad) {

    //             if (runtime && runtime->sqlite) {
    //                 runtime->sqlite->receivePayLoad(m_payLoad);
    //             }
    // });
    /**********************************************************
 *       Recording Send  to SQLite PayLoad End            *
 **********************************************************/

    //createEntitiesCreate(QString parentId,QString ID,QString EntityName,bool Profile);
    //    connect(runtime->replay, &Replay::createEntitiesCreate,
    //            this, [=](QString parentId,QString ID,QString EntityName,bool Profile) {
    //        if (hierarchy) {
    //            hierarchy->addEntityViaLogger(parentId,ID,EntityName,Profile);
    //        }
    //    });
    //    connect(runtime->recording, &Recording::entityCreated, this, [=](const QString &parentID,
    //                                                                     const QString &id,
    //                                                                     const QString &name,
    //                                                                     const qint64  &created) {
    //        if (runtime && runtime->sqlite) {
    //            runtime->sqlite->insertEntity(parentID, id, name, created);
    //        }
    //    });
    // connect(hierarchy, &Hierarchy::entityRemoved, this , [=](std::string id,
    //                                                          qint64 deleted){
    //     if (runtime && runtime->sqlite) {
    //         runtime->sqlite->insertEntity(id, deleted);
    //     }
    // });



    //    connect(runtime->replay, &Replay::getEntities, this, [=](){
    //        if (runtime && runtime->sqlite) {
    //            runtime->replay->entitiesMap = runtime->sqlite->getEntities();
    //        }
    //    });
    //    connect(runtime->replay, &Replay::getFrame, this, [=](int s_frameIndex){
    //        if (runtime && runtime->sqlite) {
    //            runtime->replay->frame = runtime->sqlite->getFrameByFrameIndex(s_frameIndex);
    //            //runtime->sqlite->showFrameByFrameIndex(s_frameIndex);
    //        }
    //    });
    connect(runtime->replay, &Replay::render, this, [=](float deltatime){
        if (tacticalDisplay && tacticalDisplay->canvas) {
            tacticalDisplay->canvas->Render(deltatime);
            //runtime->sqlite->showFrameByFrameIndex(s_frameIndex);
        }
    });
    //=================================================================================================

    // ===== AUTO-CENTER MAP ON ENTITY SELECTION (Waris)================================================
    // When user selects an entity from hierarchy tree, automatically center map on it
    connect(treeView, &HierarchyTree::itemSelected, this, [=](QVariantMap data) {
        // Determine the type of item selected
        QString type;
        if (data["type"].type() == QVariant::Map) {
            QVariantMap typeData = data["type"].toMap();
            if (typeData.contains("type") && typeData["type"].toString() == "option") {
                type = "profile";
            }
        } else {
            type = data["type"].toString();
        }

        // Only auto-center for entity selections (not folders, profiles, or components)
        if (type == "entity" && tacticalDisplay && tacticalDisplay->canvas) {
            QString entityId = data["ID"].toString();

            // Auto-center the map on the selected entity
            // Using adjustZoom=false to preserve user's current zoom level
            tacticalDisplay->canvas->centerOnEntity(entityId, false);
        }
    });
    //=================================================================================================

}
void RuntimeEditor::setupEnhancedDockWidgets()
{
    tacticalDisplay = new TacticalDisplay(this);
    setCentralWidget(tacticalDisplay);

    // Set main window background
    this->setStyleSheet("QMainWindow { background-color: #0F2636; }");

    // --- 1. Helper Lambda with Side Logic ---
    auto setupOverlay = [this](CustomResizableOverlayDock* &dock, QWidget* content, const QString &title, bool isLeftPanel) {
        dock = new CustomResizableOverlayDock(title, this);
        dock->setWidget(content);
        dock->setFloating(true);
        dock->setParent(this);
        dock->setWindowFlags(Qt::SubWindow | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
        dock->setMinimumWidth(280);
        dock->setMinimumHeight(300);

        QString dockStyleSheet = "CustomResizableOverlayDock { "
                                 "background-color: #0F2636; "
                                 "color: white; ";

        if (isLeftPanel) {
            dock->handlePos = CustomResizableOverlayDock::Right;
            dockStyleSheet += "border-right: 4px solid #00BFFF; }";
        } else {
            dock->handlePos = CustomResizableOverlayDock::Left;
            dockStyleSheet += "border-left: 4px solid #00BFFF; }";
        }

        dockStyleSheet += "CustomResizableOverlayDock::title { "
                          "background-color: #1A3A4F; "
                          "color: white; "
                          "font-weight: bold; }";

        dock->setStyleSheet(dockStyleSheet);

        if (content) {
            content->setStyleSheet("background-color: #0F2636; color: white;");
        }
    };

    // --- 2. Sidebar (Top-Right) ---
    sidebarDock = new CustomResizableOverlayDock("Sidebar", this);
    SidebarWidget *sidebar = new SidebarWidget(this);
    sidebar->setSensorsButtonVisible(true);  // Enable sensors button for runtime
    sidebar->setStyleSheet("background-color: #0F2636; color: white;");
    sidebarDock->setWidget(sidebar);
    sidebarDock->setFloating(true);
    sidebarDock->setParent(this);
    sidebarDock->setWindowFlags(Qt::SubWindow | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
    sidebarDock->setFixedHeight(50);
    sidebarDock->setMinimumWidth(280);
    sidebarDock->handlePos = CustomResizableOverlayDock::Left;
    sidebarDock->setStyleSheet("CustomResizableOverlayDock { "
                               "background-color: #0F2636; "
                               "color: white; "
                               "border-left: 4px solid #00BFFF; }"
                               "CustomResizableOverlayDock::title { "
                               "background-color: #1A3A4F; "
                               "color: white; "
                               "font-weight: bold; }");

    // --- 3. Right Side Panels (isLeftPanel = false) ---
    inspector = new Inspector(this);
    if (hierarchy) inspector->setHierarchy(hierarchy);
    inspector->setStyleSheet("background-color: #0F2636; color: white;");
    setupOverlay(inspectorDock, inspector, "Inspector", false);
    inspectorDock->enableLockButton();

    libTreeView = new HierarchyTree(this);
    libTreeView->islib = true;
    libTreeView->setStyleSheet("background-color: #0F2636; color: white;");
    setupOverlay(libraryDock, libTreeView, "Library", false);

    textScriptView = new TextScriptWidget(this);
    textScriptView->setStyleSheet("background-color: #0F2636; color: white;");
    setupOverlay(textScriptDock, textScriptView, "TestScript", false);

    // --- 4. Display (Sensors) Panel ---
    displayDock = new CustomResizableOverlayDock("Sensors", this);
    displayTabs = new QTabWidget(this);
    displayTabs->setStyleSheet("QTabWidget::pane { background-color: #0F2636; }"
                               "QTabBar::tab { background-color: #1A3A4F; color: white; padding: 8px; }"
                               "QTabBar::tab:selected { background-color: #00BFFF; }");
    displayDock->handlePos = CustomResizableOverlayDock::Left;

    // ADD THIS LINE — lock button sirf Sensors dock mein show hoga
    displayDock->enableLockButton();

    // Create sensor displays
    radarDisplayUI = new RadarDisplay(displayTabs);
    if (hierarchy) radarDisplayUI->setHierarchy(hierarchy);
    displayTabs->addTab(radarDisplayUI, "Radar");

    iffDisplayUI = new IFFDisplay(displayTabs);
    if (hierarchy) iffDisplayUI->setHierarchy(hierarchy);
    displayTabs->addTab(iffDisplayUI, "IFF");

    radioDisplayUI = new RADIODisplay(displayTabs);
    if (hierarchy) radioDisplayUI->setHierarchy(hierarchy);
    displayTabs->addTab(radioDisplayUI, "RADIO");

    esmDisplayUI = new ESMDisplay(displayTabs);
    if (hierarchy) esmDisplayUI->setHierarchy(hierarchy);
    displayTabs->addTab(esmDisplayUI, "ESM");

    csmDisplayUI = new CSMDisplay(displayTabs);
    if (hierarchy) csmDisplayUI->setHierarchy(hierarchy);
    displayTabs->addTab(csmDisplayUI, "CSM");

    displayTabs->setCurrentIndex(0);
    displayDock->setWidget(displayTabs);
    displayDock->setFloating(true);
    displayDock->setParent(this);
    displayDock->setWindowFlags(Qt::SubWindow | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
    displayDock->setMinimumWidth(280);
    displayDock->setMinimumHeight(300);
    displayDock->handlePos = CustomResizableOverlayDock::Left;
    displayDock->setStyleSheet("CustomResizableOverlayDock { "
                               "background-color: #0F2636; "
                               "color: white; "
                               "border-left: 4px solid #00BFFF; }"
                               "CustomResizableOverlayDock::title { "
                               "background-color: #1A3A4F; "
                               "color: white; "
                               "font-weight: bold; }");

    // --- 5. Logger Panel ---
    loggerDock = new CustomResizableOverlayDock("Logger", this);
    loggerDialog = new LoggerDialog(this);
    loggerDialog->setStyleSheet("background-color: #0F2636; color: white;");
    loggerDock->setWidget(loggerDialog);
    loggerDock->setFloating(true);
    loggerDock->setParent(this);
    loggerDock->setWindowFlags(Qt::SubWindow | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
    loggerDock->setMinimumWidth(280);
    loggerDock->setMinimumHeight(300);
    loggerDock->handlePos = CustomResizableOverlayDock::Left;
    loggerDock->setStyleSheet("CustomResizableOverlayDock { "
                              "background-color: #0F2636; "
                              "color: white; "
                              "border-left: 4px solid #00BFFF; }"
                              "CustomResizableOverlayDock::title { "
                              "background-color: #1A3A4F; "
                              "color: white; "
                              "font-weight: bold; }");

    // --- 6. Left Side Panel (isLeftPanel = true) ---
    treeView = new HierarchyTree(this);
    treeView->setStyleSheet("background-color: #0F2636; color: white;");
    setupOverlay(hierarchyDock, treeView, "Editor", true);

    // --- 7. LAYERS PANEL ---
    layerPanel = new LayerPanel(this);
    layerPanel->setStyleSheet("background-color: #0F2636; color: white;");
    setupOverlay(layerDock, layerPanel, "Layers", true);

    // Connect layer panel to canvas
    if (tacticalDisplay && tacticalDisplay->canvas) {
        tacticalDisplay->canvas->setLayerPanel(layerPanel);
        if (tacticalDisplay->canvas->getShapesFeature()) {
            tacticalDisplay->canvas->getShapesFeature()->setLayerPanel(layerPanel);
            layerPanel->setCanvasWidget(tacticalDisplay->canvas);
        }
    }

    // --- 8. CONSOLE (Bottom) ---
    consoleDock = new CustomResizableOverlayDock("Console", this);
    consoleView = new ConsoleView(this);
    consoleView->setStyleSheet("background-color: #0F2636; color: white;");
    consoleDock->setWidget(consoleView);
    consoleDock->setFloating(true);
    consoleDock->setParent(this);
    // Enable title bar with close button for console
    consoleDock->setWindowFlags(Qt::SubWindow | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
    consoleDock->setMinimumWidth(400);
    consoleDock->setMinimumHeight(150);
    consoleDock->handlePos = CustomResizableOverlayDock::Left;
    consoleDock->setStyleSheet("CustomResizableOverlayDock { "
                               "background-color: #0F2636; "
                               "color: white; "
                               "border: 2px solid #00BFFF; }"
                               "CustomResizableOverlayDock::title { "
                               "background-color: #1A3A4F; "
                               "color: white; "

                               "font-weight: bold; }");
    // --- POSITIONING ---
    int winW = width() > 0 ? width() : 1100;
    int winH = height() > 0 ? height() : 600;
    int panelWidth = 300;
    int rightX = winW - panelWidth - 20;
    int leftX = 20;
    int topY = 80;  // Start below toolbar

    // LEFT SIDE
    int hierarchyHeight = 400;
    hierarchyDock->setGeometry(leftX, topY, panelWidth, hierarchyHeight);
    hierarchyDock->show();

    int hierarchyBottom = hierarchyDock->y() + hierarchyDock->height();
    layerDock->setGeometry(leftX, hierarchyBottom + 5, panelWidth, 150);
    layerDock->show();

    // RIGHT SIDE
    sidebarDock->setGeometry(rightX, topY, panelWidth, 45);
    sidebarDock->show();

    int sidebarBottom = sidebarDock->y() + sidebarDock->height();
    int rightPanelHeight = winH - sidebarBottom - 100;

    inspectorDock->setGeometry(rightX, sidebarBottom + 5, panelWidth, rightPanelHeight);
    inspectorDock->show();
    inspectorDock->raise();

    libraryDock->setGeometry(rightX, sidebarBottom + 5, panelWidth, rightPanelHeight);
    libraryDock->hide();

    textScriptDock->setGeometry(rightX, sidebarBottom + 5, panelWidth, rightPanelHeight);
    textScriptDock->hide();

    displayDock->setGeometry(rightX, sidebarBottom + 5, panelWidth, rightPanelHeight);
    displayDock->hide();

    loggerDock->setGeometry(rightX, sidebarBottom + 5, panelWidth, rightPanelHeight);
    loggerDock->hide();

    consoleDock->setGeometry(20, winH - 190, winW - 40, 150);
    consoleDock->hide();

    // Ensure proper z-order
    hierarchyDock->raise();
    layerDock->raise();
    sidebarDock->raise();
    inspectorDock->raise();

    // --- CONNECT SIDEBAR VIEW SELECTION ---
    connect(sidebar, &SidebarWidget::viewSelected, this, [this](const QString &viewName) {
        inspectorDock->hide();
        libraryDock->hide();
        textScriptDock->hide();
        // displayDock->hide();
        // loggerDock->hide();

        CustomResizableOverlayDock* target = nullptr;
        if (viewName == "Inspector") {
            target = inspectorDock;
        } else if (viewName == "Library") {
            target = libraryDock;
        } else if (viewName == "TextScript") {
            target = textScriptDock;
        } else if (viewName == "Sensors") {
            target = displayDock;
        } else if (viewName == "Logger") {
            target = loggerDock;
        } else if (viewName == "Console") {
            consoleDock->setVisible(!consoleDock->isVisible());
            if (consoleDock->isVisible()) consoleDock->raise();
            return;
        }

        if (target) {
            QRect sGeo = sidebarDock->geometry();
            target->setGeometry(sGeo.x(), sGeo.y() + sGeo.height() + 5,
                                sGeo.width(), height() - sGeo.y() - sGeo.height() - 150);
            target->show();
            target->raise();
        }
    });

    // --- CONNECT MOVE/RESIZE SIGNALS ---
    connect(sidebarDock, &CustomResizableOverlayDock::moved, this, [this](QPoint oldPos, QPoint newPos) {
        QPoint delta = newPos - oldPos;
        if (inspectorDock && inspectorDock->isVisible())
            inspectorDock->move(inspectorDock->pos() + delta);
        if (libraryDock && libraryDock->isVisible())
            libraryDock->move(libraryDock->pos() + delta);
        if (textScriptDock && textScriptDock->isVisible())
            textScriptDock->move(textScriptDock->pos() + delta);
        if (displayDock && displayDock->isVisible())
            displayDock->move(displayDock->pos() + delta);
        if (loggerDock && loggerDock->isVisible())
            loggerDock->move(loggerDock->pos() + delta);
    });

    connect(sidebarDock, &CustomResizableOverlayDock::resized, this, [this](QSize oldSize, QSize newSize) {
        int newWidth = newSize.width();
        if (inspectorDock && inspectorDock->isVisible())
            inspectorDock->setFixedWidth(newWidth);
        if (libraryDock && libraryDock->isVisible())
            libraryDock->setFixedWidth(newWidth);
        if (textScriptDock && textScriptDock->isVisible())
            textScriptDock->setFixedWidth(newWidth);
        if (displayDock && displayDock->isVisible())
            displayDock->setFixedWidth(newWidth);
        if (loggerDock && loggerDock->isVisible())
            loggerDock->setFixedWidth(newWidth);
    });

    // --- CONNECT LIBRARY TITLE UPDATE ---
    connect(libTreeView, &HierarchyTree::libraryFileNameChanged,
            this, [=](const QString& fileName) {
                if (fileName == "Library") {
                    libraryDock->setWindowTitle("Library");
                } else {
                    libraryDock->setWindowTitle("Library - " + fileName);
                }
            });

    // --- INITIALIZE INSPECTOR LIST ---
    inspectorDocks.append(inspectorDock);
    inspectors.append(inspector);
}
void RuntimeEditor::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    int winW = width();
    int winH = height();
    int panelWidth = 300;
    int rightX = winW - panelWidth - 20;
    int leftX = 20;
    int toolbarHeight = designToolBar ? designToolBar->height() : 30;
    int topY = toolbarHeight + 80;
    int hierarchyHeight = 500;
    if (hierarchyDock) {
        hierarchyDock->setGeometry(leftX, topY, panelWidth, hierarchyHeight);
    }

    if (layerDock) {
        int hierarchyBottom = hierarchyDock ? hierarchyDock->y() + hierarchyDock->height() : topY + hierarchyHeight;
        layerDock->setGeometry(leftX, hierarchyBottom + 5, panelWidth, 150);
    }
    if (sidebarDock) {
        sidebarDock->setGeometry(rightX, topY, panelWidth, 45);
    }

    int sidebarBottom = sidebarDock ? sidebarDock->y() + sidebarDock->height() : topY + 45;
    int rightPanelHeight = winH - sidebarBottom - 100;

    if (inspectorDock && inspectorDock->isVisible()) {
        inspectorDock->setGeometry(rightX, sidebarBottom, panelWidth, rightPanelHeight);
    }
    if (libraryDock && libraryDock->isVisible()) {
        libraryDock->setGeometry(rightX, sidebarBottom, panelWidth, rightPanelHeight);
    }
    if (textScriptDock && textScriptDock->isVisible()) {
        textScriptDock->setGeometry(rightX, sidebarBottom, panelWidth, rightPanelHeight);
    }

    // CONSOLE
    if (consoleDock && consoleDock->isVisible()) {
        consoleDock->setGeometry(20, winH - 190, winW - 40, 150);
    }
}
// Add this showEvent
void RuntimeEditor::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // Force a resize event when window is shown
    QResizeEvent *re = new QResizeEvent(size(), size());
    resizeEvent(re);
    delete re;
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
        // Hide other right-side panels (but save inspector state)
        libraryDock->hide();
        textScriptDock->hide();
        // loggerDock->hide();

        QRect sGeo = sidebarDock->geometry();

        // Check if inspector is open
        if (inspectorDock->isVisible()) {
            // If inspector is open, place sensors at SAME POSITION
            QRect inspectorGeo = inspectorDock->geometry();
            displayDock->setGeometry(inspectorGeo);
            displayDock->show();
            displayDock->raise(); // Sensors on top
            // inspector remains behind - don't hide it
        } else {
            // If no inspector, place below sidebar
            displayDock->setGeometry(sGeo.x(), sGeo.y() + sGeo.height() + 5,
                                     sGeo.width(), height() - sGeo.y() - sGeo.height() - 150);
            displayDock->show();
            displayDock->raise();
        }

        SidebarWidget *sidebar = qobject_cast<SidebarWidget*>(sidebarDock->widget());
        if (sidebar) {
            sidebar->setActiveButton("Sensors");
        }
    } else {
        displayDock->hide();
        // Agar sensors hide ho raha hai aur inspector open tha, to inspector ko raise karein
        if (inspectorDock->isVisible()) {
            inspectorDock->raise();
        }

        SidebarWidget *sidebar = qobject_cast<SidebarWidget*>(sidebarDock->widget());
        if (sidebar) {
            sidebar->setActiveButton("");
        }
    }

    QAction *radarToggle = runtimeToolBar->findChild<QAction*>("radarToggleAction");
    if (radarToggle) {
        radarToggle->setChecked(displayDock->isVisible());
    }
}
// Logger
void RuntimeEditor::toggleLoggerDisplay(bool checked)
{
    if (checked && !loggerDock->isVisible()) {
        // Hide other right-side panels
        inspectorDock->hide();
        libraryDock->hide();
        textScriptDock->hide();
        // displayDock->hide();

        // Use manual positioning (same as in sidebar view selection)
        QRect sGeo = sidebarDock->geometry();
        loggerDock->setGeometry(sGeo.x(), sGeo.y() + sGeo.height() + 5,
                                sGeo.width(), height() - sGeo.y() - sGeo.height() - 150);
        loggerDock->show();
        loggerDock->raise();

        SidebarWidget *sidebar = qobject_cast<SidebarWidget*>(sidebarDock->widget());
        if (sidebar) {
            sidebar->setActiveButton("Logger");
        }
    } else if (!checked && loggerDock->isVisible()) {
        loggerDock->hide();
        SidebarWidget *sidebar = qobject_cast<SidebarWidget*>(sidebarDock->widget());
        if (sidebar) {
            sidebar->setActiveButton("");
        }
    }

    QAction *loggerAction = runtimeToolBar->findChild<QAction*>("loggerAction");
    if (loggerAction) {
        loggerAction->setChecked(loggerDock->isVisible());
    }
}
void RuntimeEditor::setupToolBars()
{
    // designToolBar = new DesignToolBar(this);
    designToolBar = new DesignToolBar(this, m_scenarioConfig);
    addToolBar(Qt::TopToolBarArea, designToolBar);
    runtimeToolBar = new RuntimeToolBar(this);
    addToolBar(Qt::TopToolBarArea, runtimeToolBar);
    networkToolBar = new NetworkToolbar(this);
    addToolBar(Qt::TopToolBarArea, networkToolBar);
    networkToolBar->hide();

    designToolBar->setMovable(true);
    runtimeToolBar->setMovable(true);
    networkToolBar->setMovable(true);

    // Set context menu policy for ALL toolbars
    designToolBar->setContextMenuPolicy(Qt::CustomContextMenu);
    networkToolBar->setContextMenuPolicy(Qt::CustomContextMenu);
    runtimeToolBar->setContextMenuPolicy(Qt::CustomContextMenu);

    // Connect ALL toolbars to the same slot
    connect(designToolBar, &QToolBar::customContextMenuRequested,
            this, &RuntimeEditor::showPanelContextMenu);
    connect(networkToolBar, &QToolBar::customContextMenuRequested,
            this, &RuntimeEditor::showPanelContextMenu);
    connect(runtimeToolBar, &QToolBar::customContextMenuRequested,
            this, &RuntimeEditor::showPanelContextMenu);
}

void RuntimeEditor::setupToolBarConnections()
{
    DesignToolBar *designToolBar = findChild<DesignToolBar*>();
    if (!designToolBar || !tacticalDisplay || !tacticalDisplay->canvas) {
        qWarning() << "Toolbar connection setup failed - required components missing";
        return;
    }
    connect(designToolBar, &DesignToolBar::modeChanged,
            this, [=](int mode) {
                tacticalDisplay->canvas->setTransformMode(static_cast<TransformMode>(mode));
            });
    connect(designToolBar, &DesignToolBar::shapeSelected,
            this, [=](const QString &shape) {
                tacticalDisplay->canvas->setShapeDrawingMode(true, shape);
            });
    connect(designToolBar, &DesignToolBar::gridPlaneXToggled,
            tacticalDisplay->canvas, &CanvasWidget::setXGridVisible);
    connect(designToolBar, &DesignToolBar::gridPlaneYToggled,
            tacticalDisplay->canvas, &CanvasWidget::setYGridVisible);
    connect(designToolBar, &DesignToolBar::gridOpacityChanged,
            this, [=](int opacity) {
                tacticalDisplay->canvas->setGridOpacity(opacity);
            });
    connect(designToolBar, &DesignToolBar::tooltipOptionsChanged,
            tacticalDisplay->canvas, &CanvasWidget::setTooltipOptions);
    //====================waris=======================
    connect(designToolBar, &DesignToolBar::layerOptionToggled,
            tacticalDisplay->canvas, &CanvasWidget::toggleLayerVisibility);
    connect(designToolBar, &DesignToolBar::bitmapImageSelected,
            tacticalDisplay->canvas, &CanvasWidget::onBitmapImageSelected);
    connect(designToolBar, &DesignToolBar::bitmapSelected,
            this, [=](const QString &fileName) {
                tacticalDisplay->canvas->onBitmapSelected(fileName);
            });
    if (tacticalDisplay && tacticalDisplay->mapWidget) {
        connect(designToolBar, &DesignToolBar::mapLayerChanged,
                this, [=](const QString &layers) {
                    tacticalDisplay->setMapLayers(layers.split(",", Qt::SkipEmptyParts));
                });
        connect(designToolBar, &DesignToolBar::customMapAdded,
                tacticalDisplay, &TacticalDisplay::addCustomMap);
        connect(designToolBar, &DesignToolBar::customMapAdded,
                this, [=](const QString &name, int zoomMin, int zoomMax, const QString &url) {
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
            }
        });
    } else {
        qCritical() << "Map widget not available for layer connections";
    }
    connect(designToolBar->getAddTrajectoryAction(), &QAction::triggered,
            this, [=]() {
                tacticalDisplay->canvas->setTrajectoryDrawingMode(true);
            });
    connect(designToolBar->getMeasureDistanceAction(), &QAction::triggered,
            this, [=]() {
                bool isChecked = designToolBar->getMeasureDistanceAction()->isChecked();
                tacticalDisplay->canvas->setTransformMode(isChecked ? MeasureDistance : Translate);
            });
    connect(designToolBar, &DesignToolBar::presetLayerSelected,
            tacticalDisplay->canvas, &CanvasWidget::onPresetLayerSelected);
    connect(designToolBar, &DesignToolBar::importGeoJsonTriggered,
            tacticalDisplay->canvas, &CanvasWidget::importGeoJsonLayer);
    connect(tacticalDisplay->canvas, &CanvasWidget::geoJsonLayerAdded,
            designToolBar, &DesignToolBar::onGeoJsonLayerAdded);
    connect(designToolBar, &DesignToolBar::geoJsonLayerToggled,
            tacticalDisplay->canvas, &CanvasWidget::onGeoJsonLayerToggled);
    connect(designToolBar, &DesignToolBar::bitmapSelected,
            this, [=](const QString &fileName) {
                tacticalDisplay->canvas->onBitmapSelected(fileName);
            });
    connect(designToolBar, &DesignToolBar::shapeSelected,
            this, [=](const QString &shape) {
                tacticalDisplay->canvas->setShapeDrawingMode(true, shape);
            });
    connect(designToolBar, &DesignToolBar::coordinateSystemChanged,
            tacticalDisplay->mapWidget, &GISlib::setCoordinateSystem);
    //===========================================
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

    feedbackWindow->show();
}

void RuntimeEditor::loadFromJsonFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open JSON file:" << filePath;
        QMessageBox::warning(this, "Error", QString("Failed to open JSON file: %1").arg(filePath));
        // loadingDialog->deleteLater();
        return;
    }

    QByteArray data = file.readAll();
    file.close();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse JSON:" << err.errorString();
        QMessageBox::warning(this, "Error", QString("Failed to parse JSON: %1").arg(err.errorString()));
        // loadingDialog->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("hierarchy")) {
        // loadingDialog->setLabelText("Loading...");
        QCoreApplication::processEvents();
        QJsonObject hier = obj["hierarchy"].toObject();
        hierarchy->fromJson(hier);
        if (runtimeToolBar) {
            runtimeToolBar->storeSnapshot(hier);
        }
        QCoreApplication::processEvents();
        if (treeView && treeView->getTreeWidget()) {
            treeView->getTreeWidget()->update();
        } else {
            qWarning() << "Failed to update HierarchyTree: treeView or treeWidget is null";
        }
        QCoreApplication::processEvents();
    } else {
        qWarning() << "JSON file does not contain 'hierarchy' key";
    }
    if (tacticalDisplay && obj.contains("tactical")) {
        // loadingDialog->setLabelText("Loading...");
        QCoreApplication::processEvents();
        QJsonObject tac = obj["tactical"].toObject();
        tacticalDisplay->canvas->fromJson(tac);
    } else {
        qWarning() << "JSON file does not contain 'tactical' key or tacticalDisplay is null";
    }

    lastSavedFilePath = filePath;
    clearUnsavedChanges();
    // loadingDialog->close();
    // loadingDialog->deleteLater();

    // updateStatusBar("Project loaded: " + QFileInfo(filePath).fileName());
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
    for (int i = inspectorDocks.size() - 1; i >= 0; --i) {
        QDockWidget* dock = inspectorDocks[i];
        if (dock != inspectorDock) {
            inspectors.removeAt(i);
            dock->deleteLater();
        }
    }
    if (inspectorDocks.size() > 1) {
        inspectorDocks = QList<QDockWidget*>{inspectorDock};
        inspectors = QList<Inspector*>{inspector};
    }
    inspectorCount = 0;
    hierarchyDock->hide();
    tacticalDisplayDock->hide();
    consoleDock->hide();
    inspectorDock->hide();
    libraryDock->hide();
    sidebarDock->hide();
    textScriptDock->hide();
    displayDock->hide();
    loggerDock->hide();
    removeDockWidget(hierarchyDock);
    removeDockWidget(tacticalDisplayDock);
    removeDockWidget(consoleDock);
    removeDockWidget(inspectorDock);
    removeDockWidget(libraryDock);
    removeDockWidget(sidebarDock);
    removeDockWidget(textScriptDock);
    removeDockWidget(displayDock);
    removeDockWidget(loggerDock);
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
    QTimer::singleShot(100, this, [=]() {
        QMainWindow::resize(1100, 600);

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

    });
}

RuntimeEditor::~RuntimeEditor()
{
    std::cout << "RuntimeEditor delete";
    delete runtime;

}
void RuntimeEditor::updateStatusBar(const QString &message) {
    if (statusBar) {
        //statusBar->showMessage(message);
    }
}
void RuntimeEditor::loadRecentProject(const QString& filePath)
{
    QProgressDialog* loadingDialog = new QProgressDialog(this);

    loadingDialog->setLabelText("Loading...");
    loadingDialog->setCancelButton(nullptr);
    loadingDialog->setRange(0, 0);
    loadingDialog->setMinimumDuration(0);
    loadingDialog->setWindowModality(Qt::WindowModal);
    loadingDialog->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint);
    loadingDialog->setWindowTitle("");
    loadingDialog->setFixedSize(250, 80);
    loadingDialog->setStyleSheet(R"(
        QProgressDialog {
            background-color: #f0f0f0;
            border: 1px solid #cccccc;
            border-radius: 0px;
        }
        QLabel {
            color: #333333;
            font-size: 13px;
            font-family: "Segoe UI";
            padding: 5px;
            margin: 0px;
        }
        QProgressBar {
            border: 1px solid #bbbbbb;
            background-color: #ffffff;
            text-align: center;
            height: 20px;
        }
        QProgressBar::chunk {
            background-color: #4da6ff;
            border: none;
        }
    )");


    loadingDialog->move(geometry().center() - loadingDialog->rect().center());
    loadingDialog->show();
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

    loadingDialog->setLabelText("Loading...");
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

}
void RuntimeEditor::onRecentProjectTriggered()
{
    RecentProjectsManager::instance()->showRecentProjectsMenu(this,
                                                              RecentProjectsManager::RuntimeEditor);
}

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
void RuntimeEditor::triggerSidebarView(const QString &viewName)
{
    SidebarWidget *sidebar = qobject_cast<SidebarWidget*>(sidebarDock->widget());
    if (!sidebar) {
        qWarning() << "Sidebar widget not found!";
        return;
    }

    QButtonGroup *buttonGroup = sidebar->findChild<QButtonGroup*>();
    if (!buttonGroup) {
        qWarning() << "Button group not found!";
        return;
    }

    QList<QAbstractButton*> buttons = buttonGroup->buttons();



    for (QAbstractButton *button : buttons) {
        QString btnViewName = button->property("viewName").toString();
        if (btnViewName == viewName) {
            button->click();
            return;
        }
    }

    qWarning() << "No button found with viewName:" << viewName;
}
void RuntimeEditor::triggerDisplayTab(const QString &tabName)
{
    if (!displayTabs) {
        qWarning() << "displayTabs not found!";
        return;
    }
    for (int i = 0; i < displayTabs->count(); ++i) {
        if (displayTabs->tabText(i) == tabName) {
            displayTabs->setCurrentIndex(i);
            return;
        }
    }
}

void RuntimeEditor::onRunScriptFileRequested(const QString& filePath)
{
    if (!QFile::exists(filePath)) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    QString scriptSource = in.readAll();
    file.close();
    runtime->scriptengine->loadAndCompileScript(scriptSource);
}
void RuntimeEditor::showPanelContextMenu(const QPoint &pos)
{
    QMenu contextMenu("Panels", this);
    contextMenu.setStyleSheet(R"(
        QMenu {
        background-color: #1A3652;
           color: white;
            border: 1px solid #cccccc;
            border-radius: 3px;
            padding: 5px 0px;
            font-size: 12px;
        }
        QMenu::item {
            background-color: transparent;
               color: white;
            padding: 6px 30px 6px 30px;  /* Space for indicator */
            margin: 2px 5px;
            border-radius: 2px;
        }
        QMenu::item:selected {
            background-color: #27446d;
              color: white;
        }
        /* CHECKED ITEMS - BOLD */
        QMenu::item:checked {
            background-color: #1A3652;
           color: white;
            font-weight: bold;
        }
        QMenu::item:checked:selected {
            background-color: #27446d
        }
        /* INDICATOR (CHECKBOX) STYLING */
        QMenu::indicator {
            width: 16px;
            height: 16px;
            margin-left: 5px;
        }
        /* UNCHECKED STATE - EMPTY BOX */
        QMenu::indicator:unchecked {
            image: none;
            border: 1px solid #999999;
            background-color: white;
            border-radius: 2px;
        }
        /* CHECKED STATE - YOUR CHECK.PNG IMAGE */
        QMenu::indicator:checked {
            image: url(:/icons/images/check.png);
            border: 1px solid #0078D4;
            background-color: #0078D4;
            border-radius: 2px;
        }
        /* SEPARATOR */
        QMenu::separator {
            height: 1px;
            background-color: #e0e0e0;
            margin: 5px 10px;
        }
    )");
    QAction *showHierarchy = contextMenu.addAction("Hierarchy");
    showHierarchy->setCheckable(true);
    showHierarchy->setChecked(hierarchyDock->isVisible());

    QAction *showLayers = contextMenu.addAction("Layers");
    showLayers->setCheckable(true);
    showLayers->setChecked(layerDock->isVisible());

    contextMenu.addSeparator();

    QAction *showConsole = contextMenu.addAction("Console");
    showConsole->setCheckable(true);
    showConsole->setChecked(consoleDock->isVisible());

    QAction *showSidebar = contextMenu.addAction("Sidebar");
    showSidebar->setCheckable(true);
    showSidebar->setChecked(sidebarDock->isVisible());

    QAction *selected = contextMenu.exec(mapToGlobal(pos));

    if (selected == showHierarchy) {
        hierarchyDock->setVisible(!hierarchyDock->isVisible());
        if (hierarchyDock->isVisible()) hierarchyDock->raise();
    }
    else if (selected == showLayers) {
        layerDock->setVisible(!layerDock->isVisible());
        if (layerDock->isVisible()) layerDock->raise();
    }
    else if (selected == showConsole) {
        consoleDock->setVisible(!consoleDock->isVisible());
        if (consoleDock->isVisible()) {
            consoleDock->raise();
            int winW = width();
            int winH = height();
            consoleDock->setGeometry(20, winH - 200, winW - 40, 150);
        }
    }
    else if (selected == showSidebar) {
        sidebarDock->setVisible(!sidebarDock->isVisible());
        if (sidebarDock->isVisible()) sidebarDock->raise();
    }
}
