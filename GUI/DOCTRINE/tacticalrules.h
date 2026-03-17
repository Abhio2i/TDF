// /* ========================================================================= */
// /* File: tacticalrules.h                                                   */
// /* Purpose: Defines the Tactical Rules panel widget                          */
// /* ========================================================================= */

// #ifndef TACTICALRULES_H
// #define TACTICALRULES_H

// #include <QWidget>
// #include <QLineEdit>
// #include <QComboBox>
// #include <QPushButton>
// #include <QLabel>
// #include <QGridLayout>
// #include <QVBoxLayout>
// #include <QHBoxLayout>
// #include <QJsonObject>
// #include <QFrame>
// #include <QDoubleSpinBox>

// // %%% Class Definition %%%
// /* Widget for editing tactical rules parameters */
// class TacticalRules : public QWidget
// {
//     Q_OBJECT

// public:
//     explicit TacticalRules(QWidget *parent = nullptr);
//     ~TacticalRules() = default;

//     // Load data from JSON
//     void loadFromJson(const QJsonObject &data);
//     // Export current values to JSON
//     QJsonObject toJson() const;
//     // Reset all fields to defaults
//     void resetState();

// signals:
//     void valueChanged(const QJsonObject &data);
//     void applyRequested(const QJsonObject &data);

// private slots:
//     void onApplyChanges();
//     void onResetRules();
//     void onAnyValueChanged();

// private:
//     void setupUI();
//     void applyStyles();
//     void populateDropdowns();

//     // %%% Input Fields %%%
//     QDoubleSpinBox *maxEngagementRange;
//     QComboBox      *weaponReleaseAuthority;
//     QComboBox      *sensorActivationRule;
//     QComboBox      *formationType;
//     QDoubleSpinBox *supportRequestThreshold; // percentage
//     QDoubleSpinBox *fuelSafetyMargin;        // percentage

//     // %%% Buttons %%%
//     QPushButton *applyChangesBtn;
//     QPushButton *resetRulesBtn;
// };

// #endif // TACTICALRULES_H
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
