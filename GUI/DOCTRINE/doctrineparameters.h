/* =============================================================================
 * FILE:         doctrineparameters.h
 * MODULE:       Doctrine Parameters Configuration
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the DoctrineParameters class which provides a
 *               configuration widget for Blue and Red force doctrine parameters.
 *               The widget includes a tabbed interface (Blue/Red) with controls
 *               for doctrine name, mission objective, mission type, rules of
 *               engagement, engagement policy, retreat policy, detection policy,
 *               and zone clearance. Supports loading/saving to JSON, resetting
 *               state, and emitting signals on value changes.
 *
 * REQUIREMENTS: REQ-DOCTRINE-030  Blue/Red force doctrine parameter editing
 *               REQ-DOCTRINE-031  Doctrine name input field
 *               REQ-DOCTRINE-032  Mission objective text field
 *               REQ-DOCTRINE-033  Mission type selection dropdown
 *               REQ-DOCTRINE-034  Rules of Engagement (ROE) dropdown
 *               REQ-DOCTRINE-035  Engagement policy dropdown
 *               REQ-DOCTRINE-036  Retreat policy dropdown
 *               REQ-DOCTRINE-037  Detection policy dropdown
 *               REQ-DOCTRINE-038  Clear zones button
 *               REQ-DOCTRINE-039  JSON serialization/deserialization
 *               REQ-DOCTRINE-040  Reset to default state
 *               REQ-DOCTRINE-041  Signal on force type change
 *               REQ-DOCTRINE-042  Signal on any parameter value change
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-DOCTRINE-003
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
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFrame>
#include <QStackedWidget>
#include <QJsonObject>

class DoctrineParameters : public QWidget
{
    Q_OBJECT

public:
    static const int FORCE_BLUE = 0;
    static const int FORCE_RED  = 1;

    explicit DoctrineParameters(QWidget *parent = nullptr);

    void         loadFromJson(const QJsonObject &data);
    QJsonObject  toJson() const;
    void         resetState();
    int          currentForce() const { return m_currentForce; }
    QString getForceType() const;

signals:
    void valueChanged(QJsonObject data);
    void forceTypeChanged(int forceId);

private slots:
    void onBlueTabClicked();
    void onRedTabClicked();
    void onClearZones();
    void onAnyValueChanged();

private:
    // ── Top Tab Bar ──────────────────────────────────────
    QPushButton    *btnBlue;
    QPushButton    *btnRed;
    int             m_currentForce = FORCE_BLUE;

    // ── Stacked pages ────────────────────────────────────
    QStackedWidget *stackedPages;

    // ── Blue team widgets ────────────────────────────────
    QLineEdit   *blueDoctrineNameEdit;
    QLineEdit   *blueMissionObjectiveEdit;
    QComboBox   *blueMissionType;
    QComboBox   *blueRulesOfEngagement;
    QComboBox   *blueEngagementPolicy;
    QComboBox   *blueRetreatPolicy;
    QComboBox   *blueDetectionPolicy;
    QPushButton *blueClearZonesBtn;

    // ── Red team widgets ─────────────────────────────────
    QLineEdit   *redDoctrineNameEdit;
    QLineEdit   *redMissionObjectiveEdit;
    QComboBox   *redMissionType;
    QComboBox   *redRulesOfEngagement;
    QComboBox   *redEngagementPolicy;
    QComboBox   *redRetreatPolicy;
    QComboBox   *redDetectionPolicy;
    QPushButton *redClearZonesBtn;
    QString currentDoctrineName() const;

    // ── Helpers ──────────────────────────────────────────
    void    setupUI();
    void    applyStyles();
    void    populateDropdowns();
    void    updateTabStyle(int activeForce);
    void    switchToTeam(int forceId);

    QWidget* buildTeamPage(int forceId,
                           QLineEdit   *&nameEdit,
                           QLineEdit   *&objectiveEdit,
                           QComboBox   *&missionTypeCb,
                           QComboBox   *&roeCb,
                           QComboBox   *&engagementCb,
                           QComboBox   *&retreatCb,
                           QComboBox   *&detectionCb,
                           QPushButton *&clearBtn);

    void populateCombo(QComboBox *cb, const QStringList &items);
    QJsonObject teamToJson(int forceId) const;
    void        loadTeamFromJson(int forceId, const QJsonObject &obj);
};
