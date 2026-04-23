/* =============================================================================
 * FILE:         tacticalrules.cpp
 * MODULE:       Tactical Rules Configuration
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the TacticalRules class which provides a configuration
 *               widget for tactical parameters such as engagement range, weapon
 *               release authority, sensor activation rules, formation type,
 *               support request threshold, and fuel safety margin. Supports
 *               per-team (Blue/Red) configuration with JSON serialization,
 *               reset functionality, and signals for value changes and apply
 *               requests.
 *
 * REQUIREMENTS: Implements REQ-TACTICAL-010 through REQ-TACTICAL-021
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-TACTICAL-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#include "tacticalrules.h"
#include "tactical-styles.h"
#include <QJsonArray>
#include <QJsonValue>
#include "tests/tacticalrulestest/tacticalrules_test.h"
// ── Constructor ──────────────────────────────────────────────────────────────
TacticalRules::TacticalRules(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    applyStyles();
    populateDropdowns();
}

// ── UI Setup ─────────────────────────────────────────────────────────────────
void TacticalRules::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Title
    QLabel *titleLabel = new QLabel("Tactical Rules", this);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setFixedHeight(32);
    mainLayout->addWidget(titleLabel);

    // Divider
    QFrame *divider = new QFrame(this);
    divider->setObjectName("divider");
    divider->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(divider);

    // Form
    QWidget *formWidget = new QWidget(this);
    formWidget->setStyleSheet("background-color: #0F2636;");
    QGridLayout *grid = new QGridLayout(formWidget);
    grid->setContentsMargins(12, 14, 12, 10);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(12);
    grid->setColumnStretch(0, 0);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 0);

    // Row 0 — Max Engagement Range
    QLabel *lblRange = new QLabel("Max Engagement Range:", this);
    lblRange->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    maxEngagementRange = new QDoubleSpinBox(this);
    maxEngagementRange->setRange(0.0, 9999.0);
    maxEngagementRange->setDecimals(0);
    maxEngagementRange->setValue(30.0);
    maxEngagementRange->setSingleStep(1.0);
    QLabel *lblKm = new QLabel("km", this);
    lblKm->setObjectName("unitLabel");
    grid->addWidget(lblRange,           0, 0);
    grid->addWidget(maxEngagementRange, 0, 1);
    grid->addWidget(lblKm,              0, 2);

    // Row 1 — Weapon Release Authority
    QLabel *lblWeapon = new QLabel("Weapon Release Authority:", this);
    lblWeapon->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    weaponReleaseAuthority = new QComboBox(this);
    grid->addWidget(lblWeapon,              1, 0);
    grid->addWidget(weaponReleaseAuthority, 1, 1, 1, 2);

    // Row 2 — Sensor Activation Rule
    QLabel *lblSensor = new QLabel("Sensor Activation Rule:", this);
    lblSensor->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sensorActivationRule = new QComboBox(this);
    grid->addWidget(lblSensor,            2, 0);
    grid->addWidget(sensorActivationRule, 2, 1, 1, 2);

    // Row 3 — Formation Type
    QLabel *lblFormation = new QLabel("Formation Type:", this);
    lblFormation->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formationType = new QComboBox(this);
    grid->addWidget(lblFormation,  3, 0);
    grid->addWidget(formationType, 3, 1, 1, 2);

    // Row 4 — Support Request Threshold
    QLabel *lblSupport = new QLabel("Support Request Threshold:", this);
    lblSupport->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    supportRequestThreshold = new QDoubleSpinBox(this);
    supportRequestThreshold->setRange(0.0, 100.0);
    supportRequestThreshold->setDecimals(0);
    supportRequestThreshold->setValue(50.0);
    supportRequestThreshold->setSingleStep(5.0);
    QLabel *lblPct1 = new QLabel("%", this);
    lblPct1->setObjectName("unitLabel");
    grid->addWidget(lblSupport,              4, 0);
    grid->addWidget(supportRequestThreshold, 4, 1);
    grid->addWidget(lblPct1,                 4, 2);

    // Row 5 — Fuel Safety Margin
    QLabel *lblFuel = new QLabel("Fuel Safety Margin:", this);
    lblFuel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    fuelSafetyMargin = new QDoubleSpinBox(this);
    fuelSafetyMargin->setRange(0.0, 100.0);
    fuelSafetyMargin->setDecimals(0);
    fuelSafetyMargin->setValue(20.0);
    fuelSafetyMargin->setSingleStep(5.0);
    QLabel *lblPct2 = new QLabel("%", this);
    lblPct2->setObjectName("unitLabel");
    grid->addWidget(lblFuel,          5, 0);
    grid->addWidget(fuelSafetyMargin, 5, 1);
    grid->addWidget(lblPct2,          5, 2);

    // Row 6 — Buttons
    applyChangesBtn = new QPushButton("Apply Changes", this);
    applyChangesBtn->setObjectName("applyBtn");
    applyChangesBtn->setCursor(Qt::PointingHandCursor);
    resetRulesBtn = new QPushButton("Reset Rules", this);
    resetRulesBtn->setObjectName("resetBtn");
    resetRulesBtn->setCursor(Qt::PointingHandCursor);
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);
    btnRow->addWidget(applyChangesBtn);
    btnRow->addWidget(resetRulesBtn);
    btnRow->addStretch();
    QWidget *btnWidget = new QWidget(this);
    btnWidget->setLayout(btnRow);
    btnWidget->setStyleSheet("background-color: transparent;");
    grid->addWidget(btnWidget, 6, 0, 1, 3);

    mainLayout->addWidget(formWidget);
    mainLayout->addStretch();

    // Connections
    connect(applyChangesBtn, &QPushButton::clicked, this, &TacticalRules::onApplyChanges);
    connect(resetRulesBtn,   &QPushButton::clicked, this, &TacticalRules::onResetRules);
    connect(maxEngagementRange,      QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TacticalRules::onAnyValueChanged);
    connect(supportRequestThreshold, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TacticalRules::onAnyValueChanged);
    connect(fuelSafetyMargin,        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TacticalRules::onAnyValueChanged);
    connect(weaponReleaseAuthority,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TacticalRules::onAnyValueChanged);
    connect(sensorActivationRule,    QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TacticalRules::onAnyValueChanged);
    connect(formationType,           QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TacticalRules::onAnyValueChanged);
}

void TacticalRules::applyStyles()
{
    setStyleSheet(TacticalStyles::PanelStyle);
}

void TacticalRules::populateDropdowns()
{
    weaponReleaseAuthority->addItems({
        "AUTOMATIC", "SEMI_AUTOMATIC", "COMMAND_APPROVAL_REQUIRED",
        "WEAPON_FREE", "WEAPON_TIGHT", "WEAPON_HOLD"
    });
    sensorActivationRule->addItems({
        "PASSIVE_SENSORS_ONLY", "ACTIVE_RADAR_ALLOWED",
        "FULL_SENSOR_USAGE", "STEALTH_MODE",
        "EMCON_PASSIVE", "INTERMITTENT_RADAR"
    });
    formationType->addItems({
        "LINE_ABREAST", "COLUMN", "WEDGE", "DIAMOND",
        "ECHELON_LEFT", "ECHELON_RIGHT",
        "DEFENSIVE_CIRCLE", "OFFENSIVE_SWEEP"
    });
}

// ── Team data cache helpers ───────────────────────────────────────────────────
void TacticalRules::saveCurrentTeamData()
{
    QJsonObject obj = toJson();
    if (m_currentForce == 0)
        m_blueData = obj;
    else
        m_redData = obj;
}

void TacticalRules::loadCurrentTeamData()
{
    QJsonObject data = (m_currentForce == 0) ? m_blueData : m_redData;
    if (!data.isEmpty())
        loadFromJson(data);
    else
        resetState();
}

// ── setForceType — called when user switches Blue/Red tab ────────────────────
void TacticalRules::setForceType(int forceId)
{
    if (forceId == m_currentForce) return;

    saveCurrentTeamData();   // save old team's UI state
    m_currentForce = forceId;
    loadCurrentTeamData();   // load new team's UI state

    emit valueChanged(toJsonBothTeams());
}

// ── JSON (single team — current) ─────────────────────────────────────────────
QJsonObject TacticalRules::toJson() const
{
    QJsonObject obj;
    obj["maxEngagementRange"]      = maxEngagementRange->value();
    obj["weaponReleaseAuthority"]  = weaponReleaseAuthority->currentText();
    obj["sensorActivationRule"]    = sensorActivationRule->currentText();
    obj["formationType"]           = formationType->currentText();
    obj["supportRequestThreshold"] = supportRequestThreshold->value();
    obj["fuelSafetyMargin"]        = fuelSafetyMargin->value();
    return obj;
}

void TacticalRules::loadFromJson(const QJsonObject &data)
{
    blockSignals(true);
    if (data.contains("maxEngagementRange"))
        maxEngagementRange->setValue(data["maxEngagementRange"].toDouble());
    if (data.contains("weaponReleaseAuthority")) {
        int idx = weaponReleaseAuthority->findText(data["weaponReleaseAuthority"].toString());
        if (idx >= 0) weaponReleaseAuthority->setCurrentIndex(idx);
    }
    if (data.contains("sensorActivationRule")) {
        int idx = sensorActivationRule->findText(data["sensorActivationRule"].toString());
        if (idx >= 0) sensorActivationRule->setCurrentIndex(idx);
    }
    if (data.contains("formationType")) {
        int idx = formationType->findText(data["formationType"].toString());
        if (idx >= 0) formationType->setCurrentIndex(idx);
    }
    if (data.contains("supportRequestThreshold"))
        supportRequestThreshold->setValue(data["supportRequestThreshold"].toDouble());
    if (data.contains("fuelSafetyMargin"))
        fuelSafetyMargin->setValue(data["fuelSafetyMargin"].toDouble());
    blockSignals(false);
}

void TacticalRules::resetState()
{
    blockSignals(true);
    maxEngagementRange->setValue(30.0);
    weaponReleaseAuthority->setCurrentIndex(0);
    sensorActivationRule->setCurrentIndex(0);
    formationType->setCurrentIndex(0);
    supportRequestThreshold->setValue(50.0);
    fuelSafetyMargin->setValue(20.0);
    blockSignals(false);
}

// ── JSON (both teams) ─────────────────────────────────────────────────────────
QJsonObject TacticalRules::toJsonBothTeams() const
{
    QJsonObject current = toJson();
    QJsonObject result;
    if (m_currentForce == 0) {
        result["blue"] = current;
        result["red"]  = m_redData.isEmpty() ? QJsonObject() : m_redData;
    } else {
        result["red"]  = current;
        result["blue"] = m_blueData.isEmpty() ? QJsonObject() : m_blueData;
    }
    return result;
}

void TacticalRules::loadBothTeamsFromJson(const QJsonObject &data)
{
    if (data.contains("blue") || data.contains("red")) {
        m_blueData = data["blue"].toObject();
        m_redData  = data["red"].toObject();
        loadCurrentTeamData();
    } else {
        // Legacy single format — load into blue
        m_blueData = data;
        if (m_currentForce == 0) loadFromJson(data);
    }
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void TacticalRules::onApplyChanges()
{
    emit applyRequested(toJsonBothTeams());
    emit valueChanged(toJsonBothTeams());
}

void TacticalRules::onResetRules()
{
    resetState();
    // Clear current team's cache too
    if (m_currentForce == 0) m_blueData = QJsonObject();
    else                      m_redData  = QJsonObject();
    emit valueChanged(toJsonBothTeams());
}

void TacticalRules::onAnyValueChanged()
{
    emit valueChanged(toJsonBothTeams());
}
int TacticalRules::getRulesCount() const
{

    return 0;
}
