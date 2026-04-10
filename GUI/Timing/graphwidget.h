/* ========================================================================= */
/* File: graphwidget.h                                                       */
/* Purpose: Defines timeline graph widget for entity visualization           */
//               Written by Arti Rajpoot
/* ========================================================================= */

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
    Q_OBJECT                                   // Qt meta-object macros

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
    static void runUnitTestsOnce();

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
    void drawYTick(QPainter &p, int startY, int endY, int x, int canvasHeight);

    /* Draw entity timeline data */
    void drawData(QPainter &p, int startX, int bottomY, int canvasWidth, int canvasHeight);
};

#endif // GRAPHWIDGET_H
