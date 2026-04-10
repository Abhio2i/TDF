
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
static void runUnitTestsOnce();

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
