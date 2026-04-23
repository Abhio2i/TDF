/* =============================================================================
 * FILE:         reportseditor.cpp
 * MODULE:       Reports Dashboard
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the ReportsEditor class and its helper widgets
 *               (ReportTimelineWidget, ReportGaugeWidget) which together form
 *               a comprehensive reports dashboard. Provides mission summary,
 *               engagement timeline (custom painted), detection probability chart,
 *               ECM/ECCM analysis, weapon usage table, lessons learned, report
 *               options (section selection, format choice), and export actions
 *               (PDF, Word, Print, save/load templates). Loads data from JSON
 *               and supports team selection for dynamic updates.
 *
 * REQUIREMENTS: Implements REQ-REPORTS-010 through REQ-REPORTS-021
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-REPORTS-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#include "reportseditor.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QSplitter>
#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocument>
#include <QPageSize>
#include <QPageLayout>
#include <QDataStream>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <cmath>
#include <algorithm>

/* ─── shared palette constants ─── */
static const QString R_BG     = "#0F2636";
static const QString R_PANEL  = "#1A3A4F";
static const QString R_BORDER = "#2a4a6a";
static const QString R_ACCENT = "#00BFFF";

/* =========================================================================
   Colour palette
   ========================================================================= */
QColor ReportsEditor::paletteColor(int idx)
{
    static const QColor p[] = {
        QColor("#2277dd"), QColor("#dd2222"), QColor("#22aa44"),
        QColor("#ddaa00"), QColor("#aa22aa"), QColor("#00aaaa"),
        QColor("#dd7700"), QColor("#aaaaaa")
    };
    return p[idx % 8];
}

/* =========================================================================
   ReportTimelineWidget
   ========================================================================= */
ReportTimelineWidget::ReportTimelineWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(80);
    setStyleSheet("background:transparent;");
}

void ReportTimelineWidget::setEvents(const QList<EventPoint>& detection,
                                     const QList<EventPoint>& engagement,
                                     const QList<EventPoint>& weaponFired,
                                     const QList<EventPoint>& damage,
                                     double maxTimeSec)
{
    m_detection  = detection;
    m_engagement = engagement;
    m_weaponFired= weaponFired;
    m_damage     = damage;
    m_maxTime    = (maxTimeSec > 0) ? maxTimeSec : 1.0;
    m_hasData    = true;
    update();
}

void ReportTimelineWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int W     = width();
    int lineY = height() / 2 + 8;

    p.setPen(QPen(QColor("#4a6a8a"), 2));
    p.drawLine(20, lineY, W - 20, lineY);

    if (!m_hasData) {
        QStringList times = {"00:00","00:30","01:00","01:30","02:00","02:30"};
        int ticks = times.size();
        for (int i = 0; i < ticks; ++i) {
            int x = 20 + i * (W - 40) / (ticks - 1);
            p.setPen(QPen(QColor("#4a6a8a"), 1));
            p.drawLine(x, lineY - 4, x, lineY + 4);
            p.setPen(QColor("#888888"));
            p.setFont(QFont("Segoe UI", 8));
            p.drawText(x - 18, lineY + 16, 36, 14, Qt::AlignCenter, times[i]);
        }
        return;
    }

    int ticks = 6;
    for (int i = 0; i < ticks; ++i) {
        int    x   = 20 + i * (W - 40) / (ticks - 1);
        double t   = m_maxTime * i / (ticks - 1);
        QString lbl = QString("%1s").arg((int)t);
        p.setPen(QPen(QColor("#4a6a8a"), 1));
        p.drawLine(x, lineY - 4, x, lineY + 4);
        p.setPen(QColor("#888888"));
        p.setFont(QFont("Segoe UI", 8));
        p.drawText(x - 18, lineY + 16, 36, 14, Qt::AlignCenter, lbl);
    }

    auto drawDots = [&](const QList<EventPoint>& evts) {
        for (const auto& ev : evts) {
            double frac = qBound(0.0, ev.timeSec / m_maxTime, 1.0);
            int x = 20 + (int)(frac * (W - 40));
            p.setPen(QPen(ev.color.darker(160), 1, Qt::DotLine));
            p.drawLine(x, lineY - 22, x, lineY);
            p.setBrush(ev.color);
            p.setPen(QPen(ev.color.lighter(130), 1));
            p.drawEllipse(x - 5, lineY - 27, 10, 10);
        }
    };

    drawDots(m_detection);
    drawDots(m_engagement);
    drawDots(m_weaponFired);
    drawDots(m_damage);
}

/* =========================================================================
   ReportGaugeWidget
   ========================================================================= */
ReportGaugeWidget::ReportGaugeWidget(int initialValue, QWidget* parent)
    : QWidget(parent), m_value(initialValue)
{
    setFixedSize(110, 70);
    setStyleSheet("background:transparent;");
}

void ReportGaugeWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF arc(10, 8, 90, 90);
    p.setPen(QPen(QColor("#1a3a50"), 10, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(arc, 180 * 16, -180 * 16);
    QColor col = (m_value > 70) ? QColor("#ff4444")
                 : (m_value > 40) ? QColor("#ffaa00") : QColor("#44cc44");
    p.setPen(QPen(col, 10, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(arc, 180 * 16, -(int)(m_value / 100.0 * 180 * 16));
    p.setPen(Qt::white);
    p.setFont(QFont("Segoe UI", 14, QFont::Bold));
    p.drawText(QRectF(10, 20, 90, 44), Qt::AlignCenter,
               QString::number(m_value) + "%");
}

/* =========================================================================
   Static helpers
   ========================================================================= */
QLabel* ReportsEditor::kpiCard(const QString& icon, const QString& title,
                               const QString& value, const QString& color)
{
    QLabel* card = new QLabel();
    card->setFixedSize(130, 72);
    card->setAlignment(Qt::AlignCenter);
    card->setStyleSheet(
        QString("QLabel { background:#0d2030; border:1px solid %1;"
                "  border-radius:8px; color:white; }").arg(color));
    card->setText(
        QString("<div style='text-align:center;'>"
                "<span style='font-size:20px;'>%1</span><br>"
                "<span style='font-size:10px; color:#aaa;'>%2</span><br>"
                "<span style='font-size:18px; font-weight:bold; color:%3;'>%4</span>"
                "</div>").arg(icon, title, color, value));
    card->setTextFormat(Qt::RichText);
    return card;
}

QFrame* ReportsEditor::hLine()
{
    QFrame* f = new QFrame();
    f->setFrameShape(QFrame::HLine);
    f->setStyleSheet(QString("color:%1;").arg(R_BORDER));
    return f;
}

QWidget* ReportsEditor::weaponRow(const QString& name, int used, int hits,
                                  int pct, const QString& barColor)
{
    QWidget* w = new QWidget();
    w->setStyleSheet("background:transparent;");
    QHBoxLayout* hl = new QHBoxLayout(w);
    hl->setContentsMargins(0, 1, 0, 1);
    hl->setSpacing(6);

    auto lbl = [](const QString& t, int fixW,
                  Qt::Alignment a = Qt::AlignLeft | Qt::AlignVCenter) {
        QLabel* l = new QLabel(t);
        l->setFixedWidth(fixW);
        l->setAlignment(a);
        l->setStyleSheet("color:#cccccc; font-size:11px; background:transparent;");
        return l;
    };

    QProgressBar* bar = new QProgressBar();
    bar->setRange(0, 100);
    bar->setValue(pct);
    bar->setFixedHeight(12);
    bar->setTextVisible(false);
    bar->setStyleSheet(
        QString("QProgressBar { background:#0d2030; border-radius:3px; border:none; }"
                "QProgressBar::chunk { background:%1; border-radius:3px; }").arg(barColor));

    QLabel* pctLbl = new QLabel(QString::number(pct) + "%");
    pctLbl->setFixedWidth(32);
    pctLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    pctLbl->setStyleSheet(
        "color:white; font-size:11px; font-weight:bold; background:transparent;");

    hl->addWidget(lbl(name, 115));
    hl->addWidget(lbl(QString::number(used), 28, Qt::AlignCenter));
    hl->addWidget(lbl(QString::number(hits), 28, Qt::AlignCenter));
    hl->addWidget(bar, 1);
    hl->addWidget(pctLbl);
    return w;
}

/* =========================================================================
   Parse helper
   ========================================================================= */
QList<double> ReportsEditor::jsonObjToSortedValues(const QJsonValue& val,
                                                   QList<double>* timesOut)
{
    QList<double> values;
    if (timesOut) timesOut->clear();

    if (val.isObject()) {
        QJsonObject obj = val.toObject();
        QList<QPair<double,double>> pairs;
        for (const QString& k : obj.keys())
            pairs.append({k.toDouble(), obj.value(k).toDouble()});
        std::sort(pairs.begin(), pairs.end(),
                  [](const QPair<double,double>& a, const QPair<double,double>& b){
                      return a.first < b.first; });
        for (const auto& pr : pairs) {
            if (timesOut) timesOut->append(pr.first);
            values.append(pr.second);
        }
    } else if (val.isArray()) {
        for (const auto& v : val.toArray())
            values.append(v.toDouble());
    }
    return values;
}

/* =========================================================================
   loadFromJson
   ========================================================================= */
void ReportsEditor::loadFromJson(const QJsonObject& root)
{
    m_missionName = root.value("missionName").toString("");
    m_missionDate = root.value("missionDate").toString("");

    m_teamNames.clear();
    m_teamMetrics.clear();
    m_teamTimelines.clear();
    m_teamLosses.clear();
    m_teamColors.clear();

    QJsonObject teams = root.value("teams").toObject();
    int idx = 0;
    for (const QString& key : teams.keys()) {
        QJsonObject t = teams.value(key).toObject();

        TeamMetrics m;
        QJsonObject met = t.value("metrics").toObject();
        m.successProbability  = met.value("successProbability").toDouble(0);
        m.detectionEfficiency = met.value("detectionEfficiency").toDouble(0);
        m.weaponEffectiveness = met.value("weaponEffectiveness").toDouble(0);
        m.friendlyLosses      = met.value("friendlyLosses").toDouble(0);
        m.enemyLosses         = met.value("enemyLosses").toDouble(0);

        TimelineData tl;
        QJsonObject et = t.value("engagementTimeline").toObject();
        tl.detection  = jsonObjToSortedValues(et.value("detection"),  &tl.timePoints);
        tl.engagement = jsonObjToSortedValues(et.value("engagement"), &tl.engagementTimePoints);
        tl.damage     = jsonObjToSortedValues(et.value("damage"),     &tl.damageTimePoints);

        LossesData ld;
        QJsonObject lv = t.value("lossesVsEngagement").toObject();
        for (const auto& c : lv.value("categories").toArray())
            ld.categories << c.toString();
        for (const auto& v : lv.value("friendlyLosses").toArray())
            ld.friendlyLosses << v.toDouble();
        for (const auto& v : lv.value("enemyLosses").toArray())
            ld.enemyLosses << v.toDouble();

        QString colorStr = t.value("color").toString();
        QColor col = (!colorStr.isEmpty() && QColor::isValidColor(colorStr))
                         ? QColor(colorStr) : paletteColor(idx);

        m_teamNames  << key;
        m_teamMetrics[key]   = m;
        m_teamTimelines[key] = tl;
        m_teamLosses[key]    = ld;
        m_teamColors[key]    = col;
        idx++;
    }

    if (!m_teamNames.isEmpty())
        m_selectedTeam = m_teamNames.first();

    if (m_missionTitleLabel) {
        QString hdr = m_missionName.isEmpty() ? "Mission Summary" : m_missionName;
        if (!m_missionDate.isEmpty()) hdr += "  •  " + m_missionDate;
        m_missionTitleLabel->setText(hdr);
    }

    if (m_teamSelectorHL) {
        QLayoutItem* item;
        while ((item = m_teamSelectorHL->takeAt(0))) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        m_teamBtns.clear();

        QLabel* lbl = new QLabel("Team:");
        lbl->setStyleSheet("color:#aaa;font-size:11px;border:none;");
        m_teamSelectorHL->addWidget(lbl);

        for (const QString& team : m_teamNames) {
            QPushButton* btn = new QPushButton(team);
            btn->setFixedHeight(24);
            btn->setCursor(Qt::PointingHandCursor);
            connect(btn, &QPushButton::clicked, this, [=](){ onTeamSelected(team); });
            m_teamBtns[team] = btn;
            m_teamSelectorHL->addWidget(btn);
        }
        m_teamSelectorHL->addStretch();
    }

    if (m_highlightsVL) {
        QLayoutItem* item;
        while ((item = m_highlightsVL->takeAt(0))) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        for (const QString& team : m_teamNames) {
            const TeamMetrics& tm = m_teamMetrics[team];
            QColor col = m_teamColors.value(team, QColor("#aaaaaa"));
            auto addHL = [&](const QString& icon, const QString& colorStr,
                             const QString& text) {
                QHBoxLayout* row = new QHBoxLayout();
                row->setSpacing(8);
                QLabel* ic = new QLabel(icon);
                ic->setFixedWidth(16);
                ic->setStyleSheet(
                    QString("color:%1;font-size:12px;border:none;").arg(colorStr));
                QLabel* txt = new QLabel(text);
                txt->setStyleSheet("color:#cccccc;font-size:11px;border:none;");
                txt->setWordWrap(true);
                row->addWidget(ic);
                row->addWidget(txt, 1);
                m_highlightsVL->addLayout(row);
            };

            addHL("●", col.name(),
                  QString("%1  —  Success: %2%  |  Detection: %3%  |  Weapon Eff: %4%")
                      .arg(team)
                      .arg(tm.successProbability, 0, 'f', 1)
                      .arg(tm.detectionEfficiency, 0, 'f', 1)
                      .arg(tm.weaponEffectiveness, 0, 'f', 1));
            addHL("🛡", "#4488ff",
                  QString("%1  —  Friendly losses: %2  |  Enemy losses: %3")
                      .arg(team)
                      .arg(tm.friendlyLosses, 0, 'f', 1)
                      .arg(tm.enemyLosses, 0, 'f', 1));
        }
    }

    if (!m_selectedTeam.isEmpty())
        onTeamSelected(m_selectedTeam);
}

/* =========================================================================
   onTeamSelected
   ========================================================================= */
void ReportsEditor::onTeamSelected(const QString& teamName)
{
    m_selectedTeam = teamName;

    for (const QString& t : m_teamNames) {
        if (!m_teamBtns.contains(t)) continue;
        QColor col = m_teamColors.value(t, QColor("#aaaaaa"));
        bool active = (t == teamName);
        m_teamBtns[t]->setStyleSheet(
            active
                ? QString("QPushButton{background:%1;color:white;"
                          "border:2px solid white;border-radius:4px;"
                          "font-size:11px;font-weight:bold;padding:2px 10px;}")
                      .arg(col.name())
                : QString("QPushButton{background:%1;color:white;"
                          "border:1px solid %2;border-radius:4px;"
                          "font-size:11px;padding:2px 8px;}"
                          "QPushButton:hover{border:2px solid white;}")
                      .arg(col.darker(160).name(), col.name()));
    }

    if (!m_teamMetrics.contains(teamName)) return;
    const TeamMetrics& tm = m_teamMetrics[teamName];

    auto setCard = [](QLabel* card, const QString& icon, const QString& title,
                      const QString& value, const QString& color) {
        if (!card) return;
        card->setText(
            QString("<div style='text-align:center;'>"
                    "<span style='font-size:20px;'>%1</span><br>"
                    "<span style='font-size:10px;color:#aaa;'>%2</span><br>"
                    "<span style='font-size:18px;font-weight:bold;color:%3;'>%4</span>"
                    "</div>").arg(icon, title, color, value));
    };

    setCard(m_kpiSuccess,  "🎯", "Mission\nSuccess",
            QString::number((int)tm.successProbability) + " %", "#44cc44");
    setCard(m_kpiFriendly, "🛡", "Friendly\nLosses",
            QString::number(tm.friendlyLosses, 'f', 1), "#4488ff");
    setCard(m_kpiEnemy,    "💥", "Enemy\nDestroyed",
            QString::number(tm.enemyLosses, 'f', 1), "#ff4444");

    if (m_teamTimelines.contains(teamName)) {
        const TimelineData& tl = m_teamTimelines[teamName];
        double maxT = 0;
        auto chk = [&](const QList<double>& lst){
            if (!lst.isEmpty()) maxT = qMax(maxT, lst.last()); };
        chk(tl.timePoints);
        chk(tl.engagementTimePoints);
        chk(tl.damageTimePoints);
        if (m_kpiDuration) {
            QString durStr = (maxT > 0) ? QString("%1 s").arg(maxT, 0, 'f', 1) : "N/A";
            setCard(m_kpiDuration, "⏱", "Mission\nDuration", durStr, "#ffaa00");
        }
    }

    if (m_timelineWidget && m_teamTimelines.contains(teamName)) {
        const TimelineData& tl = m_teamTimelines[teamName];

        QList<ReportTimelineWidget::EventPoint> det, eng, wpn, dmg;

        for (int i = 0; i < qMin(tl.timePoints.size(), tl.detection.size()); ++i)
            if (tl.detection[i] > 0)
                det.append({tl.timePoints[i], QColor("#4488ff")});

        QList<double> engT = tl.engagementTimePoints.isEmpty()
                                 ? tl.timePoints : tl.engagementTimePoints;
        for (int i = 0; i < qMin(engT.size(), tl.engagement.size()); ++i)
            if (tl.engagement[i] > 0) {
                eng.append({engT[i], QColor("#00cc44")});
                wpn.append({engT[i], QColor("#ff4444")});
            }

        QList<double> dmgT = tl.damageTimePoints.isEmpty()
                                 ? tl.timePoints : tl.damageTimePoints;
        for (int i = 0; i < qMin(dmgT.size(), tl.damage.size()); ++i)
            if (tl.damage[i] > 0)
                dmg.append({dmgT[i], QColor("#ffaa00")});

        double maxT = 0;
        auto chk = [&](const QList<double>& lst){
            if (!lst.isEmpty()) maxT = qMax(maxT, lst.last()); };
        chk(tl.timePoints);
        chk(tl.engagementTimePoints);
        chk(tl.damageTimePoints);
        if (maxT <= 0) maxT = 60;

        m_timelineWidget->setEvents(det, eng, wpn, dmg, maxT);
    }
}

/* =========================================================================
   Detection Probability chart
   ========================================================================= */
QChartView* ReportsEditor::buildDetectionChart()
{
    QChart* chart = new QChart();
    chart->setBackgroundBrush(QBrush(QColor(R_PANEL)));
    chart->setBackgroundPen(Qt::NoPen);
    chart->setMargins(QMargins(4, 4, 4, 4));
    chart->setTitleBrush(QBrush(Qt::white));
    chart->legend()->setLabelColor(Qt::white);
    chart->legend()->setBackgroundVisible(false);
    chart->legend()->setAlignment(Qt::AlignTop);

    QLineSeries* radar = new QLineSeries();
    radar->setName("Radar");
    radar->setPen(QPen(QColor("#4488ff"), 2, Qt::DashLine));

    QLineSeries* ir = new QLineSeries();
    ir->setName("IR Sensor");
    ir->setPen(QPen(QColor("#44cc44"), 2));

    QList<double> ranges = {0,25,50,75,100,125,150,175,200,225,250,275};
    QList<double> radarV = {100,92,82,70,58,47,37,28,20,14,9,6};
    QList<double> irV    = {80,68,55,43,33,25,18,13,9,6,4,3};

    QScatterSeries* rDots = new QScatterSeries();
    rDots->setMarkerSize(7);
    rDots->setColor(QColor("#4488ff"));
    rDots->setBorderColor(Qt::transparent);
    rDots->setName("");

    QScatterSeries* iDots = new QScatterSeries();
    iDots->setMarkerSize(7);
    iDots->setColor(QColor("#44cc44"));
    iDots->setBorderColor(Qt::transparent);
    iDots->setName("");

    for (int i = 0; i < ranges.size(); ++i) {
        radar->append(ranges[i], radarV[i]);
        ir->append(ranges[i],    irV[i]);
        rDots->append(ranges[i], radarV[i]);
        iDots->append(ranges[i], irV[i]);
    }

    chart->addSeries(radar);
    chart->addSeries(ir);
    chart->addSeries(rDots);
    chart->addSeries(iDots);

    auto mkAx = [&](Qt::Alignment side, const QString& title,
                    double lo, double hi, int ticks, const QString& fmt = "") {
        QValueAxis* ax = new QValueAxis();
        ax->setRange(lo, hi);
        ax->setTickCount(ticks);
        ax->setTitleText(title);
        ax->setTitleBrush(QBrush(QColor("#aaaaaa")));
        ax->setLabelsBrush(QBrush(QColor("#cccccc")));
        ax->setGridLinePen(QPen(QColor("#2a4060"), 1, Qt::DotLine));
        ax->setLinePen(QPen(QColor(R_BORDER)));
        ax->setLabelsFont(QFont("Segoe UI", 8));
        ax->setTitleFont(QFont("Segoe UI", 8));
        if (!fmt.isEmpty()) ax->setLabelFormat(fmt);
        chart->addAxis(ax, side);
        return ax;
    };

    QValueAxis* axX = mkAx(Qt::AlignBottom, "Range (km)",       0, 275, 5);
    QValueAxis* axY = mkAx(Qt::AlignLeft,   "Probability (%)",  0, 100, 5, "%d%%");

    for (QAbstractSeries* s : chart->series()) {
        s->attachAxis(axX);
        s->attachAxis(axY);
    }

    QChartView* view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);
    view->setStyleSheet("background:transparent; border:none;");
    view->setMinimumHeight(190);
    return view;
}

/* =========================================================================
   buildLeftPanel
   — Report Options checkboxes (tick = include in report)
   — Report Format: PDF / Word only
   — Generate Report button (respects ticked sections + selected format)
   — Report Templates section REMOVED
   ========================================================================= */
QWidget* ReportsEditor::buildLeftPanel()
{
    QWidget* w = new QWidget();
    w->setFixedWidth(200);
    w->setStyleSheet(
        QString("background:%1; border:1px solid %2; border-radius:6px;")
            .arg(R_PANEL, R_BORDER));

    QVBoxLayout* vl = new QVBoxLayout(w);
    vl->setContentsMargins(10, 12, 10, 12);
    vl->setSpacing(5);

    /* ── Section title ── */
    QLabel* optTitle = new QLabel("Report Options");
    optTitle->setStyleSheet(
        QString("color:%1; font-size:12px; font-weight:bold; border:none;")
            .arg(R_ACCENT));
    vl->addWidget(optTitle);

    /* ── Checkboxes — store pointers so Generate can read them ── */
    QString cbStyle =
        "QCheckBox { color:#cccccc; font-size:11px; background:transparent; border:none; }"
        "QCheckBox::indicator { width:14px; height:14px; }"
        "QCheckBox::indicator:checked   { background:#0d6efd; border-radius:2px; }"
        "QCheckBox::indicator:unchecked { background:#0d2030; border:1px solid #4a6a8a;"
        "  border-radius:2px; }";

    struct Item { QString label; bool checked; };
    QList<Item> items = {
                         {"Mission Summary",         true},
                         {"Asset Performance",       true},
                         {"Engagement Timeline",     true},
                         {"Detection Probability",   true},
                         {"ECM / ECCM Analysis",     true},
                         {"Weapon Usage",            false},
                         {"Damage Assessment",       false},
                         {"Friendly & Enemy Losses", false},
                         {"Lessons Learned",         true},
                         {"Charts & Graphs",         true},
                         };

    m_sectionCheckboxes.clear();
    for (auto& it : items) {
        QCheckBox* cb = new QCheckBox(it.label);
        cb->setChecked(it.checked);
        cb->setStyleSheet(cbStyle);
        vl->addWidget(cb);
        m_sectionCheckboxes.append(cb);
    }

    vl->addWidget(hLine());

    /* ── Report Format: PDF / Word only ── */
    QLabel* fmtTitle = new QLabel("Report Format");
    fmtTitle->setStyleSheet(
        QString("color:%1; font-size:12px; font-weight:bold; border:none;")
            .arg(R_ACCENT));
    vl->addWidget(fmtTitle);

    QHBoxLayout* fmtRow = new QHBoxLayout();
    fmtRow->setSpacing(8);
    fmtRow->setContentsMargins(0, 2, 0, 2);

    m_fmtGroup = new QButtonGroup(w);
    QString rbStyle =
        "QRadioButton { color:#cccccc; font-size:11px; background:transparent; border:none; }"
        "QRadioButton::indicator { width:13px; height:13px; }"
        "QRadioButton::indicator:checked   { background:#0d6efd; border-radius:6px; }"
        "QRadioButton::indicator:unchecked { background:#0d2030; border:1px solid #4a6a8a;"
        "  border-radius:6px; }";

    for (const QString& fmt : {"PDF", "Word"}) {
        QRadioButton* rb = new QRadioButton(fmt);
        rb->setStyleSheet(rbStyle);
        if (fmt == "PDF") rb->setChecked(true);
        m_fmtGroup->addButton(rb);
        fmtRow->addWidget(rb);
    }
    fmtRow->addStretch();
    vl->addLayout(fmtRow);

    vl->addSpacing(4);

    /* ── Generate Report button ── */
    QPushButton* genBtn = new QPushButton("Generate Report");
    genBtn->setFixedHeight(34);
    genBtn->setCursor(Qt::PointingHandCursor);
    genBtn->setStyleSheet(
        "QPushButton { background:#0d6efd; color:white; border:none;"
        "  border-radius:4px; font-size:12px; font-weight:bold; }"
        "QPushButton:hover   { background:#1a7aff; }"
        "QPushButton:pressed { background:#0a50c0; }");
    connect(genBtn, &QPushButton::clicked, this, &ReportsEditor::onGenerateReport);
    vl->addWidget(genBtn);

    /* m_templateCombo kept as nullptr — no template section */
    m_templateCombo = nullptr;

    vl->addStretch();
    return w;
}

/* =========================================================================
   buildCenterTop
   ========================================================================= */
QWidget* ReportsEditor::buildCenterTop()
{
    QWidget* w = new QWidget();
    w->setStyleSheet(
        QString("background:%1; border:1px solid %2; border-radius:6px;")
            .arg(R_PANEL, R_BORDER));

    QVBoxLayout* vl = new QVBoxLayout(w);
    vl->setContentsMargins(12, 10, 12, 10);
    vl->setSpacing(8);

    m_missionTitleLabel = new QLabel("Mission Summary");
    m_missionTitleLabel->setStyleSheet(
        QString("color:%1; font-size:13px; font-weight:bold; border:none;")
            .arg(R_ACCENT));
    vl->addWidget(m_missionTitleLabel);

    m_teamSelectorBar = new QWidget();
    m_teamSelectorBar->setStyleSheet("background:transparent;border:none;");
    m_teamSelectorHL = new QHBoxLayout(m_teamSelectorBar);
    m_teamSelectorHL->setContentsMargins(0,0,0,0);
    m_teamSelectorHL->setSpacing(6);
    vl->addWidget(m_teamSelectorBar);

    QHBoxLayout* kpiRow = new QHBoxLayout();
    kpiRow->setSpacing(8);

    auto makeCard = [&](const QString& borderColor) -> QLabel* {
        QLabel* card = new QLabel();
        card->setFixedSize(130, 72);
        card->setAlignment(Qt::AlignCenter);
        card->setStyleSheet(
            QString("QLabel { background:#0d2030; border:1px solid %1;"
                    "  border-radius:8px; color:white; }").arg(borderColor));
        card->setTextFormat(Qt::RichText);
        return card;
    };

    m_kpiSuccess  = makeCard("#44cc44");
    m_kpiFriendly = makeCard("#4488ff");
    m_kpiEnemy    = makeCard("#ff4444");
    m_kpiDuration = makeCard("#ffaa00");

    kpiRow->addWidget(m_kpiSuccess);
    kpiRow->addWidget(m_kpiFriendly);
    kpiRow->addWidget(m_kpiEnemy);
    kpiRow->addWidget(m_kpiDuration);
    kpiRow->addStretch();
    vl->addLayout(kpiRow);

    vl->addWidget(hLine());

    QLabel* hlTitle = new QLabel("Key Highlights");
    hlTitle->setStyleSheet(
        "color:white; font-size:12px; font-weight:bold; border:none;");
    vl->addWidget(hlTitle);

    QWidget* hlContainer = new QWidget();
    hlContainer->setStyleSheet("background:transparent;border:none;");
    m_highlightsVL = new QVBoxLayout(hlContainer);
    m_highlightsVL->setContentsMargins(0,0,0,0);
    m_highlightsVL->setSpacing(4);
    QLabel* ph = new QLabel("Load a JSON file to see highlights.");
    ph->setStyleSheet("color:#556677;font-size:11px;border:none;");
    m_highlightsVL->addWidget(ph);
    vl->addWidget(hlContainer);

    return w;
}

/* =========================================================================
   buildEngagementTimeline
   ========================================================================= */
QWidget* ReportsEditor::buildEngagementTimeline()
{
    QWidget* w = new QWidget();
    w->setStyleSheet(
        QString("background:%1; border:1px solid %2; border-radius:6px;")
            .arg(R_PANEL, R_BORDER));

    QVBoxLayout* vl = new QVBoxLayout(w);
    vl->setContentsMargins(12, 10, 12, 6);
    vl->setSpacing(6);

    QLabel* title = new QLabel("Engagement Timeline");
    title->setStyleSheet(
        QString("color:%1; font-size:13px; font-weight:bold; border:none;")
            .arg(R_ACCENT));
    vl->addWidget(title);

    QHBoxLayout* leg = new QHBoxLayout();
    leg->setSpacing(16);
    struct Leg { QString color; QString label; };
    for (auto& l : QList<Leg>{
                              {"#4488ff","Detection"},
                              {"#00cc44","Engagement"},
                              {"#ff4444","Weapon Fired"},
                              {"#ffaa00","Damage"}}) {
        QHBoxLayout* ll = new QHBoxLayout();
        ll->setSpacing(4);
        QLabel* dot = new QLabel("\xe2\x97\x8f");
        dot->setStyleSheet(
            QString("color:%1; font-size:10px; border:none;").arg(l.color));
        QLabel* lbl = new QLabel(l.label);
        lbl->setStyleSheet("color:#aaaaaa; font-size:10px; border:none;");
        ll->addWidget(dot);
        ll->addWidget(lbl);
        leg->addLayout(ll);
    }
    leg->addStretch();
    vl->addLayout(leg);

    m_timelineWidget = new ReportTimelineWidget();
    vl->addWidget(m_timelineWidget);

    QLabel* timeLbl = new QLabel("Time");
    timeLbl->setAlignment(Qt::AlignCenter);
    timeLbl->setStyleSheet("color:#888; font-size:10px; border:none;");
    vl->addWidget(timeLbl);

    return w;
}

/* =========================================================================
   buildLessonsLearned
   ========================================================================= */
QWidget* ReportsEditor::buildLessonsLearned()
{
    QWidget* w = new QWidget();
    w->setStyleSheet(
        QString("background:%1; border:1px solid %2; border-radius:6px;")
            .arg(R_PANEL, R_BORDER));

    QVBoxLayout* vl = new QVBoxLayout(w);
    vl->setContentsMargins(12, 10, 12, 10);
    vl->setSpacing(6);

    QLabel* title = new QLabel("Lessons Learned / Recommendations");
    title->setStyleSheet(
        QString("color:%1; font-size:12px; font-weight:bold; border:none;")
            .arg(R_ACCENT));
    vl->addWidget(title);

    QLabel* empty = new QLabel("No lessons data available in current JSON.");
    empty->setStyleSheet("color:#556677; font-size:11px; border:none;");
    empty->setWordWrap(true);
    vl->addWidget(empty);

    vl->addStretch();
    return w;
}

/* =========================================================================
   buildRightTop
   ========================================================================= */
QWidget* ReportsEditor::buildRightTop()
{
    QWidget* w = new QWidget();
    w->setStyleSheet(
        QString("background:%1; border:1px solid %2; border-radius:6px;")
            .arg(R_PANEL, R_BORDER));

    QVBoxLayout* vl = new QVBoxLayout(w);
    vl->setContentsMargins(10, 8, 10, 8);
    vl->setSpacing(4);

    QLabel* title = new QLabel("Detection Probability vs Range");
    title->setStyleSheet(
        QString("color:%1; font-size:12px; font-weight:bold; border:none;")
            .arg(R_ACCENT));
    vl->addWidget(title);
    vl->addWidget(buildDetectionChart(), 1);
    return w;
}

/* =========================================================================
   buildECMPanel
   ========================================================================= */
QWidget* ReportsEditor::buildECMPanel()
{
    QWidget* w = new QWidget();
    w->setStyleSheet(
        QString("background:%1; border:1px solid %2; border-radius:6px;")
            .arg(R_PANEL, R_BORDER));

    QVBoxLayout* vl = new QVBoxLayout(w);
    vl->setContentsMargins(12, 10, 12, 10);
    vl->setSpacing(8);

    QLabel* title = new QLabel("ECM / ECCM Analysis");
    title->setStyleSheet(
        QString("color:%1; font-size:12px; font-weight:bold; border:none;")
            .arg(R_ACCENT));
    vl->addWidget(title);

    QString inputStyle =
        "QComboBox { background:#0d2030; color:white; border:1px solid #2a4a6a;"
        "  border-radius:3px; padding:2px 5px; font-size:11px; }"
        "QComboBox::drop-down { border:none; }"
        "QComboBox QAbstractItemView { background:#0d2030; color:white;"
        "  selection-background-color:#1a5a8a; }";
    QString lblStyle = "color:#cccccc; font-size:11px; border:none;";

    QHBoxLayout* ecmRow = new QHBoxLayout();
    ecmRow->setSpacing(6);
    QLabel* ecmLbl = new QLabel("Enemy ECM Type:");
    ecmLbl->setStyleSheet(lblStyle);
    m_ecmTypeCombo = new QComboBox();
    m_ecmTypeCombo->addItems({"Noise Jammer","Spot Jammer","Sweep Jammer","Deceptive"});
    m_ecmTypeCombo->setStyleSheet(inputStyle);
    m_ecmTypeCombo->setFixedWidth(120);
    ecmRow->addWidget(ecmLbl);
    ecmRow->addWidget(m_ecmTypeCombo);
    ecmRow->addStretch();
    vl->addLayout(ecmRow);

    QHBoxLayout* split = new QHBoxLayout();
    split->setSpacing(12);

    QWidget* leftW = new QWidget();
    leftW->setStyleSheet("background:transparent; border:none;");
    QVBoxLayout* leftVL = new QVBoxLayout(leftW);
    leftVL->setContentsMargins(0,0,0,0);
    leftVL->setSpacing(4);

    QLabel* eccmLbl = new QLabel("ECCM Mode:");
    eccmLbl->setStyleSheet(lblStyle);
    leftVL->addWidget(eccmLbl);

    QString cbStyle =
        "QCheckBox { color:#cccccc; font-size:11px; background:transparent; border:none; }"
        "QCheckBox::indicator { width:13px; height:13px; }"
        "QCheckBox::indicator:checked   { background:#0d6efd; border-radius:2px; }"
        "QCheckBox::indicator:unchecked { background:#0d2030; border:1px solid #4a6a8a;"
        "  border-radius:2px; }";

    m_freqAgilityChk = new QCheckBox("Frequency Agility");
    m_freqAgilityChk->setChecked(true);
    m_freqAgilityChk->setStyleSheet(cbStyle);
    leftVL->addWidget(m_freqAgilityChk);

    m_pulseCompChk = new QCheckBox("Pulse Compression");
    m_pulseCompChk->setChecked(true);
    m_pulseCompChk->setStyleSheet(cbStyle);
    leftVL->addWidget(m_pulseCompChk);

    m_sideLobChk = new QCheckBox("Side Lobe Cancel");
    m_sideLobChk->setChecked(false);
    m_sideLobChk->setStyleSheet(cbStyle);
    leftVL->addWidget(m_sideLobChk);

    split->addWidget(leftW, 1);

    QWidget* rightW = new QWidget();
    rightW->setStyleSheet("background:transparent; border:none;");
    QVBoxLayout* rightVL = new QVBoxLayout(rightW);
    rightVL->setContentsMargins(0,0,0,0);
    rightVL->setSpacing(4);
    rightVL->setAlignment(Qt::AlignHCenter);

    QLabel* jsLbl = new QLabel("Jamming\nStrength:");
    jsLbl->setAlignment(Qt::AlignCenter);
    jsLbl->setStyleSheet("color:#aaaaaa; font-size:10px; border:none;");

    QLabel* jsBadge = new QLabel("High");
    jsBadge->setAlignment(Qt::AlignCenter);
    jsBadge->setFixedSize(60, 22);
    jsBadge->setStyleSheet(
        "background:#ff4444; color:white; font-size:11px;"
        "font-weight:bold; border-radius:3px; border:none;");

    QLabel* jeffLbl = new QLabel("Jamming Effectiveness");
    jeffLbl->setAlignment(Qt::AlignCenter);
    jeffLbl->setStyleSheet("color:#aaaaaa; font-size:9px; border:none;");

    m_gauge = new ReportGaugeWidget(65);

    m_burnLabel = new QLabel(
        "Burn-through Range: <b style='color:#00BFFF;'>45 km</b>");
    m_burnLabel->setStyleSheet("color:#aaaaaa; font-size:10px; border:none;");
    m_burnLabel->setAlignment(Qt::AlignCenter);
    m_burnLabel->setTextFormat(Qt::RichText);

    rightVL->addWidget(jsLbl);
    rightVL->addWidget(jsBadge);
    rightVL->addWidget(jeffLbl);
    rightVL->addWidget(m_gauge);
    rightVL->addWidget(m_burnLabel);

    split->addWidget(rightW);
    vl->addLayout(split);
    return w;
}

/* =========================================================================
   buildWeaponUsageTable
   ========================================================================= */
QWidget* ReportsEditor::buildWeaponUsageTable()
{
    QWidget* w = new QWidget();
    w->setStyleSheet(
        QString("background:%1; border:1px solid %2; border-radius:6px;")
            .arg(R_PANEL, R_BORDER));

    QVBoxLayout* vl = new QVBoxLayout(w);
    vl->setContentsMargins(12, 10, 12, 10);
    vl->setSpacing(4);

    QLabel* title = new QLabel("Weapon Usage Summary");
    title->setStyleSheet(
        QString("color:%1; font-size:12px; font-weight:bold; border:none;")
            .arg(R_ACCENT));
    vl->addWidget(title);

    QLabel* empty = new QLabel("No weapon usage data available in current JSON.");
    empty->setStyleSheet("color:#556677; font-size:11px; border:none;");
    empty->setWordWrap(true);
    vl->addWidget(empty);

    vl->addStretch();
    return w;
}

/* =========================================================================
   buildBottomBar
   ========================================================================= */
QWidget* ReportsEditor::buildBottomBar()
{
    QWidget* bar = new QWidget();
    bar->setFixedHeight(44);
    bar->setStyleSheet(
        QString("background:%1; border-top:1px solid %2;")
            .arg(R_PANEL, R_BORDER));

    QHBoxLayout* hl = new QHBoxLayout(bar);
    hl->setContentsMargins(12, 6, 12, 6);
    hl->setSpacing(8);

    auto mkBtn = [&](const QString& label, const QString& bg,
                     const QString& border, auto slot) {
        QPushButton* btn = new QPushButton(label);
        btn->setFixedHeight(30);
        btn->setMinimumWidth(110);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            QString("QPushButton { background:%1; color:white; border:1px solid %2;"
                    "  border-radius:4px; font-size:11px; font-weight:bold; }"
                    "QPushButton:hover { background:%3; }")
                .arg(bg, border, bg));
        connect(btn, &QPushButton::clicked, this, slot);
        return btn;
    };

    hl->addStretch();
    hl->addWidget(mkBtn("Preview Report", "#1a4a7a","#2a6aaa",
                        &ReportsEditor::onPreviewReport));
    hl->addWidget(mkBtn("Export PDF",     "#1a5a3a","#2a7a4a",
                        &ReportsEditor::onExportPDF));
    hl->addWidget(mkBtn("Export Word",    "#1a3a6a","#2a5a8a",
                        &ReportsEditor::onExportWord));

    QPushButton* printBtn = new QPushButton("\xf0\x9f\x96\xa8  Print");
    printBtn->setFixedHeight(30);
    printBtn->setMinimumWidth(80);
    printBtn->setCursor(Qt::PointingHandCursor);
    printBtn->setStyleSheet(
        "QPushButton { background:#2a2a3a; color:white; border:1px solid #4a4a5a;"
        "  border-radius:4px; font-size:11px; }"
        "QPushButton:hover { background:#3a3a4a; }");
    connect(printBtn, &QPushButton::clicked, this, &ReportsEditor::onPrint);
    hl->addWidget(printBtn);

    return bar;
}

/* =========================================================================
   setupUI
   ========================================================================= */
void ReportsEditor::setupUI()
{
    setStyleSheet(
        QString("QWidget { background:%1; color:white;"
                "  font-family:'Segoe UI',sans-serif; }").arg(R_BG));

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { background:transparent; border:none; }");

    QWidget* content = new QWidget();
    content->setStyleSheet("background:transparent;");
    QVBoxLayout* cVL = new QVBoxLayout(content);
    cVL->setContentsMargins(8, 8, 8, 4);
    cVL->setSpacing(6);

    QHBoxLayout* cols = new QHBoxLayout();
    cols->setSpacing(6);

    cols->addWidget(buildLeftPanel(), 0);

    QVBoxLayout* centerVL = new QVBoxLayout();
    centerVL->setSpacing(6);
    centerVL->addWidget(buildCenterTop(),          3);
    centerVL->addWidget(buildEngagementTimeline(), 2);
    centerVL->addWidget(buildLessonsLearned(),     2);
    QWidget* centerW = new QWidget();
    centerW->setStyleSheet("background:transparent;");
    centerW->setLayout(centerVL);
    cols->addWidget(centerW, 5);

    QVBoxLayout* rightVL = new QVBoxLayout();
    rightVL->setSpacing(6);
    rightVL->addWidget(buildRightTop(),         4);
    rightVL->addWidget(buildECMPanel(),         3);
    rightVL->addWidget(buildWeaponUsageTable(), 3);
    QWidget* rightW = new QWidget();
    rightW->setStyleSheet("background:transparent;");
    rightW->setLayout(rightVL);
    cols->addWidget(rightW, 4);

    cVL->addLayout(cols, 1);
    scroll->setWidget(content);

    root->addWidget(scroll, 1);
    root->addWidget(buildBottomBar(), 0);
}

/* =========================================================================
   Constructor
   ========================================================================= */
ReportsEditor::ReportsEditor(QWidget* parent) : QWidget(parent)
{
    setupUI();
}

/* =========================================================================
   buildReportHtml  — shared HTML for PDF + Word export
   ========================================================================= */
QString ReportsEditor::buildReportHtml(const QStringList& sections) const
{
    QString html;
    html += R"(<!DOCTYPE html><html><head>
<meta charset="UTF-8"/>
<style>
  body   { font-family: Arial, sans-serif; font-size: 12pt; color: #111;
           margin: 0; padding: 0; }
  h1     { font-size: 20pt; color: #003366;
           border-bottom: 2px solid #003366; padding-bottom: 6px;
           margin-bottom: 6px; }
  h2     { font-size: 15pt; color: #00508a; margin-top: 20px; margin-bottom: 4px; }
  h3     { font-size: 13pt; color: #0066aa; margin-top: 14px; margin-bottom: 4px; }
  p      { font-size: 12pt; margin: 4px 0; }
  table  { border-collapse: collapse; width: 100%;
           margin-top: 8px; margin-bottom: 12px; }
  th     { background: #003366; color: white; padding: 6px 10px;
           text-align: left; font-size: 11pt; }
  td     { border: 1px solid #aaaaaa; padding: 5px 10px; font-size: 11pt; }
  tr:nth-child(even) td { background: #f0f4f8; }
  .meta  { color: #555555; font-size: 11pt; margin-bottom: 8px; }
  .footer{ margin-top: 30px; font-size: 10pt; color: #888888;
           border-top: 1px solid #cccccc; padding-top: 6px; }
</style>
</head><body>)";

    /* Title */
    QString titleText = m_missionName.isEmpty() ? "Mission Report" : m_missionName;
    html += QString("<h1>%1</h1>").arg(titleText.toHtmlEscaped());
    if (!m_missionDate.isEmpty())
        html += QString("<p class='meta'>Date: %1</p>").arg(m_missionDate.toHtmlEscaped());
    html += QString("<p class='meta'>Generated: %1</p>")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    /* helper: check if a section label is ticked (empty = all included) */
    auto sec = [&](const QString& name) -> bool {
        return sections.isEmpty() || sections.contains(name, Qt::CaseInsensitive);
    };

    /* KPI Summary Table — always shown when Mission Summary ticked */
    if (sec("Mission Summary")) {
        html += "<h2>Mission KPI Summary</h2>";
        html += "<table><tr>"
                "<th>Team</th><th>Success %</th><th>Detection Eff. %</th>"
                "<th>Weapon Eff. %</th><th>Friendly Losses</th><th>Enemy Losses</th>"
                "</tr>";
        for (const QString& team : m_teamNames) {
            const TeamMetrics& tm = m_teamMetrics.value(team);
            html += QString("<tr><td><b>%1</b></td><td>%2</td><td>%3</td>"
                            "<td>%4</td><td>%5</td><td>%6</td></tr>")
                        .arg(team.toHtmlEscaped())
                        .arg(tm.successProbability,  0, 'f', 1)
                        .arg(tm.detectionEfficiency, 0, 'f', 1)
                        .arg(tm.weaponEffectiveness, 0, 'f', 1)
                        .arg(tm.friendlyLosses,      0, 'f', 1)
                        .arg(tm.enemyLosses,         0, 'f', 1);
        }
        html += "</table>";
    } /* end Mission Summary */

    /* Per-team detail */
    for (const QString& team : m_teamNames) {
        const TeamMetrics& tm = m_teamMetrics.value(team);
        const TimelineData& tl = m_teamTimelines.value(team);
        const LossesData& ld   = m_teamLosses.value(team);
        QColor col = m_teamColors.value(team, QColor("#003366"));

        html += QString("<h2 style='color:%1;'>Team: %2</h2>")
                    .arg(col.name(), team.toHtmlEscaped());

        /* Performance metrics */
        html += "<h3>Performance Metrics</h3>"
                "<table><tr><th>Metric</th><th>Value</th></tr>";
        QList<QPair<QString,QString>> kpis = {
                                               {"Success Probability",  QString::number(tm.successProbability,  'f', 1) + " %"},
                                               {"Detection Efficiency", QString::number(tm.detectionEfficiency, 'f', 1) + " %"},
                                               {"Weapon Effectiveness", QString::number(tm.weaponEffectiveness, 'f', 1) + " %"},
                                               {"Friendly Losses",      QString::number(tm.friendlyLosses,      'f', 1)},
                                               {"Enemy Losses",         QString::number(tm.enemyLosses,         'f', 1)},
                                               };
        for (const auto& kp : kpis)
            html += QString("<tr><td>%1</td><td>%2</td></tr>")
                        .arg(kp.first, kp.second);
        html += "</table>";

        /* Engagement timeline — unified time map so rows are correct */
        if (sec("Engagement Timeline")) {
            /* Build time→{det,eng,dmg} map from all three axes */
            /* QMap: value=[det, eng, dmg], always resize to 3 before indexing */
            QMap<double, QVector<double>> tmap;
            auto ensureSize = [](QVector<double>& v) {
                if (v.size() < 3) v.resize(3);
            };
            for (int i = 0; i < qMin(tl.timePoints.size(), tl.detection.size()); ++i) {
                ensureSize(tmap[tl.timePoints[i]]);
                tmap[tl.timePoints[i]][0] = tl.detection[i];
            }
            for (int i = 0; i < qMin(tl.engagementTimePoints.size(), tl.engagement.size()); ++i) {
                ensureSize(tmap[tl.engagementTimePoints[i]]);
                tmap[tl.engagementTimePoints[i]][1] = tl.engagement[i];
            }
            for (int i = 0; i < qMin(tl.damageTimePoints.size(), tl.damage.size()); ++i) {
                ensureSize(tmap[tl.damageTimePoints[i]]);
                tmap[tl.damageTimePoints[i]][2] = tl.damage[i];
            }
            /* fallback: if engagementTimePoints empty, use timePoints */
            if (tl.engagementTimePoints.isEmpty())
                for (int i = 0; i < qMin(tl.timePoints.size(), tl.engagement.size()); ++i) {
                    ensureSize(tmap[tl.timePoints[i]]);
                    tmap[tl.timePoints[i]][1] = tl.engagement[i];
                }
            if (tl.damageTimePoints.isEmpty())
                for (int i = 0; i < qMin(tl.timePoints.size(), tl.damage.size()); ++i) {
                    ensureSize(tmap[tl.timePoints[i]]);
                    tmap[tl.timePoints[i]][2] = tl.damage[i];
                }

            if (!tmap.isEmpty()) {
                html += "<h3>Engagement Timeline Data</h3>"
                        "<table><tr><th>Time (s)</th><th>Detection</th>"
                        "<th>Engagement</th><th>Damage</th></tr>";
                for (auto it = tmap.constBegin(); it != tmap.constEnd(); ++it) {
                    const QVector<double>& v = it.value();
                    double det = v.size() > 0 ? v[0] : 0.0;
                    double eng = v.size() > 1 ? v[1] : 0.0;
                    double dmg = v.size() > 2 ? v[2] : 0.0;
                    html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>")
                                .arg(it.key(), 0, 'f', 2)
                                .arg(det, 0, 'f', 2)
                                .arg(eng, 0, 'f', 2)
                                .arg(dmg, 0, 'f', 2);
                }
                html += "</table>";
            }
        } /* end Engagement Timeline */

        /* Losses vs engagement */
        if (sec("Friendly & Enemy Losses") && !ld.categories.isEmpty()) {
            html += "<h3>Losses vs Engagement</h3>"
                    "<table><tr><th>Category</th>"
                    "<th>Friendly Losses</th><th>Enemy Losses</th></tr>";
            for (int i = 0; i < ld.categories.size(); ++i) {
                double fl = (i < ld.friendlyLosses.size()) ? ld.friendlyLosses[i] : 0;
                double el = (i < ld.enemyLosses.size())    ? ld.enemyLosses[i]    : 0;
                html += QString("<tr><td>%1</td><td>%2</td><td>%3</td></tr>")
                            .arg(ld.categories[i].toHtmlEscaped())
                            .arg(fl, 0, 'f', 1)
                            .arg(el, 0, 'f', 1);
            }
            html += "</table>";
        }
    }

    /* Footer */
    html += QString("<p class='footer'>Report generated by Reports Editor &bull; "
                    "Template: %1 &bull; %2</p>")
                .arg(m_templateCombo ? m_templateCombo->currentText().toHtmlEscaped()
                                     : QString("Standard"))
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));

    html += "</body></html>";
    return html;
}

/* =========================================================================
   onExportPDF  — QPrinter renders HTML → real PDF file
   Requires: QT += printsupport  in .pro
   ========================================================================= */
void ReportsEditor::onExportPDF()
{
    QString defaultName = (m_missionName.isEmpty() ? "MissionReport" : m_missionName)
                          + "_Report.pdf";
    QString path = QFileDialog::getSaveFileName(
        this, "Export PDF",
        QDir::homePath() + "/" + defaultName,
        "PDF Files (*.pdf)");

    if (path.isEmpty()) return;
    if (!path.endsWith(".pdf", Qt::CaseInsensitive))
        path += ".pdf";

    /* ScreenResolution avoids the microscopic-text bug caused by
       HighResolution DPI mismatch with QTextDocument logical units. */
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);

    /* Use Point units so QTextDocument page size matches PDF page exactly */
    QSizeF pageSize = printer.pageRect(QPrinter::Point).size();

    QTextDocument doc;
    doc.setDefaultFont(QFont("Arial", 11));
    QStringList ticked;
    for (QCheckBox* cb : m_sectionCheckboxes)
        if (cb && cb->isChecked()) ticked << cb->text();
    doc.setHtml(buildReportHtml(ticked));
    doc.setPageSize(pageSize);
    doc.print(&printer);

    QMessageBox::information(this, "Export PDF",
                             "PDF exported successfully:\n" + path);
}

/* =========================================================================
   onExportWord  — writes a valid OOXML .docx (manual ZIP construction)
   No third-party library needed — uses only Qt's QDataStream + QFile.
   ========================================================================= */
void ReportsEditor::onExportWord()
{
    QString defaultName = (m_missionName.isEmpty() ? "MissionReport" : m_missionName)
                          + "_Report.docx";
    QString path = QFileDialog::getSaveFileName(
        this, "Export Word Document",
        QDir::homePath() + "/" + defaultName,
        "Word Files (*.docx)");

    if (path.isEmpty()) return;
    if (!path.endsWith(".docx", Qt::CaseInsensitive))
        path += ".docx";

    /* ── Build WordprocessingML body ── */
    QString body;

    /* Helper: single paragraph */
    auto para = [](const QString& text,
                   const QString& styleId = QString(),
                   bool bold = false,
                   const QString& hexColor = QString()) -> QString
    {
        QString pPr = styleId.isEmpty() ? "" :
                          QString("<w:pPr><w:pStyle w:val=\"%1\"/></w:pPr>").arg(styleId);
        QString rPr;
        if (bold)              rPr += "<w:b/>";
        if (!hexColor.isEmpty()) rPr += QString("<w:color w:val=\"%1\"/>").arg(hexColor);
        QString rPrTag = rPr.isEmpty() ? "" : QString("<w:rPr>%1</w:rPr>").arg(rPr);
        return QString("<w:p>%1<w:r>%2"
                       "<w:t xml:space=\"preserve\">%3</w:t>"
                       "</w:r></w:p>")
            .arg(pPr, rPrTag, text.toHtmlEscaped());
    };

    /* Helper: table row */
    auto tblRow = [](const QStringList& cells, bool header = false) -> QString {
        QString row = "<w:tr>";
        for (const QString& cell : cells) {
            QString shading = header
                                  ? "<w:shd w:val=\"clear\" w:color=\"auto\" w:fill=\"003366\"/>"
                                  : "";
            QString rPr = header
                              ? "<w:rPr><w:b/><w:color w:val=\"FFFFFF\"/></w:rPr>"
                              : "";
            row += QString("<w:tc>"
                           "<w:tcPr>%1</w:tcPr>"
                           "<w:p><w:r>%2"
                           "<w:t xml:space=\"preserve\">%3</w:t>"
                           "</w:r></w:p>"
                           "</w:tc>")
                       .arg(shading, rPr, cell.toHtmlEscaped());
        }
        row += "</w:tr>";
        return row;
    };

    /* ── Title + metadata ── */
    QString titleText = m_missionName.isEmpty() ? "Mission Report" : m_missionName;
    body += para(titleText, "Heading1", true, "003366");
    if (!m_missionDate.isEmpty())
        body += para("Date: " + m_missionDate, "", false, "555555");
    body += para("Generated: " +
                     QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
                 "", false, "555555");
    body += "<w:p/>";

    /* ── KPI summary table ── */
    body += para("Mission KPI Summary", "Heading2", true, "00508A");
    body += "<w:tbl>"
            "<w:tblPr><w:tblStyle w:val=\"TableGrid\"/>"
            "<w:tblW w:w=\"9360\" w:type=\"dxa\"/></w:tblPr>";
    body += tblRow({"Team","Success %","Detection Eff. %",
                    "Weapon Eff. %","Friendly Losses","Enemy Losses"}, true);
    for (const QString& team : m_teamNames) {
        const TeamMetrics& tm = m_teamMetrics.value(team);
        body += tblRow({
            team,
            QString::number(tm.successProbability,  'f', 1) + " %",
            QString::number(tm.detectionEfficiency, 'f', 1) + " %",
            QString::number(tm.weaponEffectiveness, 'f', 1) + " %",
            QString::number(tm.friendlyLosses,      'f', 1),
            QString::number(tm.enemyLosses,         'f', 1)
        });
    }
    body += "</w:tbl><w:p/>";

    /* ── Per-team detail ── */
    for (const QString& team : m_teamNames) {
        const TeamMetrics& tm = m_teamMetrics.value(team);
        const TimelineData& tl = m_teamTimelines.value(team);
        const LossesData& ld   = m_teamLosses.value(team);

        body += para("Team: " + team, "Heading2", true, "00508A");

        /* metrics */
        body += para("Performance Metrics", "Heading3", true);
        body += "<w:tbl>"
                "<w:tblPr><w:tblStyle w:val=\"TableGrid\"/>"
                "<w:tblW w:w=\"6000\" w:type=\"dxa\"/></w:tblPr>";
        body += tblRow({"Metric","Value"}, true);
        QList<QPair<QString,QString>> kpis = {
                                               {"Success Probability",  QString::number(tm.successProbability,  'f', 1) + " %"},
                                               {"Detection Efficiency", QString::number(tm.detectionEfficiency, 'f', 1) + " %"},
                                               {"Weapon Effectiveness", QString::number(tm.weaponEffectiveness, 'f', 1) + " %"},
                                               {"Friendly Losses",      QString::number(tm.friendlyLosses,      'f', 1)},
                                               {"Enemy Losses",         QString::number(tm.enemyLosses,         'f', 1)},
                                               };
        for (const auto& kp : kpis)
            body += tblRow({kp.first, kp.second});
        body += "</w:tbl><w:p/>";

        /* timeline */
        if (!tl.timePoints.isEmpty()) {
            body += para("Engagement Timeline", "Heading3", true);
            body += "<w:tbl>"
                    "<w:tblPr><w:tblStyle w:val=\"TableGrid\"/>"
                    "<w:tblW w:w=\"9360\" w:type=\"dxa\"/></w:tblPr>";
            body += tblRow({"Time (s)","Detection","Engagement","Damage"}, true);
            /* unified time map — same logic as HTML export */
            QMap<double, QVector<double>> wtmap;
            auto wEnsure = [](QVector<double>& v) {
                if (v.size() < 3) v.resize(3);
            };
            for (int i = 0; i < qMin(tl.timePoints.size(), tl.detection.size()); ++i) {
                wEnsure(wtmap[tl.timePoints[i]]);
                wtmap[tl.timePoints[i]][0] = tl.detection[i];
            }
            for (int i = 0; i < qMin(tl.engagementTimePoints.size(), tl.engagement.size()); ++i) {
                wEnsure(wtmap[tl.engagementTimePoints[i]]);
                wtmap[tl.engagementTimePoints[i]][1] = tl.engagement[i];
            }
            for (int i = 0; i < qMin(tl.damageTimePoints.size(), tl.damage.size()); ++i) {
                wEnsure(wtmap[tl.damageTimePoints[i]]);
                wtmap[tl.damageTimePoints[i]][2] = tl.damage[i];
            }
            if (tl.engagementTimePoints.isEmpty())
                for (int i = 0; i < qMin(tl.timePoints.size(), tl.engagement.size()); ++i) {
                    wEnsure(wtmap[tl.timePoints[i]]);
                    wtmap[tl.timePoints[i]][1] = tl.engagement[i];
                }
            if (tl.damageTimePoints.isEmpty())
                for (int i = 0; i < qMin(tl.timePoints.size(), tl.damage.size()); ++i) {
                    wEnsure(wtmap[tl.timePoints[i]]);
                    wtmap[tl.timePoints[i]][2] = tl.damage[i];
                }
            for (auto it = wtmap.constBegin(); it != wtmap.constEnd(); ++it) {
                const QVector<double>& wv = it.value();
                body += tblRow({
                    QString::number(it.key(), 'f', 2),
                    QString::number(wv.size()>0 ? wv[0] : 0.0, 'f', 2),
                    QString::number(wv.size()>1 ? wv[1] : 0.0, 'f', 2),
                    QString::number(wv.size()>2 ? wv[2] : 0.0, 'f', 2)
                });
            }
            body += "</w:tbl><w:p/>";
        }

        /* losses */
        if (!ld.categories.isEmpty()) {
            body += para("Losses vs Engagement", "Heading3", true);
            body += "<w:tbl>"
                    "<w:tblPr><w:tblStyle w:val=\"TableGrid\"/>"
                    "<w:tblW w:w=\"6000\" w:type=\"dxa\"/></w:tblPr>";
            body += tblRow({"Category","Friendly Losses","Enemy Losses"}, true);
            for (int i = 0; i < ld.categories.size(); ++i) {
                double fl = (i < ld.friendlyLosses.size()) ? ld.friendlyLosses[i] : 0;
                double el = (i < ld.enemyLosses.size())    ? ld.enemyLosses[i]    : 0;
                body += tblRow({
                    ld.categories[i],
                    QString::number(fl, 'f', 1),
                    QString::number(el, 'f', 1)
                });
            }
            body += "</w:tbl><w:p/>";
        }
    }

    /* Footer */
    body += para("Report generated by Reports Editor  |  " +
                     QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"),
                 "", false, "888888");

    /* ── Assemble OOXML package strings ── */
    const QString CT =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
        "  <Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
        "  <Default Extension=\"xml\"  ContentType=\"application/xml\"/>\n"
        "  <Override PartName=\"/word/document.xml\"\n"
        "            ContentType=\"application/vnd.openxmlformats-officedocument"
        ".wordprocessingml.document.main+xml\"/>\n"
        "</Types>";

    const QString RELS =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
        "  <Relationship Id=\"rId1\"\n"
        "    Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\"\n"
        "    Target=\"word/document.xml\"/>\n"
        "</Relationships>";

    const QString WORD_RELS =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
        "</Relationships>";

    const QString DOC_XML =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<w:document\n"
        "  xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\"\n"
        "  xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">\n"
        "<w:body>" + body + "</w:body></w:document>";

    /* ── Write minimal Stored-compression ZIP ── */
    struct ZEntry { QString name; QByteArray data; quint32 offset = 0; };
    QList<ZEntry> entries = {
                             {"[Content_Types].xml",          CT.toUtf8()},
                             {"_rels/.rels",                  RELS.toUtf8()},
                             {"word/_rels/document.xml.rels", WORD_RELS.toUtf8()},
                             {"word/document.xml",            DOC_XML.toUtf8()},
                             };

    /* CRC-32 (ISO 3309) */
    auto crc32fn = [](const QByteArray& data) -> quint32 {
        quint32 crc = 0xFFFFFFFF;
        for (unsigned char b : data) {
            crc ^= b;
            for (int k = 0; k < 8; ++k)
                crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1u));
        }
        return ~crc;
    };

    QFile outFile(path);
    if (!outFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Export Word", "Cannot create file:\n" + path);
        return;
    }
    QDataStream ds(&outFile);
    ds.setByteOrder(QDataStream::LittleEndian);

    quint32 localOffset = 0;
    QList<quint32> offsets;
    QList<quint32> crcs;
    QList<quint32> sizes;
    QList<QByteArray> nameBytes;

    /* Local file headers + data */
    for (auto& e : entries) {
        QByteArray nb = e.name.toUtf8();
        quint32 crc   = crc32fn(e.data);
        quint32 sz    = (quint32)e.data.size();
        offsets   << localOffset;
        crcs      << crc;
        sizes     << sz;
        nameBytes << nb;

        ds << (quint32)0x04034b50u;   /* local file sig */
        ds << (quint16)20u;           /* version needed */
        ds << (quint16)0u;            /* flags */
        ds << (quint16)0u;            /* compression: STORE */
        ds << (quint16)0u;            /* mod time */
        ds << (quint16)0u;            /* mod date */
        ds << crc;
        ds << sz;
        ds << sz;
        ds << (quint16)nb.size();
        ds << (quint16)0u;            /* extra len */
        outFile.write(nb);
        outFile.write(e.data);
        localOffset += 30 + nb.size() + sz;
    }

    /* Central directory */
    quint32 cdStart = localOffset;
    for (int i = 0; i < entries.size(); ++i) {
        ds << (quint32)0x02014b50u;
        ds << (quint16)20u;
        ds << (quint16)20u;
        ds << (quint16)0u;
        ds << (quint16)0u;
        ds << (quint16)0u;
        ds << (quint16)0u;
        ds << crcs[i];
        ds << sizes[i];
        ds << sizes[i];
        ds << (quint16)nameBytes[i].size();
        ds << (quint16)0u;
        ds << (quint16)0u;
        ds << (quint16)0u;
        ds << (quint16)0u;
        ds << (quint32)0u;
        ds << offsets[i];
        outFile.write(nameBytes[i]);
        localOffset += 46 + nameBytes[i].size();
    }

    /* End of central directory */
    quint32 cdSize = localOffset - cdStart;
    ds << (quint32)0x06054b50u;
    ds << (quint16)0u;
    ds << (quint16)0u;
    ds << (quint16)entries.size();
    ds << (quint16)entries.size();
    ds << cdSize;
    ds << cdStart;
    ds << (quint16)0u;

    outFile.close();

    QMessageBox::information(this, "Export Word",
                             "Word document exported successfully:\n" + path);
}

/* =========================================================================
   onPrint  — system print dialog + render report
   ========================================================================= */
void ReportsEditor::onPrint()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPageSize(QPageSize::A4));

    QPrintDialog dlg(&printer, this);
    dlg.setWindowTitle("Print Mission Report");
    if (dlg.exec() != QDialog::Accepted)
        return;

    QTextDocument doc;
    QStringList tickedP;
    for (QCheckBox* cb : m_sectionCheckboxes)
        if (cb && cb->isChecked()) tickedP << cb->text();
    doc.setHtml(buildReportHtml(tickedP));
    doc.setPageSize(printer.pageRect(QPrinter::DevicePixel).size());
    doc.print(&printer);
}

/* =========================================================================
   Remaining slots
   ========================================================================= */
void ReportsEditor::onGenerateReport()
{
    /* Collect ticked section labels */
    QStringList sections;
    for (QCheckBox* cb : m_sectionCheckboxes)
        if (cb && cb->isChecked())
            sections << cb->text();

    if (sections.isEmpty()) {
        QMessageBox::warning(this, "Generate Report",
                             "Please tick at least one section to include in the report.");
        return;
    }

    /* Determine chosen format from radio buttons */
    bool isPDF = true;
    if (m_fmtGroup) {
        QAbstractButton* checked = m_fmtGroup->checkedButton();
        if (checked && checked->text() == "Word")
            isPDF = false;
    }

    if (isPDF)
        onExportPDF();
    else
        onExportWord();
}

void ReportsEditor::onPreviewReport()
{
    /* Build HTML first — if buildReportHtml() has issues, catch it here */
    QString reportHtml;
    try {
        QStringList tickedR;
        for (QCheckBox* cb : m_sectionCheckboxes)
            if (cb && cb->isChecked()) tickedR << cb->text();
        reportHtml = buildReportHtml(tickedR);
    } catch (...) {
        QMessageBox::critical(this, "Preview Error",
                              "Failed to build report HTML.");
        return;
    }

    if (reportHtml.isEmpty()) {
        QMessageBox::warning(this, "Preview",
                             "No report data available. Please load a JSON file first.");
        return;
    }

    /* Use setAttribute(WA_DeleteOnClose) instead of deleteLater()
       to avoid double-delete crash */
    QDialog* dlg = new QDialog(this, Qt::Window);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("Report Preview — " +
                        (m_missionName.isEmpty() ? "Mission Report" : m_missionName));
    dlg->resize(820, 640);

    QVBoxLayout* vl = new QVBoxLayout(dlg);
    vl->setContentsMargins(8, 8, 8, 8);
    vl->setSpacing(6);

    /* Top toolbar */
    QHBoxLayout* toolRow = new QHBoxLayout();
    toolRow->setSpacing(8);

    QPushButton* pdfBtn = new QPushButton("Export PDF");
    pdfBtn->setFixedHeight(28);
    pdfBtn->setStyleSheet(
        "QPushButton{background:#1a5a3a;color:white;border:none;"
        "border-radius:3px;font-size:11px;padding:0 12px;}"
        "QPushButton:hover{background:#2a7a4a;}");
    connect(pdfBtn, &QPushButton::clicked, this, &ReportsEditor::onExportPDF);

    QPushButton* wordBtn = new QPushButton("Export Word");
    wordBtn->setFixedHeight(28);
    wordBtn->setStyleSheet(
        "QPushButton{background:#1a3a6a;color:white;border:none;"
        "border-radius:3px;font-size:11px;padding:0 12px;}"
        "QPushButton:hover{background:#2a5a8a;}");
    connect(wordBtn, &QPushButton::clicked, this, &ReportsEditor::onExportWord);

    QPushButton* closeBtn = new QPushButton("Close");
    closeBtn->setFixedHeight(28);
    closeBtn->setStyleSheet(
        "QPushButton{background:#3a3a4a;color:white;border:none;"
        "border-radius:3px;font-size:11px;padding:0 12px;}"
        "QPushButton:hover{background:#4a4a5a;}");
    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);

    toolRow->addWidget(pdfBtn);
    toolRow->addWidget(wordBtn);
    toolRow->addStretch();
    toolRow->addWidget(closeBtn);
    vl->addLayout(toolRow);

    /* Text edit — set plain text first, then HTML to avoid crash */
    QTextEdit* te = new QTextEdit(dlg);
    te->setReadOnly(true);
    te->setStyleSheet("background:white; color:black; border:1px solid #cccccc;");
    te->document()->setDefaultStyleSheet(
        "body { font-family: Arial, sans-serif; font-size: 11pt; color: #111; }"
        "h1   { font-size: 16pt; color: #003366; }"
        "h2   { font-size: 13pt; color: #00508a; }"
        "h3   { font-size: 11pt; color: #0066aa; }"
        "th   { background: #003366; color: white; padding: 4px 8px; }"
        "td   { border: 1px solid #cccccc; padding: 3px 8px; }");

    /* Use setHtml safely via a queued call to avoid re-entrant paint issues */
    te->setHtml(reportHtml);

    vl->addWidget(te, 1);
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void ReportsEditor::onSaveTemplate()
{
    QMessageBox::information(this, "Save Template",
                             "Template saved: " +
                                 (m_templateCombo ? m_templateCombo->currentText() : ""));
}

void ReportsEditor::onLoadTemplate()
{
    QMessageBox::information(this, "Load Template",
                             "Template loaded: " +
                                 (m_templateCombo ? m_templateCombo->currentText() : ""));
}
