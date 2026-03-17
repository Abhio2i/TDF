
// /* ========================================================================= */
// /* File: doctrineparameters.h                                              */
// /* Purpose: Defines the Doctrine Parameters panel widget                     */
// /* ========================================================================= */

// #ifndef DOCTRINEPARAMETERS_H
// #define DOCTRINEPARAMETERS_H

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
// #include <QRadioButton>
// #include <QButtonGroup>

// // %%% Class Definition %%%
// /* Widget for editing doctrine/mission parameters */
// class DoctrineParameters : public QWidget
// {
//     Q_OBJECT

// public:
//     explicit DoctrineParameters(QWidget *parent = nullptr);
//     ~DoctrineParameters() = default;

//     // Load data from JSON
//     void loadFromJson(const QJsonObject &data);
//     // Export current values to JSON
//     QJsonObject toJson() const;
//     // Reset all fields to defaults
//     void resetState();

// signals:
//     void valueChanged(const QJsonObject &data);

// private slots:
//     void onClearZones();
//     void onAnyValueChanged();
//     void onForceTypeChanged(int id);

// private:
//     void setupUI();
//     void applyStyles();
//     void populateDropdowns();
//     void updateForceStyle(int id);  // Update radio button visuals on selection

//     // %%% Force Type Selection %%%
//     QRadioButton *radioBlue;        // Blue force radio button
//     QRadioButton *radioRed;         // Red force radio button
//     QButtonGroup *forceGroup;       // Exclusive button group

//     // %%% Input Fields %%%
//     QLineEdit   *doctrineName;
//     QComboBox   *missionType;
//     QLineEdit   *missionObjective;
//     QComboBox   *rulesOfEngagement;
//     QComboBox   *engagementPolicy;
//     QComboBox   *retreatPolicy;
//     QComboBox   *detectionPolicy;
//     QPushButton *clearZonesBtn;

//     // %%% Force IDs %%%
//     static constexpr int FORCE_BLUE = 0;
//     static constexpr int FORCE_RED  = 1;
// };

// #endif // DOCTRINEPARAMETERS_H
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
