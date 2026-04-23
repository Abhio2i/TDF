/* =============================================================================
 * FILE:         reportseditor.h
 * MODULE:       Reports Dashboard
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the ReportsEditor class and its helper widgets
 *               (ReportTimelineWidget, ReportGaugeWidget) which together form
 *               a comprehensive reports dashboard. Provides mission summary,
 *               engagement timeline (custom painted), detection probability chart,
 *               ECM/ECCM analysis, weapon usage table, lessons learned, report
 *               options (section selection, format choice), and export actions
 *               (PDF, Word, Print, save/load templates). Loads data from JSON
 *               and supports team selection for dynamic updates.
 *
 * REQUIREMENTS: REQ-REPORTS-010  Mission summary KPI cards
 *               REQ-REPORTS-011  Engagement timeline with event points
 *               REQ-REPORTS-012  Detection probability chart (QChart)
 *               REQ-REPORTS-013  ECM/ECCM analysis panel
 *               REQ-REPORTS-014  Weapon usage table with hit percentages
 *               REQ-REPORTS-015  Lessons Learned text section
 *               REQ-REPORTS-016  Report section selection checkboxes
 *               REQ-REPORTS-017  Export to PDF and Word
 *               REQ-REPORTS-018  Print report
 *               REQ-REPORTS-019  Save/load report templates
 *               REQ-REPORTS-020  Team selector for per-team data views
 *               REQ-REPORTS-021  Semi-circle gauge widget for burn‑through range
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-REPORTS-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#ifndef REPORTSEDITOR_H
#define REPORTSEDITOR_H

#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QFrame>
#include <QProgressBar>
#include <QButtonGroup>
#include <QTextEdit>
#include <QPainter>
#include <QPainterPath>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextDocument>
#include <QPrinter>
#include <QPrintDialog>
#include <QPageSize>
#include <QPageLayout>
#include <QDataStream>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

QT_CHARTS_USE_NAMESPACE

    /* =========================================================================
   ReportTimelineWidget  — custom painted engagement timeline
   ========================================================================= */
    class ReportTimelineWidget : public QWidget
{
public:
    explicit ReportTimelineWidget(QWidget* parent = nullptr);

    struct EventPoint { double timeSec; QColor color; };

    /* Call to populate with real data */
    void setEvents(const QList<EventPoint>& detection,
                   const QList<EventPoint>& engagement,
                   const QList<EventPoint>& weaponFired,
                   const QList<EventPoint>& damage,
                   double maxTimeSec);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QList<EventPoint> m_detection;
    QList<EventPoint> m_engagement;
    QList<EventPoint> m_weaponFired;
    QList<EventPoint> m_damage;
    double            m_maxTime = 60.0;
    bool              m_hasData = false;
};

/* =========================================================================
   ReportGaugeWidget  — semi-circle gauge
   ========================================================================= */
class ReportGaugeWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue)
public:
    explicit ReportGaugeWidget(int initialValue, QWidget* parent = nullptr);
    int  value() const   { return m_value; }
    void setValue(int v) { m_value = v; update(); }
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    int m_value;
};

/* =========================================================================
   ReportsEditor  — full dashboard
   ========================================================================= */
class ReportsEditor : public QWidget
{
    Q_OBJECT

public:
    explicit ReportsEditor(QWidget* parent = nullptr);
    ~ReportsEditor() = default;

    /* ── Data structs ── */
    struct TeamMetrics {
        double successProbability  = 0.0;
        double detectionEfficiency = 0.0;
        double weaponEffectiveness = 0.0;
        double friendlyLosses      = 0.0;
        double enemyLosses         = 0.0;
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

    /* Entry point — call after JSON load */
    void loadFromJson(const QJsonObject& root);

signals:
    void closeRequested();

private slots:
    void onGenerateReport();
    void onPreviewReport();
    void onExportPDF();
    void onExportWord();
    void onPrint();
    void onSaveTemplate();
    void onLoadTemplate();
    void onTeamSelected(const QString& teamName);

private:
    /* ── build helpers ── */
    void        setupUI();
    QWidget*    buildLeftPanel();
    QWidget*    buildCenterTop();
    QWidget*    buildEngagementTimeline();
    QWidget*    buildLessonsLearned();
    QWidget*    buildRightTop();
    QWidget*    buildECMPanel();
    QWidget*    buildWeaponUsageTable();
    QWidget*    buildBottomBar();
    QChartView* buildDetectionChart();

    /* ── static style helpers ── */
    static QLabel*  kpiCard(const QString& icon, const QString& title,
                           const QString& value, const QString& color);
    static QFrame*  hLine();
    static QWidget* weaponRow(const QString& name, int used, int hits,
                              int pct, const QString& barColor);

    /* ── parse helper ── */
    static QList<double> jsonObjToSortedValues(const QJsonValue& val,
                                               QList<double>* timesOut = nullptr);

    /* ── export helper ── */
    QString buildReportHtml(const QStringList& sections = QStringList()) const;

    /* ── loaded data ── */
    QString                     m_missionName;
    QString                     m_missionDate;
    QStringList                 m_teamNames;
    QMap<QString, TeamMetrics>  m_teamMetrics;
    QMap<QString, TimelineData> m_teamTimelines;
    QMap<QString, LossesData>   m_teamLosses;
    QMap<QString, QColor>       m_teamColors;
    QString                     m_selectedTeam;

    static QColor paletteColor(int idx);

    /* ── member widgets ── */
    QComboBox*         m_templateCombo  = nullptr;
    QComboBox*         m_ecmTypeCombo   = nullptr;
    QCheckBox*         m_freqAgilityChk = nullptr;
    QCheckBox*         m_pulseCompChk   = nullptr;
    QCheckBox*         m_sideLobChk     = nullptr;
    ReportGaugeWidget* m_gauge          = nullptr;
    QLabel*            m_burnLabel      = nullptr;

    /* ── live-updatable widgets ── */
    QLabel*      m_missionTitleLabel = nullptr;
    QLabel*      m_kpiSuccess        = nullptr;
    QLabel*      m_kpiFriendly       = nullptr;
    QLabel*      m_kpiEnemy          = nullptr;
    QLabel*      m_kpiDuration       = nullptr;

    QWidget*     m_teamSelectorBar   = nullptr;
    QHBoxLayout* m_teamSelectorHL    = nullptr;
    QMap<QString, QPushButton*> m_teamBtns;

    ReportTimelineWidget* m_timelineWidget = nullptr;
    QVBoxLayout*          m_highlightsVL   = nullptr;

    /* ── Left panel: section checkboxes + format radio group ── */
    QList<QCheckBox*>  m_sectionCheckboxes;
    QButtonGroup*      m_fmtGroup = nullptr;
};

#endif // REPORTSEDITOR_H
