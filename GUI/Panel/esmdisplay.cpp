//============================================================================
// FILE:         esmdisplay.cpp
// MODULE:       ESM (Electronic Support Measures) Display
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen 2 Innovation (O2I).
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Implements the ESMDisplay panel: PPI / Signal Spectrum modes
//               and a shared observation table. Integrates with Hierarchy and
//               Sensor/Platform entities. All observation data flows through
//               one ESMObservationModel which is refreshed from
//               ESM::detect in updateRadar() (the application's existing
//               update call); this class owns no timers.
//
// REQUIREMENTS: Implements REQ-ESM-010 through REQ-ESM-017
//
// AUTHOR:       Arti Rajpoot
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-ESM-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation. Basic ESM display.
//   Rev 2  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       Added requirement references and structured file header.
//   Rev 3  20 Oct 2026  Panel with PPI / Signal Spectrum modes and shared
//                       observation table. PPI painting moved to ESMPpiWidget.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
//               Restricted circulation — defence simulation use only.
//============================================================================
#include "esmdisplay.h"                           // For ESM panel class
// esmdisplay.cpp
#include "core/Hierarchy/EntityProfiles/SensorProfiles/ESM/esmobservationmodel.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/ESM/esmppiwidget.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/ESM/esmspectrumwidget.h"
// #include "esmppiwidget.h"
// #include "esmspectrumwidget.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableView>
#include <QVBoxLayout>
#include <core/Debug/console.h>                    // For console error logging

namespace {

const char* kStyleSheet = R"(

QWidget#esmRoot { background-color: #070b08; }
QLabel { color: #9fe8b0; background: transparent; }
QLabel#esmTitle { color: #35ff7a; font-size: 15px; font-weight: bold; }
QLabel#esmStatus { color: #7fcf94; font-size: 11px; }
QLabel#esmSection { color: #35ff7a; font-size: 11px; font-weight: bold; padding: 1px 4px; }
QPushButton { background-color: #0a140c; color: #7fcf94; border: 1px solid #0e5a26;
              padding: 5px 20px; font-size: 12px; }
QPushButton:hover { border: 1px solid #1fa04a; }
QPushButton:checked { background-color: #0c3a1a; color: #c9ffd9; border: 1px solid #2ee06a; }
QComboBox { background-color: #001a00; color: #00ff00; border: 1px solid #00ff00;
            font-size: 10px; padding: 2px; }
QComboBox QAbstractItemView { background-color: #001a00; color: #00ff00;
                              selection-background-color: #003300; }
QTableView { background-color: #050805; alternate-background-color: #09100a;
             color: #b5f0c4; gridline-color: #0b3a1a; border: 1px solid #0e5a26;
             selection-background-color: #0f4a22; selection-color: #eafff0; font-size: 10px; }
QHeaderView::section { background-color: #0a160c; color: #35ff7a; border: 0px;
                       border-right: 1px solid #0b3a1a; border-bottom: 1px solid #0e5a26;
                       padding: 2px 3px; font-size: 10px; font-weight: bold; }
QSplitter::handle { background-color: #0b3a1a; }
QScrollBar:vertical { background: #050805; width: 8px; }
QScrollBar::handle:vertical { background: #0e5a26; min-height: 20px; }
QScrollBar:horizontal { background: #050805; height: 8px; }
QScrollBar::handle:horizontal { background: #0e5a26; min-width: 20px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0px; height: 0px; }
QToolTip { background-color: #050805; color: #b5f0c4; border: 1px solid #0e5a26; }
)";

// Observation table geometry (matches the 10px stylesheet font)
constexpr int kRowHeight       = 18;
constexpr int kHeaderHeight    = 20;
constexpr int kMinVisibleRows  = 4;   // table never gets less than this many rows
constexpr int kScrollBarExtent = 8;   // must match QScrollBar width/height above
} // namespace

// %%% Constructor %%%
/* Initialize ESM panel */
ESMDisplay::ESMDisplay(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("esmRoot");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QString::fromLatin1(kStyleSheet));
    setMinimumSize(480, 480);
    buildUi();
    refreshObservations();
}

void ESMDisplay::buildUi()
{
    model = new ESMObservationModel(this);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(4, 4, 4, 4);
    rootLayout->setSpacing(4);

    // ---- header: title, sensor status, sensor selector ----
    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("ESM"), this);
    title->setObjectName("esmTitle");
    statusLabel = new QLabel(this);
    statusLabel->setObjectName("esmStatus");
    sensorDropdown = new QComboBox(this);
    sensorDropdown->hide();
    connect(sensorDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ESMDisplay::onSensorSelected);
    header->addWidget(title);
    header->addSpacing(16);
    header->addWidget(statusLabel, 1);
    header->addWidget(sensorDropdown);
    rootLayout->addLayout(header);

    // ---- mode buttons ----
    auto* modes = new QHBoxLayout;
    ppiButton = new QPushButton(QStringLiteral("PPI"), this);
    spectrumButton = new QPushButton(QStringLiteral("SIGNAL SPECTRUM"), this);
    ppiButton->setCheckable(true);
    spectrumButton->setCheckable(true);
    ppiButton->setCursor(Qt::PointingHandCursor);
    spectrumButton->setCursor(Qt::PointingHandCursor);
    connect(ppiButton, &QPushButton::clicked, this, [this]() { setMode(0); });
    connect(spectrumButton, &QPushButton::clicked, this, [this]() { setMode(1); });
    modes->addWidget(ppiButton);
    modes->addWidget(spectrumButton);
    modes->addStretch(1);
    rootLayout->addLayout(modes);

    // ---- active display (stacked) ----
    stack = new QStackedWidget(this);
    ppi = new ESMPpiWidget(model, stack);
    spectrum = new ESMSpectrumWidget(stack);
    stack->addWidget(ppi);
    stack->addWidget(spectrum);
    connect(ppi, &ESMPpiWidget::observationSelected,
            this, &ESMDisplay::onPpiObservationSelected);

    // ---- observation table (shared) ----
    auto* observationsPanel = new QWidget(this);
    auto* obsLayout = new QVBoxLayout(observationsPanel);
    obsLayout->setContentsMargins(0, 0, 0, 0);
    obsLayout->setSpacing(2);
    observationsLabel = new QLabel(observationsPanel);
    observationsLabel->setObjectName("esmSection");
    table = new QTableView(observationsPanel);
    table->setModel(model);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setWordWrap(false);
    table->setShowGrid(true);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    table->verticalHeader()->hide();
    table->verticalHeader()->setDefaultSectionSize(kRowHeight);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table->setTextElideMode(Qt::ElideRight);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);   // fallback only
    table->setMinimumHeight(kHeaderHeight + kMinVisibleRows * kRowHeight
                            + kScrollBarExtent + 2);

    QHeaderView* hh = table->horizontalHeader();
    hh->setHighlightSections(false);
    hh->setSectionsClickable(false);
    hh->setStretchLastSection(false);
    hh->setMinimumSectionSize(20);
    hh->setDefaultAlignment(Qt::AlignCenter);
    hh->setFixedHeight(kHeaderHeight);

    // Compact fixed widths (px) for the 10px font. Platform ID and Name are
    // 0 here because they use Stretch and share whatever width is left.
    static const int widths[ESMObservationModel::ColumnCount] = {
        22,   // #
        0,    // Platform ID  (stretch)
        0,    // Name         (stretch)
        34,   // Emitter ID
        42,   // Bearing   "045.0°"
        38,   // Range     "175.2"
        38,   // Frequency "8000"
        24,   // Bandwidth
        24,   // Rx Power
        28,   // SNR
        30,   // Type
        72,   // Status    "Beyond range"
        30    // Last Seen
    };
    for (int c = 0; c < ESMObservationModel::ColumnCount; ++c) {
        if (c == ESMObservationModel::ColPlatformId || c == ESMObservationModel::ColName) {
            hh->setSectionResizeMode(c, QHeaderView::Stretch);
        } else {
            hh->setSectionResizeMode(c, QHeaderView::Fixed);
            hh->resizeSection(c, widths[c]);
        }
    }
    connect(table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() { onTableSelectionChanged(); });
    obsLayout->addWidget(observationsLabel);
    obsLayout->addWidget(table, 1);

    splitter = new QSplitter(Qt::Vertical, this);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(4);
    splitter->addWidget(stack);
    splitter->addWidget(observationsPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    rootLayout->addWidget(splitter, 1);

    setMode(0);
}

void ESMDisplay::setMode(int mode)
{
    const int m = (mode == 1) ? 1 : 0;
    stack->setCurrentIndex(m);
    ppiButton->setChecked(m == 0);
    spectrumButton->setChecked(m == 1);
}

// %%% Size Management %%%
/* Provide size hint for widget */
QSize ESMDisplay::sizeHint() const
{
    return QSize(720, 780);
}

void ESMDisplay::setRange(float value)
{
    range = static_cast<int>(value);
    if (ppi)
        ppi->update();
}

/* Rebuild sensorlist from the selected platform; keep the current sensor if
 * it is still present, otherwise fall back to the first ESM sensor. */
void ESMDisplay::collectSensors()
{
    ESM* previous = sensor;
    sensor = nullptr;
    sensorlist.clear();
    if (!entity)
        return;

    for (auto const& pair : entity->sensors.sensors) {
        Sensor* s = pair.second;
        if (s && s->subType == Sensor::SubType::ESM) {
            ESM* esm = dynamic_cast<ESM*>(s);
            if (esm)
                sensorlist.append(esm);
        }
    }
    if (previous && sensorlist.contains(previous))
        sensor = previous;
    else if (!sensorlist.isEmpty())
        sensor = sensorlist.first();
}

/* Select and configure entity for display */
void ESMDisplay::selectEntity(Entity* entit)
{
    entity = nullptr;
    sensor = nullptr;
    sensorlist.clear();
    id = "";
    selectedKey.clear();

    Platform* platform = dynamic_cast<Platform*>(entit);
    if (!platform) {
        Console::error("Entity is not a Platform");
        setWindowTitle("ESM Display (No Platform)");
        updateDropdown();
        refreshObservations();
        applySelection(false);
        return;
    }

    // Set entity ID and pointer
    id = QString::fromStdString(platform->ID);
    entity = platform;

    collectSensors();
    if (!sensorlist.isEmpty())
        setWindowTitle("ESM Display (" + QString::fromStdString(entity->Name) + ")");

    updateDropdown();
    refreshObservations();
    applySelection(false);
}

/* Remove entity if ID matches */
void ESMDisplay::RemoveEntity(QString ID)
{
    if (id == ID) {
        entity = nullptr;
        sensor = nullptr;
        sensorlist.clear();
        if (sensorDropdown) {
            QSignalBlocker blocker(sensorDropdown);
            sensorDropdown->clear();
            sensorDropdown->hide();
        }

        id.clear();
        selectedKey.clear();
        setWindowTitle("ESM Display");
        refreshObservations();
        applySelection(false);
    }
}

/* Update display data (called by the application's existing update path) */
void ESMDisplay::updateRadar()
{
    if (entity && sensor) {
        bool stillValid = false;
        for (auto const& pair : entity->sensors.sensors) {
            if (pair.second == sensor) { stillValid = true; break; }
        }
        if (!stillValid) {
            // Sensor removed: rebuild the list from the platform, never keep
            // a dangling ESM pointer.
            collectSensors();
            updateDropdown();
        }
    }
    refreshObservations();
}

/* Pull the current detections into the shared model and keep the selection
 * valid. Cheap when nothing changed (model reports "no change"). */
// void ESMDisplay::refreshObservations()
// {
//     bool changed;
//     if (entity && sensor) {
//         setRange(sensor->range);
//         changed = model->refresh(sensor);
//     } else {
//         changed = model->clear();
//     }

//     // A selected observation that no longer exists is dropped.
//     if (!selectedKey.isEmpty() && model->rowForKey(selectedKey) < 0) {
//         selectedKey.clear();
//         changed = true;
//     }
//     if (changed)
//         applySelection(false);

//     updateStatusLabel();
// }

//Vansh

void ESMDisplay::refreshObservations()
{
    bool changed;
    if (entity && sensor) {
        setRange(sensor->range);
        changed = model->refresh(sensor);
    } else {
        changed = model->clear();
    }

    if (!selectedKey.isEmpty() && model->rowForKey(selectedKey) < 0) {
        selectedKey.clear();
        changed = true;
    }
    if (changed)
        applySelection(false);

    // Spectrum feed: har tick, "changed" se independent (graph live rehna chahiye)
    if (entity && sensor && sensor->Active && sensor->hasSpectrum())
        spectrum->setSpectrum(sensor->spectrumFreqMHz(), sensor->spectrumPsdDbmPerHz());
    else
        spectrum->clearSpectrum();      // "Spectrum data unavailable" wapas

    updateStatusLabel();
}

/* Push selectedKey to the table, the PPI and the spectrum header. */
void ESMDisplay::applySelection(bool scrollToSelection)
{
    const bool previous = syncing;
    syncing = true;

    const int row = model->rowForKey(selectedKey);
    if (row >= 0) {
        table->selectRow(row);
        if (scrollToSelection)
            table->scrollTo(model->index(row, 0));
    } else {
        table->clearSelection();
    }
    ppi->setSelectedKey(selectedKey);
    updateSelectedInfo();

    syncing = previous;
}

void ESMDisplay::updateSelectedInfo()
{
    const int row = model->rowForKey(selectedKey);
    if (row < 0) {
        spectrum->setSelectedInfo(QString());
        spectrum->clearMarker();
        return;
    }
    spectrum->setSelectedInfo(QStringLiteral("SELECTED  ") + model->summaryText(row));
    const ESMObservation& o = model->observations().at(row);
    if (o.emitterValid)
        spectrum->setMarkerFrequencyMHz(o.frequencyMHz);
    else
        spectrum->clearMarker();
}

void ESMDisplay::updateStatusLabel()
{
    QString text;
    switch (model->sensorState()) {
    case ESMObservationModel::SensorState::NoSensor:
        text = QStringLiteral("NO SENSOR");
        break;
    case ESMObservationModel::SensorState::Inactive:
    case ESMObservationModel::SensorState::Active: {
        const bool active = (model->sensorState() == ESMObservationModel::SensorState::Active);
        const QString rangeText = model->rangeKm() > 0.0
                                      ? QString::number(model->rangeKm(), 'g', 5) + QStringLiteral(" km")
                                      : QStringLiteral("--");
        text = QString("%1   |   RANGE %2   |   DETECTIONS %3")
                   .arg(active ? QStringLiteral("ACTIVE") : QStringLiteral("INACTIVE"))
                   .arg(rangeText)
                   .arg(model->rowCount());
        break;
    }
    }
    statusLabel->setText(text);
    observationsLabel->setText(QString("OBSERVATIONS (%1)").arg(model->rowCount()));
}

void ESMDisplay::onPpiObservationSelected(const QString& key)
{
    if (syncing)
        return;
    selectedKey = key;
    applySelection(true);
}

void ESMDisplay::onTableSelectionChanged()
{
    if (syncing)
        return;

    const QModelIndexList rows = table->selectionModel()->selectedRows();
    selectedKey = rows.isEmpty() ? QString() : model->keyAt(rows.first().row());

    const bool previous = syncing;
    syncing = true;
    ppi->setSelectedKey(selectedKey);
    updateSelectedInfo();
    syncing = previous;
}

void ESMDisplay::updateDropdown()
{
    if (!sensorDropdown) return;

    QSignalBlocker blocker(sensorDropdown);
    sensorDropdown->clear();

    if (sensorlist.isEmpty()) {
        sensorDropdown->hide();
        return;
    }

    for (int i = 0; i < sensorlist.size(); ++i) {
        Sensor* s = sensorlist[i];
        QString name = s ? QString::fromStdString(s->Name) : QString("ESM %1").arg(i + 1);
        if (name.trimmed().isEmpty())
            name = QString("ESM %1").arg(i + 1);
        sensorDropdown->addItem(name);
    }

    int currentIdx = sensorlist.indexOf(sensor);
    if (currentIdx >= 0)
        sensorDropdown->setCurrentIndex(currentIdx);

    sensorDropdown->setVisible(sensorlist.size() > 1);
}

void ESMDisplay::onSensorSelected(int index)
{
    if (index < 0 || index >= sensorlist.size()) return;

    ESM* s = sensorlist[index];
    if (!s) return;

    sensor = s;
    selectedKey.clear();
    refreshObservations();
    applySelection(false);
}
