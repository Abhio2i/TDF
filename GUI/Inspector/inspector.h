/* ========================================================================= */
/* File: inspector.h                                                        */
/* Purpose: Inspector panel for viewing and editing hierarchical data       */
// Written by   : Arti Rajpoot
/* ========================================================================= */

#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <QDockWidget>                            // For dock widget base class
#include <QTableWidget>                           // For table widget display
#include <QJsonObject>                            // For JSON object handling
#include <QLabel>                                 // For label display
#include <QLineEdit>                              // For text input
#include <QWheelEvent>                            // For wheel event handling
#include <QPushButton>                            // For button controls
#include <QMenu>                                  // For context menus
#include <QListWidget>                            // For list displays
#include <QJsonArray>                             // For JSON array handling
#include "core/Hierarchy/hierarchy.h"             // For hierarchy data structure
#include "qboxlayout.h"                           // For layout management

// %%% Forward Declarations %%%
class ColorTemplate;
class ImageTemplate;
class GeocordsTemplate;
class OptionTemplate;
class VectorTemplate;
class CustomParameterDialog;

// %%% WheelableLineEdit Class %%%
/* Custom line edit widget that supports value adjustment via mouse wheel */
class WheelableLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    // %%% Constructor %%%
    /* Initialize wheelable line edit with optional parent */
    explicit WheelableLineEdit(QWidget *parent = nullptr);

protected:
    // %%% Event Handler %%%
    /* Handle mouse wheel events for value adjustment */
    void wheelEvent(QWheelEvent *event) override;
};

// %%% Inspector Class %%%
/* Dock widget for inspecting, editing, and managing hierarchical data */
class Inspector : public QDockWidget
{
    Q_OBJECT

public:
    // %%% Constructor %%%
    /* Initialize inspector panel */
    explicit Inspector(QWidget *parent = nullptr);

    // %%% Event Handling %%%
    /* Filter and handle events */
    bool eventFilter(QObject *watched, QEvent *event) override;

    // %%% Hierarchy Management %%%
    /* Set hierarchy data structure reference */
    void setHierarchy(Hierarchy* h) { hierarchy = h; }

    // %%% Utility Methods %%%
    /* Format numeric values for UI display */
    static QString formatNumberForUI(double value);

    // %%% State Management %%%
    /* Set locked state to prevent edits */
    void setLocked(bool locked);
    /* Check if inspector is locked */
    bool isLocked() const;
    /* Get main ID of inspected item */
    QString getMainID() const { return mainID; }
    QString getName() const { return Name; }
    QString getConnectedID() const { return ConnectedID; }

public slots:
    // %%% Data Initialization %%%
    /* Initialize inspector with specific data */
    void init(QString ID, QString name, QJsonObject obj);

    // %%% Table Management %%%
    /* Add simple key-value row to table */
    int addSimpleRow(int row, const QString &key, const QJsonValue &value);
    /* Setup value cell with appropriate widget */
    void setupValueCell(int row, const QString &fullKey, const QJsonValue &value);

    // %%% Data Update Methods %%%
    /* Update trajectory waypoints display */
    void updateTrajectory(QString entityId, QJsonArray waypoints);
    /* Refresh inspector for developer mode */
    void refreshForDeveloperMode();
    /* Reset inspector to initial state */
    void resetState();
    void storeInitialData(const QJsonObject& data);

signals:
    // %%% Focus Signals %%%
    /* Signal to focus on specific entity */
    void foucsEntity(QString ID);

    // %%% Data Change Signals %%%
    /* Signal value changes in inspected item */
    void valueChanged(QString ID, QString name, QJsonObject delta);
    /* Signal request to add new tab */
    void addTabRequested();
    /* Signal parameter addition/removal */
    void parameterChanged(QString ID, QString name, QString key,
                          QString parameterType, bool add);
    /* Signal trajectory waypoints modification */
    void trajectoryWaypointsChanged(QString entityId, QJsonArray waypoints);
void resetComponentRequested(const QString& entityID, const QString& componentName);
private slots:
    // %%% Clipboard Operations %%%
    /* Copy current component data */
    void copyCurrentComponent();
    /* Paste data to current component */
    void pasteToCurrentComponent();

    // %%% UI Action Handlers %%%
    /* Handle add tab action */
    void handleAddTab();
    /* Handle add parameter action */
    void handleAddParameter();
    /* Handle remove parameter action */
    void handleRemoveParameter();

private:
    // %%% UI Component Members %%%
    QTableWidget *tableWidget;        // Main table for data display
    QLabel *titleLabel;               // Title display label
    QString ConnectedID;              // ID of currently connected item
    QString mainID;                   // Main ID of inspected item
    QString Name;                     // Name of inspected item
    QMap<int, QString> rowToKeyPath;  // Map table rows to data key paths
    QSet<QString> customParameterKeys;// Set of custom parameter keys
    QWidget *titleBarWidget;          // Custom title bar widget
    QPushButton *menuButton;          // Menu access button
    QJsonObject copiedComponentData;  // Copied component data storage
    QString copiedComponentType;      // Type of copied component
    Hierarchy* hierarchy = nullptr;   // Reference to hierarchy structure
    QJsonObject copiedVectorData;     // Copied vector data storage
    bool m_locked;                    // Locked state flag
    QPushButton *currentlyExpandedButton = nullptr; // Currently expanded section button
    int m_itemHeight;
    QJsonObject m_initialComponentData;
    // %%% Multi-Component Handling %%%
    /* Process multi-component container */
    void handleMultiComponentContainer(QString ID, QString name, QJsonObject object);
    /* Add multi-component container row */
    int addMultiComponentContainerRow(int row, const QString &key,
                                      const QJsonObject &containerObj,
                                      const QString &componentName);
    /* Create subcomponent widget */
    QWidget* createSubcomponentWidget(const QString &subKey,
                                      const QJsonObject &subObj,
                                      const QString &parentComponentName);
    /* Create value widget for JSON data */
    QWidget* createValueWidgetForJson(const QString &key, const QJsonValue &value,
                                      const QString &subComponentId,
                                      const QString &parentKey);
    /* Add section to layout */
    void addSectionToLayout(const QString &sectionName, const QJsonObject &sectionObj,
                            QVBoxLayout *parentLayout, const QString &subComponentId);

    // %%% UI Setup Methods %%%
    /* Setup user interface components */
    void setupUI();
    /* Setup custom title bar */
    void setupTitleBar();
    /* Create context menu */
    QMenu *createContextMenu();

    // %%% Cell Setup Methods %%%
    /* Setup boolean value cell */
    void setupBooleanCell(int row, const QString &fullKey, bool value);
    /* Setup array value cell */
    void setupArrayCell(int row, const QString &fullKey, const QJsonArray &array);
    /* Setup string value cell */
    void setupStringCell(int row, const QString &fullKey, const QString &value);
    /* Setup numeric value cell */
    void setupNumberCell(int row, const QString &fullKey, double value);
    /* Setup generic object cell */
    void setupGenericObjectCell(int row, const QString &fullKey, const QJsonObject &obj);
    /* Setup section cell with expandable content */
    void setupSectionCell(int row, const QString &fullKey, const QJsonObject &sectionObj);
    /* Setup unit parameter in section */
    void setupUnitParamInSection(QVBoxLayout *parentLayout,
                                 const QString &fullKey,
                                 const QString &paramKey,
                                 const QJsonObject &paramObj);
    /* Setup regular parameter in section */
    void setupRegularParamInSection(QVBoxLayout *parentLayout,
                                    const QString &fullKey,
                                    const QString &paramKey,
                                    const QJsonValue &paramValue);
    /* Setup unit parameter cell */
    void setupUnitParameterCell(int row, const QString &fullKey, const QJsonObject &paramObj);

    // %%% Parameter Management %%%
    /* Add parameter row to table */
    void addParameterRow(const QString &parameterName, int row);
    /* Create remove button for parameters */
    QPushButton *createRemoveButton(const QString &parameterName);

    // %%% Section Information Structure %%%
    /* Structure to track section state */
    struct SectionInfo {
        int headerRow;          // Row number of section header
        int parameterCount;     // Number of parameters in section
        bool isExpanded;        // Expansion state

        SectionInfo() : headerRow(-1), parameterCount(0), isExpanded(true) {}
        SectionInfo(int row, int count, bool expanded = true)
            : headerRow(row), parameterCount(count), isExpanded(expanded) {}
    };

    QMap<QString, SectionInfo> sectionInfo;  // Map of section keys to info
    QMap<int, QString> sectionRows;          // Map of row numbers to section keys

    // %%% Section Management Methods %%%
    /* Create section header widget */
    QWidget* createSectionHeader(const QString &sectionKey, int headerRow,
                                 const QJsonObject &sectionObj);
    /* Toggle section expansion state */
    void toggleSectionExpansion(const QString &sectionKey, int headerRow,
                                QPushButton *dropdownButton);

    // %%% Utility Methods %%%
    /* Capitalize first letter of string */
    static QString capitalizeFirstLetter(const QString &s)
    {
        if (s.isEmpty()) return s;
        return s[0].toUpper() + s.mid(1);
    }
};

#endif // INSPECTOR_H
