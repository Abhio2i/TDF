/* ========================================================================= */
/* File: reportseditor.cpp                                                   */
/* Purpose: Full implementation of ReportsEditor dashboard.                  */
/*          All custom widgets are top-level classes (MOC-safe).             */
/* Written by: Arti Rajpoot                                                  */
/* ========================================================================= */

#include "reportseditor.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QSplitter>
#include <QPrinter>
#include <QPrintDialog>
#include <cmath>

/* ─── shared palette constants ─── */
static const QString R_BG     = "#0F2636";
static const QString R_PANEL  = "#1A3A4F";
static const QString R_BORDER = "#2a4a6a";
static const QString R_ACCENT = "#00BFFF";

/* =========================================================================
   ReportTimelineWidget implementation
   ========================================================================= */
ReportTimelineWidget::ReportTimelineWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(80);
    setStyleSheet("background:transparent;");
}

void ReportTimelineWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int W = width();
    int lineY = height() / 2 + 8;

    /* baseline */
    p.setPen(QPen(QColor("#4a6a8a"), 2));
    p.drawLine(20, lineY, W - 20, lineY);

    /* tick marks + time labels */
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

    /* event dots above the line */
    struct Evt { double frac; QColor color; };

    QList<Evt> detections  = {{0.08,QColor("#4488ff")},{0.22,QColor("#4488ff")},
                             {0.38,QColor("#4488ff")},{0.55,QColor("#4488ff")},
                             {0.72,QColor("#4488ff")}};
    QList<Evt> engagements = {{0.15,QColor("#00cc44")},{0.32,QColor("#00cc44")},
                              {0.48,QColor("#00cc44")},{0.65,QColor("#00cc44")}};
    QList<Evt> weaponFired = {{0.20,QColor("#ff4444")},{0.36,QColor("#ff4444")},
                              {0.52,QColor("#ff4444")},{0.68,QColor("#ff4444")},
                              {0.80,QColor("#ff4444")}};
    QList<Evt> damage      = {{0.28,QColor("#ffaa00")},{0.44,QColor("#ffaa00")},
                         {0.60,QColor("#ffaa00")},{0.76,QColor("#ffaa00")}};

    auto drawDots = [&](const QList<Evt>& evts) {
        for (const auto& ev : evts) {
            int x = 20 + (int)(ev.frac * (W - 40));
            /* connector */
            p.setPen(QPen(ev.color.darker(160), 1, Qt::DotLine));
            p.drawLine(x, lineY - 22, x, lineY);
            /* dot */
            p.setBrush(ev.color);
            p.setPen(QPen(ev.color.lighter(130), 1));
            p.drawEllipse(x - 5, lineY - 27, 10, 10);
        }
    };

    drawDots(detections);
    drawDots(engagements);
    drawDots(weaponFired);
    drawDots(damage);
}

/* =========================================================================
   ReportGaugeWidget implementation
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

    /* background track */
    p.setPen(QPen(QColor("#1a3a50"), 10, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(arc, 180 * 16, -180 * 16);

    /* value arc */
    QColor col = (m_value > 70) ? QColor("#ff4444")
                 : (m_value > 40) ? QColor("#ffaa00")
                                  : QColor("#44cc44");
    p.setPen(QPen(col, 10, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(arc, 180 * 16, -(int)(m_value / 100.0 * 180 * 16));

    /* centre text */
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

    /* Radar — dashed blue */
    QLineSeries* radar = new QLineSeries();
    radar->setName("Radar");
    radar->setPen(QPen(QColor("#4488ff"), 2, Qt::DashLine));

    /* IR Sensor — solid green */
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
   LEFT PANEL — Report Options + Format + Templates
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

    /* ── Report Options ── */
    QLabel* optTitle = new QLabel("Report Options");
    optTitle->setStyleSheet(
        QString("color:%1; font-size:12px; font-weight:bold; border:none;")
            .arg(R_ACCENT));
    vl->addWidget(optTitle);

    QString cbStyle =
        "QCheckBox { color:#cccccc; font-size:11px; background:transparent; border:none; }"
        "QCheckBox::indicator { width:14px; height:14px; }"
        "QCheckBox::indicator:checked   { background:#0d6efd; border-radius:2px; }"
        "QCheckBox::indicator:unchecked { background:#0d2030; border:1px solid #4a6a8a;"
        "  border-radius:2px; }";

    struct Item { QString label; bool checked; };
    QList<Item> items = {
                         {"Mission Summary",       true},
                         {"Asset Performance",     true},
                         {"Engagement Timeline",   true},
                         {"Detection Probability", true},
                         {"ECM / ECCM Analysis",   true},
                         {"Weapon Usage",          false},
                         {"Damage Assessment",     false},
                         {"Friendly & Enemy Losses",false},
                         {"Lessons Learned",       true},
                         {"Charts & Graphs",       true},
                         };
    for (auto& it : items) {
        QCheckBox* cb = new QCheckBox(it.label);
        cb->setChecked(it.checked);
        cb->setStyleSheet(cbStyle);
        vl->addWidget(cb);
    }

    vl->addWidget(hLine());

    /* ── Report Format ── */
    QLabel* fmtTitle = new QLabel("Report Format");
    fmtTitle->setStyleSheet(
        QString("color:%1; font-size:12px; font-weight:bold; border:none;")
            .arg(R_ACCENT));
    vl->addWidget(fmtTitle);

    QHBoxLayout* fmtRow = new QHBoxLayout();
    fmtRow->setSpacing(2);
    fmtRow->setContentsMargins(0,0,0,0);
    QButtonGroup* fmtGrp = new QButtonGroup(w);
    QString rbStyle =
        "QRadioButton { color:#cccccc; font-size:10px; background:transparent; border:none; }"
        "QRadioButton::indicator { width:12px; height:12px; }"
        "QRadioButton::indicator:checked   { background:#0d6efd; border-radius:6px; }"
        "QRadioButton::indicator:unchecked { background:#0d2030; border:1px solid #4a6a8a;"
        "  border-radius:6px; }";
    for (const QString& fmt : {"PDF","Word","Excel","HTML"}) {
        QRadioButton* rb = new QRadioButton(fmt);
        rb->setStyleSheet(rbStyle);
        if (fmt == "PDF") rb->setChecked(true);
        fmtGrp->addButton(rb);
        fmtRow->addWidget(rb);
    }

    vl->addLayout(fmtRow);

    /* Generate Report button */
    QPushButton* genBtn = new QPushButton("Generate Report");
    genBtn->setFixedHeight(32);
    genBtn->setCursor(Qt::PointingHandCursor);
    genBtn->setStyleSheet(
        "QPushButton { background:#0d6efd; color:white; border:none;"
        "  border-radius:4px; font-size:12px; font-weight:bold; }"
        "QPushButton:hover   { background:#1a7aff; }"
        "QPushButton:pressed { background:#0a50c0; }");
    connect(genBtn, &QPushButton::clicked, this, &ReportsEditor::onGenerateReport);
    vl->addWidget(genBtn);

    vl->addWidget(hLine());

    /* ── Report Templates ── */
    QLabel* tplTitle = new QLabel("Report Templates");
    tplTitle->setStyleSheet(
        QString("color:%1; font-size:12px; font-weight:bold; border:none;")
            .arg(R_ACCENT));
    vl->addWidget(tplTitle);

    m_templateCombo = new QComboBox();
    m_templateCombo->addItems({"Standard Detailed Report","Executive Summary",
                               "Technical Analysis","Quick Overview"});
    m_templateCombo->setStyleSheet(
        "QComboBox { background:#0d2030; color:white; border:1px solid #2a4a6a;"
        "  border-radius:3px; padding:3px 6px; font-size:11px; }"
        "QComboBox::drop-down { border:none; }"
        "QComboBox QAbstractItemView { background:#0d2030; color:white;"
        "  selection-background-color:#1a5a8a; }");
    vl->addWidget(m_templateCombo);

    QHBoxLayout* tplRow = new QHBoxLayout();
    tplRow->setSpacing(6);
    auto tplBtn = [&](const QString& label, auto slot) {
        QPushButton* btn = new QPushButton(label);
        btn->setFixedHeight(26);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { background:#1a3a5a; color:#cccccc; border:1px solid #2a4a6a;"
            "  border-radius:3px; font-size:11px; }"
            "QPushButton:hover { background:#2a4a6a; color:white; }");
        connect(btn, &QPushButton::clicked, this, slot);
        tplRow->addWidget(btn);
    };
    tplBtn("Save Template", &ReportsEditor::onSaveTemplate);
    tplBtn("Load Template", &ReportsEditor::onLoadTemplate);
    vl->addLayout(tplRow);

    vl->addStretch();
    return w;
}

/* =========================================================================
   CENTER TOP — Mission Summary KPI cards + Key Highlights
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

    QLabel* title = new QLabel("Mission Summary");
    title->setStyleSheet(
        QString("color:%1; font-size:13px; font-weight:bold; border:none;")
            .arg(R_ACCENT));
    vl->addWidget(title);

    /* KPI row */
    QHBoxLayout* kpiRow = new QHBoxLayout();
    kpiRow->setSpacing(8);
    kpiRow->addWidget(kpiCard("🎯","Mission\nSuccess",  "80 %",    "#44cc44"));
    kpiRow->addWidget(kpiCard("💥","Enemy\nDestroyed", "5",       "#ff4444"));
    kpiRow->addWidget(kpiCard("🛡","Friendly\nLosses",  "3",       "#4488ff"));
    kpiRow->addWidget(kpiCard("⏱","Mission\nDuration", "02:45:30","#ffaa00"));
    kpiRow->addStretch();
    vl->addLayout(kpiRow);

    vl->addWidget(hLine());

    QLabel* hlTitle = new QLabel("Key Highlights");
    hlTitle->setStyleSheet(
        "color:white; font-size:12px; font-weight:bold; border:none;");
    vl->addWidget(hlTitle);

    struct HL { QString icon; QString color; QString text; };
    QList<HL> hls = {
                     {"✔","#44cc44","Primary objective achieved \xe2\x80\x93 Coastal assets protected"},
                     {"✔","#44cc44","All patrol routes executed successfully"},
                     {"⚠","#ffaa00","One aircraft lost due to ECM interference"},
                     {"●","#4488ff","High weapon effectiveness: 68%"},
                     {"✔","#44cc44","Detection probability met in 75% scenarios"},
                     };
    for (auto& h : hls) {
        QHBoxLayout* row = new QHBoxLayout();
        row->setSpacing(8);
        QLabel* icon = new QLabel(h.icon);
        icon->setFixedWidth(16);
        icon->setStyleSheet(
            QString("color:%1; font-size:12px; border:none;").arg(h.color));
        QLabel* txt = new QLabel(h.text);
        txt->setStyleSheet("color:#cccccc; font-size:11px; border:none;");
        txt->setWordWrap(true);
        row->addWidget(icon);
        row->addWidget(txt, 1);
        vl->addLayout(row);
    }
    return w;
}

/* =========================================================================
   Engagement Timeline panel
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

    /* legend */
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

    vl->addWidget(new ReportTimelineWidget());

    QLabel* timeLbl = new QLabel("Time");
    timeLbl->setAlignment(Qt::AlignCenter);
    timeLbl->setStyleSheet("color:#888; font-size:10px; border:none;");
    vl->addWidget(timeLbl);

    return w;
}

/* =========================================================================
   Lessons Learned panel
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

    QList<QString> items = {
        "Increase ECCM support for aircraft",
        "Improve early detection using radar network",
        "Maintain fuel reserves for extended patrols",
    };
    for (auto& item : items) {
        QHBoxLayout* row = new QHBoxLayout();
        row->setSpacing(8);
        QLabel* dot = new QLabel("\xe2\x80\xa2");
        dot->setFixedWidth(12);
        dot->setStyleSheet("color:#aaaaaa; font-size:14px; border:none;");
        QLabel* txt = new QLabel(item);
        txt->setStyleSheet("color:#cccccc; font-size:11px; border:none;");
        txt->setWordWrap(true);
        row->addWidget(dot);
        row->addWidget(txt, 1);
        vl->addLayout(row);
    }
    vl->addStretch();
    return w;
}

/* =========================================================================
   RIGHT TOP — Detection Probability chart
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
   ECM / ECCM Analysis panel
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

    /* Enemy ECM Type */
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

    /* split: left checkboxes | right gauge */
    QHBoxLayout* split = new QHBoxLayout();
    split->setSpacing(12);

    /* left: ECCM checkboxes */
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

    /* right: jamming badge + gauge */
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
   Weapon Usage Summary
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

    /* header */
    QHBoxLayout* hdr = new QHBoxLayout();
    hdr->setSpacing(6);
    QList<QPair<QString,int>> cols = {
                                       {"Weapon Type",115},{"Used",28},{"Hits",28},{"Effectiveness",-1}};
    for (auto& c : cols) {
        QLabel* l = new QLabel(c.first);
        l->setStyleSheet("color:#888888; font-size:10px; border:none;");
        if (c.second > 0) l->setFixedWidth(c.second);
        hdr->addWidget(l, c.second < 0 ? 1 : 0);
    }
    vl->addLayout(hdr);
    vl->addWidget(hLine());

    vl->addWidget(weaponRow("Anti-Ship Missile", 5, 4, 80,  "#44cc44"));
    vl->addWidget(weaponRow("Torpedo",           3, 2, 66,  "#ffaa00"));
    vl->addWidget(weaponRow("Air Strike",        2, 2, 100, "#44cc44"));
    vl->addWidget(weaponRow("Gunfire",           7, 5, 71,  "#4488ff"));

    vl->addStretch();
    return w;
}

/* =========================================================================
   Bottom action bar
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
   setupUI — assemble 3-column layout
   ========================================================================= */
void ReportsEditor::setupUI()
{
    setStyleSheet(
        QString("QWidget { background:%1; color:white;"
                "  font-family:'Segoe UI',sans-serif; }").arg(R_BG));

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    /* scrollable content */
    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { background:transparent; border:none; }");

    QWidget* content = new QWidget();
    content->setStyleSheet("background:transparent;");
    QVBoxLayout* cVL = new QVBoxLayout(content);
    cVL->setContentsMargins(8, 8, 8, 4);
    cVL->setSpacing(6);

    /* 3-column layout */
    QHBoxLayout* cols = new QHBoxLayout();
    cols->setSpacing(6);

    /* LEFT */
    cols->addWidget(buildLeftPanel(), 0);

    /* CENTER */
    QVBoxLayout* centerVL = new QVBoxLayout();
    centerVL->setSpacing(6);
    centerVL->addWidget(buildCenterTop(),          3);
    centerVL->addWidget(buildEngagementTimeline(), 2);
    centerVL->addWidget(buildLessonsLearned(),     2);
    QWidget* centerW = new QWidget();
    centerW->setStyleSheet("background:transparent;");
    centerW->setLayout(centerVL);
    cols->addWidget(centerW, 5);

    /* RIGHT */
    QVBoxLayout* rightVL = new QVBoxLayout();
    rightVL->setSpacing(6);
    rightVL->addWidget(buildRightTop(),          4);
    rightVL->addWidget(buildECMPanel(),          3);
    rightVL->addWidget(buildWeaponUsageTable(),  3);
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
ReportsEditor::ReportsEditor(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

/* =========================================================================
   Slots
   ========================================================================= */
void ReportsEditor::onGenerateReport()
{
    QMessageBox::information(this, "Generate Report",
                             "Report generation started.\nTemplate: " + m_templateCombo->currentText());
}

void ReportsEditor::onPreviewReport()
{
    QMessageBox::information(this, "Preview Report",
                             "Opening report preview window...");
}

void ReportsEditor::onExportPDF()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Export PDF",
        QDir::homePath() + "/Mission_Report.pdf",
        "PDF Files (*.pdf)");
    if (!path.isEmpty())
        QMessageBox::information(this, "Export PDF",
                                 "PDF exported to:\n" + path);
}

void ReportsEditor::onExportWord()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Export Word",
        QDir::homePath() + "/Mission_Report.docx",
        "Word Files (*.docx)");
    if (!path.isEmpty())
        QMessageBox::information(this, "Export Word",
                                 "Word document exported to:\n" + path);
}

void ReportsEditor::onPrint()
{
    QMessageBox::information(this, "Print",
                             "Sending report to printer...");
}

void ReportsEditor::onSaveTemplate()
{
    QMessageBox::information(this, "Save Template",
                             "Template saved: " + m_templateCombo->currentText());
}

void ReportsEditor::onLoadTemplate()
{
    QMessageBox::information(this, "Load Template",
                             "Template loaded: " + m_templateCombo->currentText());
}
