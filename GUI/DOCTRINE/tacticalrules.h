/* =============================================================================
 * FILE:         tacticalrules.h
 * MODULE:       Tactical Rules Configuration
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the TacticalRules class which provides a configuration
 *               widget for tactical parameters such as engagement range, weapon
 *               release authority, sensor activation rules, formation type,
 *               support request threshold, and fuel safety margin. Supports
 *               per-team (Blue/Red) configuration with JSON serialization,
 *               reset functionality, and signals for value changes and apply
 *               requests.
 *
 * REQUIREMENTS: REQ-TACTICAL-010  Maximum engagement range configuration
 *               REQ-TACTICAL-011  Weapon release authority dropdown
 *               REQ-TACTICAL-012  Sensor activation rule dropdown
 *               REQ-TACTICAL-013  Formation type dropdown
 *               REQ-TACTICAL-014  Support request threshold (percentage)
 *               REQ-TACTICAL-015  Fuel safety margin (percentage)
 *               REQ-TACTICAL-016  Apply changes button
 *               REQ-TACTICAL-017  Reset rules to default button
 *               REQ-TACTICAL-018  JSON serialization/deserialization
 *               REQ-TACTICAL-019  Per-team data storage (Blue/Red)
 *               REQ-TACTICAL-020  Signal on any value change
 *               REQ-TACTICAL-021  Signal on apply request
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-TACTICAL-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QJsonObject>

class TacticalRules : public QWidget
{
    Q_OBJECT

public:
    explicit TacticalRules(QWidget *parent = nullptr);

    // Single-team JSON (current active team) — backward compat
    QJsonObject  toJson() const;
    void         loadFromJson(const QJsonObject &data);
    void         resetState();

    // Multi-team JSON (both teams)
    QJsonObject  toJsonBothTeams() const;
    void         loadBothTeamsFromJson(const QJsonObject &data);
    int getRulesCount() const;


public slots:
    // Called by DoctrineParameters::forceTypeChanged
    void setForceType(int forceId);

signals:
    void valueChanged(QJsonObject data);
    void applyRequested(QJsonObject data);

private slots:
    void onApplyChanges();
    void onResetRules();
    void onAnyValueChanged();

private:
    // ── Widgets ──────────────────────────────────────────
    QDoubleSpinBox *maxEngagementRange;
    QComboBox      *weaponReleaseAuthority;
    QComboBox      *sensorActivationRule;
    QComboBox      *formationType;
    QDoubleSpinBox *supportRequestThreshold;
    QDoubleSpinBox *fuelSafetyMargin;
    QPushButton    *applyChangesBtn;
    QPushButton    *resetRulesBtn;

    // ── Per-team cached data ──────────────────────────────
    QJsonObject m_blueData;
    QJsonObject m_redData;
    int         m_currentForce = 0;   // 0 = BLUE, 1 = RED

    void setupUI();
    void applyStyles();
    void populateDropdowns();
    void saveCurrentTeamData();
    void loadCurrentTeamData();
};
