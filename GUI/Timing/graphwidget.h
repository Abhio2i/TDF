/* =============================================================================
 * FILE:         graphwidget.h
 * MODULE:       Timeline Graph Widget
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the GraphWidget class which provides a timeline graph
 *               widget for visualising entity activity over time. It displays
 *               horizontal bars representing each entity's active period,
 *               supports zooming via mouse wheel, and updates a status table
 *               with real‑time entity state information. Integrates with
 *               Hierarchy to access entity data.
 *
 * REQUIREMENTS: REQ-GRAPH-010  Timeline graph for entity activity
 *               REQ-GRAPH-011  Display horizontal bars per entity from start
 *                              to end time
 *               REQ-GRAPH-012  X‑axis with time ticks (HH:MM:SS format)
 *               REQ-GRAPH-013  Y‑axis with entity labels
 *               REQ-GRAPH-014  Zoom in/out with mouse wheel
 *               REQ-GRAPH-015  Real‑time refresh with current simulation time
 *               REQ-GRAPH-016  Status table showing current entity state
 *               REQ-GRAPH-017  Integration with Hierarchy for entity data
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-GRAPH-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include "core/Hierarchy/hierarchy.h"          // For hierarchy data structure
#include <QWidget>                             // Base widget class
#include <QTableWidget>                        // For entity status table
#include <QVBoxLayout>                         // For vertical layout

// %%% Data Structure %%%
/* Structure to store entity timeline data */
struct GraphData {
    int entityNum;                             // Entity identifier
    double startTime;                          // Timeline start time
    double endTime;                            // Timeline end time
    QColor color;                              // Display color
};

// %%% Forward Declaration %%%
class GraphCanvas;                             // Custom canvas widget

// %%% Main Widget Class %%%
/* Timeline graph widget for visualizing entity activity */
class GraphWidget : public QWidget
{
    Q_OBJECT
public:
    // %%% Constructor %%%
    /* Initialize graph widget with parent */
    explicit GraphWidget(QWidget *parent = nullptr);

    // %%% Public Variables %%%
    int entity = 10;                           // Number of entities (default)
    float t = 0;                               // Current simulation time
    int time = 10;                             // Total timeline duration
    int zoom = 100;                            // Zoom level percentage
    QVector<GraphData> graphDataList;          // List of graph data entries

    // %%% Hierarchy Management %%%
    /* Set hierarchy for entity data access */
    void setHierarchy(Hierarchy* hier);

    // %%% Drawing Methods %%%
    /* Draw graph on specified canvas */
    void drawGraph(QWidget* canvas);

    /* Handle mouse wheel events for zoom */
    void handleWheel(QWheelEvent *event);

    // %%% Utility Functions %%%
    /* Format seconds to HH:MM:SS string */
    static QString formatTime(double seconds);

public slots:
    // %%% Update Slot %%%
    /* Refresh display with time delta */
    void refresh(float delta);

private:
    // %%% Private Variables %%%
    Hierarchy* h = nullptr;
    QTableWidget* tableWidget;
    QWidget* graphCanvas;

    // %%% Private Methods %%%
    /* Update table with current entity status */
    void updateTable();

    /* Draw X-axis ticks and time labels */
    void drawXTick(QPainter &p, int startX, int endX, int y, int canvasWidth);

    /* Draw Y-axis ticks and entity labels */
    void drawYTick(QPainter &p, int startY, int endY, int x, int canvasHeight,
                   QVector<QString> entityNames);

    /* Draw entity timeline data */
    void drawData(QPainter &p, int startX, int bottomY, int canvasWidth, int canvasHeight);
    QVector<Platform*> getSortedPlatforms();


};

#endif // GRAPHWIDGET_H
