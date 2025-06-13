/* Header guard section */
#ifndef HIERARCHYCONNECTOR_H
#define HIERARCHYCONNECTOR_H

/* Includes section */
#include "GUI/Inspector/inspector.h"
#include "GUI/Tacticaldisplay/tacticaldisplay.h"
#include "hierarchytree.h"

#include <core/Hierarchy/hierarchy.h>
#include <QMainWindow>
#include <QObject>

/* Class declaration section */
class HierarchyConnector : public QObject
{
    /* Qt meta-object section */
    Q_OBJECT

public:
    /* Public methods section */
    static HierarchyConnector* instance();

    void connectSignals(Hierarchy* hierarchy, HierarchyTree* treeView,
                        TacticalDisplay* tactical = nullptr,
                        Inspector* inspector = nullptr);
    void connectLibrarySignals(Hierarchy* library, HierarchyTree* libTree);
    static void initializeDummyData(Hierarchy* hierarchy);
    static void setupFileOperations(QMainWindow* parent, Hierarchy* hierarchy);
    static void initializeLibraryData(Hierarchy* library);

    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    void setLibrary(Hierarchy* lib) { library = lib; }
    void setLibTreeView(HierarchyTree* tree) { libTreeView = tree; }

public slots:
    /* Public slots section */
    void handleLibraryToHierarchyDrop(QVariantMap sourceData, QVariantMap targetData);
    void handleHierarchyToLibraryDrop(QVariantMap sourceData, QVariantMap targetData);

private:
    /* Private members section */
    explicit HierarchyConnector(QObject* parent = nullptr);
    static HierarchyConnector* m_instance;

    QVariantMap copydata;
    Hierarchy* copySource = nullptr;
    Hierarchy* hierarchy = nullptr;
    Hierarchy* library = nullptr;
    HierarchyTree* libTreeView = nullptr;
};

/* End of header guard section */
#endif
