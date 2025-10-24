/* ========================================================================= */
/* File: inspector.h                                                        */
/* Purpose: Defines inspector widget for editing hierarchy data              */
/* ========================================================================= */

#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <QDockWidget>                            // For dock widget base class
#include <QTableWidget>                           // For table widget
#include <QJsonObject>                            // For JSON object handling
#include <QLabel>                                 // For label widget
#include <QLineEdit>                              // For text input widget
#include <QWheelEvent>                            // For wheel event handling
#include <QPushButton>                            // For push button widget
#include <QMenu>                                  // For menu widget
#include <QListWidget>                            // For list widget
#include <QJsonArray>                             // For JSON array handling
#include "core/Hierarchy/hierarchy.h"             // For hierarchy data structure

// %%% Forward Declarations %%%
class ColorTemplate;
class ImageTemplate;
class GeocordsTemplate;
class OptionTemplate;
class VectorTemplate;
class CustomParameterDialog;

// %%% WheelableLineEdit Class %%%
/* Custom line edit with wheel event handling */
class WheelableLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    // Initialize line edit
    explicit WheelableLineEdit(QWidget *parent = nullptr);

protected:
    // Handle wheel events
    void wheelEvent(QWheelEvent *event) override;
};

// %%% Inspector Class %%%
/* Dock widget for inspecting and editing data */
class Inspector : public QDockWidget
{
    Q_OBJECT

public:
    // Initialize inspector
    explicit Inspector(QWidget *parent = nullptr);
    // Handle event filtering
    bool eventFilter(QObject *watched, QEvent *event) override;
    // Set hierarchy instance
    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    // Format number for UI display
    static QString formatNumberForUI(double value);
    // Set locked state
    void setLocked(bool locked);
    // Get locked state
    bool isLocked() const;

public slots:
    // Initialize with data
    void init(QString ID, QString name, QJsonObject obj);
    // Add simple row to table
    int addSimpleRow(int row, const QString &key, const QJsonValue &value);
    // Setup value cell in table
    void setupValueCell(int row, const QString &fullKey, const QJsonValue &value);
    // Update trajectory waypoints
    void updateTrajectory(QString entityId, QJsonArray waypoints);

signals:
    // Signal focus entity
    void foucsEntity(QString ID); // Note: Typo in code
    // Signal value change
    void valueChanged(QString ID, QString name, QJsonObject delta);
    // Signal add tab request
    void addTabRequested();
    // Signal parameter change
    void parameterChanged(QString ID, QString name, QString key, QString parameterType, bool add);
    // Signal trajectory waypoints change
    void trajectoryWaypointsChanged(QString entityId, QJsonArray waypoints);

private slots:
    // Copy current component
    void copyCurrentComponent();
    // Paste to current component
    void pasteToCurrentComponent();
    // Handle add tab action
    void handleAddTab();
    // Handle add parameter action
    void handleAddParameter();
    // Handle remove parameter action
    void handleRemoveParameter();

private:
    // %%% UI Components %%%
    // Table widget for data
    QTableWidget *tableWidget;
    // Title label
    QLabel *titleLabel;
    // Connected item ID
    QString ConnectedID;
    // Item name
    QString Name;
    // Map of row to key path
    QMap<int, QString> rowToKeyPath;
    // Set of custom parameter keys
    QSet<QString> customParameterKeys;
    // Title bar widget
    QWidget *titleBarWidget;
    // Menu button
    QPushButton *menuButton;
    // Copied component data
    QJsonObject copiedComponentData;
    // Copied component type
    QString copiedComponentType;
    // Hierarchy instance
    Hierarchy* hierarchy = nullptr;
    // Copied vector data
    QJsonObject copiedVectorData;
    // Locked state flag
    bool m_locked;

    // %%% UI Setup Methods %%%
    // Configure UI components
    void setupUI();
    // Setup title bar
    void setupTitleBar();
    // Create context menu
    QMenu *createContextMenu();
    // Setup boolean cell
    void setupBooleanCell(int row, const QString &fullKey, bool value);
    // Setup array cell
    void setupArrayCell(int row, const QString &fullKey, const QJsonArray &array);
    // Setup string cell
    void setupStringCell(int row, const QString &fullKey, const QString &value);
    // Setup number cell
    void setupNumberCell(int row, const QString &fullKey, double value);
    // Setup generic object cell
    void setupGenericObjectCell(int row, const QString &fullKey, const QJsonObject &obj);
    // Add parameter row
    void addParameterRow(const QString &parameterName, int row);
    // Create remove button
    QPushButton *createRemoveButton(const QString &parameterName);
};

#endif // INSPECTOR_H
