/* =============================================================================
 * FILE:         scenarioeditor.cpp
 * MODULE:       Scenario Editor Main Window
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the ScenarioEditor class which provides the main
 *               window for scenario creation and editing. It manages hierarchy
 *               tree views (mission and library), tactical display (2D canvas),
 *               inspector panels, console view, toolbars, menu bar, text script
 *               widget, layer panel, and dock widgets. Supports loading/saving
 *               JSON scenario files, tracking unsaved changes, resetting layout,
 *               recent projects, script execution, and displaying profile/
 *               application information.
 *
 * REQUIREMENTS: Implements REQ-SCENARIO-010 through REQ-SCENARIO-025
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-SCENARIO-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#include "GUI/Editors/scenarioeditor.h"            // For scenario editor class
#include "GUI/Menubars/menubar.h"                 // For menu bar
#include "GUI/Sidebar/sidebarwidget.h"            // For sidebar widget
#include "GUI/Tacticaldisplay/tacticaldisplay.h"  // For tactical display
#include "GUI/Feedback/projectinformation.h"               // For feedback window
#include <QFile>                                  // For file operations
#include <QTextStream>                            // For text streaming
#include <QListWidget>                            // For list widget
#include <QDockWidget>                            // For dock widget
#include <QSplitter>                              // For splitter widget
#include <core/structure/scenario.h>              // For scenario structure
#include "GUI/Tacticaldisplay/canvaswidget.h"     // For canvas widget
#include "GUI/Toolbars/standardtoolbar.h"         // For standard toolbar
#include "qstandardpaths.h"
#include "qthread.h"                              // For thread operations
#include <core/Render/scenerenderer.h>            // For scene renderer
#include <core/structure/runtime.h>               // For runtime structure
#include <core/Hierarchy/Components/transform.h>  // For transform component
#include <core/Hierarchy/Components/mesh.h>       // For mesh component
#include <QJsonDocument>                          // For JSON document handling
#include <QJsonParseError>                        // For JSON parse errors
#include <QMessageBox>                            // For message box
#include <GUI/measuredistance/measuredistancedialog.h> // For measure distance dialog
#include <QApplication>                           // For application instance
#include <QTimer>                                 // For delayed operations
#include <GUI/Menubars/profileinfodialog.h>       // For profile info dialog
#include <GUI/Editors/recentprojectsmanager.h>    // For recent projects manager
#include <GUI/Settings/applicationdialog.h>       // For application settings dialog
#include <QProgressDialog>                        // For progress indication
#include <QShowEvent>
#include <core/Config/scenarioconfig.h>
#include "GUI/Tacticaldisplay/Gis/layerpanel.h"
#include <GUI/Editors/customresizableoverlaydock.h>
// %%% String Utility Function %%%
/* Capitalize the first letter of a string */
static QString capitalizeFirstLetter(const QString &str)
{
    if (str.isEmpty()) return str;
    return str[0].toUpper() + str.mid(1);
}
// %%% Filter Helper %%%
/* Remove sub-component sections from entity JSON for inspector display */
static QJsonObject filterEntityJsonForInspector(const QJsonObject &entityJson)
{
    // Keys to suppress from the inspector entity view
    static const QStringList hiddenKeys = {
        "Iffs", "iffs",
        "Sensors", "sensors",
        "Radios", "radios",
        "Weapons", "weapons"
    };

    QJsonObject filtered = entityJson;
    for (const QString &key : hiddenKeys) {
        filtered.remove(key);
    }
    return filtered;
}
// %%% Constructor %%%
/* Initialize scenario editor */
ScenarioEditor::ScenarioEditor(QWidget *parent)
    : QMainWindow(parent)
{

    m_scenarioConfig = new ScenarioConfig(this);
    setWindowTitle("Scenario Editor");
    resize(1100, 600);
    setupEnhancedDockWidgets();
    setupToolBars();
    // Initialize scenario components
    Scenario *scenario = new Scenario();
    hierarchy = scenario->hierarchy;
    SceneRenderer *renderer = scenario->scenerenderer;
    console = scenario->console;
    scriptengine = scenario->scriptengine;
    library = scenario->Library;
    lastSavedFilePath = "";
    // Setup script engine
    scenario->scriptengine->setHierarchy(hierarchy, treeView, renderer);
    connect(textScriptView, &TextScriptWidget::runScriptstring,
            scriptengine, &ScriptEngine::loadAndCompileScript);
    connect(textScriptView, &TextScriptWidget::runScriptFile,
            this, &ScenarioEditor::onRunScriptFileRequested);
    HierarchyConnector::instance()->setHierarchy(hierarchy);
    HierarchyConnector::instance()->setLibrary(library);
    HierarchyConnector::instance()->setLibTreeView(libTreeView);
    connect(RecentProjectsManager::instance(), &RecentProjectsManager::projectSelected,
            this, [=](const QString& filePath, RecentProjectsManager::EditorType type) {
                if (type == RecentProjectsManager::ScenarioEditor) {
                    loadRecentProject(filePath);
                }
            });
    connect(this, &ScenarioEditor::Activated,
            tacticalDisplay->canvas, &CanvasWidget::ReInit);
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

    // Setup tactical display and script engine connections
    if (tacticalDisplay && tacticalDisplay->canvas && tacticalDisplay->mapWidget) {
        scriptengine->setCanvas(tacticalDisplay->canvas);
        scriptengine->setGIS(tacticalDisplay->mapWidget);
        // Connect trajectory signals
        connect(tacticalDisplay->canvas, &CanvasWidget::trajectoryUpdated,
                inspector, [=](QString entityId, QJsonArray waypoints) {
                    if (inspector->getName() == "trajectory" &&
                        inspector->getConnectedID() == entityId) {
                        inspector->updateTrajectory(entityId, waypoints);
                    }
                });
        connect(tacticalDisplay->canvas, &CanvasWidget::trajectoryUpdated,
                this, [=](QString entityId, QJsonArray /*waypoints*/) {
                    auto it = tacticalDisplay->canvas->Meshes.find(entityId.toStdString());
                    if (it != tacticalDisplay->canvas->Meshes.end() && it->second.trajectory) {
                        QJsonObject trajData = it->second.trajectory->toJson();
                        hierarchy->UpdateComponent(entityId, "Trajectory", trajData);
                        treeView->getTreeWidget()->update();
                        markUnsavedChanges();
                    } else {
                    }
                });
        connect(inspector, &Inspector::trajectoryWaypointsChanged,
                tacticalDisplay->canvas, &CanvasWidget::updateWaypointsFromInspector);
    }
    // Connect renderer signals
    connect(renderer, &SceneRenderer::addMesh, tacticalDisplay, &TacticalDisplay::addMesh);
    connect(hierarchy, &Hierarchy::entityRemoved, tacticalDisplay, &TacticalDisplay::removeMesh);

    if (tacticalDisplay && tacticalDisplay->canvas) {
        connect(renderer, &SceneRenderer::Render,
                tacticalDisplay->canvas, &CanvasWidget::Render);
        connect(renderer, &SceneRenderer::Render,
                tacticalDisplay->scene3dwidget, &Scene3DWidget::updateEntities);
    }
    // Connect inspector value changes
    connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
    connect(inspector, &Inspector::valueChanged, this, [=]{
        renderer->Render(0.01f);
        markUnsavedChanges();
    });
    // Configure hierarchy connector signals
    HierarchyConnector::instance()->connectSignals(hierarchy,library, treeView,
                                                   tacticalDisplay, inspector);
    HierarchyConnector::instance()->connectLibrarySignals(library, libTreeView);
    HierarchyConnector::instance()->initializeDummyData(hierarchy);
    HierarchyConnector::instance()->initializeLibraryData(library);
    HierarchyConnector::instance()->setupFileOperations(this, hierarchy, tacticalDisplay);
    // Connect canvas selection signals
    if (tacticalDisplay && tacticalDisplay->canvas) {
        connect(tacticalDisplay->canvas, &CanvasWidget::selectEntitybyCursor,
                treeView, &HierarchyTree::selectEntityById);
        connect(tacticalDisplay->canvas, &CanvasWidget::selectEntitybyCursor,
                this, [=](const QString& entityId) {
                    // if (!hierarchy->Entities) return;
                    auto it = hierarchy->Entities.find(entityId.toStdString());
                    if (it == hierarchy->Entities.end()) return;

                    Entity* entity = it->second;
                    QString entityName = QString::fromStdString(entity->Name);
                    QString displayName = capitalizeFirstLetter(entityName);
                    if (!inspectorDock->isLocked()) {
                        for (Inspector* insp : inspectors) {
                            QJsonObject entityJson = filterEntityJsonForInspector(
                                (hierarchy->Entities)[entityId.toStdString()]->toJson()
                                );
                            insp->init(entityId, displayName + "_self", entityJson);
                        }
                    }
                    if (!inspectorDock->isVisible()) {
                        if (libraryDock) libraryDock->hide();
                        if (textScriptDock) textScriptDock->hide();
                        inspectorDock->show();
                        inspectorDock->raise();
                    }
                });
    }

    // Connect tree view item selection
    connect(treeView, &HierarchyTree::itemSelected, this, [=](QVariantMap data) {
        QString type;
        if (data["type"].type() == QVariant::Map) {
            QVariantMap typeData = data["type"].toMap();
            if (typeData.contains("type") && typeData["type"].toString() == "option") {
                type = "profile";
            }
        } else {
            type = data["type"].toString();
        }
        // ===== FORMATION MULTI-SELECT LOGIC =====
        if (type == "entity") {

        }
        QString name = data["name"].toString();
        QString ID = data["parentId"].toString();
        QString displayName = capitalizeFirstLetter(name);
        if (!inspectorDock->isLocked()) {

        for (Inspector* inspector : inspectors) {
            if (type == "subcomponent") {
                QJsonObject componentData = (hierarchy->Components)[data["parentId"].toString()
                                                                         .toStdString()]->getsubComponentData(data["ID"].toString()
                                                                          .toStdString());
                if (!componentData.isEmpty()) {
                    inspector->init(ID, displayName + "_sub", componentData);
                }
            } else if (type == "component") {
                QJsonObject componentData = hierarchy->getComponentData(ID, name);
                if (!componentData.isEmpty()) {
                    inspector->init(ID, displayName, componentData);
                }
            } else if (type == "profile") {
                inspector->init(ID, displayName + "_self",
                                (hierarchy->ProfileCategories)[data["ID"].toString()
                                                                   .toStdString()]->toJson());
            } else if (type == "folder") {
                inspector->init(ID, displayName + "_self",
                                (hierarchy->Folders)[data["ID"].toString()
                                                          .toStdString()]->toJson());
            } else if (type == "entity") {
                QJsonObject entityJson = filterEntityJsonForInspector(
                    (hierarchy->Entities)[data["ID"].toString().toStdString()]->toJson()
                    );
                inspector->init(data["ID"].toString(), displayName + "_self", entityJson);
            } else {
                inspector->init(ID, displayName, QJsonObject());
            }
        }
}
        if (!inspectorDock->isVisible()) {
            // Hide other right panel docks
            if (libraryDock && libraryDock->isVisible()) {
                libraryDock->hide();
            }
            if (textScriptDock && textScriptDock->isVisible()) {
                textScriptDock->hide();
            }
            // Show inspector
            addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
            splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
            inspectorDock->show();
            inspectorDock->raise();
        }
        if (tacticalDisplay && type == "entity") {
            tacticalDisplay->selectedMesh(data["ID"].toString());
        }
        else if (tacticalDisplay && (type == "subcomponent" || type == "component")) {
            QString parentEntityId = data["parentId"].toString();
            if (type == "subcomponent") {
                if (hierarchy ) {
                    auto it = hierarchy->Components.find(
                        data["parentId"].toString().toStdString());
                    if (it != hierarchy->Components.end()) {
                        parentEntityId = QString::fromStdString(it->second->parentID);
                    }
                }
            }
            if (hierarchy ) {
                auto it = hierarchy->Entities.find(parentEntityId.toStdString());
                if (it != hierarchy->Entities.end()) {
                    tacticalDisplay->selectedMesh(parentEntityId);
                }
            }
        }
    });
    connect(treeView, &HierarchyTree::formationSelectWithEntity,
            this, [this](const QString& formationId) {
                if (!hierarchy) return;
                auto it = hierarchy->Entities.find(formationId.toStdString());
                if (it == hierarchy->Entities.end()) return;
                Formation* formation = dynamic_cast<Formation*>(it->second);
                if (!formation) return;
                QList<QString> entityIds;
                entityIds.append(formationId);
                // Mothership
                if (formation->mothership && formation->mothership->entity) {
                    QString motherId = QString::fromStdString(formation->mothership->entity->ID);
                    if (motherId != "dummy" && !motherId.isEmpty())
                        entityIds.append(motherId);
                }

                // Allies
                if (formation->formationPositions) {
                    for (const auto& pair : *formation->formationPositions) {
                        FormationPosition* pos = pair.second;
                        if (pos && pos->entity) {
                            QString allyId = QString::fromStdString(pos->entity->ID);
                            if (allyId != "dummy" && !allyId.isEmpty())
                                entityIds.append(allyId);
                        }
                    }
                }
                treeView->selectMultipleEntitiesInTree(entityIds);
                if (tacticalDisplay && tacticalDisplay->canvas)
                    tacticalDisplay->canvas->selectMultipleEntities(entityIds);
            });
    // ===== AUTO-CENTER MAP ON ENTITY SELECTION =====
    connect(treeView, &HierarchyTree::itemSelected, this, [=](QVariantMap data) {
        QString type;
        if (data["type"].type() == QVariant::Map) {
            QVariantMap typeData = data["type"].toMap();
            if (typeData.contains("type") && typeData["type"].toString() == "option") {
                type = "profile";
            }
        } else {
            type = data["type"].toString();
        }
        if (type == "entity" && tacticalDisplay && tacticalDisplay->canvas) {
            QString entityId = data["ID"].toString();
            tacticalDisplay->canvas->centerOnEntity(entityId, false);
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
    // Connect inspector tab signals
    connect(inspector, &Inspector::addTabRequested, this, &ScenarioEditor::addInspectorTab);
    inspectorDocks.append(inspectorDock);
    inspectors.append(inspector);
    inspector->setHierarchy(hierarchy);
    setupToolBarConnections();
    connect(hierarchy, &Hierarchy::profileAdded, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderAdded, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityAdded, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::componentAdded, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::subComponentAdded, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::profileRemoved, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderRemoved, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityRemoved, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::componentRemoved, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::subComponentRemoved, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::profileRenamed, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderRenamed, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityRenamed, this, &ScenarioEditor::markUnsavedChanges);

}
void ScenarioEditor::setupEnhancedDockWidgets()
{
    tacticalDisplay = new TacticalDisplay(this);
    setCentralWidget(tacticalDisplay);
    this->setStyleSheet("QMainWindow { background-color: #0F2636; }");
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
                          // "padding: 4px; "
                          "font-weight: bold; }";

        dock->setStyleSheet(dockStyleSheet);
        if (content) {
            content->setStyleSheet("background-color: #0F2636; color: white;");
        }
    };

    // --- 2. Sidebar (Top-Right) ---
    sidebarDock = new CustomResizableOverlayDock("Sidebar", this);
    SidebarWidget *sidebar = new SidebarWidget(this);
    sidebar->setSensorsButtonVisible(false);
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

    inspector = new Inspector(this);
    inspector->setHierarchy(hierarchy);
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

    // --- 4. Left Side Panel (isLeftPanel = true) ---
    treeView = new HierarchyTree(this);
    treeView->setStyleSheet("background-color: #0F2636; color: white;");
    setupOverlay(hierarchyDock, treeView, "Hierarchy", true);

    // --- 5. LAYERS PANEL (Left Side, Hierarchy ke neeche) ---
    layerPanel = new LayerPanel(this);
    layerPanel->setStyleSheet("background-color: #0F2636; color: white;");
    layerDock = new CustomResizableOverlayDock("Layers", this);
    layerDock->setWidget(layerPanel);
    layerDock->setFloating(true);
    layerDock->setParent(this);
    // Enable title bar with close button for layers panel
    layerDock->setWindowFlags(Qt::SubWindow | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
    layerDock->setMinimumWidth(280);
    layerDock->setMinimumHeight(150);
    layerDock->handlePos = CustomResizableOverlayDock::Right;
    layerDock->setStyleSheet("CustomResizableOverlayDock { "
                             "background-color: #0F2636; "
                             "color: white; "
                             "border-right: 4px solid #00BFFF; }"
                             "CustomResizableOverlayDock::title { "
                             "background-color: #1A3A4F; "
                             "color: white; "
                             "font-weight: bold; }");
    // Connect layer panel to canvas
    if (tacticalDisplay && tacticalDisplay->canvas) {
        tacticalDisplay->canvas->setLayerPanel(layerPanel);
        if (tacticalDisplay->canvas->getShapesFeature()) {
            tacticalDisplay->canvas->getShapesFeature()->setLayerPanel(layerPanel);
            layerPanel->setCanvasWidget(tacticalDisplay->canvas);
        }
        connect(layerPanel, &LayerPanel::shapeClicked,
                tacticalDisplay->canvas, &CanvasWidget::centerOnShape);
        connect(tacticalDisplay->canvas, &CanvasWidget::shapeSelectedFromCanvas,
                layerPanel, &LayerPanel::selectShapeInPanel);
    }

    // --- 6. CONSOLE (Bottom) ---
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
    int winW = width() > 0 ? width() : 1100;
    int winH = height() > 0 ? height() : 600;
    int panelWidth = 300;
    int rightToolbarWidth = 30;
    int rightX = winW - panelWidth - rightToolbarWidth - 5;
    int leftX = 20;
    int topY = 80;
    int hierarchyHeight = 500;
    hierarchyDock->setGeometry(leftX, topY, panelWidth, hierarchyHeight);
    hierarchyDock->show();

    int hierarchyBottom = hierarchyDock->y() + hierarchyDock->height();
    layerDock->setGeometry(leftX, hierarchyBottom + 5, panelWidth, 150);
    layerDock->show();

    sidebarDock->setGeometry(rightX, topY, panelWidth, 28);
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

    consoleDock->setGeometry(20, winH - 180, winW - rightToolbarWidth - 25, 150);
    consoleDock->hide();
    hierarchyDock->raise();
    layerDock->raise();
    sidebarDock->raise();
    inspectorDock->raise();

    // --- CONNECT SIDEBAR VIEW SELECTION ---
    connect(sidebar, &SidebarWidget::viewSelected, this, [this](const QString &viewName) {
        inspectorDock->hide();
        libraryDock->hide();
        textScriptDock->hide();
        if (viewName == "Library") {
            // Pehle se open ho toh raise karo
            for (QDialog *dlg : findChildren<QDialog*>()) {
                if (dlg->windowTitle().startsWith("Library")) {
                    dlg->raise();
                    dlg->activateWindow();
                    return;
                }
            }
            QDialog *libDialog = new QDialog(this);
            QString currentTitle = libraryDock->windowTitle();
            libDialog->setWindowTitle(currentTitle.isEmpty() ? "Library" : currentTitle);
            libDialog->setAttribute(Qt::WA_DeleteOnClose);
            libDialog->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint |
                                      Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
            libDialog->resize(350, 600);
            QVBoxLayout *layout = new QVBoxLayout(libDialog);
            layout->setContentsMargins(0, 0, 0, 0);
            libTreeView->setParent(libDialog);
            layout->addWidget(libTreeView);
            libDialog->setStyleSheet("QDialog { background-color: #0F2636; color: white; }");
            connect(libTreeView, &HierarchyTree::libraryFileNameChanged,
                    libDialog, [=](const QString& fileName) {
                        libDialog->setWindowTitle(
                            (fileName.isEmpty() || fileName == "Library")
                                ? "Library" : "Library - " + fileName);
                    });
            connect(libDialog, &QDialog::finished, this, [=]() {
                libTreeView->setParent(libraryDock);
                libraryDock->setWidget(libTreeView);
            });
            libDialog->show();
            return;
        } else if (viewName == "TextScript") {
            for (QDialog *dlg : findChildren<QDialog*>()) {
                if (dlg->windowTitle() == "TestScript") {
                    dlg->raise();
                    dlg->activateWindow();
                    return;
                }
            }
            QDialog *tsDialog = new QDialog(this);
            tsDialog->setWindowTitle("TestScript");
            tsDialog->setAttribute(Qt::WA_DeleteOnClose);
            tsDialog->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint |
                                     Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
            tsDialog->resize(450, 600);
            QVBoxLayout *layout = new QVBoxLayout(tsDialog);
            layout->setContentsMargins(4, 4, 4, 4);
            textScriptView->setParent(tsDialog);
            layout->addWidget(textScriptView);
            tsDialog->setStyleSheet("QDialog { background-color: #0F2636; color: white; }");
            connect(tsDialog, &QDialog::finished, this, [=]() {
                textScriptView->setParent(textScriptDock);
                textScriptDock->setWidget(textScriptView);
            });
            tsDialog->show();
            return;
        } else if (viewName == "Console") {
            consoleDock->setVisible(!consoleDock->isVisible());
            if (consoleDock->isVisible()) consoleDock->raise();
            return;
        }
        CustomResizableOverlayDock* target = nullptr;
        if (viewName == "Inspector") target = inspectorDock;

        if (target) {
            QRect sGeo = sidebarDock->geometry();
            target->setGeometry(sGeo.x(), sGeo.y() + sGeo.height(),
                                sGeo.width(), height() - sGeo.y() - sGeo.height() - 150);
            target->show();
            target->raise();
        }
    });
    connect(sidebarDock, &CustomResizableOverlayDock::moved, this, [this](QPoint oldPos, QPoint newPos) {
        QPoint delta = newPos - oldPos;
        if (inspectorDock && inspectorDock->isVisible())
            inspectorDock->move(inspectorDock->pos() + delta);
        if (libraryDock && libraryDock->isVisible())
            libraryDock->move(libraryDock->pos() + delta);
        if (textScriptDock && textScriptDock->isVisible())
            textScriptDock->move(textScriptDock->pos() + delta);
    });
    connect(sidebarDock, &CustomResizableOverlayDock::resized, this, [this](QSize oldSize, QSize newSize) {
        int newWidth = newSize.width();
        if (inspectorDock && inspectorDock->isVisible())
            inspectorDock->setFixedWidth(newWidth);
        if (libraryDock && libraryDock->isVisible())
            libraryDock->setFixedWidth(newWidth);
        if (textScriptDock && textScriptDock->isVisible())
            textScriptDock->setFixedWidth(newWidth);
    });
    connect(libTreeView, &HierarchyTree::libraryFileNameChanged,
            this, [=](const QString& fileName) {
                if (fileName == "Library") {
                    libraryDock->setWindowTitle("Library");
                } else {
                    libraryDock->setWindowTitle("Library - " + fileName);
                }
            });
    inspectorDocks.append(inspectorDock);
    inspectors.append(inspector);
}


void ScenarioEditor::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    int winW = width();
    int winH = height();
    int panelWidth = 300;
    int rightToolbarWidth = rightToolBar ? rightToolBar->width() : 30;
    int rightX = winW - panelWidth - rightToolbarWidth - 5;
    int leftX = 20;
    int toolbarHeight = designToolBar ? designToolBar->height() : 30;
    int topY = toolbarHeight + 80;
    int hierarchyHeight = 500;
    if (hierarchyDock)
        hierarchyDock->setGeometry(leftX, topY, panelWidth, hierarchyHeight);
    if (layerDock) {
        int hierarchyBottom = hierarchyDock
                                  ? hierarchyDock->y() + hierarchyDock->height()
                                  : topY + hierarchyHeight;
        layerDock->setGeometry(leftX, hierarchyBottom + 5, panelWidth, 150);
    }
    if (sidebarDock)
        sidebarDock->setGeometry(rightX, topY, panelWidth, 45);
    int sidebarBottom = sidebarDock
                            ? sidebarDock->y() + sidebarDock->height()
                            : topY + 45;
    int rightPanelHeight = winH - sidebarBottom - 100;
    if (inspectorDock && inspectorDock->isVisible())
        inspectorDock->setGeometry(rightX, sidebarBottom, panelWidth, rightPanelHeight);
    if (libraryDock && libraryDock->isVisible())
        libraryDock->setGeometry(rightX, sidebarBottom, panelWidth, rightPanelHeight);
    if (textScriptDock && textScriptDock->isVisible())
        textScriptDock->setGeometry(rightX, sidebarBottom, panelWidth, rightPanelHeight);
    if (consoleDock && consoleDock->isVisible())
        consoleDock->setGeometry(leftX, winH - 190,
                                 winW - rightToolbarWidth - leftX - 5, 150);
}
void ScenarioEditor::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    QTimer::singleShot(50, this, [this]() {
        QResizeEvent *re = new QResizeEvent(size(), size());
        resizeEvent(re);
        delete re;
    });
}
/* Setup dock widgets - legacy method */
void ScenarioEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures)
{
    setupEnhancedDockWidgets();
}

// %%% Dock Visibility Handler %%%
/* Handle dock visibility changes */
void ScenarioEditor::onDockVisibilityChanged(bool visible)
{
    QDockWidget* dock = qobject_cast<QDockWidget*>(sender());
    if (dock && visible) {
        dock->raise();
    }
}
// %%% Toolbars Setup %%%
/* Setup toolbars */
void ScenarioEditor::setupToolBars()
{
    designToolBar = new DesignToolBar(this, m_scenarioConfig);
    addToolBar(Qt::TopToolBarArea, designToolBar);
    designToolBar->setMovable(true);
    designToolBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(designToolBar, &QToolBar::customContextMenuRequested,
            this, &ScenarioEditor::showPanelContextMenu);

    rightToolBar = new QToolBar("Right Tools", this);
    rightToolBar->setOrientation(Qt::Vertical);
    rightToolBar->setMovable(false);
    rightToolBar->setIconSize(QSize(20, 20));
    rightToolBar->setStyleSheet(
        "QToolBar { background-color: #0F2636; border-left: 1px solid #00BFFF; padding: 0px; }"
        );
    addToolBar(Qt::RightToolBarArea, rightToolBar);
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    rightToolBar->addWidget(spacer);
    QToolButton *libBtn = new QToolButton();
    libBtn->setToolTip("Library");
    libBtn->setIcon(QIcon(":/icons/images/library.png"));
    libBtn->setFixedSize(20, 20);
    libBtn->setCheckable(true);
    libBtn->setStyleSheet(
        "QToolButton { background-color: #1A3A4F; color: #00BFFF; "
        "border: 1px solid #00BFFF; border-radius: 4px; }"
        "QToolButton:hover { background-color: #00BFFF; color: #0F2636; }"
        "QToolButton:pressed { background-color: #0090CC; }"
        "QToolButton:checked { background-color: #00BFFF; color: #0F2636; }"
        );

    connect(libBtn, &QToolButton::clicked, this, [=](bool checked) {
        if (checked) {
            for (QDialog *dlg : findChildren<QDialog*>()) {
                if (dlg->windowTitle().startsWith("Library")) {
                    dlg->raise();
                    dlg->activateWindow();
                    libBtn->setChecked(true);
                    return;
                }
            }
            QDialog *libDialog = new QDialog(this);
            QString currentTitle = libraryDock->windowTitle();
            libDialog->setWindowTitle(currentTitle.isEmpty() ? "Library" : currentTitle);
            libDialog->setAttribute(Qt::WA_DeleteOnClose);
            libDialog->setWindowFlags(Qt::Dialog |
                                      Qt::WindowTitleHint |
                                      Qt::WindowCloseButtonHint |
                                      Qt::WindowMinMaxButtonsHint);
            libDialog->resize(350, 600);
            QVBoxLayout *layout = new QVBoxLayout(libDialog);
            layout->setContentsMargins(0, 0, 0, 0);
            libTreeView->setParent(libDialog);
            layout->addWidget(libTreeView);
            libDialog->setStyleSheet("QDialog { background-color: #0F2636; color: white; }");
            connect(libTreeView, &HierarchyTree::libraryFileNameChanged,
                    libDialog, [=](const QString& fileName) {
                        if (fileName == "Library" || fileName.isEmpty()) {
                            libDialog->setWindowTitle("Library");
                        } else {
                            libDialog->setWindowTitle("Library - " + fileName);
                        }
                    });

            connect(libDialog, &QDialog::finished, this, [=]() {
                libBtn->setChecked(false);
                libTreeView->setParent(libraryDock);
                libraryDock->setWidget(libTreeView);
            });
            libDialog->show();
        } else {
            for (QDialog *dlg : findChildren<QDialog*>()) {
                if (dlg->windowTitle().startsWith("Library")) {
                    dlg->close();
                    break;
                }
            }
        }
    });
    connect(libraryDock, &CustomResizableOverlayDock::visibilityChanged,
            this, [=](bool visible) { libBtn->setChecked(visible); });
    rightToolBar->addWidget(libBtn);
    QToolButton *tsBtn = new QToolButton();
    tsBtn->setToolTip("TestScript");
    tsBtn->setIcon(QIcon(":/icons/images/TestScript.png"));
    tsBtn->setFixedSize(20, 20);
    tsBtn->setCheckable(true);
    tsBtn->setStyleSheet(
        "QToolButton { background-color: #1A3A4F; color: #00BFFF; "
        "border: 1px solid #00BFFF; border-radius: 4px; }"
        "QToolButton:hover { background-color: #00BFFF; color: #0F2636; }"
        "QToolButton:pressed { background-color: #0090CC; }"
        "QToolButton:checked { background-color: #00BFFF; color: #0F2636; }"
        );

    connect(tsBtn, &QToolButton::clicked, this, [=](bool checked) {
        if (checked) {
            for (QDialog *dlg : findChildren<QDialog*>()) {
                if (dlg->windowTitle() == "TestScript") {
                    dlg->raise();
                    dlg->activateWindow();
                    tsBtn->setChecked(true);
                    return;
                }
            }
            QDialog *tsDialog = new QDialog(this);
            tsDialog->setWindowTitle("TestScript");
            tsDialog->setAttribute(Qt::WA_DeleteOnClose);
            tsDialog->setWindowFlags(Qt::Dialog |
                                     Qt::WindowTitleHint |
                                     Qt::WindowCloseButtonHint |
                                     Qt::WindowMinMaxButtonsHint);
            tsDialog->resize(450, 600);
            QVBoxLayout *layout = new QVBoxLayout(tsDialog);
            layout->setContentsMargins(4, 4, 4, 4);
            textScriptView->setParent(tsDialog);
            layout->addWidget(textScriptView);
            tsDialog->setStyleSheet("QDialog { background-color: #0F2636; color: white; }");
            connect(tsDialog, &QDialog::finished, this, [=]() {
                tsBtn->setChecked(false);
                textScriptView->setParent(textScriptDock);
                textScriptDock->setWidget(textScriptView);
            });
            tsDialog->show();
        } else {
            for (QDialog *dlg : findChildren<QDialog*>()) {
                if (dlg->windowTitle() == "TestScript") {
                    dlg->close();
                    break;
                }
            }
        }
    });
    connect(textScriptDock, &CustomResizableOverlayDock::visibilityChanged,
            this, [=](bool visible) { tsBtn->setChecked(visible); });
    rightToolBar->addWidget(tsBtn);
}
// %%% Toolbar Connections Setup %%%
/* Setup toolbar connections */
void ScenarioEditor::setupToolBarConnections()
{
    DesignToolBar *designToolBar = findChild<DesignToolBar*>();
    if (!designToolBar || !tacticalDisplay || !tacticalDisplay->canvas) {
        return;
    }
    // Connect coordinate system changes
    connect(designToolBar, &DesignToolBar::coordinateSystemChanged,
            tacticalDisplay->mapWidget, &GISlib::setCoordinateSystem);
    // Connect transform mode changes
    connect(designToolBar, &DesignToolBar::modeChanged,
            this, [=](int mode) {
                tacticalDisplay->canvas->setTransformMode(static_cast<TransformMode>(mode));
            });
    // Connect shape selection
    connect(designToolBar, &DesignToolBar::shapeSelected,
            this, [=](const QString &shape) {
                tacticalDisplay->canvas->setShapeDrawingMode(true, shape);
            });
    // Connect grid visibility
    connect(designToolBar, &DesignToolBar::gridPlaneXToggled,
            tacticalDisplay->canvas, &CanvasWidget::setXGridVisible);
    connect(designToolBar, &DesignToolBar::gridPlaneYToggled,
            tacticalDisplay->canvas, &CanvasWidget::setYGridVisible);
    // Connect grid opacity
    connect(designToolBar, &DesignToolBar::gridOpacityChanged,
            this, [=](int opacity) {
                tacticalDisplay->canvas->setGridOpacity(opacity);
            });
    connect(designToolBar, &DesignToolBar::tooltipOptionsChanged,
            tacticalDisplay->canvas, &CanvasWidget::setTooltipOptions);
    // Connect layer visibility
    connect(designToolBar, &DesignToolBar::layerOptionToggled,
            tacticalDisplay->canvas, &CanvasWidget::toggleLayerVisibility);
    // Connect bitmap selection
    connect(designToolBar, &DesignToolBar::bitmapImageSelected,
            tacticalDisplay->canvas, &CanvasWidget::onBitmapImageSelected);
    connect(designToolBar, &DesignToolBar::presetLayerSelected,
            tacticalDisplay->canvas, &CanvasWidget::onPresetLayerSelected);
    connect(designToolBar, &DesignToolBar::bitmapSelected,
            this, [=](const QString &fileName) {
                tacticalDisplay->canvas->onBitmapSelected(fileName);
            });
    // Connect map layer changes
    if (tacticalDisplay && tacticalDisplay->mapWidget) {
        connect(designToolBar, &DesignToolBar::mapLayerChanged,
                this, [=](const QString &layers) {
                    tacticalDisplay->setMapLayers(layers.split(",", Qt::SkipEmptyParts));
                });
        connect(designToolBar, &DesignToolBar::customMapAdded,
                tacticalDisplay, &TacticalDisplay::addCustomMap);
        connect(designToolBar, &DesignToolBar::searchPlaceTriggered,
                tacticalDisplay->mapWidget, &GISlib::serachPlace);
        connect(designToolBar, &DesignToolBar::searchCoordinatesTriggered,
                tacticalDisplay->mapWidget, &GISlib::searchByCoordinates);
        connect(designToolBar->zoomInAction, &QAction::triggered,
                tacticalDisplay, &TacticalDisplay::zoomIn);
        connect(designToolBar->zoomOutAction, &QAction::triggered,
                tacticalDisplay, &TacticalDisplay::zoomOut);
        connect(designToolBar->selectCenterAction, &QAction::triggered, this, [=]() {
            if (tacticalDisplay && tacticalDisplay->mapWidget) {
                tacticalDisplay->mapWidget->setCenter(0, 0);
            }
        });
    }

    // Connect trajectory actions
    connect(designToolBar->getAddTrajectoryAction(), &QAction::triggered,
            this, [=]() {
                tacticalDisplay->canvas->setTrajectoryDrawingMode(true);
            });
    connect(designToolBar, &DesignToolBar::importLayerTriggered,
            tacticalDisplay->getCanvasWidget(), &CanvasWidget::importLayer);
    connect(tacticalDisplay->canvas, &CanvasWidget::geoJsonLayerAdded,
            designToolBar, &DesignToolBar::onGeoJsonLayerAdded);
    connect(designToolBar, &DesignToolBar::geoJsonLayerToggled,
            tacticalDisplay->canvas, &CanvasWidget::onGeoJsonLayerToggled);
    if (designToolBar->getMeasureDistanceAction()) {
        connect(designToolBar->getMeasureDistanceAction(), &QAction::triggered,
                tacticalDisplay->canvas, [=]() {
                    bool isChecked = designToolBar->getMeasureDistanceAction()->isChecked();
                    tacticalDisplay->canvas->setTransformMode(isChecked ? MeasureDistance : Translate);
                });
    }
}

// %%% Layout Reset %%%
/* Reset layout to initial state */
void ScenarioEditor::resetLayout()
{
    // Cleanup additional inspector docks
    for (int i = inspectorDocks.size() - 1; i >= 0; --i) {
        QDockWidget* dock = inspectorDocks[i];
        if (dock != inspectorDock) {
            inspectors.removeAt(i);
            dock->deleteLater();
        }
    }

    // Reset inspector lists
    if (inspectorDocks.size() > 1) {
        inspectorDocks = QList<QDockWidget*>{inspectorDock};
        inspectors = QList<Inspector*>{inspector};
    }
    inspectorCount = 0;

    // Hide all docks
    hierarchyDock->hide();
    tacticalDisplayDock->hide();
    consoleDock->hide();
    inspectorDock->hide();
    libraryDock->hide();
    sidebarDock->hide();
    textScriptDock->hide();

    // Remove all docks
    removeDockWidget(hierarchyDock);
    removeDockWidget(tacticalDisplayDock);
    removeDockWidget(consoleDock);
    removeDockWidget(inspectorDock);
    removeDockWidget(libraryDock);
    removeDockWidget(sidebarDock);
    removeDockWidget(textScriptDock);

    // Re-add docks to initial positions
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);
    addDockWidget(Qt::RightDockWidgetArea, tacticalDisplayDock);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);
    addDockWidget(Qt::RightDockWidgetArea, sidebarDock);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    addDockWidget(Qt::RightDockWidgetArea, libraryDock);
    addDockWidget(Qt::RightDockWidgetArea, textScriptDock);

    // Hide non-default docks
    libraryDock->hide();
    textScriptDock->hide();

    // Recreate initial splits
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

    // Reset sizes with delay
    QTimer::singleShot(100, this, [=]() {
        QMainWindow::resize(1100, 600);

        int totalWidth = this->width();
        int totalHeight = this->height();

        int hierarchyWidth = static_cast<int>(totalWidth * 0.10);
        int tacticalWidth = static_cast<int>(totalWidth * 0.75);
        int sidebarWidth = static_cast<int>(totalWidth * 0.06);
        int inspectorWidth = static_cast<int>(totalWidth * 0.09);
        int consoleHeight = static_cast<int>(totalHeight * 0.15);

        resizeDocks({hierarchyDock}, {hierarchyWidth}, Qt::Horizontal);
        resizeDocks({tacticalDisplayDock}, {tacticalWidth}, Qt::Horizontal);
        resizeDocks({sidebarDock}, {sidebarWidth}, Qt::Horizontal);
        resizeDocks({inspectorDock}, {inspectorWidth}, Qt::Horizontal);
        resizeDocks({consoleDock}, {consoleHeight}, Qt::Vertical);

        // Bring docks to front
        hierarchyDock->raise();
        tacticalDisplayDock->raise();
        consoleDock->raise();
        sidebarDock->raise();
        inspectorDock->raise();

    });
}

// %%% Inspector Tab Addition %%%
/* Add new inspector tab */
void ScenarioEditor::addInspectorTab()
{
    // Create new inspector dock
    QDockWidget *newInspectorDock = new QDockWidget("Inspector " +
                                                        QString::number(++inspectorCount), this);
    newInspectorDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    newInspectorDock->setFeatures(QDockWidget::DockWidgetClosable |
                                  QDockWidget::DockWidgetMovable |
                                  QDockWidget::DockWidgetFloatable);

    Inspector *newInspector = new Inspector(newInspectorDock);
    newInspectorDock->setWidget(newInspector);
    newInspectorDock->setMinimumWidth(200);
    newInspectorDock->setTitleBarWidget(nullptr);

    // Add to lists
    inspectorDocks.append(newInspectorDock);
    inspectors.append(newInspector);

    // Connect inspector signals
    connect(newInspector, &Inspector::valueChanged,
            hierarchy, &Hierarchy::UpdateComponent);
    connect(newInspector, &Inspector::valueChanged,
            this, &ScenarioEditor::markUnsavedChanges);
    connect(newInspector, &Inspector::addTabRequested,
            this, &ScenarioEditor::addInspectorTab);
    connect(newInspectorDock, &QDockWidget::visibilityChanged,
            this, &ScenarioEditor::onDockVisibilityChanged);

    // Handle dock destruction
    connect(newInspectorDock, &QDockWidget::destroyed, this, [=]() {
        inspectorDocks.removeOne(newInspectorDock);
        inspectors.removeOne(newInspector);
    });

    // Position and show new dock
    if (inspectorDock->isVisible()) {
        splitDockWidget(inspectorDock, newInspectorDock, Qt::Horizontal);
    } else {
        addDockWidget(Qt::RightDockWidgetArea, newInspectorDock);
    }

    newInspectorDock->show();
    newInspectorDock->raise();
}

// %%% Feedback Window %%%
/* Show feedback window */
void ScenarioEditor::showFeedbackWindow()
{
    Feedback *feedbackWindow = new Feedback(this);
    feedbackWindow->show();
}

// %%% Item Selection Handlers %%%
/* Handle item selection */
void ScenarioEditor::onItemSelected(QVariantMap /*data*/)
{
    // TODO: Implement item selection logic
}

/* Handle library item selection */
void ScenarioEditor::onLibraryItemSelected(QVariantMap /*data*/)
{
    // TODO: Implement library item selection logic
}

// %%% Destructor %%%
/* Cleanup resources */
ScenarioEditor::~ScenarioEditor()
{
    // Cleanup managed by Qt's parent-child relationships
}

// %%% JSON File Loading %%%
/* Load scenario from JSON file */
void ScenarioEditor::loadFromJsonFile(const QString &filePath)
{

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error",
                             QString("Failed to open JSON file: %1").arg(filePath));
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    // Parse JSON
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, "Error",
                             QString("Failed to parse JSON: %1").arg(err.errorString()));
        return;
    }

    QJsonObject obj = doc.object();

    // Load hierarchy data
    if (obj.contains("hierarchy")) {
        QCoreApplication::processEvents();
        QJsonObject hier = obj["hierarchy"].toObject();
        hierarchy->fromJson(hier);
        QCoreApplication::processEvents();
        if (treeView && treeView->getTreeWidget()) {
            treeView->getTreeWidget()->update();
        }
        QCoreApplication::processEvents();
    }

    // Load tactical display data
    if (tacticalDisplay && obj.contains("tactical")) {
        QCoreApplication::processEvents();
        QJsonObject tac = obj["tactical"].toObject();
        tacticalDisplay->canvas->fromJson(tac);
    }

    // Update state
    lastSavedFilePath = filePath;
    clearUnsavedChanges();

}

// %%% Unsaved Changes Management %%%
/* Mark unsaved changes */
void ScenarioEditor::markUnsavedChanges()
{
    if (!hasUnsavedChanges) {
        hasUnsavedChanges = true;
        emit unsavedChangesChanged(true);
        setWindowTitle("Scenario Editor *");
    }
}

/* Clear unsaved changes */
void ScenarioEditor::clearUnsavedChanges()
{
    if (hasUnsavedChanges) {
        hasUnsavedChanges = false;
        emit unsavedChangesChanged(false);
        setWindowTitle("Scenario Editor");
    }
}



// %%% Recent Project Loading %%%
/* Load recent project */
void ScenarioEditor::loadRecentProject(const QString& filePath)
{
    // Create loading dialog
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

    // Open file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Failed to open scenario file");
        loadingDialog->deleteLater();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    // Parse JSON
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, "Error", "Invalid scenario file format");
        loadingDialog->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("hierarchy")) {
        QMessageBox::warning(this, "Error", "Not a valid scenario file");
        loadingDialog->deleteLater();
        return;
    }

    // Load data
    loadingDialog->setLabelText("Loading...");
    QCoreApplication::processEvents();

    QJsonObject hier = obj["hierarchy"].toObject();
    hierarchy->fromJson(hier);

    loadingDialog->setLabelText("Loading...");
    QCoreApplication::processEvents();

    if (tacticalDisplay && obj.contains("tactical")) {
        QJsonObject tac = obj["tactical"].toObject();
        tacticalDisplay->canvas->fromJson(tac);
    }

    // Update state
    lastSavedFilePath = filePath;
    clearUnsavedChanges();

    // Add to recent projects
    RecentProjectsManager::instance()->addToRecentProjects(filePath,
                                                           RecentProjectsManager::ScenarioEditor);

    // Cleanup
    loadingDialog->close();
    loadingDialog->deleteLater();

}

// %%% Recent Project Trigger %%%
/* Handle recent project menu trigger */
void ScenarioEditor::onRecentProjectTriggered()
{
    RecentProjectsManager::instance()->showRecentProjectsMenu(this,
                                                              RecentProjectsManager::ScenarioEditor);
}

// %%% Profile Info Dialog %%%
/* Show profile information dialog */
void ScenarioEditor::showProfileInfo()
{
    ProfileInfoDialog::showProfileInfo(this);
}

// %%% Application Settings Dialog %%%
/* Show application settings dialog */
void ScenarioEditor::showApplicationDialog()
{
    ApplicationDialog dialog(this);
    connect(&dialog, &ApplicationDialog::canvasIconState,
            tacticalDisplay->canvas, &CanvasWidget::setImageScale);
    dialog.exec();
}

void ScenarioEditor::onRunScriptFileRequested(const QString& filePath)
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
    scriptengine->loadAndCompileScript(scriptSource);
}


void ScenarioEditor::showPanelContextMenu(const QPoint &pos)
{
    QMenu contextMenu("Panels", this);

    // === STYLESHEET WITH YOUR CHECK.PNG IMAGE ===
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
            consoleDock->setGeometry(20, winH - 190, winW - 40, 150);
        }
    }
    else if (selected == showSidebar) {
        sidebarDock->setVisible(!sidebarDock->isVisible());
        if (sidebarDock->isVisible()) sidebarDock->raise();
    }
}

