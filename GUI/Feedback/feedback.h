
#ifndef FEEDBACK_H
#define FEEDBACK_H

#include "core/Hierarchy/hierarchy.h"
#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QComboBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QFile>


// Custom widget for drawing simple graphs (unchanged)
class CustomGraphWidget : public QWidget
{
    Q_OBJECT
public:
    enum GraphType { Bar, Line, Pie };
    CustomGraphWidget(GraphType type, QWidget *parent = nullptr) : QWidget(parent), m_graphType(type) {}

    void setData(const QList<qreal> &data, const QStringList &labels = QStringList()) {
        m_data = data;
        m_labels = labels;
        update();
    }

    void setLineData(const QList<QList<qreal>> &linesData) {
        m_linesData = linesData;
        update();
    }

protected:
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
    GraphType m_graphType;
    QList<qreal> m_data;
    QStringList m_labels;
    QList<QList<qreal>> m_linesData;
};

class Feedback : public QMainWindow
{
    Q_OBJECT

public:
    Feedback(QWidget *parent = nullptr);
    ~Feedback();
    Hierarchy *h;
    void loadDashboardData(const QString& jsonData);

private slots:
    void onSidebarItemClicked(QListWidgetItem *item);
    void onDetailsButtonClicked();
    void onEntityComboChanged(const QString &text);
    void onIffComboChanged(const QString &text);
    void onFormationComboChanged(const QString &text); // New slot for Formation dropdown
    void onFixedPointComboChanged(const QString &text); // New slot for FixedPoint dropdown
    void onWeaponComboChanged(const QString &text); // New slot for Weapon dropdown


private:
    void setupUi();
    void setupOverviewWidget();
    void setupStorageWidget();
    void setupSensorsWidget();
    void setupRadioWidget();
    void setupNetworkWidget();
    void setupLogsWidget();
    void setupCanvasWidget();
    void setupEntityWidget();
    void setupIffWidget();
    void setupFormationWidget();
    void setupFixedPointWidget();
    void setupWeaponWidget();
     // void loadDashboardData(const QString& jsonData);

    // Sidebar
    QListWidget *sidebar;

    // Stacked widget for content
    QStackedWidget *contentStack;

    // Widgets for each section
    QWidget *overviewWidget;
    QWidget *storageWidget;
    QWidget *sensorsWidget;
    QWidget *radioWidget;
    QWidget *networkWidget;
    QWidget *logsWidget;
    QWidget *canvasWidget;
    QWidget *entityWidget;
    QWidget *iffWidget;
    QWidget *formationWidget;
    QWidget *fixedPointWidget;
    QWidget *weaponWidget;

    // Shared UI elements
    QLabel *systemLabel;
    QLabel *simLabel;
    QLabel *rtcLabel;
    QLabel *filterLabel;
    QComboBox *timeRangeCombo;
    QComboBox *sensorTypeCombo;
    QComboBox *entityIdCombo;
    QLabel *uptimeLabel;
    QLabel *entitiesLabel;
    QLabel *feedbackEventsLabel;
    QLabel *systemStatusLabel;
    QProgressBar *cpuProgressBar;
    QTableWidget *sensorTable;
    QTableWidget *interactionsTable;
    QTextEdit *logsTextEdit;

    CustomGraphWidget *storageChart;
    CustomGraphWidget *rtcLogsChart;
    QLabel *mongoDbLabel;
    QLabel *logsLabel;
    QLabel *scenariosLabel;
    QLabel *totalLabel;

    // Section Headers
    QLabel *overviewHeader;
    QLabel *interactionsHeader;
    QLabel *storageHeader;
    QLabel *rtcLogsHeader;
    CustomGraphWidget *sensorChart;
    QLabel *radarFeedbackLabel;
    QLabel *iffFeedbackLabel;

    // Radio Section Labels
    QLabel *radioSystemLabel;
    QLabel *frequencyLabel;
    QLabel *signalStrengthLabel;

    // Network Section Labels
    QLabel *connectivityLabel;
    QLabel *bandwidthLabel;
    QLabel *latencyLabel;

    // Entity Section UI Elements
    QFrame *entityFrame;
    QLabel *totalEntitiesLabel;
    QLabel *activeEntitiesLabel;
    QComboBox *selectEntityCombo;
    QPushButton *detailsButton;
    QWidget *detailedViewWidget;
    QLabel *detailedViewHeader;
    QTextEdit *detailedViewTextEdit;
    QTextEdit *detailedViewLogsTextEdit;
    QTableWidget *interactionFreqTable;
    QScrollArea *entityScrollArea;
    QScrollArea *scrollArea;

    // IFF Section UI Elements
    QFrame *iffFrame;
    QLabel *totalIffsLabel;
    QLabel *activeIffsLabel;
    QComboBox *selectIffCombo;
    QWidget *iffDetailedViewWidget;
    QTextEdit *iffDetailedViewTextEdit;

    // Formation Section UI Elements
    QFrame *formationFrame;
    QLabel *totalFormationsLabel;
    QLabel *activeFormationsLabel;
    QComboBox *selectFormationCombo;
    QWidget *formationDetailedViewWidget;
    QTextEdit *formationDetailedViewTextEdit;

    // FixedPoint Section UI Elements
    QFrame *fixedPointFrame;
    QLabel *totalFixedPointsLabel;
    QLabel *activeFixedPointsLabel;
    QComboBox *selectFixedPointCombo;
    QWidget *fixedPointDetailedViewWidget;
    QTextEdit *fixedPointDetailedViewTextEdit;

    // Weapon Section UI Elements
    QFrame *weaponFrame;
    QLabel *totalWeaponsLabel;
    QLabel *activeWeaponsLabel;
    QComboBox *selectWeaponCombo;
    QWidget *weaponDetailedViewWidget;
    QTextEdit *weaponDetailedViewTextEdit;

    QComboBox *selectSensorCombo;
    QComboBox *selectRadioCombo;
    QLabel *totalSensorsLabel;
    QLabel *activeSensorsLabel;
    QLabel *totalRadiosLabel;
    QLabel *activeRadiosLabel;
    QWidget *sensorDetailedViewWidget;
    QTextEdit *sensorDetailedViewTextEdit;
    QWidget *radioDetailedViewWidget;
    QTextEdit *radioDetailedViewTextEdit;

};

#endif // FEEDBACK_H
