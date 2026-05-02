/* =============================================================================
 * FILE:         hierarchyconnector.h
 * MODULE:       Hierarchy UI Connector
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the HierarchyConnector class, a singleton that
 *               provides connectivity between the core Hierarchy data model
 *               and UI components (HierarchyTree, TacticalDisplay, Inspector,
 *               QMainWindow). It manages signal/slot connections, file
 *               operations (new/open/save), copy/paste, library data
 *               initialisation, dummy data generation, recent projects,
 *               and feedback data collection.
 *
 * REQUIREMENTS: REQ-CONN-010  Singleton connector for hierarchy-UI integration
 *               REQ-CONN-011  Connect hierarchy signals to tree view and displays
 *               REQ-CONN-012  Connect library hierarchy signals to library tree
 *               REQ-CONN-013  File operations (new, open, save) via main window
 *               REQ-CONN-014  Initialise dummy data for testing
 *               REQ-CONN-015  Initialise library data from profiles
 *               REQ-CONN-016  Recent projects management (add, get, clear)
 *               REQ-CONN-017  Generate feedback data as JSON
 *               REQ-CONN-018  Load XML files into hierarchy
 *               REQ-CONN-019  Copy/paste support between hierarchies
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-CONN-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef HIERARCHYCONNECTOR_H
#define HIERARCHYCONNECTOR_H

#include "GUI/Inspector/inspector.h"               // For inspector panel
#include "GUI/Tacticaldisplay/tacticaldisplay.h"   // For tactical display
#include "hierarchytree.h"                        // For hierarchy tree view
#include <core/Hierarchy/hierarchy.h>             // For hierarchy data structure
#include <QObject>                                // For QObject base class
#include <QVariantMap>                            // For key-value data mapping
#include <GUI/Editors/recentprojectsmanager.h>

// %%% Forward Declarations %%%
class QMainWindow;

// %%% Class Definition %%%
/* Connector for hierarchy and UI interactions */
class HierarchyConnector : public QObject
{
    Q_OBJECT
public:
    // Get singleton instance
    static HierarchyConnector* instance();
    // Connect signals for hierarchy and UI
    void connectSignals(Hierarchy* hierarchy, Hierarchy* library, HierarchyTree* treeView,
                        TacticalDisplay* tactical = nullptr,
                        Inspector* inspector = nullptr);
    // Connect library signals
    void connectLibrarySignals(Hierarchy* library, HierarchyTree* libTree);
    // Initialize dummy data
    static void initializeDummyData(Hierarchy* hierarchy);
    // Setup file operations
    void setupFileOperations(QMainWindow* parent, Hierarchy* hierarchy, TacticalDisplay* tacticalDisplay);
    // Initialize library data
    static void initializeLibraryData(Hierarchy* library);
    // Set hierarchy instance
    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    // Set library instance
    void setLibrary(Hierarchy* lib) { library = lib; }
    // Set library tree view
    void setLibTreeView(HierarchyTree* tree) { libTreeView = tree; }
    // Get last saved file path
    QString getLastSavedFilePath(QMainWindow* parent);
    // Get feedback data as JSON
    QJsonObject getFeedbackData(Hierarchy* hierarchy);
    void addToRecentProjects(const QString& filePath);
    QStringList getRecentProjects() const;
    void clearRecentProjects();
    void newfile(Hierarchy* hierarchy);
    void openXmlFile(Hierarchy* hierarchy,QString filePath);
    QDomElement getDom(QString filePath);
    Hierarchy* library = nullptr;


public slots:
    void loadToLibrary(QMainWindow* parent);
private:

    // Initialize connector
    explicit HierarchyConnector(QObject* parent = nullptr);
    // Singleton instance
    static HierarchyConnector* m_instance;
    // Store copied data
    QVariantMap copydata;
    // Source hierarchy for copy
    Hierarchy* copySource = nullptr;
    // Main hierarchy instance
    Hierarchy* hierarchy = nullptr;
    // Library tree view widget
    HierarchyTree* libTreeView = nullptr;
    // Hierarchy tree view widget
    HierarchyTree* treeView;
};

#endif // HIERARCHYCONNECTOR_H
