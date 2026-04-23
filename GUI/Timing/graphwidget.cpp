
/* =============================================================================
 * FILE:         graphwidget.cpp
 * MODULE:       Timeline Graph Widget
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the GraphWidget class which provides a timeline graph
 *               widget for visualising entity activity over time. It displays
 *               horizontal bars representing each entity's active period,
 *               supports zooming via mouse wheel, and updates a status table
 *               with real‑time entity state information. Integrates with
 *               Hierarchy to access entity data.
 *
 * REQUIREMENTS: Implements REQ-GRAPH-010 through REQ-GRAPH-017
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-GRAPH-001
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#include "graphwidget.h"
#include "qpainter.h"
#include <QtMath>
#include <QWheelEvent>
#include <QHeaderView>
#include <QTime>
#include <algorithm>
#include <cctype>
#include <core/Simulation/simulation.h>


// %%% Helper Functions %%%
/* Convert seconds to HH:MM:SS format */
QString GraphWidget::formatTime(double seconds) {
    int totalSeconds = static_cast<int>(seconds);
    int hours   = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int secs    = totalSeconds % 60;
    return QString("%1:%2:%3")
        .arg(hours,   2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(secs,    2, 10, QChar('0'));
}

// %%% Natural Sort Helpers %%%

static int extractTrailingNumber(const std::string& s) {
    int i = static_cast<int>(s.size()) - 1;
    while (i >= 0 && std::isdigit(static_cast<unsigned char>(s[i]))) i--;
    std::string numStr = s.substr(i + 1);
    if (numStr.empty()) return -1;
    return std::stoi(numStr);
}

static bool naturalLess(Platform* a, Platform* b) {
    const std::string& na = a->Name;
    const std::string& nb = b->Name;

    // Find where strings diverge
    size_t i = 0;
    while (i < na.size() && i < nb.size() && na[i] == nb[i]) i++;

    // If both diverge at a digit — compare numerically
    if (i < na.size() && i < nb.size() &&
        std::isdigit(static_cast<unsigned char>(na[i])) &&
        std::isdigit(static_cast<unsigned char>(nb[i]))) {
        int numA = extractTrailingNumber(na);
        int numB = extractTrailingNumber(nb);
        if (numA != numB) return numA < numB;
    }

    return na < nb;
}

// %%% Sorted Platforms Helper %%%

QVector<Platform*> GraphWidget::getSortedPlatforms() {
    QVector<Platform*> sorted;
    for (auto& [key, platform] : h->Platforms) {
        if (platform) sorted.append(platform);
    }

    std::sort(sorted.begin(), sorted.end(), naturalLess);
    return sorted;
}

// %%% Custom Canvas Class %%%

class GraphCanvas : public QWidget {
public:
    GraphWidget* parent;
    GraphCanvas(GraphWidget* p) : QWidget(p), parent(p) {
        setStyleSheet("background-color: black;");
    }

protected:

    void paintEvent(QPaintEvent *) override {
        parent->drawGraph(this);
    }

    void wheelEvent(QWheelEvent *event) override {
        parent->handleWheel(event);
    }
};

// %%% Constructor %%%
/* Initialize graph widget with default settings */
GraphWidget::GraphWidget(QWidget *parent): QWidget(parent) {
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    setStyleSheet("background-color: black;");

    time   = 0;
    zoom   = 100;
    entity = 0;

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(5);
    tableWidget->setHorizontalHeaderLabels({"Entity Name", "Start Time", "End Time",
                                            "Total Duration", "Status"});
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

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

    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->horizontalHeader()->setStretchLastSection(true);
    tableWidget->setMaximumHeight(150);

    graphCanvas = new GraphCanvas(this);

    mainLayout->addWidget(tableWidget);
    mainLayout->addWidget(graphCanvas, 1);
    setLayout(mainLayout);

    graphDataList.append({0, 1.5, 4.0, Qt::blue});
    graphDataList.append({1, 0.5, 3.2, Qt::green});
    graphDataList.append({2, 5.0, 8.5, Qt::red});

    update();
}

// %%% Hierarchy Management %%%

void GraphWidget::setHierarchy(Hierarchy* hier){
    h = hier;
}

// %%% Refresh Method %%%

void GraphWidget::refresh(float delta){
    if(h){
        entity = h->Platforms.size();
        t     += delta;
        t      = Simulation::simulationTime;
        time   = Simulation::simulationTime;
        updateTable();
        if(graphCanvas) graphCanvas->update();
    }
}

// %%% Table Update %%%

void GraphWidget::updateTable(){
    if(!h) return;

    tableWidget->setRowCount(0);


    QVector<Platform*> sorted = getSortedPlatforms();

    for (Platform* platform : sorted) {
        if (!platform || !platform->dynamicModel) continue;

        double startTime     = platform->dynamicModel->startTime;
        double endTime       = platform->dynamicModel->endTime < 0
                             ? t : platform->dynamicModel->endTime;
        if (endTime < startTime) endTime = startTime;

        double totalDuration = endTime - startTime;

        QString status;
        if (t < startTime) {
            status = "Pending";
        } else if (t >= startTime && t <= endTime) {
            status = "Running";
        } else {
            status = "Completed";
        }

        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);

        // ✅ Actual entity name
        tableWidget->setItem(row, 0, new QTableWidgetItem(
                                         QString::fromStdString(platform->Name)));
        tableWidget->setItem(row, 1, new QTableWidgetItem(formatTime(startTime)));
        tableWidget->setItem(row, 2, new QTableWidgetItem(formatTime(endTime)));
        tableWidget->setItem(row, 3, new QTableWidgetItem(formatTime(totalDuration)));

        QTableWidgetItem* statusItem = new QTableWidgetItem(status);
        if (status == "Running") {
            statusItem->setForeground(Qt::green);
        } else if (status == "Completed") {
            statusItem->setForeground(Qt::cyan);
        } else {
            statusItem->setForeground(Qt::yellow);
        }
        tableWidget->setItem(row, 4, statusItem);
    }
}

// %%% Event Handling %%%
/* Handle mouse wheel events for zoom control */
void GraphWidget::handleWheel(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0) {
        zoom += 10;
    } else {
        if (zoom > 20) zoom -= 10;
    }
    if(!h) return;
    graphCanvas->update();
}

// %%% Graph Drawing %%%
/* Main graph drawing method */
void GraphWidget::drawGraph(QWidget* canvas) {
    if(!h) return;

    QPainter p(canvas);
    p.setRenderHint(QPainter::Antialiasing);

    int w = canvas->width();
    int h = canvas->height();


    QFontMetrics fm(p.font());
    int maxNameWidth = 50;
    for (auto& [key, platform] : this->h->Platforms) {
        if (platform) {
            int nameWidth = fm.horizontalAdvance(
                                QString::fromStdString(platform->Name)) + 15;
            if (nameWidth > maxNameWidth) maxNameWidth = nameWidth;
        }
    }

    int leftPadding  = maxNameWidth;
    int rightPadding = 50;
    int topPadding   = 50;
    int botPadding   = 50;

    p.setPen(QPen(Qt::white, 2));

    int startX  = leftPadding;
    int endX    = w - rightPadding;
    int bottomY = h - botPadding;
    int topY    = topPadding;

    // Draw X-axis
    p.drawLine(startX, bottomY, endX, bottomY);
    drawXTick(p, startX, endX, bottomY, w);

    QVector<QString> entityNames;
    QVector<Platform*> sorted = getSortedPlatforms();
    for (Platform* platform : sorted) {
        entityNames.append(QString::fromStdString(platform->Name));
    }

    // Draw Y-axis
    p.drawLine(startX, bottomY, startX, topY);
    drawYTick(p, bottomY, topY, startX, h, entityNames);

    // Set clipping area
    QRect chartArea(startX, topY, endX - startX, bottomY - topY);
    p.setClipRect(chartArea);

    drawData(p, startX, bottomY, w, h);

    p.setClipping(false);
}

/* Draw entity timeline data — naturally sorted order */
void GraphWidget::drawData(QPainter &p, int startX, int bottomY,
                           int canvasWidth, int canvasHeight) {
    if (!h || h->Platforms.size() <= 0) return;

    int availableWidth  = (canvasWidth  - 100);
    int availableHeight = (canvasHeight - 100);

    double xSpacing = (availableWidth  / (double)time)   * (zoom / 100.0);
    double ySpacing = (availableHeight / (double)entity) * (zoom / 100.0);

    // ✅ Same naturally sorted order as Y-axis labels
    QVector<Platform*> sorted = getSortedPlatforms();

    int entityNum = 0;
    for (Platform* platform : sorted) {
        if (!platform || !platform->dynamicModel) continue;

        double startTime = platform->dynamicModel->startTime;
        double endTime   = platform->dynamicModel->endTime < 0
                             ? t : platform->dynamicModel->endTime;
        if (endTime < startTime) endTime = startTime;

        int x1 = startX + (startTime * xSpacing);
        int x2 = startX + (endTime   * xSpacing);
        int y  = bottomY - (entityNum * ySpacing);

        int barHeight = 4;
        int dotRadius = 4;
        int barWidth  = x2 - x1;

        // Draw timeline bar (cyan)
        p.setBrush(Qt::cyan);
        p.setPen(Qt::NoPen);
        p.drawRect(x1, y - (barHeight / 2), barWidth, barHeight);

        // Draw start dot (yellow)
        p.setBrush(Qt::yellow);
        p.setPen(QPen(Qt::white, 1));
        p.drawEllipse(QPoint(x1, y), dotRadius, dotRadius);

        // Draw end dot (red)
        p.setBrush(Qt::red);
        p.drawEllipse(QPoint(x2, y), dotRadius, dotRadius);

        entityNum++;
    }
}

/* Draw X-axis ticks and time labels */
void GraphWidget::drawXTick(QPainter &p, int startX, int endX, int y, int canvasWidth) {
    int availableWidth = endX - startX;
    if (availableWidth <= 0 || time <= 0) return;

    double tickSpacing = (availableWidth / (double)time) * (zoom / 100.0);

    QFontMetrics fm(p.font());
    QString sampleTime = formatTime(time);
    int textWidth = fm.horizontalAdvance(sampleTime) + 10;

    int step = 1;
    double requiredSpacing = textWidth / tickSpacing;

    if (tickSpacing < 1) {
        step = qMax(1, static_cast<int>(qCeil(requiredSpacing)));
    } else if (tickSpacing * step < textWidth) {
        step = qMax(1, static_cast<int>(qCeil(textWidth / tickSpacing)));
    }

    if (step > time / 4) {
        step = qMax(1, time / 4);
    }

    for (int i = 0; i <= time; i += step) {
        int x = startX + (i * tickSpacing);
        if (x > endX) break;

        p.drawLine(x, y - 5, x, y + 5);

        QString timeLabel = formatTime(i);

        bool canDrawHorizontal = true;
        if (i > 0) {
            int prevX         = startX + ((i - step) * tickSpacing);
            int prevTextWidth = fm.horizontalAdvance(formatTime(i - step));
            if (x - prevX < (prevTextWidth + textWidth) / 2) {
                canDrawHorizontal = false;
            }
        }

        if (canDrawHorizontal) {
            QRect textRect(x - 40, y + 10, 80, 20);
            p.drawText(textRect, Qt::AlignCenter, timeLabel);
        } else {
            p.save();
            p.translate(x, y + 25);
            p.rotate(45);
            p.drawText(0, 0, timeLabel);
            p.restore();
        }
    }

    if (step > 1) {
        QRect startRect(startX - 20, y + 10, 40, 20);
        p.drawText(startRect, Qt::AlignCenter, formatTime(0));

        QRect endRect(endX - 20, y + 10, 40, 20);
        p.drawText(endRect, Qt::AlignCenter, formatTime(time));
    }
}

/* Draw Y-axis ticks and entity labels */
void GraphWidget::drawYTick(QPainter &p, int startY, int endY, int x,
                            int canvasHeight, QVector<QString> entityNames) {
    int availableHeight = startY - endY;
    int numEntities     = entity;

    if (numEntities == 0) return;

    double spacing = (availableHeight / (double)numEntities) * (zoom / 100.0);

    int minVerticalGap = 25;
    int step = 1;
    if (spacing < minVerticalGap) {
        step = qCeil(minVerticalGap / spacing);
    }

    for (int i = 0; i < numEntities; i += step) {
        int y = startY - (i * spacing);
        if (y < endY) break;

        p.drawLine(x - 5, y, x + 5, y);


        QString label = (i < entityNames.size())
                            ? entityNames[i]
                            : "Entity" + QString::number(i);

        if (i > 0 && step == 1 && spacing < 30) {
            if (i % 2 == 0) {
                QRect textRect(0, y - 10, x - 8, 20);
                p.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, label);
            }
        } else {
            QRect textRect(0, y - 10, x - 8, 20);
            p.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, label);
        }
    }

    if (step > 1 && numEntities > 0) {
        // First entity
        QString firstLabel = entityNames.isEmpty()
                                 ? "Entity0" : entityNames[0];
        int y = startY;
        p.drawLine(x - 5, y, x + 5, y);
        QRect firstRect(0, y - 10, x - 8, 20);
        p.drawText(firstRect, Qt::AlignRight | Qt::AlignVCenter, firstLabel);

        // Last entity
        if (numEntities > 1) {
            QString lastLabel = ((numEntities - 1) < entityNames.size())
                                    ? entityNames[numEntities - 1]
                                    : "Entity" + QString::number(numEntities - 1);
            y = startY - ((numEntities - 1) * spacing);
            if (y >= endY) {
                p.drawLine(x - 5, y, x + 5, y);
                QRect lastRect(0, y - 10, x - 8, 20);
                p.drawText(lastRect, Qt::AlignRight | Qt::AlignVCenter, lastLabel);
            }
        }
    }
}
