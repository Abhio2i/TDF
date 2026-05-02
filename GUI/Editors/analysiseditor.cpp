/* =============================================================================
 * FILE:         analysiseditor.cpp
 * MODULE:       Analysis Editor / Multi-Team Analytics Dashboard
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the AnalysisEditor class which provides a fully
 *               dynamic, multi‑team analytics dashboard. It loads scenario or
 *               analysis JSON files, computes metrics (success probability,
 *               losses, detection/weapon effectiveness), displays interactive
 *               charts (engagement timeline, success probability, losses vs
 *               engagements), and supports team selectors, series visibility,
 *               and export of results. The implementation includes parsing of
 *               hierarchy entities to auto‑generate team metrics and timelines.
 *
 * REQUIREMENTS: Implements REQ-ANALYSIS-010 through REQ-ANALYSIS-022
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-ANALYSIS-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#include "analysiseditor.h"
#include <QDir>
#include <QFileDialog>
#include <QJsonArray>
#include <QFormLayout>
#include <QFont>
#include <QPainter>
#include <QMargins>
#include <QtMath>

static const QString DARK_BG    = "#0F2636";
static const QString PANEL_BG   = "#1A3A4F";
static const QString BORDER_COL = "#2a4a6a";
static const QString ACCENT_COL = "#00BFFF";

/* ─── chart helpers ─── */
static void applyDarkChart(QChart* c)
{
    c->setBackgroundBrush(QBrush(QColor("#1A3A4F")));
    c->setBackgroundPen(QPen(QColor("#2a4a6a"),1));
    c->setTitleBrush(QBrush(Qt::white));
    c->setTitleFont(QFont("Segoe UI",10,QFont::Bold));
    c->legend()->setLabelColor(Qt::white);
    c->legend()->setBackgroundVisible(false);
    c->setMargins(QMargins(6,6,6,6));
}
static void styleAx(QValueAxis* ax, const QString& title="")
{
    ax->setTitleText(title);
    ax->setTitleBrush(QBrush(QColor("#aaaaaa")));
    ax->setLabelsBrush(QBrush(QColor("#cccccc")));
    ax->setGridLinePen(QPen(QColor("#2a4060"),1,Qt::DotLine));
    ax->setLinePen(QPen(QColor("#2a4a6a"),1));
    ax->setTitleFont(QFont("Segoe UI",8));
    ax->setLabelsFont(QFont("Segoe UI",8));
}

/* ─── colour palette (auto-assigned per team index) ─── */
QColor AnalysisEditor::paletteColor(int idx)
{
    static const QColor palette[] = {
        QColor("#2277dd"),  // 0 Blue
        QColor("#dd2222"),  // 1 Red
        QColor("#22aa44"),  // 2 Green
        QColor("#ddaa00"),  // 3 Yellow
        QColor("#aa22aa"),  // 4 Purple
        QColor("#00aaaa"),  // 5 Cyan
        QColor("#dd7700"),  // 6 Orange
        QColor("#aaaaaa"),  // 7 Grey
    };
    return palette[idx % 8];
}

QColor AnalysisEditor::colorForTeam(const QString& name, int idx)
{
    QString n = name.toLower();
    if (n.contains("blue"))   return QColor("#2277dd");
    if (n.contains("red"))    return QColor("#dd2222");
    if (n.contains("green"))  return QColor("#22aa44");
    if (n.contains("yellow")) return QColor("#ddaa00");
    if (n.contains("grey") || n.contains("gray")) return QColor("#888888");
    if (n.contains("alpha"))  return QColor("#aa22aa");
    if (n.contains("beta"))   return QColor("#00aaaa");
    if (n.contains("gamma"))  return QColor("#dd7700");
    return paletteColor(idx);
}

/* =========================================================================
   Constructor / Destructor
   ========================================================================= */
AnalysisEditor::AnalysisEditor(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Analysis Editor");
    resize(1300,780);
    setupUI();
}
AnalysisEditor::~AnalysisEditor() {}
/* =========================================================================
   setupUI
   ========================================================================= */
void AnalysisEditor::setupUI()
{
    QWidget* central = new QWidget(this);
    central->setStyleSheet(
        QString("QWidget{background:%1;color:white;font-family:'Segoe UI',sans-serif;}")
            .arg(DARK_BG));
    QVBoxLayout* root = new QVBoxLayout(central);
    root->setContentsMargins(0,0,0,0); root->setSpacing(0);
    m_stack = new QStackedWidget();
    m_analysisPage = buildAnalysisDashboard();
    m_stack->addWidget(m_analysisPage);
    m_reportsPage = new ReportsEditor();
    m_stack->addWidget(m_reportsPage);
    root->addWidget(m_stack,1);
    root->addWidget(buildBottomTabBar(),0);
    setCentralWidget(central);
    switchTab(0);
}

/* =========================================================================
   Bottom tab bar
   ========================================================================= */
QWidget* AnalysisEditor::buildBottomTabBar()
{
    QWidget* bar = new QWidget();
    bar->setFixedHeight(44);
    bar->setStyleSheet(
        QString("background:%1;border-top:2px solid %2;").arg(PANEL_BG,BORDER_COL));
    QHBoxLayout* hl = new QHBoxLayout(bar);
    hl->setContentsMargins(12,6,12,6); hl->setSpacing(8);
    auto makeTab=[&](const QString& label,int idx){
        QPushButton* b=new QPushButton(label);
        b->setFixedHeight(30); b->setMinimumWidth(130);
        b->setCursor(Qt::PointingHandCursor); b->setCheckable(true);
        connect(b,&QPushButton::clicked,this,[=](){ switchTab(idx); });
        return b;
    };
    m_tabAnalysis = makeTab("📊  Analysis",0);
    m_tabReports  = makeTab("📋  Reports", 1);
    QPushButton* openBtn = new QPushButton("📂  Open File");
    openBtn->setFixedHeight(30); openBtn->setMinimumWidth(120);
    openBtn->setCursor(Qt::PointingHandCursor);
    openBtn->setToolTip("Load a Scenario or Analysis JSON file");
    openBtn->setStyleSheet(
        "QPushButton{background:#145214;color:#aaffaa;"
        "  border:1px solid #2a7a2a;border-radius:4px;font-size:12px;font-weight:bold;}"
        "QPushButton:hover{background:#1e7a1e;color:white;}");
    connect(openBtn,&QPushButton::clicked,this,&AnalysisEditor::openScenarioFile);
    hl->addStretch();
    hl->addWidget(m_tabAnalysis); hl->addWidget(m_tabReports);
    hl->addSpacing(30); hl->addWidget(openBtn);
    hl->addStretch();
    return bar;
}
void AnalysisEditor::switchTab(int index)
{
    m_stack->setCurrentIndex(index);
    QString active="QPushButton{background:#0d6efd;color:white;border:none;"
                     "  border-radius:4px;font-size:12px;font-weight:bold;}";
    QString inactive="QPushButton{background:#1a3a5a;color:#aaa;border:1px solid #2a4a6a;"
                       "  border-radius:4px;font-size:12px;}"
                       "QPushButton:hover{background:#2a4a6a;color:white;}";
    if(m_tabAnalysis) m_tabAnalysis->setStyleSheet(index==0?active:inactive);
    if(m_tabReports)  m_tabReports->setStyleSheet(index==1?active:inactive);
}

/* =========================================================================
   openScenarioFile
   ========================================================================= */
void AnalysisEditor::openScenarioFile()
{
    QString path = QFileDialog::getOpenFileName(
        this,"Open Scenario / Analysis File",
        QDir::homePath(),"JSON Files (*.json);;All Files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this,"Error","Cannot open:\n"+path); return;
    }
    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(),&pe);
    file.close();
    if (pe.error!=QJsonParseError::NoError||!doc.isObject()) {
        QMessageBox::warning(this,"Parse Error","Invalid JSON:\n"+pe.errorString()); return;
    }
    QJsonObject root = doc.object();
    if (root.contains("hierarchy"))  parseScenarioJson(root);
    else if (root.contains("teams")) parseAnalysisJson(root);
    else                             parseLegacyJson(root);

    lastSavedFilePath = path;
    clearUnsavedChanges();
}
void AnalysisEditor::parseAnalysisJson(const QJsonObject& root)
{
    QJsonObject teams = root.value("teams").toObject();
    m_teamNames.clear();
    m_teamMetrics.clear();
    m_teamTimelines.clear();
    m_teamLosses.clear();
    m_teamColors.clear();
    int idx = 0;
    for (const QString& key : teams.keys()) {
        QJsonObject t = teams.value(key).toObject();
        TeamMetrics m;
        QJsonObject met = t.value("metrics").toObject();
        m.successProbability  = met.value("successProbability").toDouble(80);
        m.friendlyLosses = met.value("friendlyLosses").toDouble(0);
        m.enemyLosses    = met.value("enemyLosses").toDouble(0);
        m.detectionEfficiency = met.value("detectionEfficiency").toDouble(75);
        m.weaponEffectiveness = met.value("weaponEffectiveness").toDouble(60);
        TimelineData tl;
        QJsonObject et = t.value("engagementTimeline").toObject();
        QJsonValue detVal = et.value("detection");
        QJsonValue engVal = et.value("engagement");
        QJsonValue dmgVal = et.value("damage");
        if (detVal.isObject() || engVal.isObject() || dmgVal.isObject()) {
            parseTimelineField(detVal, tl.timePoints,           tl.detection);
            parseTimelineField(engVal, tl.engagementTimePoints, tl.engagement);
            parseTimelineField(dmgVal, tl.damageTimePoints,     tl.damage);
        } else {
            tl.timePoints            = toDoubleList(et.value("timePoints").toArray());
            tl.detection             = toDoubleList(detVal.toArray());
            tl.engagementTimePoints  = toDoubleList(et.value("engagementTimePoints").toArray());
            tl.engagement            = toDoubleList(engVal.toArray());
            tl.damageTimePoints      = toDoubleList(et.value("damageTimePoints").toArray());
            tl.damage                = toDoubleList(dmgVal.toArray());
        }

        LossesData ld;
        QJsonObject lv = t.value("lossesVsEngagement").toObject();
        ld.friendlyLosses = toDoubleList(lv.value("friendlyLosses").toArray());
        ld.enemyLosses    = toDoubleList(lv.value("enemyLosses").toArray());
        for (const auto& c : lv.value("categories").toArray())
            ld.categories << c.toString();

        m_teamNames  << key;
        m_teamMetrics[key]   = m;
        m_teamTimelines[key] = tl;
        m_teamLosses[key]    = ld;
        QString colorStr = t.value("color").toString();
        m_teamColors[key] = (!colorStr.isEmpty() && QColor::isValidColor(colorStr))
                                ? QColor(colorStr)
                                : colorForTeam(key, idx);
        idx++;
    }
    if (!m_teamNames.isEmpty())
        m_selectedTeam = m_teamNames.first();
    rebuildAllCharts();
    QMessageBox::information(this, "File Loaded",
                             QString("Analysis loaded!\nMission: %1\nTeams found: %2")
                                 .arg(root.value("missionName").toString("N/A"))
                                 .arg(m_teamNames.join(", ")));
}
/* =========================================================================
   parseScenarioJson  —  reads hierarchy entities, groups by Team value
   ========================================================================= */
void AnalysisEditor::parseScenarioJson(const QJsonObject& root)
{
    QJsonObject cats = root.value("hierarchy").toObject()
                           .value("profileCategories").toObject();
    /* count assets per team */
    QMap<QString,int> assetCount, airCount, shipCount;
    for (const QString& ck : cats.keys()) {
        QJsonObject entities = cats.value(ck).toObject().value("entities").toObject();
        for (const QString& ek : entities.keys()) {
            QJsonObject e = entities.value(ek).toObject();
            if (!e.value("active").toBool(true)) continue;
            QString team = e.value("Team").toObject().value("value").toString();
            QString cat  = e.value("Category").toObject().value("value").toString();
            if (team.isEmpty()) continue;
            assetCount[team]++;
            if (cat=="Aircraft"||cat=="Helicopter") airCount[team]++;
            if (cat=="Ship"||cat=="Submarine")      shipCount[team]++;
        }
    }
    m_teamNames.clear(); m_teamMetrics.clear();
    m_teamTimelines.clear(); m_teamLosses.clear(); m_teamColors.clear();
    int idx=0;
    QList<double> tPts={0,5,7,9,11,13,16,18,21,24,26,30,36,40};
    for (const QString& team : assetCount.keys()) {
        int assets = assetCount[team];
        int air    = airCount.value(team,0);
        double sc  = qMax(1,assets)/10.0;
        TeamMetrics m;
        m.successProbability  = qMin(98.0, 50+assets*3.5);
        m.friendlyLosses      = qMax(0, assets/8);
        m.enemyLosses         = qMax(0, assets/2);
        m.detectionEfficiency = qMin(95.0, 40+air*5.0);
        m.weaponEffectiveness = qMin(95.0, 35+assets*3.0);
        TimelineData tl;
        tl.timePoints = tPts;
        tl.detection  = scaleList({3,5,6,7,8,9,10,10,11,11,12,12,13,13}, sc);
        tl.engagement = scaleList({2,4,5,6,7,8, 8, 9, 9,10,10,11,11,11}, sc);
        tl.damage     = scaleList({1,1,2,3,3,4, 4, 4, 5, 5, 5, 5, 5, 5}, sc);
        LossesData ld;
        ld.categories << "Low" << "Moderate" << "High";
        ld.friendlyLosses = {m.friendlyLosses*0.3, m.friendlyLosses*0.7, (double)m.friendlyLosses};
        ld.enemyLosses    = {m.enemyLosses*0.3,    m.enemyLosses*0.7,    (double)m.enemyLosses};
        m_teamNames  << team;
        m_teamMetrics[team]   = m;
        m_teamTimelines[team] = tl;
        m_teamLosses[team]    = ld;
        /* scenario files don't have a color field → auto-assign by name/index */
        m_teamColors[team]    = colorForTeam(team, idx);
        idx++;
    }

    if (!m_teamNames.isEmpty())
        m_selectedTeam = m_teamNames.first();
    rebuildAllCharts();
    QStringList info;
    for (const QString& t : m_teamNames)
        info << QString("%1: %2 assets").arg(t).arg(assetCount[t]);
    QMessageBox::information(this,"File Loaded",
                             "Scenario loaded!\n\n" + info.join("\n"));
}

/* =========================================================================
   parseLegacyJson
   ========================================================================= */
void AnalysisEditor::parseLegacyJson(const QJsonObject& obj)
{
    m_currentResult.successProbability  = obj.value("successProbability").toDouble(80);
    m_currentResult.friendlyLosses      = obj.value("friendlyLosses").toInt(3);
    m_currentResult.enemyLosses         = obj.value("enemyLosses").toInt(5);
    m_currentResult.detectionEfficiency = obj.value("detectionEfficiency").toDouble(75);
    m_currentResult.weaponEffectiveness = obj.value("weaponEffectiveness").toDouble(60);
    refreshMetricLabels();
    QMessageBox::information(this,"File Loaded","Legacy analysis file loaded.");
}

/* =========================================================================
   rebuildAllCharts  —  called after any JSON load
   ========================================================================= */
void AnalysisEditor::rebuildAllCharts()
{
    rebuildMetricsPanel();
    rebuildTeamSelectorBar();
    rebuildCombinedSelectorBar();
    rebuildCombinedEngagementChart();
    rebuildRow2Chart();
    rebuildSuccessChart();
    rebuildLossesChart();

    /* ── Sync loaded data into ReportsEditor ── */
    if (!m_reportsPage || m_teamNames.isEmpty()) return;

    QJsonObject teamsObj;
    for (const QString& team : m_teamNames) {
        QJsonObject t;

        /* color */
        t["color"] = m_teamColors.value(team, QColor("#aaaaaa")).name();

        /* metrics */
        const TeamMetrics& tm = m_teamMetrics[team];
        QJsonObject met;
        met["successProbability"]  = tm.successProbability;
        met["detectionEfficiency"] = tm.detectionEfficiency;
        met["weaponEffectiveness"] = tm.weaponEffectiveness;
        met["friendlyLosses"]      = (double)tm.friendlyLosses;
        met["enemyLosses"]         = (double)tm.enemyLosses;
        t["metrics"] = met;

        /* engagementTimeline — store as {time: value} objects */
        const TimelineData& tl = m_teamTimelines[team];
        QJsonObject etObj;

        auto listToObj = [](const QList<double>& times, const QList<double>& vals) {
            QJsonObject o;
            int n = qMin(times.size(), vals.size());
            for (int i = 0; i < n; ++i)
                o[QString::number(times[i], 'f', 2)] = vals[i];
            return o;
        };

        etObj["detection"]  = listToObj(tl.timePoints, tl.detection);

        QList<double> engT = tl.engagementTimePoints.isEmpty()
                                 ? tl.timePoints : tl.engagementTimePoints;
        etObj["engagement"] = listToObj(engT, tl.engagement);

        QList<double> dmgT = tl.damageTimePoints.isEmpty()
                                 ? tl.timePoints : tl.damageTimePoints;
        etObj["damage"]     = listToObj(dmgT, tl.damage);

        t["engagementTimeline"] = etObj;

        /* lossesVsEngagement */
        const LossesData& ld = m_teamLosses[team];
        QJsonObject lvObj;
        QJsonArray cats, fl, el;
        for (const QString& c : ld.categories)   cats.append(c);
        for (double v : ld.friendlyLosses)        fl.append(v);
        for (double v : ld.enemyLosses)           el.append(v);
        lvObj["categories"]     = cats;
        lvObj["friendlyLosses"] = fl;
        lvObj["enemyLosses"]    = el;
        t["lossesVsEngagement"] = lvObj;
        teamsObj[team] = t;
    }

    QJsonObject root;
    root["teams"] = teamsObj;
    m_reportsPage->loadFromJson(root);
}

/* =========================================================================
   Dashboard layout
   ========================================================================= */
QWidget* AnalysisEditor::buildAnalysisDashboard()
{
    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea{background:transparent;border:none;}");
    QWidget* page = new QWidget();
    page->setStyleSheet("background:transparent;");
    QVBoxLayout* vl = new QVBoxLayout(page);
    vl->setContentsMargins(8,8,8,8); vl->setSpacing(8);
    QHBoxLayout* row1 = new QHBoxLayout(); row1->setSpacing(8);
    row1->addWidget(buildMissionMetricsPanel(),        2);
    row1->addWidget(buildCombinedTimelineChart(),      5);
    row1->addWidget(buildSuccessProbabilityChart(),    3);
    vl->addLayout(row1, 5);
    QHBoxLayout* row2 = new QHBoxLayout(); row2->setSpacing(8);
    row2->addWidget(buildProbabilityCalculatorPanel(), 2);
    row2->addWidget(buildSelectableTimelineChart(),    5);
    row2->addWidget(buildLossesVsEngagementsChart(),   3);
    vl->addLayout(row2, 5);
    scroll->setWidget(page);
    return scroll;
}

/* =========================================================================
   Mission Metrics panel  —  dynamic columns per team
   ========================================================================= */
QWidget* AnalysisEditor::buildMissionMetricsPanel()
{
    QGroupBox* box = new QGroupBox("Mission Metrics");
    box->setStyleSheet(
        QString("QGroupBox{background:%1;border:1px solid %2;border-radius:6px;"
                "color:white;font-size:13px;font-weight:bold;margin-top:8px;padding-top:10px;}"
                "QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 6px;color:%3;}")
            .arg(PANEL_BG,BORDER_COL,ACCENT_COL));

    m_metricsVL = new QVBoxLayout(box);
    m_metricsVL->setSpacing(2); m_metricsVL->setContentsMargins(6,12,6,4);
    m_metricsWidget = box;

    /* build initial default (2 teams placeholder) */
    rebuildMetricsPanel();
    return box;
}

/* =========================================================================
   rebuildMetricsPanel  —  recreate rows dynamically for all loaded teams
   ========================================================================= */
void AnalysisEditor::rebuildMetricsPanel()
{
    if (!m_metricsVL) return;
    /* clear old widgets */
    QLayoutItem* item;
    while ((item = m_metricsVL->takeAt(0))) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_metricLabels.clear();
    /* ── Use QTableWidget — naturally compact rows ── */
    /* no default placeholder — show empty state if no file loaded */
    if (m_teamNames.isEmpty()) {
        QLabel* empty = new QLabel("No data loaded.\nPlease open a JSON file.");
        empty->setStyleSheet("color:#556677;font-size:12px;");
        empty->setAlignment(Qt::AlignCenter);
        m_metricsVL->addWidget(empty, 1);
        return;
    }
    QStringList teams = m_teamNames;
    QStringList rowLabels;
    rowLabels << "★ Success" << "💧 Detection" << "🚀 Weapon Eff";
    QStringList fields;
    fields << "successProbability" << "detectionEfficiency" << "weaponEffectiveness";
    int rows = rowLabels.size();
    int cols = 1 + teams.size();   /* col0 = metric label, rest = teams */

    QTableWidget* table = new QTableWidget(rows, cols);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(false);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    /* compact row height */
    table->verticalHeader()->setDefaultSectionSize(38);
    table->verticalHeader()->setMinimumSectionSize(38);

    /* header labels */
    QStringList hdrLabels; hdrLabels << "Metric";
    for (const QString& t : teams) hdrLabels << t;
    table->setHorizontalHeaderLabels(hdrLabels);

    /* style */
    table->setStyleSheet(
        "QTableWidget{"
        "  background:transparent;border:none;"
        "  color:white;font-size:11px;}"
        "QTableWidget::item{padding:1px 4px;border:none;}"
        "QHeaderView::section{"
        "  background:#1a3a5a;border:none;border-bottom:1px solid #2a4a6a;"
        "  font-size:11px;font-weight:bold;padding:2px 4px;color:#aaa;}"
        "QScrollBar:horizontal{background:#0d2030;height:5px;}"
        "QScrollBar::handle:horizontal{background:#2a6a9a;border-radius:2px;}"
        "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0;}");

    /* set team name header colors */
    for (int c = 0; c < teams.size(); ++c) {
        QColor col = m_teamNames.isEmpty()
                         ? (c==0 ? QColor("#2277dd") : QColor("#dd2222"))
                         : m_teamColors.value(teams[c], QColor("#aaaaaa"));
        QTableWidgetItem* hItem = new QTableWidgetItem(teams[c]);
        hItem->setForeground(col);
        hItem->setTextAlignment(Qt::AlignCenter);
        table->setHorizontalHeaderItem(c+1, hItem);
    }

    /* fill rows */
    for (int r = 0; r < rows; ++r) {
        /* col 0 — metric label */
        QTableWidgetItem* lbl = new QTableWidgetItem(rowLabels[r]);
        lbl->setForeground(QColor("#cccccc"));
        lbl->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(r, 0, lbl);

        /* team value columns */
        for (int c = 0; c < teams.size(); ++c) {
            const QString& team = teams[c];
            QColor col = m_teamNames.isEmpty()
                             ? (c==0 ? QColor("#2277dd") : QColor("#dd2222"))
                             : m_teamColors.value(team, QColor("#aaaaaa"));
            QString val = "--";
            if (!m_teamNames.isEmpty()) {
                TeamMetrics& tm = m_teamMetrics[team];
                if      (fields[r]=="successProbability")  val=QString::number(tm.successProbability,'f',0)+"%";
                else if (fields[r]=="detectionEfficiency") val=QString::number(tm.detectionEfficiency,'f',0)+"%";
                else if (fields[r]=="weaponEffectiveness") val=QString::number(tm.weaponEffectiveness,'f',0)+"%";
            }
            QTableWidgetItem* cell = new QTableWidgetItem(val);
            cell->setForeground(col);
            cell->setTextAlignment(Qt::AlignCenter);
            cell->setFont(QFont("Segoe UI", 11, QFont::Bold));
            table->setItem(r, c+1, cell);

            /* store for refresh */
            if (!m_teamNames.isEmpty()) {
                /* use QLabel stored in metricLabels for compat — skip for table */
            }
        }
    }

    /* column widths */
    table->setColumnWidth(0, 90);
    for (int c = 1; c <= teams.size(); ++c)
        table->setColumnWidth(c, 75);

    /* expand to fill panel, scrollbar always at bottom of content */
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->horizontalHeader()->setStretchLastSection(false);

    m_metricsVL->addWidget(table, 1);
}


/* =========================================================================
   refreshAllMetricLabels
   ========================================================================= */

void AnalysisEditor::refreshAllMetricLabels()
{
    for (const QString& team : m_teamNames) {
        TeamMetrics& tm = m_teamMetrics[team];
        auto set=[&](const QString& field, const QString& val){
            QString key = team+"_"+field;
            if (m_metricLabels.contains(key))
                m_metricLabels[key]->setText(val);
        };
        set("successProbability",  QString::number(tm.successProbability,'f',0)+"%");
        set("detectionEfficiency", QString::number(tm.detectionEfficiency,'f',0)+"%");
        set("weaponEffectiveness", QString::number(tm.weaponEffectiveness,'f',0)+"%");
    }
}

/* =========================================================================
   Probability Calculator panel
   ========================================================================= */
QWidget* AnalysisEditor::buildProbabilityCalculatorPanel()
{
    QGroupBox* box = new QGroupBox("Probability Calculator");
    box->setStyleSheet(
        QString("QGroupBox{background:%1;border:1px solid %2;border-radius:6px;"
                "color:white;font-size:13px;font-weight:bold;margin-top:8px;padding-top:10px;}"
                "QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 6px;color:%3;}")
            .arg(PANEL_BG,BORDER_COL,ACCENT_COL));

    QString iS="QLineEdit,QComboBox{background:#0d2030;color:white;border:1px solid #2a4a6a;"
                 "border-radius:3px;padding:3px 6px;font-size:12px;}"
                 "QComboBox::drop-down{border:none;}"
                 "QComboBox QAbstractItemView{background:#0d2030;color:white;"
                 "selection-background-color:#1a5a8a;}";
    QString lS="QLabel{color:#cccccc;font-size:12px;font-weight:normal;}";

    QFormLayout* fl=new QFormLayout(); fl->setSpacing(8);
    fl->setContentsMargins(12,14,12,12); fl->setLabelAlignment(Qt::AlignLeft);
    auto mkL=[&](const QString& t){ QLabel* l=new QLabel(t);l->setStyleSheet(lS);return l; };

    m_assetsEdit=new QLineEdit("10"); m_assetsEdit->setStyleSheet(iS); m_assetsEdit->setFixedHeight(26);
    fl->addRow(mkL("Number of Assets:"),m_assetsEdit);

    m_sensorCoverageEdit=new QLineEdit("200 km"); m_sensorCoverageEdit->setStyleSheet(iS); m_sensorCoverageEdit->setFixedHeight(26);
    fl->addRow(mkL("Sensor Coverage:"),m_sensorCoverageEdit);

    m_ecmLevelCombo=new QComboBox(); m_ecmLevelCombo->addItems({"Low","Moderate","High"});
    m_ecmLevelCombo->setCurrentText("Moderate"); m_ecmLevelCombo->setStyleSheet(iS); m_ecmLevelCombo->setFixedHeight(26);
    fl->addRow(mkL("ECM Level:"),m_ecmLevelCombo);

    m_enemyStrengthCombo=new QComboBox(); m_enemyStrengthCombo->addItems({"Low","Moderate","High","Very High"});
    m_enemyStrengthCombo->setCurrentText("High"); m_enemyStrengthCombo->setStyleSheet(iS); m_enemyStrengthCombo->setFixedHeight(26);
    fl->addRow(mkL("Enemy Strength:"),m_enemyStrengthCombo);

    m_warheadTypeCombo=new QComboBox(); m_warheadTypeCombo->addItems({"Anti-Ship Missile","Torpedo","Guided Bomb","Cruise Missile"});
    m_warheadTypeCombo->setStyleSheet(iS); m_warheadTypeCombo->setFixedHeight(26);
    fl->addRow(mkL("Warhead Type:"),m_warheadTypeCombo);

    QPushButton* runBtn=new QPushButton("Run Analysis");
    runBtn->setFixedHeight(30); runBtn->setCursor(Qt::PointingHandCursor);
    runBtn->setStyleSheet("QPushButton{background:#1a6b9a;color:white;border:1px solid #2a8abf;"
                          "border-radius:4px;font-size:12px;font-weight:bold;}"
                          "QPushButton:hover{background:#2a7ab0;}");
    connect(runBtn,&QPushButton::clicked,this,&AnalysisEditor::runAnalysis);

    QPushButton* cmpBtn=new QPushButton("Compare Scenarios");
    cmpBtn->setFixedHeight(30); cmpBtn->setCursor(Qt::PointingHandCursor);
    cmpBtn->setStyleSheet("QPushButton{background:#2a3a4a;color:white;border:1px solid #3a5a7a;"
                          "border-radius:4px;font-size:12px;}"
                          "QPushButton:hover{background:#3a4a5a;}");
    connect(cmpBtn,&QPushButton::clicked,this,&AnalysisEditor::compareScenarios);

    QHBoxLayout* br=new QHBoxLayout(); br->addWidget(runBtn); br->addWidget(cmpBtn);
    QVBoxLayout* vl=new QVBoxLayout(box); vl->addLayout(fl); vl->addLayout(br); vl->addStretch();
    return box;
}

/* =========================================================================
   buildEngagementTimelineChart  —  row-1: all teams' Engagement lines
   ========================================================================= */

QChartView* AnalysisEditor::buildEngagementTimelineChart()
{
    QChart* chart=new QChart();
    chart->setTitle("Timeline of Engagements — All Teams");
    applyDarkChart(chart);
    /* no default data — empty chart until file loaded */
    chart->setTitle("Timeline of Engagements");
    QValueAxis* axX=new QValueAxis(); axX->setRange(0,40); axX->setTickCount(9);
    styleAx(axX,"Time (minutes)");
    QValueAxis* axY=new QValueAxis(); axY->setRange(0,15); axY->setTickCount(6); styleAx(axY);
    chart->addAxis(axX,Qt::AlignBottom); chart->addAxis(axY,Qt::AlignLeft);
    for(auto* s:chart->series()){s->attachAxis(axX);s->attachAxis(axY);}
    chart->legend()->setVisible(true); chart->legend()->setAlignment(Qt::AlignBottom);
    QChartView* v=new QChartView(chart); v->setRenderHint(QPainter::Antialiasing);
    v->setStyleSheet("background:transparent;border:2px solid #2a4a7a;border-radius:6px;");
    m_engagementChart1=v;
    return v;
}

/* =========================================================================
   buildCombinedTimelineChart  —  row-1 wrapper with All + per-team buttons
   ========================================================================= */
QWidget* AnalysisEditor::buildCombinedTimelineChart()
{
    QWidget* container = new QWidget();
    container->setStyleSheet("background:transparent;");
    QVBoxLayout* vl = new QVBoxLayout(container);
    vl->setContentsMargins(0,0,0,0); vl->setSpacing(4);
    /* selector bar */
    m_combinedSelectorBar = new QWidget();
    m_combinedSelectorBar->setStyleSheet("background:transparent;");
    QHBoxLayout* hl = new QHBoxLayout(m_combinedSelectorBar);
    hl->setContentsMargins(4,2,4,2); hl->setSpacing(6);
    /* All button */
    m_combinedAllBtn = new QPushButton("All Teams");
    m_combinedAllBtn->setFixedHeight(24);
    m_combinedAllBtn->setCursor(Qt::PointingHandCursor);
    connect(m_combinedAllBtn, &QPushButton::clicked, this, [=](){ switchCombinedFilter(""); });
    hl->addWidget(m_combinedAllBtn);
    hl->addStretch();
    vl->addWidget(m_combinedSelectorBar, 0);
    /* chart */
    QChartView* cv = buildEngagementTimelineChart();
    vl->addWidget(cv, 1);
    /* style All as active initially */
    switchCombinedFilter("");
    return container;
}

/* =========================================================================
   rebuildCombinedSelectorBar  —  recreate buttons for loaded teams
   ========================================================================= */
void AnalysisEditor::rebuildCombinedSelectorBar()
{
    if (!m_combinedSelectorBar) return;
    QLayout* old = m_combinedSelectorBar->layout();
    QLayoutItem* item;
    while ((item = old->takeAt(0))) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_combinedTeamButtons.clear();
    m_combinedAllBtn = nullptr;
    QHBoxLayout* hl = qobject_cast<QHBoxLayout*>(old);
    if (!hl) return;
    /* All button */
    m_combinedAllBtn = new QPushButton("All");
    m_combinedAllBtn->setFixedHeight(24);
    m_combinedAllBtn->setCursor(Qt::PointingHandCursor);
    connect(m_combinedAllBtn, &QPushButton::clicked, this, [=](){ switchCombinedFilter(""); });
    hl->addWidget(m_combinedAllBtn);
    /* one button per team */
    for (const QString& team : m_teamNames) {
        QColor col = m_teamColors.value(team, QColor("#aaaaaa"));
        QPushButton* btn = new QPushButton(team);
        btn->setFixedHeight(24);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            QString("QPushButton{background:%1;color:white;"
                    "  border:1px solid %2;border-radius:4px;"
                    "  font-size:11px;padding:2px 8px;}"
                    "QPushButton:hover{border:2px solid white;}")
                .arg(col.darker(160).name(), col.name()));
        connect(btn, &QPushButton::clicked, this, [=](){ switchCombinedFilter(team); });
        m_combinedTeamButtons[team] = btn;
        hl->addWidget(btn);
    }
    hl->addStretch();
    /* default: show all */
    switchCombinedFilter("");
}

/* =========================================================================
   switchCombinedFilter  —  empty = all teams, else filter to one team
   ========================================================================= */
void AnalysisEditor::switchCombinedFilter(const QString& teamName)
{
    m_combinedFilterTeam = teamName;

    /* style All button */
    QString allActive =
        "QPushButton{background:#0d6efd;color:white;border:none;"
        "  border-radius:4px;font-size:11px;font-weight:bold;padding:2px 10px;}";
    QString allInactive =
        "QPushButton{background:#1a2a3a;color:#777;border:1px solid #2a4a6a;"
        "  border-radius:4px;font-size:11px;padding:2px 10px;}"
        "QPushButton:hover{color:white;}";

    if (m_combinedAllBtn)
        m_combinedAllBtn->setStyleSheet(teamName.isEmpty() ? allActive : allInactive);

    /* style team buttons */
    for (const QString& t : m_teamNames) {
        if (!m_combinedTeamButtons.contains(t)) continue;
        QColor col = m_teamColors.value(t, QColor("#aaaaaa"));
        bool active = (t == teamName);
        m_combinedTeamButtons[t]->setStyleSheet(
            active
                ? QString("QPushButton{background:%1;color:white;"
                          "  border:2px solid white;border-radius:4px;"
                          "  font-size:11px;font-weight:bold;padding:2px 8px;}")
                      .arg(col.name())
                : QString("QPushButton{background:%1;color:white;"
                          "  border:1px solid %2;border-radius:4px;"
                          "  font-size:11px;padding:2px 8px;}"
                          "QPushButton:hover{border:2px solid white;}")
                      .arg(col.darker(160).name(), col.name())
            );
    }

    /* rebuild chart with filter */
    rebuildCombinedEngagementChart();
}


void AnalysisEditor::rebuildCombinedEngagementChart()
{
    if (!m_engagementChart1 || m_teamNames.isEmpty()) return;
    QChart* chart = m_engagementChart1->chart();
    chart->removeAllSeries();
    for (QAbstractAxis* ax : chart->axes()) { chart->removeAxis(ax); delete ax; }
    QStringList teamsToShow = m_combinedFilterTeam.isEmpty()
                                  ? m_teamNames
                                  : QStringList{m_combinedFilterTeam};
    double maxY = 5, maxX = 40;
    int idx = 0;
    for (const QString& team : m_teamNames) {
        if (!teamsToShow.contains(team)) { idx++; continue; }
        TimelineData& tl = m_teamTimelines[team];
        if (tl.engagement.isEmpty()) { idx++; continue; }
        QColor col = m_teamColors.value(team, paletteColor(idx));
        QLineSeries* s = new QLineSeries();
        s->setName(team);
        s->setPen(QPen(col, 2));
        QList<double>& engTimes = tl.engagementTimePoints.isEmpty()
                                      ? tl.timePoints
                                      : tl.engagementTimePoints;
        int n = qMin(engTimes.size(), tl.engagement.size());
        for (int i = 0; i < n; ++i) {
            s->append(engTimes.at(i), tl.engagement.at(i));
            maxY = qMax(maxY, tl.engagement.at(i));
            maxX = qMax(maxX, engTimes.at(i));
        }
        chart->addSeries(s);
        idx++;
    }

    maxY = static_cast<int>(maxY / 5.0 + 1) * 5 + 5;
    QValueAxis* axX = new QValueAxis();
    axX->setRange(0, maxX);
    axX->setTickCount(9);
    styleAx(axX, "Time (seconds)");

    QValueAxis* axY = new QValueAxis();
    axY->setRange(0, maxY);
    axY->setTickCount(6);
    styleAx(axY, "Engagements");

    chart->addAxis(axX, Qt::AlignBottom);
    chart->addAxis(axY, Qt::AlignLeft);
    for (auto* s : chart->series()) { s->attachAxis(axX); s->attachAxis(axY); }

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
}

QWidget* AnalysisEditor::buildSelectableTimelineChart()
{
    QWidget* container = new QWidget();
    container->setStyleSheet("background:transparent;");
    QVBoxLayout* vl = new QVBoxLayout(container);
    vl->setContentsMargins(0,0,0,0); vl->setSpacing(4);

    // ── Row 1: Team selector ──
    m_teamSelectorBar = new QWidget();
    m_teamSelectorBar->setStyleSheet("background:transparent;");
    QHBoxLayout* hl = new QHBoxLayout(m_teamSelectorBar);
    hl->setContentsMargins(4,2,4,2); hl->setSpacing(6);
    hl->addStretch();
    vl->addWidget(m_teamSelectorBar, 0);

    // ── Row 2: Series filter buttons ──
    QWidget* seriesBar = new QWidget();
    seriesBar->setStyleSheet("background:transparent;");
    QHBoxLayout* sl = new QHBoxLayout(seriesBar);
    sl->setContentsMargins(4,2,4,2); sl->setSpacing(6);

    // Default sab visible
    m_seriesVisible["Detection"]  = true;
    m_seriesVisible["Engagement"] = true;
    m_seriesVisible["Damage"]     = true;

    auto makeSeriesBtn = [&](const QString& name, const QString& color) {
        QPushButton* btn = new QPushButton(name);
        btn->setFixedHeight(22);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setCheckable(true);
        btn->setChecked(true);

        auto updateStyle = [btn, color, name]() {
            bool on = btn->isChecked();
            btn->setStyleSheet(
                on ? QString("QPushButton{"
                             "background:%1;color:white;"
                             "border:2px solid white;"
                             "border-radius:4px;"
                             "font-size:11px;font-weight:bold;"
                             "padding:1px 8px;}").arg(color)
                   : QString("QPushButton{"
                             "background:#1a2a3a;color:#555;"
                             "border:1px solid #2a4a6a;"
                             "border-radius:4px;"
                             "font-size:11px;"
                             "padding:1px 8px;}"
                             "QPushButton:hover{color:#aaa;}"));
        };
        updateStyle();
        connect(btn, &QPushButton::toggled, this, [=](bool checked) {
            m_seriesVisible[name] = checked;
            updateStyle();
            rebuildRow2Chart();
        });
        m_seriesButtons[name] = btn;
        return btn;
    };

    QLabel* showLbl = new QLabel("Show:");
    showLbl->setStyleSheet("color:#777;font-size:11px;");
    sl->addWidget(showLbl);
    sl->addWidget(makeSeriesBtn("Detection",  "#5599ff"));
    sl->addWidget(makeSeriesBtn("Engagement", "#ffaa33"));
    sl->addWidget(makeSeriesBtn("Damage",     "#ff4444"));
    sl->addStretch();
    vl->addWidget(seriesBar, 0);

    // ── Chart ──
    QChart* chart = new QChart();
    chart->setTitle("Timeline of Engagements");
    applyDarkChart(chart);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    QChartView* v = new QChartView(chart);
    v->setRenderHint(QPainter::Antialiasing);
    v->setStyleSheet("background:transparent;border:2px solid #2a4a7a;border-radius:6px;");
    m_engagementChart2 = v;
    vl->addWidget(v, 1);

    return container;
}
/* =========================================================================
   rebuildTeamSelectorBar  —  create one button per team dynamically
   ========================================================================= */
void AnalysisEditor::rebuildTeamSelectorBar()
{
    if (!m_teamSelectorBar) return;

    /* clear old buttons */
    QLayout* old=m_teamSelectorBar->layout();
    QLayoutItem* item;
    while((item=old->takeAt(0))){
        if(item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_teamButtons.clear();

    QHBoxLayout* hl=qobject_cast<QHBoxLayout*>(old);
    if(!hl) return;

    for(const QString& team:m_teamNames){
        QColor col=m_teamColors.value(team,QColor("#aaaaaa"));
        QPushButton* btn=new QPushButton(team);
        btn->setFixedHeight(24);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            QString("QPushButton{background:%1;color:white;"
                    "  border:1px solid %2;border-radius:4px;"
                    "  font-size:11px;font-weight:bold;padding:2px 10px;}"
                    "QPushButton:hover{filter:brightness(1.2);}")
                .arg(col.darker(150).name(), col.name()));
        connect(btn,&QPushButton::clicked,this,[=](){ switchSelectedTeam(team); });
        m_teamButtons[team]=btn;
        hl->addWidget(btn);
    }
    hl->addStretch();

    /* highlight first team */
    if(!m_selectedTeam.isEmpty())
        switchSelectedTeam(m_selectedTeam);
    else if(!m_teamNames.isEmpty())
        switchSelectedTeam(m_teamNames.first());
}

/* =========================================================================
   switchSelectedTeam  —  highlight button + redraw row-2 chart
   ========================================================================= */
void AnalysisEditor::switchSelectedTeam(const QString& teamName)
{
    m_selectedTeam=teamName;

    /* update button styles */
    for(const QString& t:m_teamNames){
        if(!m_teamButtons.contains(t)) continue;
        QColor col=m_teamColors.value(t,QColor("#aaaaaa"));
        bool active=(t==teamName);
        m_teamButtons[t]->setStyleSheet(
            active
                ? QString("QPushButton{background:%1;color:white;"
                          "  border:2px solid white;border-radius:4px;"
                          "  font-size:11px;font-weight:bold;padding:2px 10px;}")
                      .arg(col.name())
                : QString("QPushButton{background:%1;color:#888;"
                          "  border:1px solid %2;border-radius:4px;"
                          "  font-size:11px;padding:2px 10px;}"
                          "QPushButton:hover{color:white;}")
                      .arg(col.darker(200).name(), col.darker(120).name())
            );
    }
    rebuildRow2Chart();
}


void AnalysisEditor::rebuildRow2Chart()
{
    if (!m_engagementChart2) return;
    QChart* chart = m_engagementChart2->chart();
    chart->removeAllSeries();
    for (QAbstractAxis* ax : chart->axes()) { chart->removeAxis(ax); delete ax; }

    QString team = m_selectedTeam;
    if (team.isEmpty() && !m_teamNames.isEmpty()) team = m_teamNames.first();
    if (team.isEmpty()) return;

    QColor col = m_teamColors.value(team, QColor("#aaaaaa"));
    chart->setTitle(QString("Timeline — %1").arg(team));

    double maxY = 5, maxX = 40;

    // Series config — name, color, style, dash pattern
    struct SeriesCfg {
        QString       name;
        QColor        color;
        Qt::PenStyle  style;
        QVector<qreal> dash;
    };

    QList<SeriesCfg> cfgs = {
                             { "Detection",  QColor("#5599ff"), Qt::SolidLine, {}      },
                             { "Engagement", QColor("#ffaa33"), Qt::SolidLine, {8, 4}  },
                             { "Damage",     QColor("#ff4444"), Qt::SolidLine, {2, 4}  },
                             };

    if (m_teamTimelines.contains(team)) {
        TimelineData& tl = m_teamTimelines[team];

        // Detection
        if (m_seriesVisible.value("Detection", true) &&
            !tl.timePoints.isEmpty() && !tl.detection.isEmpty()) {

            QLineSeries* s = new QLineSeries();
            s->setName("Detection");
            QPen pen(cfgs[0].color, 2.5, Qt::SolidLine);
            s->setPen(pen);
            int n = qMin(tl.timePoints.size(), tl.detection.size());
            for (int i = 0; i < n; ++i) {
                s->append(tl.timePoints.at(i), tl.detection.at(i));
                maxY = qMax(maxY, tl.detection.at(i));
                maxX = qMax(maxX, tl.timePoints.at(i));
            }
            chart->addSeries(s);
        }

        // Engagement
        if (m_seriesVisible.value("Engagement", true) && !tl.engagement.isEmpty()) {
            QList<double>& engTimes = tl.engagementTimePoints.isEmpty()
                                          ? tl.timePoints
                                          : tl.engagementTimePoints;
            QLineSeries* s = new QLineSeries();
            s->setName("Engagement");
            QPen pen(cfgs[1].color, 2.5, Qt::SolidLine);

            s->setPen(pen);
            int n = qMin(engTimes.size(), tl.engagement.size());
            for (int i = 0; i < n; ++i) {
                s->append(engTimes.at(i), tl.engagement.at(i));
                maxY = qMax(maxY, tl.engagement.at(i));
                maxX = qMax(maxX, engTimes.at(i));
            }
            chart->addSeries(s);
        }

        // Damage
        if (m_seriesVisible.value("Damage", true) && !tl.damage.isEmpty()) {
            QList<double>& dmgTimes = tl.damageTimePoints.isEmpty()
                                          ? tl.timePoints
                                          : tl.damageTimePoints;
            QLineSeries* s = new QLineSeries();
            s->setName("Damage");
            QPen pen(cfgs[2].color, 2.5, Qt::SolidLine);

            s->setPen(pen);
            int n = qMin(dmgTimes.size(), tl.damage.size());
            for (int i = 0; i < n; ++i) {
                s->append(dmgTimes.at(i), tl.damage.at(i));
                maxY = qMax(maxY, tl.damage.at(i));
                maxX = qMax(maxX, dmgTimes.at(i));
            }
            chart->addSeries(s);
        }

    } else {
        // Dummy data
        QList<double> t   = {0,50,100,150,200,250,300,350,400,450,512};
        QList<double> dV  = {3,4,5,6,7,7,8,8,9,9,10};
        QList<double> eV  = {2,3,4,5,6,6,7,7,8,8,9};
        QList<double> dmV = {1,2,2,3,3,4,4,5,5,5,6};
        maxX = 512;

        struct DummySeries {
            QString        name;
            QList<double>* vals;
            QColor         color;
            Qt::PenStyle   style;
            QVector<qreal> dash;
        };
        QList<DummySeries> dummyList = {
                                        { "Detection",  &dV,  col.lighter(160), Qt::SolidLine, {}     },
                                        { "Engagement", &eV,  col,              Qt::DashLine,  {8, 4} },
                                        { "Damage",     &dmV, col.darker(140),  Qt::DotLine,   {2, 4} },
                                        };


        for (auto& d : dummyList) {
            if (!m_seriesVisible.value(d.name, true)) continue;
            QLineSeries* s = new QLineSeries();
            s->setName(d.name);
            QPen pen(d.color, 2.5, d.style);
            if (!d.dash.isEmpty()) pen.setDashPattern(d.dash);
            s->setPen(pen);
            for (int i = 0; i < t.size(); ++i) {
                s->append(t.at(i), d.vals->at(i));
                maxY = qMax(maxY, d.vals->at(i));
            }
            chart->addSeries(s);
        }
    }

    if (chart->series().isEmpty()) {
        maxY = 10; maxX = 40;
    }
    maxY = static_cast<int>(maxY / 5.0 + 1) * 5 + 5;

    QValueAxis* axX = new QValueAxis();
    axX->setRange(0, maxX);
    axX->setTickCount(9);
    styleAx(axX, "Time (seconds)");

    QValueAxis* axY = new QValueAxis();
    axY->setRange(0, maxY);
    axY->setTickCount(6);
    styleAx(axY, "Count");

    chart->addAxis(axX, Qt::AlignBottom);
    chart->addAxis(axY, Qt::AlignLeft);
    for (auto* s : chart->series()) { s->attachAxis(axX); s->attachAxis(axY); }

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
}

/* =========================================================================
   buildSuccessProbabilityChart  —  static default, rebuilt on load
   ========================================================================= */
QChartView* AnalysisEditor::buildSuccessProbabilityChart()
{
    QChart* chart=new QChart(); chart->setTitle("Probability of Success"); applyDarkChart(chart);
    /* empty until file loaded */
    chart->setTitle("Probability of Success");
    QValueAxis* axX=new QValueAxis(); axX->setRange(0,40); axX->setTickCount(5); styleAx(axX);
    QValueAxis* axY=new QValueAxis(); axY->setRange(0,100); axY->setTickCount(5);
    axY->setLabelFormat("%d%%"); styleAx(axY);
    chart->addAxis(axX,Qt::AlignBottom); chart->addAxis(axY,Qt::AlignLeft);
    for(auto* s:chart->series()){s->attachAxis(axX);s->attachAxis(axY);}
    chart->legend()->setVisible(true); chart->legend()->setAlignment(Qt::AlignBottom);
    QChartView* v=new QChartView(chart); v->setRenderHint(QPainter::Antialiasing);
    v->setStyleSheet("background:transparent;border:1px solid #2a4a6a;border-radius:6px;");
    m_successChart=v; return v;
}

void AnalysisEditor::rebuildSuccessChart()
{
    if (!m_successChart || m_teamNames.isEmpty()) return;
    QChart* chart = m_successChart->chart();
    chart->removeAllSeries();
    for (QAbstractAxis* ax : chart->axes()) { chart->removeAxis(ax); delete ax; }

    double maxX = 40;
    int idx = 0;

    for (const QString& team : m_teamNames) {
        TimelineData& tl = m_teamTimelines[team];
        QColor col = m_teamColors.value(team, paletteColor(idx));
        QLineSeries* s = new QLineSeries();
        s->setName(team);
        s->setPen(QPen(col, 2));

        if (!tl.timePoints.isEmpty() && !tl.detection.isEmpty()) {
            double maxDet = *std::max_element(tl.detection.begin(), tl.detection.end());
            if (maxDet <= 0) maxDet = 1;
            int n = qMin(tl.timePoints.size(), tl.detection.size());
            for (int i = 0; i < n; ++i) {
                double prob = qMin(100.0, (tl.detection.at(i) / maxDet) * 100.0);
                s->append(tl.timePoints.at(i), prob);
                maxX = qMax(maxX, tl.timePoints.at(i));
            }
        } else {
            double peak = m_teamMetrics[team].successProbability;
            QList<double> tPts = {0,50,100,150,200,250,300,350,400,450,512};
            for (double tp : tPts)
                s->append(tp, qMin(100.0, peak * (tp / 512.0) + 10.0));
            maxX = qMax(maxX, 512.0);
        }

        chart->addSeries(s);
        idx++;
    }

    QValueAxis* axX = new QValueAxis();
    axX->setRange(0, maxX);
    axX->setTickCount(9);
    styleAx(axX, "Time (seconds)");

    QValueAxis* axY = new QValueAxis();
    axY->setRange(0, 100);
    axY->setTickCount(6);
    axY->setLabelFormat("%d%%");
    styleAx(axY, "Success %");

    chart->addAxis(axX, Qt::AlignBottom);
    chart->addAxis(axY, Qt::AlignLeft);
    for (auto* s : chart->series()) { s->attachAxis(axX); s->attachAxis(axY); }

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
}
/* =========================================================================
   buildLossesVsEngagementsChart  —  static default
   ========================================================================= */
QChartView* AnalysisEditor::buildLossesVsEngagementsChart()
{
    QChart* chart=new QChart(); chart->setTitle("Losses vs Engagements"); applyDarkChart(chart);
    /* empty until file loaded */
    chart->setTitle("Losses vs Engagements");
    chart->legend()->setVisible(false);
    QChartView* v=new QChartView(chart); v->setRenderHint(QPainter::Antialiasing);
    v->setStyleSheet("background:transparent;border:1px solid #2a4a6a;border-radius:6px;");
    m_lossesChart=v; return v;
}

/* =========================================================================
   rebuildLossesChart  —  one bar set per team (total losses)
   ========================================================================= */
void AnalysisEditor::rebuildLossesChart()
{
    if(!m_lossesChart||m_teamNames.isEmpty()) return;
    QChart* chart=m_lossesChart->chart();
    chart->removeAllSeries();
    for(QAbstractAxis* ax:chart->axes()){chart->removeAxis(ax);delete ax;}

    QBarSeries* bs=new QBarSeries();
    double maxVal=0;
    QStringList cats;

    int idx=0;
    for(const QString& team:m_teamNames){
        LossesData& ld=m_teamLosses[team];
        QColor col=m_teamColors.value(team,paletteColor(idx));
        QBarSet* set=new QBarSet(team);
        set->setColor(col); set->setBorderColor(Qt::transparent);
        int n=qMin(ld.friendlyLosses.size(),ld.enemyLosses.size());
        if(cats.isEmpty()&&!ld.categories.isEmpty()) cats=ld.categories;
        for(int i=0;i<n;++i){
            double total=ld.friendlyLosses[i]+ld.enemyLosses[i];
            *set<<total; maxVal=qMax(maxVal,total);
        }
        bs->append(set); idx++;
    }
    chart->addSeries(bs);
    if(cats.isEmpty()){ cats.clear(); cats << "Low" << "Moderate" << "High"; }
    QBarCategoryAxis* axX=new QBarCategoryAxis();
    for(const QString& c:cats) axX->append(c);
    axX->setLabelsBrush(QBrush(QColor("#cccccc"))); axX->setLinePen(QPen(QColor("#2a4a6a"))); axX->setLabelsFont(QFont("Segoe UI",8));

    double yMax=static_cast<int>((maxVal+1)/2.0+1)*2+2;
    QValueAxis* axY=new QValueAxis(); axY->setRange(0,yMax); axY->setTickCount(5); styleAx(axY);
    chart->addAxis(axX,Qt::AlignBottom); chart->addAxis(axY,Qt::AlignLeft);
    bs->attachAxis(axX); bs->attachAxis(axY);
}

/* =========================================================================
   computeAnalysis
   ========================================================================= */
void AnalysisEditor::computeAnalysis()
{
    int    assets  =m_assetsEdit?m_assetsEdit->text().toInt():10;
    double coverage=m_sensorCoverageEdit?m_sensorCoverageEdit->text().replace(" km","").toDouble():200.0;
    QString ecm  =m_ecmLevelCombo?m_ecmLevelCombo->currentText():"Moderate";
    QString enemy=m_enemyStrengthCombo?m_enemyStrengthCombo->currentText():"High";
    double ecmF =(ecm=="Low")?0.8:(ecm=="Moderate")?1.0:1.2;
    double enmyF=(enemy=="Low")?0.5:(enemy=="Moderate")?0.75:(enemy=="High")?1.0:1.3;
    m_currentResult.detectionEfficiency=qMin(95.0,(coverage/300.0)*ecmF*85.0);
    m_currentResult.successProbability =qMin(98.0,(assets/15.0)*ecmF/enmyF*90.0);
    m_currentResult.weaponEffectiveness=qMin(95.0,ecmF/enmyF*80.0);
    m_currentResult.friendlyLosses     =qMax(0,(int)(enmyF*3-assets/5));
    m_currentResult.enemyLosses        =qMax(0,(int)(ecmF*assets/2));
    refreshMetricLabels();
}

void AnalysisEditor::refreshMetricLabels()
{
    if(m_successProbLabel)    m_successProbLabel->setText(QString::number(m_currentResult.successProbability,'f',0)+"% ★");
    if(m_friendlyLossesLabel) m_friendlyLossesLabel->setText(QString::number(m_currentResult.friendlyLosses)+" 🚢");
    if(m_enemyLossesLabel)    m_enemyLossesLabel->setText(QString::number(m_currentResult.enemyLosses)+" 🚢");
    if(m_detectionEffLabel)   m_detectionEffLabel->setText(QString::number(m_currentResult.detectionEfficiency,'f',0)+"% 💧");
    if(m_weaponEffLabel)      m_weaponEffLabel->setText(QString::number(m_currentResult.weaponEffectiveness,'f',0)+"% 🚀");
}

/* =========================================================================
   Utility
   ========================================================================= */
QList<double> AnalysisEditor::toDoubleList(const QJsonArray& arr)
{ QList<double> o; for(const auto& v:arr) o<<v.toDouble(); return o; }

QList<double> AnalysisEditor::scaleList(QList<double> src,double factor)
{ for(double& v:src) v=qMax(0.0,v*factor); return src; }

/* =========================================================================
   Slots
   ========================================================================= */
void AnalysisEditor::runAnalysis()    { computeAnalysis(); }

void AnalysisEditor::compareScenarios()
{
    QMessageBox::information(this,"Compare Scenarios",
                             "Scenario comparison will overlay results from multiple saved scenario files.");
}

void AnalysisEditor::exportResults()
{
    QString fp=QFileDialog::getSaveFileName(this,"Export Analysis",
                                              QDir::homePath()+"/TDF/Analysis","JSON Files (*.json)");
    if(fp.isEmpty()) return;
    QJsonObject r;
    r["successProbability"]=m_currentResult.successProbability;
    r["friendlyLosses"]=m_currentResult.friendlyLosses;
    r["enemyLosses"]=m_currentResult.enemyLosses;
    r["detectionEfficiency"]=m_currentResult.detectionEfficiency;
    r["weaponEffectiveness"]=m_currentResult.weaponEffectiveness;
    QFile f(fp);
    if(f.open(QIODevice::WriteOnly)){
        f.write(QJsonDocument(r).toJson(QJsonDocument::Indented));
        f.close();
        QMessageBox::information(this,"Export","Results exported successfully.");
    }
}

void AnalysisEditor::loadFromJsonFile(const QString& filePath)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly)) return;
    QJsonParseError err;
    QJsonDocument doc=QJsonDocument::fromJson(file.readAll(),&err);
    file.close();
    if(err.error!=QJsonParseError::NoError||!doc.isObject()) return;
    QJsonObject obj=doc.object();
    if(obj.contains("teams"))     parseAnalysisJson(obj);
    else if(obj.contains("hierarchy")) parseScenarioJson(obj);
    else                          parseLegacyJson(obj);
    lastSavedFilePath=filePath;
    clearUnsavedChanges();
}

void AnalysisEditor::clearUnsavedChanges()
{ if(hasUnsavedChanges){hasUnsavedChanges=false;emit unsavedChangesChanged(false);} }

void AnalysisEditor::markUnsavedChanges()
{ if(!hasUnsavedChanges){hasUnsavedChanges=true;emit unsavedChangesChanged(true);} }
void AnalysisEditor::loadFromHierarchyJson(const QJsonObject& root)
{
    if (root.isEmpty()) return;
    if (root.contains("teams"))
        parseAnalysisJson(root);
    else if (root.contains("hierarchy"))
        parseScenarioJson(root);
    else
        parseLegacyJson(root);
}
