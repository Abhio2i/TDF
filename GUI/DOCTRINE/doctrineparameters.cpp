// /* ========================================================================= */
// /* File: doctrineparameters.cpp                                            */
// /* Purpose: Implements the Doctrine Parameters panel widget                  */
// /* ========================================================================= */

// #include "doctrineparameters.h"
// #include "doctrine-styles.h"       // All CSS lives here

// // %%% Constructor %%%
// DoctrineParameters::DoctrineParameters(QWidget *parent)
//     : QWidget(parent)
// {
//     setupUI();
//     applyStyles();
//     populateDropdowns();

//     // %%% Default: Blue force selected %%%
//     radioBlue->setChecked(true);
//     updateForceStyle(FORCE_BLUE);
// }

// // %%% UI Setup %%%
// void DoctrineParameters::setupUI()
// {
//     QVBoxLayout *mainLayout = new QVBoxLayout(this);
//     mainLayout->setContentsMargins(0, 0, 0, 0);
//     mainLayout->setSpacing(0);

//     // %%% Title Bar %%%
//     QLabel *titleLabel = new QLabel("Doctrine Parameters", this);
//     titleLabel->setObjectName("titleLabel");
//     titleLabel->setFixedHeight(32);
//     mainLayout->addWidget(titleLabel);

//     // %%% Force Type Selection Bar (Blue / Red) %%%
//     QFrame *forceBar = new QFrame(this);
//     forceBar->setObjectName("forceBar");
//     forceBar->setFixedHeight(46);

//     QHBoxLayout *forceLayout = new QHBoxLayout(forceBar);
//     forceLayout->setContentsMargins(12, 6, 12, 6);
//     forceLayout->setSpacing(10);

//     radioBlue = new QRadioButton("🔵  Blue", forceBar);
//     radioBlue->setCursor(Qt::PointingHandCursor);

//     radioRed = new QRadioButton("🔴  Red", forceBar);
//     radioRed->setCursor(Qt::PointingHandCursor);

//     forceGroup = new QButtonGroup(this);
//     forceGroup->addButton(radioBlue, FORCE_BLUE);
//     forceGroup->addButton(radioRed,  FORCE_RED);
//     forceGroup->setExclusive(true);

//     forceLayout->addWidget(radioBlue);
//     forceLayout->addWidget(radioRed);
//     forceLayout->addStretch();
//     mainLayout->addWidget(forceBar);

//     // %%% Divider %%%
//     QFrame *divider = new QFrame(this);
//     divider->setObjectName("divider");
//     divider->setFrameShape(QFrame::HLine);
//     mainLayout->addWidget(divider);

//     // %%% Form Area %%%
//     QWidget *formWidget = new QWidget(this);
//     formWidget->setStyleSheet("background-color: #0F2636;");
//     QGridLayout *grid = new QGridLayout(formWidget);
//     grid->setContentsMargins(12, 12, 12, 12);
//     grid->setHorizontalSpacing(10);
//     grid->setVerticalSpacing(10);
//     grid->setColumnStretch(1, 2);
//     grid->setColumnStretch(3, 2);

//     // %%% Row 0: Doctrine Name %%%
//     QLabel *lblDoctrineName = new QLabel("Doctrine Name:", this);
//     lblDoctrineName->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
//     doctrineName = new QLineEdit(this);
//     doctrineName->setPlaceholderText("Enter doctrine name...");
//     grid->addWidget(lblDoctrineName, 0, 0);
//     grid->addWidget(doctrineName,    0, 1, 1, 3);

//     // %%% Row 1: Mission Type %%%
//     QLabel *lblMissionType = new QLabel("Mission Type:", this);
//     lblMissionType->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
//     missionType = new QComboBox(this);
//     grid->addWidget(lblMissionType, 1, 0);
//     grid->addWidget(missionType,    1, 1, 1, 3);

//     // %%% Row 2: Mission Objective %%%
//     QLabel *lblObjective = new QLabel("Mission Objective:", this);
//     lblObjective->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
//     missionObjective = new QLineEdit(this);
//     missionObjective->setPlaceholderText("Describe the mission objective...");
//     grid->addWidget(lblObjective,     2, 0);
//     grid->addWidget(missionObjective, 2, 1, 1, 3);

//     // %%% Row 3: Rules of Engagement + Detection Policy %%%
//     QLabel *lblROE = new QLabel("Rules of Engagement:", this);
//     lblROE->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
//     rulesOfEngagement = new QComboBox(this);

//     QLabel *lblDetection = new QLabel("Detection Policy:", this);
//     lblDetection->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
//     detectionPolicy = new QComboBox(this);

//     grid->addWidget(lblROE,            3, 0);
//     grid->addWidget(rulesOfEngagement, 3, 1);
//     grid->addWidget(lblDetection,      3, 2);
//     grid->addWidget(detectionPolicy,   3, 3);

//     // %%% Row 4: Engagement Policy + Retreat Policy %%%
//     QLabel *lblEngagement = new QLabel("Engagement Policy:", this);
//     lblEngagement->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
//     engagementPolicy = new QComboBox(this);

//     QLabel *lblRetreat = new QLabel("Retreat Policy:", this);
//     lblRetreat->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
//     retreatPolicy = new QComboBox(this);

//     grid->addWidget(lblEngagement,    4, 0);
//     grid->addWidget(engagementPolicy, 4, 1);
//     grid->addWidget(lblRetreat,       4, 2);
//     grid->addWidget(retreatPolicy,    4, 3);

//     // %%% Row 5: Clear Zones Button %%%
//     clearZonesBtn = new QPushButton("↺  Clear Zones", this);
//     clearZonesBtn->setObjectName("clearZonesBtn");
//     clearZonesBtn->setFixedSize(130, 30);
//     clearZonesBtn->setCursor(Qt::PointingHandCursor);

//     QHBoxLayout *btnRow = new QHBoxLayout();
//     btnRow->addStretch();
//     btnRow->addWidget(clearZonesBtn);

//     QWidget *btnWidget = new QWidget(this);
//     btnWidget->setLayout(btnRow);
//     btnWidget->setStyleSheet("background-color: transparent;");
//     grid->addWidget(btnWidget, 5, 0, 1, 4);

//     mainLayout->addWidget(formWidget);
//     mainLayout->addStretch();

//     // %%% Signal Connections %%%
//     connect(clearZonesBtn,    &QPushButton::clicked,
//             this, &DoctrineParameters::onClearZones);
//     connect(doctrineName,     &QLineEdit::textChanged,
//             this, &DoctrineParameters::onAnyValueChanged);
//     connect(missionObjective, &QLineEdit::textChanged,
//             this, &DoctrineParameters::onAnyValueChanged);
//     connect(missionType,      QOverload<int>::of(&QComboBox::currentIndexChanged),
//             this, &DoctrineParameters::onAnyValueChanged);
//     connect(rulesOfEngagement,QOverload<int>::of(&QComboBox::currentIndexChanged),
//             this, &DoctrineParameters::onAnyValueChanged);
//     connect(engagementPolicy, QOverload<int>::of(&QComboBox::currentIndexChanged),
//             this, &DoctrineParameters::onAnyValueChanged);
//     connect(retreatPolicy,    QOverload<int>::of(&QComboBox::currentIndexChanged),
//             this, &DoctrineParameters::onAnyValueChanged);
//     connect(detectionPolicy,  QOverload<int>::of(&QComboBox::currentIndexChanged),
//             this, &DoctrineParameters::onAnyValueChanged);
//     connect(forceGroup, QOverload<int>::of(&QButtonGroup::idClicked),
//             this, &DoctrineParameters::onForceTypeChanged);
// }

// // %%% Style Application %%%
// /* Loads all CSS from doctrine-styles.h */
// void DoctrineParameters::applyStyles()
// {
//     setStyleSheet(DoctrineStyles::PanelStyle);
// }

// // %%% Force Button Visuals %%%
// /* Switches radio button stylesheet between active/inactive per selection */
// void DoctrineParameters::updateForceStyle(int id)
// {
//     if (id == FORCE_BLUE) {
//         radioBlue->setStyleSheet(DoctrineStyles::BlueActive);
//         radioRed->setStyleSheet(DoctrineStyles::RedInactive);
//     } else {
//         radioRed->setStyleSheet(DoctrineStyles::RedActive);
//         radioBlue->setStyleSheet(DoctrineStyles::BlueInactive);
//     }
// }

// // %%% Dropdown Population %%%
// void DoctrineParameters::populateDropdowns()
// {
//     missionType->addItems({
//         "PATROL", "SURVEILLANCE", "INTERCEPTION", "STRIKE", "ESCORT",
//         "AREA_DENIAL", "SEARCH_AND_RESCUE", "BLOCKADE",
//         "RECONNAISSANCE", "DEFENSIVE_HOLD"
//     });

//     rulesOfEngagement->addItems({
//         "HOLD_FIRE", "RETURN_FIRE_ONLY", "DEFENSIVE_ONLY",
//         "FIRE_ON_DETECTION", "FIRE_ON_IDENTIFICATION",
//         "FREE_FIRE", "COMMAND_AUTHORIZATION_REQUIRED"
//     });

//     engagementPolicy->addItems({
//         "NEAREST_TARGET", "HIGHEST_THREAT", "LOWEST_HEALTH_TARGET",
//         "ASSIGNED_TARGET_ONLY", "HIGH_VALUE_TARGET",
//         "GROUP_ENGAGEMENT", "SEQUENTIAL_ENGAGEMENT"
//     });

//     retreatPolicy->addItems({
//         "NEVER_RETREAT", "RETREAT_IF_OUTNUMBERED",
//         "RETREAT_IF_DAMAGE_EXCEEDS_THRESHOLD", "RETREAT_IF_FUEL_LOW",
//         "RETREAT_IF_AMMO_DEPLETED", "RETREAT_IF_COMMAND_ORDERED",
//         "TACTICAL_WITHDRAWAL"
//     });

//     detectionPolicy->addItems({
//         "PASSIVE_SENSORS_ONLY", "ACTIVE_RADAR_ALLOWED",
//         "FULL_SENSOR_USAGE", "STEALTH_MODE",
//         "EMCON_PASSIVE", "INTERMITTENT_RADAR"
//     });
// }

// // %%% JSON Loading %%%
// void DoctrineParameters::loadFromJson(const QJsonObject &data)
// {
//     blockSignals(true);

//     if (data.contains("doctrineName"))
//         doctrineName->setText(data["doctrineName"].toString());

//     if (data.contains("missionType")) {
//         int idx = missionType->findText(data["missionType"].toString());
//         if (idx >= 0) missionType->setCurrentIndex(idx);
//     }

//     if (data.contains("missionObjective"))
//         missionObjective->setText(data["missionObjective"].toString());

//     if (data.contains("rulesOfEngagement")) {
//         int idx = rulesOfEngagement->findText(data["rulesOfEngagement"].toString());
//         if (idx >= 0) rulesOfEngagement->setCurrentIndex(idx);
//     }

//     if (data.contains("engagementPolicy")) {
//         int idx = engagementPolicy->findText(data["engagementPolicy"].toString());
//         if (idx >= 0) engagementPolicy->setCurrentIndex(idx);
//     }

//     if (data.contains("retreatPolicy")) {
//         int idx = retreatPolicy->findText(data["retreatPolicy"].toString());
//         if (idx >= 0) retreatPolicy->setCurrentIndex(idx);
//     }

//     if (data.contains("detectionPolicy")) {
//         int idx = detectionPolicy->findText(data["detectionPolicy"].toString());
//         if (idx >= 0) detectionPolicy->setCurrentIndex(idx);
//     }

//     if (data.contains("forceType")) {
//         if (data["forceType"].toString() == "RED") {
//             radioRed->setChecked(true);
//             updateForceStyle(FORCE_RED);
//         } else {
//             radioBlue->setChecked(true);
//             updateForceStyle(FORCE_BLUE);
//         }
//     }

//     blockSignals(false);
// }

// // %%% JSON Export %%%
// QJsonObject DoctrineParameters::toJson() const
// {
//     QJsonObject obj;
//     obj["forceType"]         = radioRed->isChecked() ? QString("RED") : QString("BLUE");
//     obj["doctrineName"]      = doctrineName->text();
//     obj["missionType"]       = missionType->currentText();
//     obj["missionObjective"]  = missionObjective->text();
//     obj["rulesOfEngagement"] = rulesOfEngagement->currentText();
//     obj["engagementPolicy"]  = engagementPolicy->currentText();
//     obj["retreatPolicy"]     = retreatPolicy->currentText();
//     obj["detectionPolicy"]   = detectionPolicy->currentText();
//     return obj;
// }

// // %%% Reset %%%
// void DoctrineParameters::resetState()
// {
//     blockSignals(true);
//     radioBlue->setChecked(true);
//     updateForceStyle(FORCE_BLUE);
//     doctrineName->clear();
//     missionObjective->clear();
//     missionType->setCurrentIndex(0);
//     rulesOfEngagement->setCurrentIndex(0);
//     engagementPolicy->setCurrentIndex(0);
//     retreatPolicy->setCurrentIndex(0);
//     detectionPolicy->setCurrentIndex(0);
//     blockSignals(false);
// }

// // %%% Slots %%%
// void DoctrineParameters::onForceTypeChanged(int id)
// {
//     updateForceStyle(id);
//     emit valueChanged(toJson());
// }

// void DoctrineParameters::onClearZones()
// {
//     resetState();
//     emit valueChanged(toJson());
// }

// void DoctrineParameters::onAnyValueChanged()
// {
//     emit valueChanged(toJson());
// }
/* =========================================================================
   File: doctrineparameters.cpp
   Purpose: Doctrine Parameters panel — Blue / Red team tabs at the top,
            each team has its own independent form data.
   ========================================================================= */

#include "doctrineparameters.h"
#include "doctrine-styles.h"

// ── Constructor ─────────────────────────────────────────────────────────────
DoctrineParameters::DoctrineParameters(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    applyStyles();
    populateDropdowns();

    // Start on Blue tab
    switchToTeam(FORCE_BLUE);
}

// ── UI Setup ─────────────────────────────────────────────────────────────────
void DoctrineParameters::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ════════════════════════════════════════════════════
    //  TOP TAB BAR  (Blue | Red)
    // ════════════════════════════════════════════════════
    QWidget *tabBar = new QWidget(this);
    tabBar->setObjectName("teamTabBar");
    tabBar->setFixedHeight(42);
    tabBar->setStyleSheet(
        "QWidget#teamTabBar {"
        "  background-color: #0a1e2e;"
        "  border-bottom: 2px solid #1a3a52;"
        "}"
        );

    QHBoxLayout *tabLayout = new QHBoxLayout(tabBar);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->setSpacing(0);

    btnBlue = new QPushButton("🔵  Blue Team", tabBar);
    btnBlue->setObjectName("tabBtnBlue");
    btnBlue->setCursor(Qt::PointingHandCursor);
    btnBlue->setFixedHeight(42);
    btnBlue->setCheckable(true);

    btnRed = new QPushButton("🔴  Red Team", tabBar);
    btnRed->setObjectName("tabBtnRed");
    btnRed->setCursor(Qt::PointingHandCursor);
    btnRed->setFixedHeight(42);
    btnRed->setCheckable(true);

    tabLayout->addWidget(btnBlue);
    tabLayout->addWidget(btnRed);
    tabLayout->addStretch();

    mainLayout->addWidget(tabBar);

    // ════════════════════════════════════════════════════
    //  STACKED PAGES
    // ════════════════════════════════════════════════════
    stackedPages = new QStackedWidget(this);
    stackedPages->setContentsMargins(0, 0, 0, 0);

    // Build Blue page (index 0)
    QWidget *bluePage = buildTeamPage(
        FORCE_BLUE,
        blueDoctrineNameEdit, blueMissionObjectiveEdit,
        blueMissionType, blueRulesOfEngagement,
        blueEngagementPolicy, blueRetreatPolicy,
        blueDetectionPolicy, blueClearZonesBtn
        );
    stackedPages->addWidget(bluePage);   // index 0

    // Build Red page (index 1)
    QWidget *redPage = buildTeamPage(
        FORCE_RED,
        redDoctrineNameEdit, redMissionObjectiveEdit,
        redMissionType, redRulesOfEngagement,
        redEngagementPolicy, redRetreatPolicy,
        redDetectionPolicy, redClearZonesBtn
        );
    stackedPages->addWidget(redPage);    // index 1

    mainLayout->addWidget(stackedPages);
    mainLayout->addStretch();

    // ── Signal connections ──────────────────────────────
    connect(btnBlue, &QPushButton::clicked, this, &DoctrineParameters::onBlueTabClicked);
    connect(btnRed,  &QPushButton::clicked, this, &DoctrineParameters::onRedTabClicked);
}

// ── Build one team's form page ───────────────────────────────────────────────
QWidget* DoctrineParameters::buildTeamPage(
    int           forceId,
    QLineEdit   *&nameEdit,
    QLineEdit   *&objectiveEdit,
    QComboBox   *&missionTypeCb,
    QComboBox   *&roeCb,
    QComboBox   *&engagementCb,
    QComboBox   *&retreatCb,
    QComboBox   *&detectionCb,
    QPushButton *&clearBtn)
{
    Q_UNUSED(forceId)

    QWidget *page = new QWidget(this);
    page->setStyleSheet("background-color: #0F2636;");

    QVBoxLayout *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    // ── Panel title ─────────────────────────────────────
    QLabel *titleLabel = new QLabel("Doctrine Parameters", page);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setFixedHeight(32);
    pageLayout->addWidget(titleLabel);

    // ── Divider ─────────────────────────────────────────
    QFrame *divider = new QFrame(page);
    divider->setObjectName("divider");
    divider->setFrameShape(QFrame::HLine);
    pageLayout->addWidget(divider);

    // ── Grid form ───────────────────────────────────────
    QWidget *formWidget = new QWidget(page);
    formWidget->setStyleSheet("background-color: #0F2636;");

    QGridLayout *grid = new QGridLayout(formWidget);
    grid->setContentsMargins(12, 12, 12, 12);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(10);
    grid->setColumnStretch(1, 2);
    grid->setColumnStretch(3, 2);

    // Row 0 — Doctrine Name
    QLabel *lblName = new QLabel("Doctrine Name:", page);
    lblName->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    nameEdit = new QLineEdit(page);
    nameEdit->setPlaceholderText("Enter doctrine name...");
    grid->addWidget(lblName,  0, 0);
    grid->addWidget(nameEdit, 0, 1, 1, 3);

    // Row 1 — Mission Type
    QLabel *lblMission = new QLabel("Mission Type:", page);
    lblMission->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    missionTypeCb = new QComboBox(page);
    grid->addWidget(lblMission,    1, 0);
    grid->addWidget(missionTypeCb, 1, 1, 1, 3);

    // Row 2 — Mission Objective
    QLabel *lblObj = new QLabel("Mission Objective:", page);
    lblObj->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    objectiveEdit = new QLineEdit(page);
    objectiveEdit->setPlaceholderText("Describe the mission objective...");
    grid->addWidget(lblObj,        2, 0);
    grid->addWidget(objectiveEdit, 2, 1, 1, 3);

    // Row 3 — Rules of Engagement + Detection Policy
    QLabel *lblROE = new QLabel("Rules of Engagement:", page);
    lblROE->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    roeCb = new QComboBox(page);

    QLabel *lblDetect = new QLabel("Detection Policy:", page);
    lblDetect->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    detectionCb = new QComboBox(page);

    grid->addWidget(lblROE,     3, 0);
    grid->addWidget(roeCb,      3, 1);
    grid->addWidget(lblDetect,  3, 2);
    grid->addWidget(detectionCb,3, 3);

    // Row 4 — Engagement Policy + Retreat Policy
    QLabel *lblEng = new QLabel("Engagement Policy:", page);
    lblEng->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    engagementCb = new QComboBox(page);

    QLabel *lblRetreat = new QLabel("Retreat Policy:", page);
    lblRetreat->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    retreatCb = new QComboBox(page);

    grid->addWidget(lblEng,      4, 0);
    grid->addWidget(engagementCb,4, 1);
    grid->addWidget(lblRetreat,  4, 2);
    grid->addWidget(retreatCb,   4, 3);

    // Row 5 — Clear Zones button
    clearBtn = new QPushButton("↺  Clear Zones", page);
    clearBtn->setObjectName("clearZonesBtn");
    clearBtn->setFixedSize(130, 30);
    clearBtn->setCursor(Qt::PointingHandCursor);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(clearBtn);

    QWidget *btnWidget = new QWidget(page);
    btnWidget->setLayout(btnRow);
    btnWidget->setStyleSheet("background-color: transparent;");
    grid->addWidget(btnWidget, 5, 0, 1, 4);

    pageLayout->addWidget(formWidget);
    pageLayout->addStretch();

    // ── Per-page signal connections ──────────────────────
    connect(clearBtn,     &QPushButton::clicked,
            this, &DoctrineParameters::onClearZones);
    connect(nameEdit,     &QLineEdit::textChanged,
            this, &DoctrineParameters::onAnyValueChanged);
    connect(objectiveEdit,&QLineEdit::textChanged,
            this, &DoctrineParameters::onAnyValueChanged);
    connect(missionTypeCb,QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DoctrineParameters::onAnyValueChanged);
    connect(roeCb,        QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DoctrineParameters::onAnyValueChanged);
    connect(engagementCb, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DoctrineParameters::onAnyValueChanged);
    connect(retreatCb,    QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DoctrineParameters::onAnyValueChanged);
    connect(detectionCb,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DoctrineParameters::onAnyValueChanged);

    return page;
}

// ── Apply styles ─────────────────────────────────────────────────────────────
void DoctrineParameters::applyStyles()
{
    // Tab button base style (applied in updateTabStyle too)
    setStyleSheet(DoctrineStyles::PanelStyle);
    updateTabStyle(FORCE_BLUE);
}

// ── Tab styling — active tab gets a colored underline/highlight ──────────────
void DoctrineParameters::updateTabStyle(int activeForce)
{
    QString baseStyle =
        "QPushButton {"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "  padding: 0 24px;"
        "  border: none;"
        "  border-bottom: 3px solid transparent;"
        "  background: transparent;"
        "  color: #8aa8bf;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(255,255,255,0.05);"
        "}";

    QString blueActiveStyle = baseStyle +
                              "QPushButton#tabBtnBlue {"
                              "  color: #4fc3f7;"
                              "  border-bottom: 3px solid #4fc3f7;"
                              "  background: rgba(79,195,247,0.08);"
                              "}";

    QString redActiveStyle = baseStyle +
                             "QPushButton#tabBtnRed {"
                             "  color: #ef5350;"
                             "  border-bottom: 3px solid #ef5350;"
                             "  background: rgba(239,83,80,0.08);"
                             "}";

    if (activeForce == FORCE_BLUE) {
        btnBlue->setStyleSheet(blueActiveStyle);
        btnRed->setStyleSheet(baseStyle);
        btnBlue->setChecked(true);
        btnRed->setChecked(false);
    } else {
        btnRed->setStyleSheet(redActiveStyle);
        btnBlue->setStyleSheet(baseStyle);
        btnRed->setChecked(true);
        btnBlue->setChecked(false);
    }
}

// ── Switch to a team's page ──────────────────────────────────────────────────
void DoctrineParameters::switchToTeam(int forceId)
{
    m_currentForce = forceId;
    stackedPages->setCurrentIndex(forceId);  // 0=Blue, 1=Red
    updateTabStyle(forceId);
    emit forceTypeChanged(forceId);
}

// ── Populate all dropdowns ───────────────────────────────────────────────────
void DoctrineParameters::populateCombo(QComboBox *cb, const QStringList &items)
{
    cb->clear();
    cb->addItems(items);
}

void DoctrineParameters::populateDropdowns()
{
    const QStringList missionItems = {
        "PATROL", "SURVEILLANCE", "INTERCEPTION", "STRIKE", "ESCORT",
        "AREA_DENIAL", "SEARCH_AND_RESCUE", "BLOCKADE",
        "RECONNAISSANCE", "DEFENSIVE_HOLD"
    };
    const QStringList roeItems = {
        "HOLD_FIRE", "RETURN_FIRE_ONLY", "DEFENSIVE_ONLY",
        "FIRE_ON_DETECTION", "FIRE_ON_IDENTIFICATION",
        "FREE_FIRE", "COMMAND_AUTHORIZATION_REQUIRED"
    };
    const QStringList engagementItems = {
        "NEAREST_TARGET", "HIGHEST_THREAT", "LOWEST_HEALTH_TARGET",
        "ASSIGNED_TARGET_ONLY", "HIGH_VALUE_TARGET",
        "GROUP_ENGAGEMENT", "SEQUENTIAL_ENGAGEMENT"
    };
    const QStringList retreatItems = {
        "NEVER_RETREAT", "RETREAT_IF_OUTNUMBERED",
        "RETREAT_IF_DAMAGE_EXCEEDS_THRESHOLD", "RETREAT_IF_FUEL_LOW",
        "RETREAT_IF_AMMO_DEPLETED", "RETREAT_IF_COMMAND_ORDERED",
        "TACTICAL_WITHDRAWAL"
    };
    const QStringList detectionItems = {
        "PASSIVE_SENSORS_ONLY", "ACTIVE_RADAR_ALLOWED",
        "FULL_SENSOR_USAGE", "STEALTH_MODE",
        "EMCON_PASSIVE", "INTERMITTENT_RADAR"
    };

    // Blue
    populateCombo(blueMissionType,        missionItems);
    populateCombo(blueRulesOfEngagement,  roeItems);
    populateCombo(blueEngagementPolicy,   engagementItems);
    populateCombo(blueRetreatPolicy,      retreatItems);
    populateCombo(blueDetectionPolicy,    detectionItems);

    // Red
    populateCombo(redMissionType,         missionItems);
    populateCombo(redRulesOfEngagement,   roeItems);
    populateCombo(redEngagementPolicy,    engagementItems);
    populateCombo(redRetreatPolicy,       retreatItems);
    populateCombo(redDetectionPolicy,     detectionItems);
}

// ── JSON helpers ─────────────────────────────────────────────────────────────
QJsonObject DoctrineParameters::teamToJson(int forceId) const
{
    QJsonObject obj;
    if (forceId == FORCE_BLUE) {
        obj["forceType"]         = "BLUE";
        obj["doctrineName"]      = blueDoctrineNameEdit->text();
        obj["missionType"]       = blueMissionType->currentText();
        obj["missionObjective"]  = blueMissionObjectiveEdit->text();
        obj["rulesOfEngagement"] = blueRulesOfEngagement->currentText();
        obj["engagementPolicy"]  = blueEngagementPolicy->currentText();
        obj["retreatPolicy"]     = blueRetreatPolicy->currentText();
        obj["detectionPolicy"]   = blueDetectionPolicy->currentText();
    } else {
        obj["forceType"]         = "RED";
        obj["doctrineName"]      = redDoctrineNameEdit->text();
        obj["missionType"]       = redMissionType->currentText();
        obj["missionObjective"]  = redMissionObjectiveEdit->text();
        obj["rulesOfEngagement"] = redRulesOfEngagement->currentText();
        obj["engagementPolicy"]  = redEngagementPolicy->currentText();
        obj["retreatPolicy"]     = redRetreatPolicy->currentText();
        obj["detectionPolicy"]   = redDetectionPolicy->currentText();
    }
    return obj;
}

void DoctrineParameters::loadTeamFromJson(int forceId, const QJsonObject &data)
{
    if (data.isEmpty()) return;

    auto setCombo = [](QComboBox *cb, const QJsonObject &d, const QString &key) {
        if (!d.contains(key)) return;
        int idx = cb->findText(d[key].toString());
        if (idx >= 0) cb->setCurrentIndex(idx);
    };

    if (forceId == FORCE_BLUE) {
        if (data.contains("doctrineName"))
            blueDoctrineNameEdit->setText(data["doctrineName"].toString());
        if (data.contains("missionObjective"))
            blueMissionObjectiveEdit->setText(data["missionObjective"].toString());
        setCombo(blueMissionType,       data, "missionType");
        setCombo(blueRulesOfEngagement, data, "rulesOfEngagement");
        setCombo(blueEngagementPolicy,  data, "engagementPolicy");
        setCombo(blueRetreatPolicy,     data, "retreatPolicy");
        setCombo(blueDetectionPolicy,   data, "detectionPolicy");
    } else {
        if (data.contains("doctrineName"))
            redDoctrineNameEdit->setText(data["doctrineName"].toString());
        if (data.contains("missionObjective"))
            redMissionObjectiveEdit->setText(data["missionObjective"].toString());
        setCombo(redMissionType,        data, "missionType");
        setCombo(redRulesOfEngagement,  data, "rulesOfEngagement");
        setCombo(redEngagementPolicy,   data, "engagementPolicy");
        setCombo(redRetreatPolicy,      data, "retreatPolicy");
        setCombo(redDetectionPolicy,    data, "detectionPolicy");
    }
}

// ── Public: toJson — dono teams ka data ──────────────────────────────────────
QJsonObject DoctrineParameters::toJson() const
{
    QJsonObject root;
    root["activeTeam"] = (m_currentForce == FORCE_BLUE) ? "BLUE" : "RED";
    root["blue"]       = teamToJson(FORCE_BLUE);
    root["red"]        = teamToJson(FORCE_RED);
    return root;
}

// ── Public: loadFromJson ──────────────────────────────────────────────────────
void DoctrineParameters::loadFromJson(const QJsonObject &data)
{
    blockSignals(true);

    if (data.contains("blue") || data.contains("red")) {
        // New multi-team format
        loadTeamFromJson(FORCE_BLUE, data["blue"].toObject());
        loadTeamFromJson(FORCE_RED,  data["red"].toObject());

        QString activeTeam = data["activeTeam"].toString("BLUE");
        switchToTeam(activeTeam == "RED" ? FORCE_RED : FORCE_BLUE);
    } else {
        // Legacy single-team format — load into Blue
        loadTeamFromJson(FORCE_BLUE, data);
        switchToTeam(FORCE_BLUE);
    }

    blockSignals(false);
}

// ── Public: resetState ────────────────────────────────────────────────────────
void DoctrineParameters::resetState()
{
    blockSignals(true);

    blueDoctrineNameEdit->clear();
    blueMissionObjectiveEdit->clear();
    blueMissionType->setCurrentIndex(0);
    blueRulesOfEngagement->setCurrentIndex(0);
    blueEngagementPolicy->setCurrentIndex(0);
    blueRetreatPolicy->setCurrentIndex(0);
    blueDetectionPolicy->setCurrentIndex(0);

    redDoctrineNameEdit->clear();
    redMissionObjectiveEdit->clear();
    redMissionType->setCurrentIndex(0);
    redRulesOfEngagement->setCurrentIndex(0);
    redEngagementPolicy->setCurrentIndex(0);
    redRetreatPolicy->setCurrentIndex(0);
    redDetectionPolicy->setCurrentIndex(0);

    switchToTeam(FORCE_BLUE);
    blockSignals(false);
}

QString DoctrineParameters::currentDoctrineName() const
{
    return (m_currentForce == FORCE_BLUE)
               ? blueDoctrineNameEdit->text()
               : redDoctrineNameEdit->text();
}

// ── Slots ────────────────────────────────────────────────────────────────────
void DoctrineParameters::onBlueTabClicked()
{
    if (m_currentForce != FORCE_BLUE)
        switchToTeam(FORCE_BLUE);
}

void DoctrineParameters::onRedTabClicked()
{
    if (m_currentForce != FORCE_RED)
        switchToTeam(FORCE_RED);
}

void DoctrineParameters::onClearZones()
{
    resetState();
    emit valueChanged(toJson());
}

void DoctrineParameters::onAnyValueChanged()
{
    emit valueChanged(toJson());
}
