
/* ========================================================================= */
/* File: analysiseditor.h                                                    */
/* Purpose: Fully dynamic multi-team analytics.                              */
/* Written by: Arti Rajpoot                                                  */
/* ========================================================================= */
#ifndef ANALYSISEDITOR_H
#define ANALYSISEDITOR_H

#include <QMainWindow>
#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QMessageBox>
#include <QFrame>
#include <QTableWidget>
#include <QHeaderView>
#include <QMap>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include "GUI/Editors/reportseditor.h"

QT_CHARTS_USE_NAMESPACE

    class AnalysisEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit AnalysisEditor(QWidget *parent = nullptr);
    ~AnalysisEditor();

    bool    hasUnsavedChanges = false;
    QString lastSavedFilePath;

    void loadFromJsonFile(const QString& filePath);
    void clearUnsavedChanges();
    void loadFromHierarchyJson(const QJsonObject& root);

signals:
    void unsavedChangesChanged(bool hasUnsaved);
    void Activated();

public slots:
    void runAnalysis();
    void compareScenarios();
    void exportResults();
    void openScenarioFile();


private:
    /* stack / tabs */
    QStackedWidget* m_stack        = nullptr;
    QWidget*        m_analysisPage = nullptr;
    ReportsEditor*  m_reportsPage  = nullptr;
    QPushButton*    m_tabAnalysis  = nullptr;
    QPushButton*    m_tabReports   = nullptr;

    /* legacy labels */
    QLabel* m_successProbLabel    = nullptr;
    QLabel* m_friendlyLossesLabel = nullptr;
    QLabel* m_enemyLossesLabel    = nullptr;
    QLabel* m_detectionEffLabel   = nullptr;
    QLabel* m_weaponEffLabel      = nullptr;

    /* calculator inputs */
    QLineEdit* m_assetsEdit         = nullptr;
    QLineEdit* m_sensorCoverageEdit = nullptr;
    QComboBox* m_ecmLevelCombo      = nullptr;
    QComboBox* m_enemyStrengthCombo = nullptr;
    QComboBox* m_warheadTypeCombo   = nullptr;

    /* chart views */
    QChartView* m_engagementChart1 = nullptr;
    QChartView* m_engagementChart2 = nullptr;
    QChartView* m_successChart     = nullptr;
    QChartView* m_lossesChart      = nullptr;

    /* ── data structs ── */
    struct TeamMetrics {
        double successProbability  = 80.0;
        int    friendlyLosses      = 3;
        int    enemyLosses         = 5;
        double detectionEfficiency = 75.0;
        double weaponEffectiveness = 60.0;
    };
    struct TimelineData {
        QList<double> timePoints;
        QList<double> detection;
        QList<double> engagementTimePoints;
        QList<double> engagement;
        QList<double> damageTimePoints;
        QList<double> damage;
    };
    struct LossesData {
        QStringList   categories;
        QList<double> friendlyLosses;
        QList<double> enemyLosses;
    };
    struct AnalysisResult {
        double successProbability  = 80.0;
        int    friendlyLosses      = 3;
        int    enemyLosses         = 5;
        double detectionEfficiency = 75.0;
        double weaponEffectiveness = 60.0;
    };

    /* ── DYNAMIC multi-team data ── */
    QStringList                 m_teamNames;
    QMap<QString,TeamMetrics>   m_teamMetrics;
    QMap<QString,TimelineData>  m_teamTimelines;
    QMap<QString,LossesData>    m_teamLosses;
    QMap<QString,QColor>        m_teamColors;

    AnalysisResult m_currentResult;

    /* row-2 selector */
    QString  m_selectedTeam;
    QWidget* m_teamSelectorBar  = nullptr;
    QMap<QString,QPushButton*> m_teamButtons;

    /* row-1 combined chart selector */
    QString  m_combinedFilterTeam;
    QWidget* m_combinedSelectorBar  = nullptr;
    QMap<QString,QPushButton*> m_combinedTeamButtons;
    QPushButton* m_combinedAllBtn   = nullptr;

    /* metrics panel (rebuilt on load) */
    QWidget*     m_metricsWidget = nullptr;
    QVBoxLayout* m_metricsVL     = nullptr;
    QMap<QString,QLabel*> m_metricLabels;

    /* ── colour palette ── */
    static QColor paletteColor(int idx);
    static QColor colorForTeam(const QString& name, int idx);
    static void parseTimelineField(const QJsonValue& val,
                                   QList<double>& times,
                                   QList<double>& values)
    {
        times.clear();
        values.clear();
        if (val.isArray()) {
            for (const auto& v : val.toArray())
                values << v.toDouble();
            return;
        }
        if (val.isObject()) {
            QJsonObject obj = val.toObject();
            QList<QPair<double,double>> pairs;
            for (const QString& k : obj.keys())
                pairs.append({k.toDouble(), obj.value(k).toDouble()});
            std::sort(pairs.begin(), pairs.end(),
                      [](const QPair<double,double>& a, const QPair<double,double>& b){
                          return a.first < b.first; });
            for (const auto& p : pairs) {
                times  << p.first;
                values << p.second;
            }
        }
    }

    /* build */
    void        setupUI();
    QWidget*    buildAnalysisDashboard();
    QWidget*    buildMissionMetricsPanel();
    QWidget*    buildProbabilityCalculatorPanel();
    QChartView* buildEngagementTimelineChart();
    QWidget*    buildSelectableTimelineChart();
    QChartView* buildSuccessProbabilityChart();
    QChartView* buildLossesVsEngagementsChart();
    QWidget*    buildBottomTabBar();
    void        switchTab(int index);

    /* parsers */
    void parseScenarioJson(const QJsonObject& root);
    void parseAnalysisJson(const QJsonObject& root);
    void parseLegacyJson(const QJsonObject& root);

    /* rebuild after load */
    void rebuildAllCharts();
    void rebuildCombinedEngagementChart();
    QWidget* buildCombinedTimelineChart();
    void rebuildCombinedSelectorBar();
    void switchCombinedFilter(const QString& teamName);
    void switchSelectedTeam(const QString& teamName);
    void rebuildRow2Chart();
    void rebuildSuccessChart();
    void rebuildLossesChart();
    void rebuildMetricsPanel();
    void rebuildTeamSelectorBar();
    void refreshAllMetricLabels();
    void refreshMetricLabels();
    void computeAnalysis();
    void markUnsavedChanges();

    static QList<double> toDoubleList(const QJsonArray& arr);
    static QList<double> scaleList(QList<double> src, double factor);
    QMap<QString, bool>        m_seriesVisible;
    QMap<QString, QPushButton*> m_seriesButtons;
};

#endif // ANALYSISEDITOR_H
