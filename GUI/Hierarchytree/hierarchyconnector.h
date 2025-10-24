
#ifndef HIERARCHYCONNECTOR_H
#define HIERARCHYCONNECTOR_H

#include "GUI/Inspector/inspector.h"
#include "GUI/Tacticaldisplay/tacticaldisplay.h"
#include "hierarchytree.h"
#include <core/Hierarchy/hierarchy.h>
#include <QObject>
#include <QVariantMap>

// Forward declarations
class QMainWindow;

class HierarchyConnector : public QObject
{
    Q_OBJECT

public:
    static HierarchyConnector* instance();

    void connectSignals(Hierarchy* hierarchy, HierarchyTree* treeView,
                        TacticalDisplay* tactical = nullptr,
                        Inspector* inspector = nullptr);
    void connectLibrarySignals(Hierarchy* library, HierarchyTree* libTree);
    static void initializeDummyData(Hierarchy* hierarchy);
    void setupFileOperations(QMainWindow* parent, Hierarchy* hierarchy, TacticalDisplay* tacticalDisplay);
    static void initializeLibraryData(Hierarchy* library);

    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    void setLibrary(Hierarchy* lib) { library = lib; }
    void setLibTreeView(HierarchyTree* tree) { libTreeView = tree; }
    QString getLastSavedFilePath(QMainWindow* parent);
    QJsonObject getFeedbackData(Hierarchy* hierarchy);

public slots:
    void handleLibraryToHierarchyDrop(QVariantMap sourceData, QVariantMap targetData);
    void handleHierarchyToLibraryDrop(QVariantMap sourceData, QVariantMap targetData);

private:
    explicit HierarchyConnector(QObject* parent = nullptr);
    static HierarchyConnector* m_instance;

    QVariantMap copydata;
    Hierarchy* copySource = nullptr;
    Hierarchy* hierarchy = nullptr;
    Hierarchy* library = nullptr;
    HierarchyTree* libTreeView = nullptr;
HierarchyTree* treeView;

private slots:
    void loadToLibrary(QMainWindow* parent);
};

#endif // HIERARCHYCONNECTOR_H
