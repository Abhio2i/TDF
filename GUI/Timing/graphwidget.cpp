/* ========================================================================= */
/* File: graphwidget.cpp                                                     */
/* Purpose: Implements timeline graph visualization for entity tracking      */
//               Written by Arti Rajpoot
/* ========================================================================= */

#include "graphwidget.h"                          // For graph widget class
#include "qpainter.h"                            // For painting operations
#include <QtMath>                                // For math functions
#include <QWheelEvent>                           // For mouse wheel events
#include <QHeaderView>                           // For table header
#include <QTime>                                 // For time formatting
#include <core/Simulation/simulation.h>
#include "tests/graphwidgettest/graphwidget_test.h"
#include "GUI/mainwindow.h"
#include <QTimer>

// %%% Helper Functions %%%
/* Convert seconds to HH:MM:SS format */
QString GraphWidget::formatTime(double seconds) {
    // Calculate hours, minutes, seconds
    int totalSeconds = static_cast<int>(seconds);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int secs = totalSeconds % 60;

    // Return formatted string with leading zeros
    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}

// %%% Custom Canvas Class %%%
/* Custom widget for graph drawing with event handling */
class GraphCanvas : public QWidget {
public:
    GraphWidget* parent;

    /* Initialize canvas with parent reference */
    GraphCanvas(GraphWidget* p) : QWidget(p), parent(p) {
        setStyleSheet("background-color: black;");
    }

protected:
    /* Handle paint events by delegating to parent */
    void paintEvent(QPaintEvent *) override {
        parent->drawGraph(this);
    }

    /* Handle wheel events for zoom functionality */
    void wheelEvent(QWheelEvent *event) override {
        parent->handleWheel(event);
    }
};

// %%% Constructor %%%
/* Initialize graph widget with default settings */
GraphWidget::GraphWidget(QWidget *parent): QWidget(parent) {
    // Configure window properties
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    setStyleSheet("background-color: black;");

    // Initialize variables
    time = 0;
    zoom = 100;
    entity = 0;

    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Create table widget for entity status
    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(5);
    tableWidget->setHorizontalHeaderLabels({"Entity Name", "Start Time", "End Time", "Total Duration", "Status"});
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Apply table styling
    tableWidget->setStyleSheet(
        "QTableWidget {"
        "   background-color: #1a1a1a;"
        "   color: white;"
        "   gridline-color: #333333;"
        "   border: 1px solid #444444;"
        "}"
        "QHeaderView::section {"
        "   background-color: #2a2a2a;"
        "   color: white;"
        "   padding: 5px;"
        "   border: 1px solid #444444;"
        "   font-weight: bold;"
        "}"
        "QTableWidget::item {"
        "   padding: 5px;"
        "}"
        );

    // Configure table behavior
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->horizontalHeader()->setStretchLastSection(true);
    tableWidget->setMaximumHeight(150); // Fixed height for table

    // Create graph canvas
    graphCanvas = new GraphCanvas(this);

    // Add widgets to layout
    mainLayout->addWidget(tableWidget);
    mainLayout->addWidget(graphCanvas, 1);

    setLayout(mainLayout);

    // Example test data
    graphDataList.append({0, 1.5, 4.0, Qt::blue});
    graphDataList.append({1, 0.5, 3.2, Qt::green});
    graphDataList.append({2, 5.0, 8.5, Qt::red});

    // Initial update
    update();
    runUnitTestsOnce();

}

// %%% Hierarchy Management %%%
/* Set hierarchy reference for entity data */
void GraphWidget::setHierarchy(Hierarchy* hier){
    h = hier;
}

// %%% Refresh Method %%%
/* Update widget with new time delta */
void GraphWidget::refresh(float delta){
    if(h){
        entity = h->Platforms.size();          // Update entity count
        t += delta;            // Accumulate time
        t = Simulation::simulationTime;
        time = Simulation::simulationTime;                               // Set current time
        updateTable();                          // Update table display
        if(graphCanvas) graphCanvas->update();  // Update canvas display
    }
}

// %%% Table Update %%%
/* Update table with current entity status */
void GraphWidget::updateTable(){
    if(!h) return;                              // Exit if no hierarchy

    // Clear existing table rows
    tableWidget->setRowCount(0);

    int entityNum = 0;                          // Entity counter
    for (auto& [key, platform] : h->Platforms) {
        if (!platform || !platform->dynamicModel) continue;

        // Get entity timing information
        double startTime = platform->dynamicModel->startTime;
        double endTime = platform->dynamicModel->endTime < 0 ? t : platform->dynamicModel->endTime;
        if (endTime < startTime) endTime = startTime;

        // Calculate total duration
        double totalDuration = endTime - startTime;

        // Determine status based on current time
        QString status;
        if (t < startTime) {
            status = "Pending";
        } else if (t >= startTime && t <= endTime) {
            status = "Running";
        } else {
            status = "Completed";
        }

        // Add new row to table
        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);

        // Populate row cells
        tableWidget->setItem(row, 0, new QTableWidgetItem("Entity " + QString::number(entityNum)));
        tableWidget->setItem(row, 1, new QTableWidgetItem(formatTime(startTime)));
        tableWidget->setItem(row, 2, new QTableWidgetItem(formatTime(endTime)));
        tableWidget->setItem(row, 3, new QTableWidgetItem(formatTime(totalDuration)));

        // Status cell with color coding
        QTableWidgetItem* statusItem = new QTableWidgetItem(status);
        if (status == "Running") {
            statusItem->setForeground(Qt::green);
        } else if (status == "Completed") {
            statusItem->setForeground(Qt::cyan);
        } else {
            statusItem->setForeground(Qt::yellow);
        }
        tableWidget->setItem(row, 4, statusItem);

        entityNum++;                            // Increment entity counter
    }
}

// %%% Event Handling %%%
/* Handle mouse wheel events for zoom control */
void GraphWidget::handleWheel(QWheelEvent *event)
{
    // Adjust zoom based on wheel direction
    if (event->angleDelta().y() > 0) {
        zoom += 10;                             // Zoom in
    } else {
        if (zoom > 20) zoom -= 10;              // Zoom out with minimum limit
    }

    // Update canvas if hierarchy exists
    if(!h) return;
    graphCanvas->update();
}

// %%% Graph Drawing %%%
/* Main graph drawing method */
void GraphWidget::drawGraph(QWidget* canvas) {
    if(!h) return;                              // Exit if no hierarchy

    QPainter p(canvas);
    p.setRenderHint(QPainter::Antialiasing);    // Enable smooth rendering

    // Get canvas dimensions
    int w = canvas->width();
    int h = canvas->height();
    int padding = 50;                           // Border padding

    // Set drawing color
    p.setPen(QPen(Qt::white, 2));

    // Calculate drawing boundaries
    int startX = padding;
    int endX = w - padding;
    int bottomY = h - padding;
    int topY = padding;

    // Draw X-axis
    p.drawLine(startX, bottomY, endX, bottomY);
    drawXTick(p, startX, endX, bottomY, w);

    // Draw Y-axis
    p.drawLine(startX, bottomY, startX, topY);
    drawYTick(p, bottomY, topY, startX, h);

    // Set clipping area for graph content
    QRect chartArea(startX, topY, endX - startX, bottomY - topY);
    p.setClipRect(chartArea);

    // Draw entity data
    drawData(p, startX, bottomY, w, h);

    p.setClipping(false);                       // Reset clipping
}

/* Draw entity timeline data */
void GraphWidget::drawData(QPainter &p, int startX, int bottomY, int canvasWidth, int canvasHeight) {
    if (!h || h->Platforms.size() <= 0) return; // Check for valid data

    // Calculate available space
    int availableWidth = (canvasWidth - 100);
    int availableHeight = (canvasHeight - 100);

    // Calculate spacing based on zoom
    double xSpacing = (availableWidth / (double)time) * (zoom / 100.0);
    double ySpacing = (availableHeight / (double)entity) * (zoom / 100.0);

    int entityNum = 0;                          // Entity counter
    for (auto& [key, platform] : h->Platforms) {
        if (!platform || !platform->dynamicModel) continue;

        // Get entity timing
        double startTime = platform->dynamicModel->startTime;
        double endTime = platform->dynamicModel->endTime < 0 ? t : platform->dynamicModel->endTime;
        if (endTime < startTime) endTime = startTime;

        // Calculate screen coordinates
        int x1 = startX + (startTime * xSpacing);
        int x2 = startX + (endTime * xSpacing);
        int y = bottomY - (entityNum * ySpacing);

        // Drawing dimensions
        int barWidth = x2 - x1;
        int barHeight = 4;
        int dotRadius = 4;

        // Draw timeline bar
        p.setBrush(Qt::cyan);
        p.setPen(Qt::NoPen);
        p.drawRect(x1, y - (barHeight / 2), barWidth, barHeight);

        // Draw start point dot (yellow)
        p.setBrush(Qt::yellow);
        p.setPen(QPen(Qt::white, 1));
        p.drawEllipse(QPoint(x1, y), dotRadius, dotRadius);

        // Draw end point dot (red)
        p.setBrush(Qt::red);
        p.drawEllipse(QPoint(x2, y), dotRadius, dotRadius);

        entityNum++;                            // Next entity
    }
}

/* Draw X-axis ticks and time labels */
void GraphWidget::drawXTick(QPainter &p, int startX, int endX, int y, int canvasWidth) {
    int availableWidth = endX - startX;
    if (availableWidth <= 0 || time <= 0) return; // Validate dimensions

    // Calculate tick spacing
    double tickSpacing = (availableWidth / (double)time) * (zoom / 100.0);

    // Calculate text dimensions for overlap checking
    QFontMetrics fm(p.font());
    QString sampleTime = formatTime(time);      // Longest possible text
    int textWidth = fm.horizontalAdvance(sampleTime) + 10; // Width with padding

    // Calculate dynamic step based on available space
    int step = 1;
    double requiredSpacing = textWidth / tickSpacing;

    // Adjust step for spacing
    if (tickSpacing < 1) {
        // Very small spacing - increase step
        step = qMax(1, static_cast<int>(qCeil(requiredSpacing)));
    } else if (tickSpacing * step < textWidth) {
        // Overlap detected - increase step
        step = qMax(1, static_cast<int>(qCeil(textWidth / tickSpacing)));
    }

    // Ensure reasonable step size
    if (step > time / 4) {
        step = qMax(1, time / 4);
    }

    // Draw ticks with intelligent positioning
    for (int i = 0; i <= time; i += step) {
        int x = startX + (i * tickSpacing);
        if (x > endX) break;                    // Stop at boundary

        // Draw tick line
        p.drawLine(x, y - 5, x, y + 5);

        // Format time label
        QString timeLabel = formatTime(i);

        // Check for text overlap
        bool canDrawHorizontal = true;
        if (i > 0) {
            int prevX = startX + ((i - step) * tickSpacing);
            int prevTextWidth = fm.horizontalAdvance(formatTime(i - step));
            if (x - prevX < (prevTextWidth + textWidth) / 2) {
                canDrawHorizontal = false;
            }
        }

        // Draw label based on available space
        if (canDrawHorizontal) {
            // Normal horizontal text
            QRect textRect(x - 40, y + 10, 80, 20);
            p.drawText(textRect, Qt::AlignCenter, timeLabel);
        } else {
            // Rotated text for dense labels
            p.save();
            p.translate(x, y + 25);
            p.rotate(45);                        // 45 degree angle
            p.drawText(0, 0, timeLabel);
            p.restore();
        }
    }

    // Always show start and end times clearly
    if (step > 1) {
        // Start time (0)
        QString startLabel = formatTime(0);
        QRect startRect(startX - 20, y + 10, 40, 20);
        p.drawText(startRect, Qt::AlignCenter, startLabel);

        // End time
        QString endLabel = formatTime(time);
        QRect endRect(endX - 20, y + 10, 40, 20);
        p.drawText(endRect, Qt::AlignCenter, endLabel);
    }
}

/* Draw Y-axis ticks and entity labels */
void GraphWidget::drawYTick(QPainter &p, int startY, int endY, int x, int canvasHeight) {
    int availableHeight = startY - endY;
    int numEntities = entity;

    if (numEntities == 0) return;               // No entities to display

    // Calculate spacing
    double spacing = (availableHeight / (double)numEntities) * (zoom / 100.0);

    // Calculate step for label density
    int minVerticalGap = 25;                    // Minimum space between labels
    int step = 1;
    if (spacing < minVerticalGap) {
        step = qCeil(minVerticalGap / spacing);
    }

    // Draw ticks with intelligent spacing
    for (int i = 0; i < numEntities; i += step) {
        int y = startY - (i * spacing);
        if (y < endY) break;                    // Stop at boundary

        // Draw tick line
        p.drawLine(x - 5, y, x + 5, y);

        // Create entity label
        QString label = "Entity" + QString::number(i);
        QFontMetrics fm(p.font());
        int labelWidth = fm.horizontalAdvance(label);

        // Check if label will fit
        if (i > 0 && step == 1 && spacing < 30) {
            // If spacing is small, show alternate labels
            if (i % 2 == 0) {
                QRect textRect(x - 100, y - 10, 100, 20);
                p.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, label);
            }
        } else {
            // Show all labels
            QRect textRect(x - 100, y - 10, 100, 20);
            p.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, label);
        }
    }

    // Always show first and last entity
    if (step > 1 && numEntities > 0) {
        // First entity
        QString firstLabel = "Entity0";
        int y = startY;
        p.drawLine(x - 5, y, x + 5, y);
        QRect firstRect(x - 100, y - 10, 100, 20);
        p.drawText(firstRect, Qt::AlignRight | Qt::AlignVCenter, firstLabel);

        // Last entity
        if (numEntities > 1) {
            QString lastLabel = "Entity" + QString::number(numEntities - 1);
            y = startY - ((numEntities - 1) * spacing);
            if (y >= endY) {
                p.drawLine(x - 5, y, x + 5, y);
                QRect lastRect(x - 100, y - 10, 100, 20);
                p.drawText(lastRect, Qt::AlignRight | Qt::AlignVCenter, lastLabel);
            }
        }
    }
}
void GraphWidget::runUnitTestsOnce()
{
    static bool testsRun = false;
    if (testsRun) return;
    testsRun = true;

    QTimer::singleShot(0, []() {
        Console* console = nullptr;
        MainWindow* mw = MainWindow::instance();
        if (mw && mw->databaseEditor && mw->databaseEditor->console) {
            console = mw->databaseEditor->console;
        }
        if (!console) {
            qDebug() << "GraphWidget: console not available, cannot run tests";
            return;
        }

        // Create a temporary GraphWidget (no parent, won't show)
        GraphWidget* testWidget = new GraphWidget(nullptr);
        runGraphWidgetTests(testWidget, console);
        testWidget->deleteLater();
    });
}
