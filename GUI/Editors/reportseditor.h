/* ========================================================================= */
/* File: reportseditor.h                                                     */
/* Purpose: Reports dashboard — Mission Summary, Engagement Timeline,        */
/*          Detection Probability chart, ECM/ECCM Analysis, Weapon Usage,   */
/*          Lessons Learned, Report Options, and export actions.             */
/* Written by: Arti Rajpoot                                                  */
/* ========================================================================= */
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
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

QT_CHARTS_USE_NAMESPACE

    /* =========================================================================
   ReportTimelineWidget
   Custom painted horizontal engagement timeline — top-level class (no MOC)
   ========================================================================= */
    class ReportTimelineWidget : public QWidget
{
public:
    explicit ReportTimelineWidget(QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent* event) override;
};

/* =========================================================================
   ReportGaugeWidget
   Semi-circle gauge for ECM effectiveness — top-level class.
   Uses Q_PROPERTY so must be top-level (not nested).
   ========================================================================= */
class ReportGaugeWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue)

public:
    explicit ReportGaugeWidget(int initialValue, QWidget* parent = nullptr);

    int  value() const  { return m_value; }
    void setValue(int v){ m_value = v; update(); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_value;
};

/* =========================================================================
   ReportsEditor
   Full-screen reports dashboard widget embedded inside AnalysisEditor.
   ========================================================================= */
class ReportsEditor : public QWidget
{
    Q_OBJECT

public:
    explicit ReportsEditor(QWidget* parent = nullptr);
    ~ReportsEditor() = default;

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

    /* ── member widgets ── */
    QComboBox*          m_templateCombo  = nullptr;
    QComboBox*          m_ecmTypeCombo   = nullptr;
    QCheckBox*          m_freqAgilityChk = nullptr;
    QCheckBox*          m_pulseCompChk   = nullptr;
    QCheckBox*          m_sideLobChk     = nullptr;
    ReportGaugeWidget*  m_gauge          = nullptr;
    QLabel*             m_burnLabel      = nullptr;
};

#endif // REPORTSEDITOR_H
