/* ========================================================================= */
/* File: hierarchyconnector.h                                               */
/* Purpose: Defines connector for hierarchy and UI components               */
/* ========================================================================= */

#ifndef HIERARCHYCONNECTOR_H
#define HIERARCHYCONNECTOR_H

#include "GUI/Inspector/inspector.h"               // For inspector panel
#include "GUI/Tacticaldisplay/tacticaldisplay.h"   // For tactical display
#include "hierarchytree.h"                        // For hierarchy tree view
#include <core/Hierarchy/hierarchy.h>             // For hierarchy data structure
#include <QObject>                                // For QObject base class
#include <QVariantMap>                            // For key-value data mapping

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
    void connectSignals(Hierarchy* hierarchy, HierarchyTree* treeView,
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

public slots:
    // Handle library to hierarchy drop
    void handleLibraryToHierarchyDrop(QVariantMap sourceData, QVariantMap targetData);
    // Handle hierarchy to library drop
    void handleHierarchyToLibraryDrop(QVariantMap sourceData, QVariantMap targetData);

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
    // Library hierarchy instance
    Hierarchy* library = nullptr;
    // Library tree view widget
    HierarchyTree* libTreeView = nullptr;
    // Hierarchy tree view widget
    HierarchyTree* treeView;

private slots:
    // Load data to library
    void loadToLibrary(QMainWindow* parent);
};

#endif // HIERARCHYCONNECTOR_H
