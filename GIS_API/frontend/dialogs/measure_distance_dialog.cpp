#include "measure_distance_dialog.h"
#include "backend/api/measure_api.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QClipboard>
#include <QApplication>
#include <QCloseEvent>
#include <QAbstractItemView>
#include <cmath>

ApiMeasureDistanceDialog::ApiMeasureDistanceDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Measure Distance"));
    setMinimumSize(420, 320);
    setWindowFlags(Qt::Window |
                   Qt::WindowTitleHint |
                   Qt::WindowSystemMenuHint |
                   Qt::WindowMinMaxButtonsHint |
                   Qt::WindowCloseButtonHint |
                   Qt::WindowStaysOnTopHint);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    setupUi();
}

ApiMeasureDistanceDialog::~ApiMeasureDistanceDialog() {}
void ApiMeasureDistanceDialog::setupUi()
{
    setStyleSheet(
        "QDialog { background-color: #1a1a2e; color: #e0e0e0; }"
        "QTableWidget {"
        "  background-color: #16213e; color: #e0e0e0;"
        "  gridline-color: #0f3460; border: 1px solid #0f3460;"
        "  selection-background-color: #0078d7;"
        "}"
        "QHeaderView::section {"
        "  background-color: #0f3460; color: white; padding: 6px;"
        "  border: none; font-weight: bold;"
        "}"
        "QComboBox {"
        "  background-color: #16213e; color: white; padding: 6px 28px 6px 10px;"
        "  border: 1px solid #0f3460; border-radius: 4px; min-width: 110px;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 24px;"
        "  border-left: 1px solid #0f3460;"
        "  background-color: #0f3460;"
        "  border-top-right-radius: 4px;"
        "  border-bottom-right-radius: 4px;"
        "}"
        "QComboBox::down-arrow {"
        "  image: url(:/icons/images/down.png);"
        "  width: 12px; height: 12px;"
        "  margin-right: 8px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #16213e; color: white; selection-background-color: #0078d7;"
        "  border: 1px solid #0f3460;"
        "  outline: none;"
        "}"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "  background-color: #16213e; color: white; selection-background-color: #0078d7;"
        "}"
        "QLabel { color: #e0e0e0; font-size: 13px; }"
        "QPushButton {"
        "  background-color: #0f3460; color: white; padding: 8px 18px;"
        "  border: none; border-radius: 4px; font-size: 13px; min-width: 80px;"
        "}"
        "QPushButton:hover { background-color: #1a4a7a; }"
        "QPushButton:pressed { background-color: #0078d7; }"
        );

    QVBoxLayout* main = new QVBoxLayout(this);
    main->setContentsMargins(12, 12, 12, 12);
    main->setSpacing(10);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("x"), tr("y"), tr("Distance")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(false);
    m_table->setShowGrid(true);
    main->addWidget(m_table, 1);

    QHBoxLayout* bottom = new QHBoxLayout();
    bottom->setSpacing(10);
    m_unitCombo = new QComboBox(this);
    m_unitCombo->addItems({tr("meters"), tr("kilometers"), tr("feet"), tr("miles"), tr("nautical miles"), tr("degrees")});
    connect(m_unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ApiMeasureDistanceDialog::onUnitChanged);
    bottom->addWidget(m_unitCombo);
    bottom->addStretch();
    m_totalLabel = new QLabel(tr("Total: 0 m"), this);
    m_totalLabel->setStyleSheet(
        "QLabel { background-color: #16213e; border: 1px solid #0f3460;"
        " border-radius: 4px; padding: 8px 14px; font-weight: bold; }");
    bottom->addWidget(m_totalLabel);
    main->addLayout(bottom);
    QHBoxLayout* buttons = new QHBoxLayout();
    buttons->setSpacing(10);
    buttons->addStretch();
    m_newBtn = new QPushButton(tr("New"), this);
    m_copyBtn = new QPushButton(tr("Copy"), this);
    m_closeBtn = new QPushButton(tr("Close"), this);
    connect(m_newBtn, &QPushButton::clicked, this, &ApiMeasureDistanceDialog::onNew);
    connect(m_copyBtn, &QPushButton::clicked, this, &ApiMeasureDistanceDialog::onCopy);
    connect(m_closeBtn, &QPushButton::clicked, this, &ApiMeasureDistanceDialog::onClose);
    buttons->addWidget(m_newBtn);
    buttons->addWidget(m_copyBtn);
    buttons->addWidget(m_closeBtn);
    main->addLayout(buttons);

}

void ApiMeasureDistanceDialog::setPoints(const QVector<QPointF>& geoPoints,
                                         const QVector<double>& segmentMeters)
{
    m_points = geoPoints;
    m_segmentMeters = segmentMeters;
    rebuildTable();
}

void ApiMeasureDistanceDialog::clearMeasurement()
{
    m_points.clear();
    m_segmentMeters.clear();
    rebuildTable();
}

void ApiMeasureDistanceDialog::rebuildTable()
{
    m_table->setRowCount(0);
    double totalMeters = 0.0;
    for (double d : m_segmentMeters)
        totalMeters += d;
    for (int i = 0; i < m_points.size(); ++i) {
        m_table->insertRow(i);
        const QPointF& p = m_points[i];
        QTableWidgetItem* xItem = new QTableWidgetItem(QString::number(p.x(), 'f', 6));
        QTableWidgetItem* yItem = new QTableWidgetItem(QString::number(p.y(), 'f', 6));
        double segM = (i < m_segmentMeters.size()) ? m_segmentMeters[i] : 0.0;
        QString distText;
        if (i == 0) {
            distText = QString::number(convertFromMeters(0.0), 'f', 3);
        } else {
            distText = QString::number(convertFromMeters(segM), 'f', 3);
        }
        QTableWidgetItem* dItem = new QTableWidgetItem(distText);
        xItem->setTextAlignment(Qt::AlignCenter);
        yItem->setTextAlignment(Qt::AlignCenter);
        dItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 0, xItem);
        m_table->setItem(i, 1, yItem);
        m_table->setItem(i, 2, dItem);
    }
    m_totalLabel->setText(
        tr("Total: %1 %2")
            .arg(convertFromMeters(totalMeters), 0, 'f', 3)
            .arg(unitSuffix()));
}

double ApiMeasureDistanceDialog::convertFromMeters(double meters) const
{
    switch (m_unitIndex) {
    case 1: return meters / 1000.0;
    case 2: return meters * 3.280839895;
    case 3: return meters / 1609.344;
    case 4: return meters / 1852.0;
    case 5: {
        const double R = 6371000.0;
        return (meters / R) * (180.0 / M_PI);
    }
    default: return meters;
    }
}

QString ApiMeasureDistanceDialog::unitSuffix() const
{
    switch (m_unitIndex) {
    case 1: return QStringLiteral("km");
    case 2: return QStringLiteral("ft");
    case 3: return QStringLiteral("mi");
    case 4: return QStringLiteral("nmi");
    case 5: return QStringLiteral("°");
    default: return QStringLiteral("m");
    }
}

void ApiMeasureDistanceDialog::onUnitChanged(int index)
{
    m_unitIndex = index;
    static const char* kUnits[] = {
        "meters", "kilometers", "feet", "miles", "nautical_miles", "degrees"
    };
    if (index >= 0 && index < 6)
        MeasureApi::instance()->setDisplayUnit(QString::fromLatin1(kUnits[index]));
    rebuildTable();
}

void ApiMeasureDistanceDialog::onNew()
{
    clearMeasurement();
    emit requestNew();
}

void ApiMeasureDistanceDialog::onCopy()
{
    QString text;
    text += "x\ty\tDistance\n";
    for (int r = 0; r < m_table->rowCount(); ++r) {
        QStringList cols;
        for (int c = 0; c < 3; ++c) {
            QTableWidgetItem* it = m_table->item(r, c);
            cols << (it ? it->text() : QString());
        }
        text += cols.join('\t') + "\n";
    }
    text += QString("Total\t\t%1 %2\n")
                .arg(m_totalLabel->text())
                .arg(unitSuffix());
    QApplication::clipboard()->setText(text);
}

void ApiMeasureDistanceDialog::onClose()
{
    emit closedByUser();
    hide();
}

void ApiMeasureDistanceDialog::closeEvent(QCloseEvent* event)
{
    emit closedByUser();
    QDialog::closeEvent(event);
}
