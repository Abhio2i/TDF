/* ========================================================================= */
/* File: feedback.h                                                         */
/* Purpose: Defines the feedback window and custom graph widget              */
/* ========================================================================= */

#ifndef FEEDBACK_H
#define FEEDBACK_H

#include "core/Hierarchy/hierarchy.h"              // For hierarchy data structure
#include <QMainWindow>                             // For main window base class
#include <QLabel>                                  // For label widgets
#include <QProgressBar>                            // For progress bar widget
#include <QTableWidget>                            // For table widget
#include <QTextEdit>                               // For text edit widget
#include <QVBoxLayout>                             // For vertical layout
#include <QHBoxLayout>                             // For horizontal layout
#include <QJsonDocument>                           // For JSON document handling
#include <QJsonObject>                             // For JSON object handling
#include <QJsonArray>                              // For JSON array handling
#include <QWidget>                                 // For base widget class
#include <QPainter>                                // For painting operations
#include <QPainterPath>                            // For painter path
#include <QComboBox>                               // For combo box widget
#include <QListWidget>                             // For list widget
#include <QStackedWidget>                          // For stacked widget
#include <QPushButton>                             // For push button widget
#include <QScrollArea>                             // For scroll area widget
#include <QTimer>                                  // For timer functionality
#include <QFile>                                   // For file operations

// %%% CustomGraphWidget Class %%%
/* Custom widget for drawing graphs */
class CustomGraphWidget : public QWidget
{
    Q_OBJECT
public:
    // Enum for graph types
    enum GraphType { Bar, Line, Pie };
    // Initialize graph widget
    CustomGraphWidget(GraphType type, QWidget *parent = nullptr) : QWidget(parent), m_graphType(type) {}

    // Set data for graph
    void setData(const QList<qreal> &data, const QStringList &labels = QStringList()) {
        m_data = data;
        m_labels = labels;
        update();
    }

    // Set data for line graph
    void setLineData(const QList<QList<qreal>> &linesData) {
        m_linesData = linesData;
        update();
    }

protected:
    // Handle paint events
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor("#2a3555"));

        int w = width();
        int h = height();
        int padding = 10;

        if (m_graphType == Bar && !m_data.isEmpty()) {
            int barWidth = (w - 2 * padding) / m_data.size();
            qreal maxValue = *std::max_element(m_data.begin(), m_data.end());
            for (int i = 0; i < m_data.size(); ++i) {
                qreal barHeight = (m_data[i] / maxValue) * (h - 2 * padding);
                painter.fillRect(padding + i * barWidth, h - barHeight - padding, barWidth - 5, barHeight, QColor("#4a90e2"));
            }
        }
        else if (m_graphType == Line && !m_linesData.isEmpty()) {
            for (const auto &line : m_linesData) {
                if (line.isEmpty()) continue;
                QPainterPath path;
                qreal maxValue = 0;
                for (const auto &val : line) {
                    maxValue = qMax(maxValue, val);
                }
                path.moveTo(padding, h - padding);
                for (int i = 0; i < line.size(); ++i) {
                    qreal x = padding + (i * (w - 2 * padding) / (line.size() - 1));
                    qreal y = h - padding - (line[i] / maxValue) * (h - 2 * padding);
                    if (i == 0) path.moveTo(x, y);
                    else path.lineTo(x, y);
                }
                painter.setPen(QPen(m_linesData.indexOf(line) == 0 ? QColor("#4a90e2") : QColor("#7f8c2d")));
                painter.drawPath(path);
            }
        }
        else if (m_graphType == Pie && !m_data.isEmpty()) {
            qreal total = 0;
            for (qreal val : m_data) total += val;
            qreal startAngle = 0;
            QList<QColor> colors = {QColor("#4a90e2"), QColor("#2a3555"), QColor("#1e2a44")};
            for (int i = 0; i < m_data.size(); ++i) {
                qreal angle = (m_data[i] / total) * 360;
                painter.setBrush(colors[i % colors.size()]);
                painter.drawPie(padding, padding, w - 2 * padding, h - 2 * padding, startAngle * 16, angle * 16);
                startAngle += angle;
            }
        }
    }

private:
    // Graph type
    GraphType m_graphType;
    // Data for graph
    QList<qreal> m_data;
    // Labels for graph
    QStringList m_labels;
    // Data for line graph
    QList<QList<qreal>> m_linesData;
};

// %%% Feedback Class %%%
/* Main window for feedback dashboard */
class Feedback : public QMainWindow
{
    Q_OBJECT

public:
    // Initialize feedback window
    Feedback(QWidget *parent = nullptr);
    // Clean up resources
    ~Feedback();
    // Hierarchy data structure
    Hierarchy *h;
    // Load dashboard data from JSON
    void loadDashboardData(const QString& jsonData);

private slots:
    // Handle sidebar item click
    void onSidebarItemClicked(QListWidgetItem *item);
    // Handle details button click
    void onDetailsButtonClicked();
    // Handle entity combo change
    void onEntityComboChanged(const QString &text);
    // Handle IFF combo change
    void onIffComboChanged(const QString &text);
    // Handle formation combo change
    void onFormationComboChanged(const QString &text);
    // Handle fixed point combo change
    void onFixedPointComboChanged(const QString &text);
    // Handle weapon combo change
    void onWeaponComboChanged(const QString &text);

private:
    // %%% UI Setup Methods %%%
    // Configure UI components
    void setupUi();
    // Setup overview widget
    void setupOverviewWidget();
    // Setup storage widget
    void setupStorageWidget();
    // Setup sensors widget
    void setupSensorsWidget();
    // Setup radio widget
    void setupRadioWidget();
    // Setup network widget
    void setupNetworkWidget();
    // Setup logs widget
    void setupLogsWidget();
    // Setup canvas widget
    void setupCanvasWidget();
    // Setup entity widget
    void setupEntityWidget();
    // Setup IFF widget
    void setupIffWidget();
    // Setup formation widget
    void setupFormationWidget();
    // Setup fixed point widget
    void setupFixedPointWidget();
    // Setup weapon widget
    void setupWeaponWidget();

    // %%% Sidebar %%%
    // Sidebar list widget
    QListWidget *sidebar;

    // %%% Content Stack %%%
    // Stacked widget for content
    QStackedWidget *contentStack;

    // %%% Section Widgets %%%
    // Overview widget
    QWidget *overviewWidget;
    // Storage widget
    QWidget *storageWidget;
    // Sensors widget
    QWidget *sensorsWidget;
    // Radio widget
    QWidget *radioWidget;
    // Network widget
    QWidget *networkWidget;
    // Logs widget
    QWidget *logsWidget;
    // Canvas widget
    QWidget *canvasWidget;
    // Entity widget
    QWidget *entityWidget;
    // IFF widget
    QWidget *iffWidget;
    // Formation widget
    QWidget *formationWidget;
    // Fixed point widget
    QWidget *fixedPointWidget;
    // Weapon widget
    QWidget *weaponWidget;

    // %%% Shared UI Elements %%%
    // System status label
    QLabel *systemLabel;
    // Simulation label
    QLabel *simLabel;
    // RTC label
    QLabel *rtcLabel;
    // Filter label
    QLabel *filterLabel;
    // Time range combo box
    QComboBox *timeRangeCombo;
    // Sensor type combo box
    QComboBox *sensorTypeCombo;
    // Entity ID combo box
    QComboBox *entityIdCombo;
    // Uptime label
    QLabel *uptimeLabel;
    // Entities count label
    QLabel *entitiesLabel;
    // Feedback events label
    QLabel *feedbackEventsLabel;
    // System status label
    QLabel *systemStatusLabel;
    // CPU usage progress bar
    QProgressBar *cpuProgressBar;
    // Sensor data table
    QTableWidget *sensorTable;
    // Interactions table
    QTableWidget *interactionsTable;
    // Logs text edit
    QTextEdit *logsTextEdit;

    // %%% Graph Widgets %%%
    // Storage chart widget
    CustomGraphWidget *storageChart;
    // RTC logs chart widget
    CustomGraphWidget *rtcLogsChart;
    // MongoDB label
    QLabel *mongoDbLabel;
    // Logs label
    QLabel *logsLabel;
    // Scenarios label
    QLabel *scenariosLabel;
    // Total label
    QLabel *totalLabel;
    // Sensor chart widget
    CustomGraphWidget *sensorChart;
    // Radar feedback label
    QLabel *radarFeedbackLabel;
    // IFF feedback label
    QLabel *iffFeedbackLabel;

    // %%% Radio Section %%%
    // Radio system label
    QLabel *radioSystemLabel;
    // Frequency label
    QLabel *frequencyLabel;
    // Signal strength label
    QLabel *signalStrengthLabel;

    // %%% Network Section %%%
    // Connectivity label
    QLabel *connectivityLabel;
    // Bandwidth label
    QLabel *bandwidthLabel;
    // Latency label
    QLabel *latencyLabel;

    // %%% Entity Section %%%
    // Entity frame
    QFrame *entityFrame;
    // Total entities label
    QLabel *totalEntitiesLabel;
    // Active entities label
    QLabel *activeEntitiesLabel;
    // Entity selection combo
    QComboBox *selectEntityCombo;
    // Details button
    QPushButton *detailsButton;
    // Detailed view widget
    QWidget *detailedViewWidget;
    // Detailed view header
    QLabel *detailedViewHeader;
    // Detailed view text edit
    QTextEdit *detailedViewTextEdit;
    // Detailed view logs text edit
    QTextEdit *detailedViewLogsTextEdit;
    // Interaction frequency table
    QTableWidget *interactionFreqTable;
    // Entity scroll area
    QScrollArea *entityScrollArea;
    // General scroll area
    QScrollArea *scrollArea;

    // %%% IFF Section %%%
    // IFF frame
    QFrame *iffFrame;
    // Total IFFs label
    QLabel *totalIffsLabel;
    // Active IFFs label
    QLabel *activeIffsLabel;
    // IFF selection combo
    QComboBox *selectIffCombo;
    // IFF detailed view widget
    QWidget *iffDetailedViewWidget;
    // IFF detailed view text edit
    QTextEdit *iffDetailedViewTextEdit;

    // %%% Formation Section %%%
    // Formation frame
    QFrame *formationFrame;
    // Total formations label
    QLabel *totalFormationsLabel;
    // Active formations label
    QLabel *activeFormationsLabel;
    // Formation selection combo
    QComboBox *selectFormationCombo;
    // Formation detailed view widget
    QWidget *formationDetailedViewWidget;
    // Formation detailed view text edit
    QTextEdit *formationDetailedViewTextEdit;

    // %%% Fixed Point Section %%%
    // Fixed point frame
    QFrame *fixedPointFrame;
    // Total fixed points label
    QLabel *totalFixedPointsLabel;
    // Active fixed points label
    QLabel *activeFixedPointsLabel;
    // Fixed point selection combo
    QComboBox *selectFixedPointCombo;
    // Fixed point detailed view widget
    QWidget *fixedPointDetailedViewWidget;
    // Fixed point detailed view text edit
    QTextEdit *fixedPointDetailedViewTextEdit;

    // %%% Weapon Section %%%
    // Weapon frame
    QFrame *weaponFrame;
    // Total weapons label
    QLabel *totalWeaponsLabel;
    // Active weapons label
    QLabel *activeWeaponsLabel;
    // Weapon selection combo
    QComboBox *selectWeaponCombo;
    // Weapon detailed view widget
    QWidget *weaponDetailedViewWidget;
    // Weapon detailed view text edit
    QTextEdit *weaponDetailedViewTextEdit;

    // %%% Sensor and Radio Section %%%
    // Sensor selection combo
    QComboBox *selectSensorCombo;
    // Radio selection combo
    QComboBox *selectRadioCombo;
    // Total sensors label
    QLabel *totalSensorsLabel;
    // Active sensors label
    QLabel *activeSensorsLabel;
    // Total radios label
    QLabel *totalRadiosLabel;
    // Active radios label
    QLabel *activeRadiosLabel;
    // Sensor detailed view widget
    QWidget *sensorDetailedViewWidget;
    // Sensor detailed view text edit
    QTextEdit *sensorDetailedViewTextEdit;
    // Radio detailed view widget
    QWidget *radioDetailedViewWidget;
    // Radio detailed view text edit
    QTextEdit *radioDetailedViewTextEdit;

};

#endif // FEEDBACK_H
