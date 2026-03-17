
/* ========================================================================= */
/* File: analysiseditor.cpp                                                  */
/* Purpose: AnalysisEditor — charts dashboard + Reports tab at bottom.       */
/*          Clicking "📋 Reports" tab shows ReportsEditor page.              */
/* Written by: Arti Rajpoot                                                  */
/* ========================================================================= */

#include "analysiseditor.h"
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QJsonArray>

/* ─── palette ─── */
static const QString DARK_BG     = "#0F2636";
static const QString PANEL_BG    = "#1A3A4F";
static const QString BORDER_COL  = "#2a4a6a";
static const QString ACCENT_COL  = "#00BFFF";

/* =========================================================================
   Constructor
   ========================================================================= */
AnalysisEditor::AnalysisEditor(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Analysis Editor");
    resize(1300, 750);
    setupUI();
}

AnalysisEditor::~AnalysisEditor() {}

/* =========================================================================
   setupUI  —  QMainWindow  central widget holds:
     [QVBoxLayout]
       QStackedWidget   (page 0 = dashboard scroll area, page 1 = reports)
       Bottom tab bar   (Analysis | Reports)
   ========================================================================= */
void AnalysisEditor::setupUI()
{
    QWidget* central = new QWidget(this);
    central->setStyleSheet(
        QString("QWidget { background:%1; color:white;"
                "  font-family:'Segoe UI',sans-serif; }").arg(DARK_BG));

    QVBoxLayout* rootVL = new QVBoxLayout(central);
    rootVL->setContentsMargins(0, 0, 0, 0);
    rootVL->setSpacing(0);

    /* ── stacked widget ── */
    m_stack = new QStackedWidget();

    /* page 0: analysis dashboard */
    m_analysisPage = buildAnalysisDashboard();
    m_stack->addWidget(m_analysisPage);

    /* page 1: reports */
    m_reportsPage = new ReportsEditor();
    m_stack->addWidget(m_reportsPage);

    rootVL->addWidget(m_stack, 1);

    /* ── bottom tab bar ── */
    rootVL->addWidget(buildBottomTabBar(), 0);

    setCentralWidget(central);

    /* start on analysis page */
    switchTab(0);
}

/* =========================================================================
   Bottom tab bar — "📊 Analysis"  |  "📋 Reports"
   ========================================================================= */
QWidget* AnalysisEditor::buildBottomTabBar()
{
    QWidget* bar = new QWidget();
    bar->setFixedHeight(40);
    bar->setStyleSheet(
        QString("background:%1; border-top:2px solid %2;")
            .arg(PANEL_BG, BORDER_COL));

    QHBoxLayout* hl = new QHBoxLayout(bar);
    hl->setContentsMargins(8, 4, 8, 4);
    hl->setSpacing(6);

    auto makeTab = [&](const QString& label, int idx) {
        QPushButton* btn = new QPushButton(label);
        btn->setFixedHeight(28);
        btn->setMinimumWidth(130);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setCheckable(true);
        connect(btn, &QPushButton::clicked, this, [=]() { switchTab(idx); });
        return btn;
    };

    m_tabAnalysis = makeTab("📊  Analysis", 0);
    m_tabReports  = makeTab("📋  Reports",  1);

    hl->addStretch();
    hl->addWidget(m_tabAnalysis);
    hl->addWidget(m_tabReports);
    hl->addStretch();

    return bar;
}

/* =========================================================================
   switchTab  —  highlight active tab + flip stack page
   ========================================================================= */
void AnalysisEditor::switchTab(int index)
{
    m_stack->setCurrentIndex(index);

    QString active =
        "QPushButton { background:#0d6efd; color:white; border:none;"
        "  border-radius:4px; font-size:12px; font-weight:bold; }";
    QString inactive =
        "QPushButton { background:#1a3a5a; color:#aaaaaa; border:1px solid #2a4a6a;"
        "  border-radius:4px; font-size:12px; }"
        "QPushButton:hover { background:#2a4a6a; color:white; }";

    if (m_tabAnalysis) m_tabAnalysis->setStyleSheet(index == 0 ? active : inactive);
    if (m_tabReports)  m_tabReports->setStyleSheet(index == 1  ? active : inactive);
}

/* =========================================================================
   buildAnalysisDashboard  —  scroll area with 2 rows of 3 panels each
   ========================================================================= */
QWidget* AnalysisEditor::buildAnalysisDashboard()
{
    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { background:transparent; border:none; }");

    QWidget* page = new QWidget();
    page->setStyleSheet("background:transparent;");

    QVBoxLayout* vl = new QVBoxLayout(page);
    vl->setContentsMargins(8, 8, 8, 8);
    vl->setSpacing(8);

    /* row 1 */
    QHBoxLayout* row1 = new QHBoxLayout();
    row1->setSpacing(8);
    row1->addWidget(buildMissionMetricsPanel(),         2);
    row1->addWidget(buildEngagementTimelineChart(false), 5);
    row1->addWidget(buildSuccessProbabilityChart(),      3);
    vl->addLayout(row1, 5);

    /* row 2 */
    QHBoxLayout* row2 = new QHBoxLayout();
    row2->setSpacing(8);
    row2->addWidget(buildProbabilityCalculatorPanel(),  2);
    row2->addWidget(buildEngagementTimelineChart(true),  5);
    row2->addWidget(buildLossesVsEngagementsChart(),     3);
    vl->addLayout(row2, 5);

    scroll->setWidget(page);
    return scroll;
}

/* =========================================================================
   Mission Metrics panel
   ========================================================================= */
QWidget* AnalysisEditor::buildMissionMetricsPanel()
{
    QGroupBox* box = new QGroupBox("Mission Metrics");
    box->setStyleSheet(
        QString("QGroupBox {"
                "  background:%1; border:1px solid %2; border-radius:6px;"
                "  color:white; font-size:13px; font-weight:bold;"
                "  margin-top:8px; padding-top:10px;"
                "}"
                "QGroupBox::title {"
                "  subcontrol-origin:margin; left:10px; padding:0 6px; color:%3;"
                "}").arg(PANEL_BG, BORDER_COL, ACCENT_COL));

    QVBoxLayout* vl = new QVBoxLayout(box);
    vl->setSpacing(8);
    vl->setContentsMargins(12, 14, 12, 12);

    auto addRow = [&](const QString& labelText, const QString& value,
                      const QString& color, QLabel*& out) {
        QHBoxLayout* hl = new QHBoxLayout();
        QLabel* lbl = new QLabel(labelText + ":");
        lbl->setStyleSheet("color:#cccccc; font-size:12px;");
        QLabel* val = new QLabel(value);
        val->setStyleSheet(
            QString("color:%1; font-size:13px; font-weight:bold;").arg(color));
        val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        out = val;
        hl->addWidget(lbl, 3);
        hl->addWidget(val, 2);
        vl->addLayout(hl);
    };

    addRow("Success Probability", "80 %★",  "#00cc44", m_successProbLabel);
    addRow("Friendly Losses",     "3 🚢",   "#ff9944", m_friendlyLossesLabel);
    addRow("Enemy Losses",        "5 🚢",   "#ff4444", m_enemyLossesLabel);
    addRow("Detection Efficiency","75 %💧", "#44aaff", m_detectionEffLabel);
    addRow("Weapon Effectiveness","60 %🚀", "#ff4444", m_weaponEffLabel);
    vl->addStretch();

    return box;
}

/* =========================================================================
   Probability Calculator panel
   ========================================================================= */
QWidget* AnalysisEditor::buildProbabilityCalculatorPanel()
{
    QGroupBox* box = new QGroupBox("Probability Calculator");
    box->setStyleSheet(
        QString("QGroupBox {"
                "  background:%1; border:1px solid %2; border-radius:6px;"
                "  color:white; font-size:13px; font-weight:bold;"
                "  margin-top:8px; padding-top:10px;"
                "}"
                "QGroupBox::title {"
                "  subcontrol-origin:margin; left:10px; padding:0 6px; color:%3;"
                "}").arg(PANEL_BG, BORDER_COL, ACCENT_COL));

    QString inputStyle =
        "QLineEdit, QComboBox {"
        "  background:#0d2030; color:white; border:1px solid #2a4a6a;"
        "  border-radius:3px; padding:3px 6px; font-size:12px; }"
        "QComboBox::drop-down { border:none; }"
        "QComboBox QAbstractItemView { background:#0d2030; color:white;"
        "  selection-background-color:#1a5a8a; }";
    QString lblStyle = "QLabel { color:#cccccc; font-size:12px; font-weight:normal; }";

    QFormLayout* fl = new QFormLayout();
    fl->setSpacing(8);
    fl->setContentsMargins(12, 14, 12, 12);
    fl->setLabelAlignment(Qt::AlignLeft);

    auto mkLabel = [&](const QString& t) {
        QLabel* l = new QLabel(t); l->setStyleSheet(lblStyle); return l;
    };

    m_assetsEdit = new QLineEdit("10");
    m_assetsEdit->setStyleSheet(inputStyle);
    m_assetsEdit->setFixedHeight(26);
    fl->addRow(mkLabel("Number of Assets:"), m_assetsEdit);

    m_sensorCoverageEdit = new QLineEdit("200 km");
    m_sensorCoverageEdit->setStyleSheet(inputStyle);
    m_sensorCoverageEdit->setFixedHeight(26);
    fl->addRow(mkLabel("Sensor Coverage:"), m_sensorCoverageEdit);

    m_ecmLevelCombo = new QComboBox();
    m_ecmLevelCombo->addItems({"Low","Moderate","High"});
    m_ecmLevelCombo->setCurrentText("Moderate");
    m_ecmLevelCombo->setStyleSheet(inputStyle);
    m_ecmLevelCombo->setFixedHeight(26);
    fl->addRow(mkLabel("ECM Level:"), m_ecmLevelCombo);

    m_enemyStrengthCombo = new QComboBox();
    m_enemyStrengthCombo->addItems({"Low","Moderate","High","Very High"});
    m_enemyStrengthCombo->setCurrentText("High");
    m_enemyStrengthCombo->setStyleSheet(inputStyle);
    m_enemyStrengthCombo->setFixedHeight(26);
    fl->addRow(mkLabel("Enemy Strength:"), m_enemyStrengthCombo);

    m_warheadTypeCombo = new QComboBox();
    m_warheadTypeCombo->addItems({"Anti-Ship Missile","Torpedo","Guided Bomb","Cruise Missile"});
    m_warheadTypeCombo->setStyleSheet(inputStyle);
    m_warheadTypeCombo->setFixedHeight(26);
    fl->addRow(mkLabel("Warhead Type:"), m_warheadTypeCombo);

    QPushButton* runBtn = new QPushButton("Run Analysis");
    runBtn->setFixedHeight(30);
    runBtn->setCursor(Qt::PointingHandCursor);
    runBtn->setStyleSheet(
        "QPushButton { background:#1a6b9a; color:white; border:1px solid #2a8abf;"
        "  border-radius:4px; font-size:12px; font-weight:bold; }"
        "QPushButton:hover { background:#2a7ab0; }");
    connect(runBtn, &QPushButton::clicked, this, &AnalysisEditor::runAnalysis);

    QPushButton* cmpBtn = new QPushButton("Compare Scenarios");
    cmpBtn->setFixedHeight(30);
    cmpBtn->setCursor(Qt::PointingHandCursor);
    cmpBtn->setStyleSheet(
        "QPushButton { background:#2a3a4a; color:white; border:1px solid #3a5a7a;"
        "  border-radius:4px; font-size:12px; }"
        "QPushButton:hover { background:#3a4a5a; }");
    connect(cmpBtn, &QPushButton::clicked, this, &AnalysisEditor::compareScenarios);

    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->addWidget(runBtn);
    btnRow->addWidget(cmpBtn);

    QVBoxLayout* vl = new QVBoxLayout(box);
    vl->addLayout(fl);
    vl->addLayout(btnRow);
    vl->addStretch();
    return box;
}

/* =========================================================================
   Chart helpers
   ========================================================================= */
static void applyDarkChart(QChart* chart)
{
    chart->setBackgroundBrush(QBrush(QColor("#1A3A4F")));
    chart->setBackgroundPen(QPen(QColor("#2a4a6a"), 1));
    chart->setTitleBrush(QBrush(Qt::white));
    chart->setTitleFont(QFont("Segoe UI", 10, QFont::Bold));
    chart->legend()->setLabelColor(Qt::white);
    chart->legend()->setBackgroundVisible(false);
    chart->setMargins(QMargins(6, 6, 6, 6));
}

static void styleAx(QValueAxis* ax, const QString& title = "")
{
    ax->setTitleText(title);
    ax->setTitleBrush(QBrush(QColor("#aaaaaa")));
    ax->setLabelsBrush(QBrush(QColor("#cccccc")));
    ax->setGridLinePen(QPen(QColor("#2a4060"), 1, Qt::DotLine));
    ax->setLinePen(QPen(QColor("#2a4a6a"), 1));
    ax->setTitleFont(QFont("Segoe UI", 8));
    ax->setLabelsFont(QFont("Segoe UI", 8));
}

QChartView* AnalysisEditor::buildEngagementTimelineChart(bool second)
{
    QChart* chart = new QChart();
    chart->setTitle("Timeline of Engagements");
    applyDarkChart(chart);

    QList<double> t  = {0,5,7,9,11,13,16,18,21,24,26,30,36,40};
    QList<double> dV = second
                           ? QList<double>{3,4,5,6,7,7,8,8,9,9,9,10,10,11}
                           : QList<double>{3,5,6,7,8,9,10,10,11,11,12,12,13,13};
    QList<double> eV = second
                           ? QList<double>{2,3,4,5,6,6,7,7,8,8,9,9,9,10}
                           : QList<double>{2,4,5,6,7,8,8,9,9,10,10,11,11,11};
    QList<double> dmgV = second
                             ? QList<double>{1,2,2,3,3,4,4,5,5,5,6,6,6,7}
                             : QList<double>{1,1,2,3,3,4,4,4,5,5,5,5,5,5};

    auto mkLine = [&](QList<double>& vals, const QString& name, QColor c) {
        QLineSeries* s = new QLineSeries();
        s->setName(name); s->setPen(QPen(c, 2));
        for (int i = 0; i < t.size(); ++i) s->append(t[i], vals[i]);
        return s;
    };

    chart->addSeries(mkLine(dV,   "Detection",  QColor("#4488ff")));
    chart->addSeries(mkLine(eV,   "Engagement", QColor("#44cc44")));
    chart->addSeries(mkLine(dmgV, "Damage",     QColor("#ff4444")));

    QValueAxis* axX = new QValueAxis(); axX->setRange(0,40); axX->setTickCount(9);
    styleAx(axX, "Time (minutes)");
    QValueAxis* axY = new QValueAxis(); axY->setRange(0,15); axY->setTickCount(6);
    styleAx(axY);

    chart->addAxis(axX, Qt::AlignBottom);
    chart->addAxis(axY, Qt::AlignLeft);
    for (auto* s : chart->series()) { s->attachAxis(axX); s->attachAxis(axY); }

    chart->legend()->setVisible(second);
    if (second) chart->legend()->setAlignment(Qt::AlignBottom);

    QChartView* v = new QChartView(chart);
    v->setRenderHint(QPainter::Antialiasing);
    v->setStyleSheet("background:transparent; border:1px solid #2a4a6a; border-radius:6px;");
    if (second) m_engagementChart2 = v; else m_engagementChart1 = v;
    return v;
}

QChartView* AnalysisEditor::buildSuccessProbabilityChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Probability of Success");
    applyDarkChart(chart);

    QList<double> t  = {0,5,10,15,20,25,30,35,40};
    QList<double> oV = {20,40,60,75,85,90,95,98,100};
    QList<double> bV = {10,25,40,55,65,75,82,88,94};
    QList<double> pV = {5,12,22,32,42,50,57,63,68};

    auto mk = [&](QList<double>& v, const QString& n, QColor c) {
        QLineSeries* s = new QLineSeries(); s->setName(n); s->setPen(QPen(c,2));
        for (int i=0;i<t.size();++i) s->append(t[i],v[i]); return s; };

    chart->addSeries(mk(oV,"Optimistic",  QColor("#4488ff")));
    chart->addSeries(mk(bV,"Base",        QColor("#44cc44")));
    chart->addSeries(mk(pV,"Pessimistic", QColor("#ff4444")));

    QValueAxis* axX = new QValueAxis(); axX->setRange(0,40); axX->setTickCount(5); styleAx(axX);
    QValueAxis* axY = new QValueAxis(); axY->setRange(0,100); axY->setTickCount(5);
    axY->setLabelFormat("%d%%"); styleAx(axY);

    chart->addAxis(axX,Qt::AlignBottom); chart->addAxis(axY,Qt::AlignLeft);
    for (auto* s:chart->series()){s->attachAxis(axX);s->attachAxis(axY);}
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    QChartView* v = new QChartView(chart);
    v->setRenderHint(QPainter::Antialiasing);
    v->setStyleSheet("background:transparent; border:1px solid #2a4a6a; border-radius:6px;");
    m_successChart = v;
    return v;
}

QChartView* AnalysisEditor::buildLossesVsEngagementsChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Losses vs Engagements");
    applyDarkChart(chart);

    QBarSet* fr = new QBarSet("Friendly Losses");
    fr->setColor(QColor("#44cc44")); fr->setBorderColor(Qt::transparent);
    *fr << 1 << 2 << 2;

    QBarSet* en = new QBarSet("Enemy Losses");
    en->setColor(QColor("#ff4444")); en->setBorderColor(Qt::transparent);
    *en << 1 << 3 << 4;

    QBarSeries* bs = new QBarSeries(); bs->append(fr); bs->append(en);
    chart->addSeries(bs);

    QBarCategoryAxis* axX = new QBarCategoryAxis();
    axX->append({"Low","Moderate","High"});
    axX->setLabelsBrush(QBrush(QColor("#cccccc")));
    axX->setGridLinePen(QPen(QColor("#2a4060"),1,Qt::DotLine));
    axX->setLinePen(QPen(QColor("#2a4a6a")));
    axX->setLabelsFont(QFont("Segoe UI",8));

    QValueAxis* axY = new QValueAxis(); axY->setRange(0,6); axY->setTickCount(4); styleAx(axY);

    chart->addAxis(axX,Qt::AlignBottom); chart->addAxis(axY,Qt::AlignLeft);
    bs->attachAxis(axX); bs->attachAxis(axY);

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignTop);
    chart->legend()->setLabelColor(Qt::white);

    QChartView* v = new QChartView(chart);
    v->setRenderHint(QPainter::Antialiasing);
    v->setStyleSheet("background:transparent; border:1px solid #2a4a6a; border-radius:6px;");
    m_lossesChart = v;
    return v;
}

/* =========================================================================
   Analysis computation
   ========================================================================= */
void AnalysisEditor::computeAnalysis()
{
    int    assets   = m_assetsEdit ? m_assetsEdit->text().toInt() : 10;
    double coverage = m_sensorCoverageEdit
                          ? m_sensorCoverageEdit->text().replace(" km","").toDouble() : 200.0;
    QString ecm   = m_ecmLevelCombo  ? m_ecmLevelCombo->currentText()      : "Moderate";
    QString enemy = m_enemyStrengthCombo ? m_enemyStrengthCombo->currentText() : "High";

    double ecmF   = (ecm   == "Low") ? 0.8 : (ecm   == "Moderate") ? 1.0 : 1.2;
    double enmyF  = (enemy == "Low") ? 0.5 : (enemy == "Moderate") ? 0.75
                                        : (enemy == "High") ? 1.0 : 1.3;

    m_currentResult.detectionEfficiency  = qMin(95.0, (coverage/300.0)*ecmF*85.0);
    m_currentResult.successProbability   = qMin(98.0, (assets/15.0)*ecmF/enmyF*90.0);
    m_currentResult.weaponEffectiveness  = qMin(95.0, ecmF/enmyF*80.0);
    m_currentResult.friendlyLosses       = qMax(0,(int)(enmyF*3 - assets/5));
    m_currentResult.enemyLosses          = qMax(0,(int)(ecmF*assets/2));
    refreshMetricLabels();
}

void AnalysisEditor::refreshMetricLabels()
{
    if (m_successProbLabel)
        m_successProbLabel->setText(
            QString::number(m_currentResult.successProbability,'f',0)+" %★");
    if (m_friendlyLossesLabel)
        m_friendlyLossesLabel->setText(
            QString::number(m_currentResult.friendlyLosses)+" 🚢");
    if (m_enemyLossesLabel)
        m_enemyLossesLabel->setText(
            QString::number(m_currentResult.enemyLosses)+" 🚢");
    if (m_detectionEffLabel)
        m_detectionEffLabel->setText(
            QString::number(m_currentResult.detectionEfficiency,'f',0)+" %💧");
    if (m_weaponEffLabel)
        m_weaponEffLabel->setText(
            QString::number(m_currentResult.weaponEffectiveness,'f',0)+" %🚀");
}

/* =========================================================================
   Slots
   ========================================================================= */
void AnalysisEditor::runAnalysis()      { computeAnalysis(); }

void AnalysisEditor::compareScenarios()
{
    QMessageBox::information(this,"Compare Scenarios",
                             "Scenario comparison will overlay results from multiple\n"
                             "saved scenario files on the timeline charts.");
}

void AnalysisEditor::exportResults()
{
    QString fp = QFileDialog::getSaveFileName(
        this,"Export Analysis",
        QDir::homePath()+"/TDF/Analysis",
        "JSON Files (*.json)");
    if (fp.isEmpty()) return;

    QJsonObject r;
    r["successProbability"]  = m_currentResult.successProbability;
    r["friendlyLosses"]      = m_currentResult.friendlyLosses;
    r["enemyLosses"]         = m_currentResult.enemyLosses;
    r["detectionEfficiency"] = m_currentResult.detectionEfficiency;
    r["weaponEffectiveness"] = m_currentResult.weaponEffectiveness;

    QFile f(fp);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(r).toJson(QJsonDocument::Indented));
        f.close();
        QMessageBox::information(this,"Export","Results exported successfully.");
    }
}

/* =========================================================================
   File I/O
   ========================================================================= */
void AnalysisEditor::loadFromJsonFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;

    QJsonObject obj = doc.object();
    m_currentResult.successProbability  = obj.value("successProbability").toDouble(80.0);
    m_currentResult.friendlyLosses      = obj.value("friendlyLosses").toInt(3);
    m_currentResult.enemyLosses         = obj.value("enemyLosses").toInt(5);
    m_currentResult.detectionEfficiency = obj.value("detectionEfficiency").toDouble(75.0);
    m_currentResult.weaponEffectiveness = obj.value("weaponEffectiveness").toDouble(60.0);

    lastSavedFilePath = filePath;
    refreshMetricLabels();
    clearUnsavedChanges();
}

void AnalysisEditor::clearUnsavedChanges()
{
    if (hasUnsavedChanges) {
        hasUnsavedChanges = false;
        emit unsavedChangesChanged(false);
    }
}

void AnalysisEditor::markUnsavedChanges()
{
    if (!hasUnsavedChanges) {
        hasUnsavedChanges = true;
        emit unsavedChangesChanged(true);
    }
}

