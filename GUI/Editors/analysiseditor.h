
/* ========================================================================= */
/* File: analysiseditor.h                                                    */
/* Purpose: AnalysisEditor — charts dashboard + Reports panel toggle.        */
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
#include <QJsonDocument>
#include <QFile>
#include <QMessageBox>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QScatterSeries>

#include "GUI/Editors/recentprojectsmanager.h"
#include "GUI/Editors/reportseditor.h"

QT_CHARTS_USE_NAMESPACE

    class AnalysisEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit AnalysisEditor(QWidget *parent = nullptr);
    ~AnalysisEditor();

    bool        hasUnsavedChanges = false;
    QString     lastSavedFilePath;

    void loadFromJsonFile(const QString& filePath);
    void clearUnsavedChanges();

signals:
    void unsavedChangesChanged(bool hasUnsaved);
    void Activated();

public slots:
    void runAnalysis();
    void compareScenarios();
    void exportResults();

private:
    /* ── page stack: 0 = analysis dashboard, 1 = reports ── */
    QStackedWidget* m_stack        = nullptr;
    QWidget*        m_analysisPage = nullptr;
    ReportsEditor*  m_reportsPage  = nullptr;

    /* ── bottom tab buttons ── */
    QPushButton*    m_tabAnalysis  = nullptr;
    QPushButton*    m_tabReports   = nullptr;

    /* ── metric labels ── */
    QLabel* m_successProbLabel     = nullptr;
    QLabel* m_friendlyLossesLabel  = nullptr;
    QLabel* m_enemyLossesLabel     = nullptr;
    QLabel* m_detectionEffLabel    = nullptr;
    QLabel* m_weaponEffLabel       = nullptr;

    /* ── calculator inputs ── */
    QLineEdit* m_assetsEdit        = nullptr;
    QLineEdit* m_sensorCoverageEdit= nullptr;
    QComboBox* m_ecmLevelCombo     = nullptr;
    QComboBox* m_enemyStrengthCombo= nullptr;
    QComboBox* m_warheadTypeCombo  = nullptr;

    /* ── charts ── */
    QChartView* m_engagementChart1 = nullptr;
    QChartView* m_successChart     = nullptr;
    QChartView* m_engagementChart2 = nullptr;
    QChartView* m_lossesChart      = nullptr;

    void        setupUI();
    QWidget*    buildAnalysisDashboard();
    QWidget*    buildMissionMetricsPanel();
    QWidget*    buildProbabilityCalculatorPanel();
    QChartView* buildEngagementTimelineChart(bool second = false);
    QChartView* buildSuccessProbabilityChart();
    QChartView* buildLossesVsEngagementsChart();
    QWidget*    buildBottomTabBar();
    void        switchTab(int index);

    struct AnalysisResult {
        double successProbability  = 80.0;
        int    friendlyLosses      = 3;
        int    enemyLosses         = 5;
        double detectionEfficiency = 75.0;
        double weaponEffectiveness = 60.0;
    };
    AnalysisResult m_currentResult;
    void           computeAnalysis();
    void           refreshMetricLabels();
    void           markUnsavedChanges();
};

#endif // ANALYSISEDITOR_H
