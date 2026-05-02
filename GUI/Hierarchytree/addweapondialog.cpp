//======================Waris===========================

#include "addweapondialog.h"
#include <QApplication>
#include <QPalette>
#include <QAction>
#include <QStyle>
#include <core/Hierarchy/hierarchy.h>
#include <core/Hierarchy/EntityProfiles/weapon.h>

// ──────────────────────────────────────────────────────────────
//  Helpers
// ──────────────────────────────────────────────────────────────
static const QString kDialogStyle = R"(
QDialog {
    background-color: #0F2636;
    color: white;
    border: 2px solid #27446d;
}
QLabel {
    color: white;
}
QLineEdit {
    color: white;
    background-color: #1A3652;
    border: 1px solid #27446d;
    padding: 4px;
}
QComboBox {
    color: white;
    background-color: #1A3652;
    border: 1px solid #27446d;
    padding: 4px;
}
QComboBox QAbstractItemView {
    background-color: #0F2636;
    color: #ffffff;
    selection-background-color: #1A3652;
    selection-color: #ffffff;
    border: 1px solid #27446d;
    outline: none;
}
QComboBox QAbstractItemView::item {
    padding: 4px 8px;
    min-height: 22px;
}
QComboBox QAbstractItemView::item:hover {
    background-color: #1A3652;
    color: #ffffff;
}
QGroupBox {
    color: white;
    border: 1px solid #27446d;
    margin-top: 8px;
    padding-top: 8px;
}
QGroupBox::title {
    color: white;
    subcontrol-origin: margin;
    left: 8px;
}
QScrollArea {
    background-color: #0F2636;
    border: none;
}
QWidget#scrollContents {
    background-color: #0F2636;
}
QDoubleSpinBox, QSpinBox {
    color: white;
    background-color: #1A3652;
    border: 1px solid #27446d;
    padding: 3px;
}
QCheckBox {
    color: white;
}
QPushButton {
    color: white;
    background-color: #1A3652;
    border: 1px solid #27446d;
    padding: 6px 14px;
    border-radius: 3px;
}
QPushButton:hover {
    background-color: #27446d;
}
QDoubleSpinBox, QSpinBox {
    color: white;
    background-color: #1A3652;
    border: 1px solid #27446d;
    padding: 3px;
}
QDoubleSpinBox::up-button, QSpinBox::up-button {
    background-color: #27446d;
    border: 1px solid #4a6fa5;
    width: 16px;
}
QDoubleSpinBox::down-button, QSpinBox::down-button {
    background-color: #27446d;
    border: 1px solid #4a6fa5;
    width: 16px;
}
QDoubleSpinBox::up-arrow, QSpinBox::up-arrow {
    image: url(:/icons/images/up.png);
    width: 12px;
    height: 12px;
}
QDoubleSpinBox::down-arrow, QSpinBox::down-arrow {
    image: url(:/icons/images/down.png);
    width: 12px;
    height: 12px;
}
QDoubleSpinBox::up-button:hover, QSpinBox::up-button:hover,
QDoubleSpinBox::down-button:hover, QSpinBox::down-button:hover {
    background-color: #3a5f8a;
}
)";

// colour for type badge labels
static const QMap<QString,QString> kTypeBadgeColor = {
    {"Missile",   "#c0392b"},
    {"Bomb",      "#e67e22"},
    {"Gun",       "#8e44ad"},
    {"Rocket",    "#16a085"},
    {"Torpedo",   "#2980b9"},
    {"Artillery", "#7f8c8d"},
    };

// ──────────────────────────────────────────────────────────────
//  Static factory helpers
// ──────────────────────────────────────────────────────────────
QGroupBox* AddWeaponDialog::makeGroup(const QString& title, QFormLayout*& outForm)
{
    QGroupBox* box = new QGroupBox(title);
    outForm = new QFormLayout(box);
    outForm->setLabelAlignment(Qt::AlignRight);
    outForm->setSpacing(6);
    outForm->setContentsMargins(10, 12, 10, 8);
    return box;
}

QWidget* AddWeaponDialog::wrapScroll(QWidget* panel)
{
    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setWidget(panel);
    panel->setObjectName("scrollContents");

    QWidget* wrapper = new QWidget();
    QVBoxLayout* wl = new QVBoxLayout(wrapper);
    wl->setContentsMargins(0,0,0,0);
    wl->addWidget(scroll);
    return wrapper;
}

static QDoubleSpinBox* makeDSpin(double min, double max, double val,
                                 int decimals = 1, const QString& suffix = "")
{
    QDoubleSpinBox* s = new QDoubleSpinBox();
    s->setRange(min, max);
    s->setValue(val);
    s->setDecimals(decimals);
    if (!suffix.isEmpty()) s->setSuffix(" " + suffix);
    s->setMinimumWidth(140);
    return s;
}

static QComboBox* makeCombo(const QStringList& items, int defaultIdx = 0)
{
    QComboBox* c = new QComboBox();
    c->addItems(items);
    c->setCurrentIndex(defaultIdx);
    return c;
}

// ──────────────────────────────────────────────────────────────
//  Constructor
// ──────────────────────────────────────────────────────────────
AddWeaponDialog::AddWeaponDialog(QWidget* parent,
                                 Hierarchy* dbHierarchy,
                                 bool isDatabaseEditor)
    : QDialog(parent)
    , m_dbHierarchy(dbHierarchy)
    , m_isDatabaseEditor(isDatabaseEditor)
{
    setWindowTitle("Add Weapon");
    setMinimumSize(460, isDatabaseEditor ? 540 : 680);
    setStyleSheet(kDialogStyle);

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setSpacing(10);
    root->setContentsMargins(14, 14, 14, 10);

    // ── Search Entity section (scenario/runtime only) ────────
    if (!isDatabaseEditor && dbHierarchy) {
        buildSearchSection(root);

        QFrame* divider = new QFrame();
        divider->setFrameShape(QFrame::HLine);
        divider->setStyleSheet("color: #4a4a4a;");
        root->addWidget(divider);
    }

    // ── Header label ─────────────────────────────────────────
    QLabel* header = new QLabel("Configure Weapon");
    header->setStyleSheet("font-size: 15px; font-weight: bold; color: #a0c8ff;");
    root->addWidget(header);

    // ── Name row ─────────────────────────────────────────────
    QHBoxLayout* nameRow = new QHBoxLayout();
    QLabel* nameLabel = new QLabel("Weapon Name:");
    nameLabel->setFixedWidth(110);
    m_nameEdit = new QLineEdit("Weapon_1");
    m_nameEdit->setPlaceholderText("Enter weapon name…");
    nameRow->addWidget(nameLabel);
    nameRow->addWidget(m_nameEdit);
    root->addLayout(nameRow);

    // ── Type row ─────────────────────────────────────────────
    QHBoxLayout* typeRow = new QHBoxLayout();
    QLabel* typeLabel = new QLabel("Weapon Type:");
    typeLabel->setFixedWidth(110);
    m_typeCombo = makeCombo({"Missile","Sonobuoy","Bomb","Gun","Rocket","Torpedo","Artillery"});
    typeRow->addWidget(typeLabel);
    typeRow->addWidget(m_typeCombo);
    root->addLayout(typeRow);

    // ── Divider ───────────────────────────────────────────────
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #4a4a4a;");
    root->addWidget(line);

    // ── Stacked parameter panels ─────────────────────────────
    m_stack = new QStackedWidget();
    m_stack->addWidget(wrapScroll(buildMissilePanel()));    // 0
    m_stack->addWidget(wrapScroll(buildSonoBuoyPanel()));    // 1
    m_stack->addWidget(wrapScroll(buildBombPanel()));       // 2
    m_stack->addWidget(wrapScroll(buildGunPanel()));        // 3
    m_stack->addWidget(wrapScroll(buildRocketPanel()));     // 4
    m_stack->addWidget(wrapScroll(buildTorpedoPanel()));    // 5
    m_stack->addWidget(wrapScroll(buildArtilleryPanel()));  // 6
    root->addWidget(m_stack, 1);

    // ── Buttons ───────────────────────────────────────────────
    QDialogButtonBox* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setObjectName("okBtn");
    btns->button(QDialogButtonBox::Ok)->setText("Add Weapon");
    btns->setStyleSheet(""); // inherit from dialog
    root->addWidget(btns);

    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddWeaponDialog::onTypeChanged);
}

// ──────────────────────────────────────────────────────────────
//  Slot: swap panel when type changes
// ──────────────────────────────────────────────────────────────
void AddWeaponDialog::onTypeChanged(int index)
{
    m_stack->setCurrentIndex(index);

    // suggest a sensible default name
    if (m_nameEdit->text().isEmpty() ||
        m_nameEdit->text().startsWith("Weapon_") ||
        m_nameEdit->text().startsWith("Sonobuoy_") ||
        m_nameEdit->text().startsWith("Missile_") ||
        m_nameEdit->text().startsWith("Bomb_") ||
        m_nameEdit->text().startsWith("Gun_") ||
        m_nameEdit->text().startsWith("Rocket_") ||
        m_nameEdit->text().startsWith("Torpedo_") ||
        m_nameEdit->text().startsWith("Artillery_"))
    {
        m_nameEdit->setText(m_typeCombo->currentText() + "_1");
    }
}

// ──────────────────────────────────────────────────────────────
//  Result accessors
// ──────────────────────────────────────────────────────────────
QString AddWeaponDialog::weaponName() const
{
    QString n = m_nameEdit->text().trimmed();
    return n.isEmpty() ? m_typeCombo->currentText() + "_1" : n;
}

QString AddWeaponDialog::weaponTypeStr() const
{
    return m_typeCombo->currentText();
}

Weapon::WeaponType AddWeaponDialog::weaponType() const
{
    static const QMap<QString, Weapon::WeaponType> map = {
                                                          {"Missile",   Weapon::WeaponType::Missile},
                                                          {"Sonobuoy",   Weapon::WeaponType::Sonobuoy},
                                                          {"Bomb",      Weapon::WeaponType::Bomb},
                                                          {"Gun",       Weapon::WeaponType::Artillery},   // closest enum
                                                          {"Rocket",    Weapon::WeaponType::Rocket},
                                                          {"Torpedo",   Weapon::WeaponType::Torpedo},
                                                          {"Artillery", Weapon::WeaponType::Artillery},
                                                          };
    return map.value(m_typeCombo->currentText(), Weapon::WeaponType::Missile);
}

QJsonObject AddWeaponDialog::configJson() const
{
    switch (m_stack->currentIndex()) {
    case PanelMissile:   return buildMissileJson();
    case PanelBomb:      return buildBombJson();
    case PanelGun:       return buildGunJson();
    case PanelRocket:    return buildRocketJson();
    case PanelTorpedo:   return buildTorpedoJson();
    case PanelArtillery: return buildArtilleryJson();
    default:             return QJsonObject();
    }
}

// ──────────────────────────────────────────────────────────────
//  Search Entity section (scenario/runtime editor)
// ──────────────────────────────────────────────────────────────
void AddWeaponDialog::buildSearchSection(QVBoxLayout* root)
{
    QLabel* searchLabel = new QLabel("Search Entity:");
    searchLabel->setStyleSheet("color: white; font-weight: bold; font-size: 13px;");
    root->addWidget(searchLabel);

    QHBoxLayout* searchRow = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Type to search entities...");
    m_searchEdit->setMinimumWidth(300);

    m_completer = new QCompleter(this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchContains);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setMaxVisibleItems(15);

    QStringListModel* model = new QStringListModel(this);
    m_completer->setModel(model);
    m_searchEdit->setCompleter(m_completer);

    // Dropdown arrow button
    QAction* showAllAction = new QAction(this);
    showAllAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowDown));
    m_searchEdit->addAction(showAllAction, QLineEdit::TrailingPosition);
    connect(showAllAction, &QAction::triggered, this, [=]() {
        populateWeaponEntities();
        m_completer->complete();
    });

    searchRow->addWidget(m_searchEdit);
    root->addLayout(searchRow);

    // Populate immediately
    populateWeaponEntities();

    // When user selects from completer
    connect(m_completer, QOverload<const QString&>::of(&QCompleter::activated),
            this, [=](const QString& text) {
                if (m_entityMap.contains(text)) {
                    m_selectedEntityId = m_entityMap[text][0].toString();
                    // Extract plain name (remove " (ProfileName)" suffix)
                    QString plainName = text;
                    int paren = plainName.indexOf(" (");
                    if (paren != -1) plainName = plainName.left(paren);
                    if (m_nameEdit) m_nameEdit->setText(plainName);
                    applyEntityConfig(m_selectedEntityId);
                }
            });
}

void AddWeaponDialog::populateWeaponEntities()
{
    if (!m_dbHierarchy || !m_completer) return;

    m_entityMap.clear();
    QStringList names;

    for (const auto& [profileId, profile] : m_dbHierarchy->ProfileCategories) {
        if (!profile) continue;
        QString profileName = QString::fromStdString(profile->Name);
        if (profileName.compare("Weapon", Qt::CaseInsensitive) != 0) continue;

        auto addEntity = [&](auto& entityMap) {
            for (const auto& [eid, entity] : entityMap) {
                if (!entity) continue;
                QString eName = QString::fromStdString(entity->Name).trimmed();
                if (eName.isEmpty()) continue;
                QString display = eName + " (" + profileName + ")";
                names.append(display);
                m_entityMap[display] = QVariantList{
                    QString::fromStdString(eid), profileName
                };
            }
        };

        addEntity(profile->Entities);
        for (const auto& [fid, folder] : profile->Folders) {
            if (folder) addEntity(folder->Entities);
        }
    }

    names.sort(Qt::CaseInsensitive);
    QStringListModel* model = qobject_cast<QStringListModel*>(m_completer->model());
    if (model) model->setStringList(names);
}

void AddWeaponDialog::applyEntityConfig(const QString& entityId)
{
    if (!m_dbHierarchy || entityId.isEmpty()) return;
    if (!m_dbHierarchy->Weapons.count(entityId.toStdString())) return;

    Weapon* w = m_dbHierarchy->Weapons[entityId.toStdString()];
    if (!w) return;

    // Detect weapon type and switch panel
    static const QMap<Weapon::WeaponType, QString> typeMap = {
                                                              {Weapon::WeaponType::Missile,   "Missile"},
                                                              {Weapon::WeaponType::Sonobuoy,  "Sonobuoy"},
                                                              {Weapon::WeaponType::Bomb,      "Bomb"},
                                                              {Weapon::WeaponType::Artillery, "Artillery"},
                                                              {Weapon::WeaponType::Rocket,    "Rocket"},
                                                              {Weapon::WeaponType::Torpedo,   "Torpedo"},
                                                              };
    QString wType = typeMap.value(w->weaponType, "Missile");
    if (m_typeCombo) {
        int idx = m_typeCombo->findText(wType, Qt::MatchFixedString);
        if (idx >= 0) m_typeCombo->setCurrentIndex(idx);
    }

    // Apply known fields from weapon's JSON into spinboxes/combos
    QJsonObject j = w->toJson();

    auto setDSpin = [&](QDoubleSpinBox* sb, const QString& key) {
        if (sb && j.contains(key)) sb->setValue(j[key].toDouble());
    };
    auto setComboStr = [&](QComboBox* cb, const QString& key) {
        if (cb && j.contains(key)) {
            int i = cb->findText(j[key].toString(), Qt::MatchFixedString);
            if (i >= 0) cb->setCurrentIndex(i);
        }
    };

    // Missile
    setComboStr(m_missileGuidance,    "guidanceType");
    setDSpin(m_missileSeekerRange,    "seekerRange");
    setDSpin(m_missileSeekerFOV,      "seekerFOV");
    setComboStr(m_missilePropulsion,  "propulsionType");
    setDSpin(m_missileThrustMain,     "thrustMain");
    setDSpin(m_missileBurnTime,       "burnTime");
    setDSpin(m_missileBlastRadius,    "blastRadius");
    setComboStr(m_missileDetonation,  "detonationType");
    setDSpin(m_missileProximity,      "proximityRange");
    setDSpin(m_missileMaxRange,       "maxRange");
    setDSpin(m_missileMaxVelocity,    "maxVelocity");

    // Bomb
    setDSpin(m_bombTotalMass,         "totalMass");
    setDSpin(m_bombPayloadMass,       "payloadMass");
    setDSpin(m_bombMaxAltitude,       "maxAltitude");
    setDSpin(m_bombBlastRadius,       "blastRadius");
    setDSpin(m_bombEffectiveRadius,   "effectiveRadius");
    setDSpin(m_bombPeakPressure,      "peakPressure");
    setComboStr(m_bombDetonation,     "detonationType");
    setDSpin(m_bombTimerDelay,        "timerDelay");
    if (m_bombWarheadType && j.contains("warheadType"))
        m_bombWarheadType->setText(j["warheadType"].toString());

    // Gun
    setDSpin(m_gunMaxVelocity,        "maxVelocity");
    setDSpin(m_gunMaxRange,           "maxRange");
    setDSpin(m_gunBlastRadius,        "blastRadius");
    setDSpin(m_gunPreflightCheck,     "preflightCheckTime");
    if (m_gunRearmTime && j.contains("rearmTime"))
        m_gunRearmTime->setValue((int)j["rearmTime"].toDouble());
    if (m_gunArmed && j.contains("armed"))
        m_gunArmed->setChecked(j["armed"].toBool());

    // Rocket
    setComboStr(m_rocketPropulsion,   "propulsionType");
    setDSpin(m_rocketThrustMain,      "thrustMain");
    setDSpin(m_rocketThrustBooster,   "thrustBooster");
    setDSpin(m_rocketBurnTime,        "burnTime");
    setDSpin(m_rocketMaxRange,        "maxRange");
    setDSpin(m_rocketBlastRadius,     "blastRadius");
    setComboStr(m_rocketDetonation,   "detonationType");

    // Torpedo
    setComboStr(m_torpedoGuidance,    "guidanceType");
    setDSpin(m_torpedoMaxVelocity,    "maxVelocity");
    setDSpin(m_torpedoMaxRange,       "maxRange");
    setDSpin(m_torpedoMaxAltitude,    "maxAltitude");
    setDSpin(m_torpedoBlastRadius,    "blastRadius");
    setDSpin(m_torpedoPayloadMass,    "payloadMass");

    // Artillery
    setDSpin(m_artilleryMaxRange,     "maxRange");
    setDSpin(m_artilleryMaxVelocity,  "maxVelocity");
    setDSpin(m_artilleryMaxG,         "maximumG");
    setDSpin(m_artilleryBlastRadius,  "blastRadius");
    setComboStr(m_artilleryDetonation,"detonationType");
    if (m_artilleryRearmTime && j.contains("rearmTime"))
        m_artilleryRearmTime->setValue((int)j["rearmTime"].toDouble());
}

// ──────────────────────────────────────────────────────────────
//  Panel builders
// ──────────────────────────────────────────────────────────────

// ── 0. MISSILE ───────────────────────────────────────────────
QWidget* AddWeaponDialog::buildMissilePanel()
{
    QWidget* w = new QWidget();
    QVBoxLayout* vl = new QVBoxLayout(w);
    vl->setSpacing(10);
    vl->setContentsMargins(4,4,4,4);

    // --- Guidance group ---
    QFormLayout* gf; QGroupBox* gg = makeGroup("Guidance System", gf);
    m_missileGuidance = makeCombo({"Fully Active","Semi Active","Passive Infrared",
                                   "Command Guided","Inertial","Unguided"}, 0);
    m_missileSeekerRange = makeDSpin(0, 500000, 100000, 0, "m");
    m_missileSeekerFOV   = makeDSpin(0, 180, 45, 1, "°");
    gf->addRow("Guidance Type:",  m_missileGuidance);
    gf->addRow("Seeker Range:",   m_missileSeekerRange);
    gf->addRow("Seeker FOV:",     m_missileSeekerFOV);
    vl->addWidget(gg);

    // --- Propulsion group ---
    QFormLayout* pf; QGroupBox* pg = makeGroup("Propulsion", pf);
    m_missilePropulsion  = makeCombo({"Solid Rocket","Liquid Rocket","Turbofan",
                                     "Ramjet","Turboprop","Gravity"}, 0);
    m_missileThrustMain  = makeDSpin(0, 2000000, 200000, 0, "N");
    m_missileBurnTime    = makeDSpin(0, 600, 45, 1, "s");
    pf->addRow("Propulsion Type:", m_missilePropulsion);
    pf->addRow("Main Thrust:",     m_missileThrustMain);
    pf->addRow("Burn Time:",       m_missileBurnTime);
    vl->addWidget(pg);

    // --- Warhead group ---
    QFormLayout* wf; QGroupBox* wg = makeGroup("Warhead", wf);
    m_missileBlastRadius = makeDSpin(0, 5000, 200, 1, "m");
    m_missileDetonation  = makeCombo({"Proximity","Impact","Timed","Command"}, 0);
    m_missileProximity   = makeDSpin(0, 2000, 100, 1, "m");
    wf->addRow("Blast Radius:",    m_missileBlastRadius);
    wf->addRow("Detonation:",      m_missileDetonation);
    wf->addRow("Proximity Range:", m_missileProximity);
    vl->addWidget(wg);

    // --- Performance group ---
    QFormLayout* rf; QGroupBox* rg = makeGroup("Performance", rf);
    m_missileMaxRange    = makeDSpin(0, 500000, 100000, 0, "m");
    m_missileMaxVelocity = makeDSpin(0, 10000,  2500,   1, "m/s");
    rf->addRow("Max Range:",    m_missileMaxRange);
    rf->addRow("Max Velocity:", m_missileMaxVelocity);
    vl->addWidget(rg);

    vl->addStretch();
    return w;
}

// ── 1. SonoBuoy ───────────────────────────────────────────────
QWidget* AddWeaponDialog::buildSonoBuoyPanel()
{
    QWidget* w = new QWidget();
    QVBoxLayout* vl = new QVBoxLayout(w);
    vl->setSpacing(10);
    vl->setContentsMargins(4,4,4,4);

    QFormLayout* gf; QGroupBox* gg = makeGroup("Guidance System", gf);
    m_count = makeDSpin(0, 50, 10, 0, "");
    gf->addRow("Count:",   m_count);
    vl->addWidget(gg);


    vl->addStretch();
    return w;
}


// ── 1. BOMB ──────────────────────────────────────────────────
QWidget* AddWeaponDialog::buildBombPanel()
{
    QWidget* w = new QWidget();
    QVBoxLayout* vl = new QVBoxLayout(w);
    vl->setSpacing(10); vl->setContentsMargins(4,4,4,4);

    // --- Aerodynamic / Mass ---
    QFormLayout* af; QGroupBox* ag = makeGroup("Aerodynamics & Mass", af);
    m_bombTotalMass    = makeDSpin(0, 5000, 250,  1, "kg");
    m_bombPayloadMass  = makeDSpin(0, 5000, 120,  1, "kg");
    m_bombMaxAltitude  = makeDSpin(0, 50000, 15000, 0, "m");
    af->addRow("Total Mass:",   m_bombTotalMass);
    af->addRow("Payload Mass:", m_bombPayloadMass);
    af->addRow("Release Alt.:", m_bombMaxAltitude);
    vl->addWidget(ag);

    // --- Explosive ---
    QFormLayout* ef; QGroupBox* eg = makeGroup("Explosive Characteristics", ef);
    m_bombBlastRadius     = makeDSpin(0, 2000, 150,  1, "m");
    m_bombEffectiveRadius = makeDSpin(0, 5000, 300,  1, "m");
    m_bombPeakPressure    = makeDSpin(0, 10000, 500, 1, "kPa");
    m_bombWarheadType     = new QLineEdit("HE");
    ef->addRow("Blast Radius:",      m_bombBlastRadius);
    ef->addRow("Effective Radius:",  m_bombEffectiveRadius);
    ef->addRow("Peak Pressure:",     m_bombPeakPressure);
    ef->addRow("Warhead Type:",      m_bombWarheadType);
    vl->addWidget(eg);

    // --- Detonation ---
    QFormLayout* df; QGroupBox* dg = makeGroup("Detonation", df);
    m_bombDetonation = makeCombo({"Impact","Proximity","Timed","Command"}, 0);
    m_bombTimerDelay = makeDSpin(0, 300, 5, 1, "s");
    df->addRow("Detonation Type:", m_bombDetonation);
    df->addRow("Timer Delay:",     m_bombTimerDelay);
    vl->addWidget(dg);

    vl->addStretch();
    return w;
}

// ── 2. GUN ───────────────────────────────────────────────────
QWidget* AddWeaponDialog::buildGunPanel()
{
    QWidget* w = new QWidget();
    QVBoxLayout* vl = new QVBoxLayout(w);
    vl->setSpacing(10); vl->setContentsMargins(4,4,4,4);

    // --- Ballistics ---
    QFormLayout* bf; QGroupBox* bg = makeGroup("Ballistics", bf);
    m_gunMaxVelocity = makeDSpin(0, 5000, 900, 1, "m/s");
    m_gunMaxRange    = makeDSpin(0, 100000, 5000, 0, "m");
    m_gunBlastRadius = makeDSpin(0, 500, 15, 1, "m");
    bf->addRow("Muzzle Velocity:", m_gunMaxVelocity);
    bf->addRow("Effective Range:", m_gunMaxRange);
    bf->addRow("Blast Radius:",    m_gunBlastRadius);
    vl->addWidget(bg);

    // --- Ammunition & Reload ---
    QFormLayout* af; QGroupBox* ag = makeGroup("Ammunition & Reload", af);
    m_gunRearmTime      = new QSpinBox();
    m_gunRearmTime->setRange(1, 3600);
    m_gunRearmTime->setValue(30);
    m_gunRearmTime->setSuffix(" s");
    m_gunPreflightCheck = makeDSpin(0, 300, 5, 1, "s");
    m_gunArmed          = new QCheckBox("Armed at spawn");
    af->addRow("Reload Time:",          m_gunRearmTime);
    af->addRow("Pre-fire Check Time:", m_gunPreflightCheck);
    af->addRow("",                      m_gunArmed);
    vl->addWidget(ag);

    vl->addStretch();
    return w;
}

// ── 3. ROCKET ────────────────────────────────────────────────
QWidget* AddWeaponDialog::buildRocketPanel()
{
    QWidget* w = new QWidget();
    QVBoxLayout* vl = new QVBoxLayout(w);
    vl->setSpacing(10); vl->setContentsMargins(4,4,4,4);

    // --- Propulsion ---
    QFormLayout* pf; QGroupBox* pg = makeGroup("Propulsion", pf);
    m_rocketPropulsion    = makeCombo({"Solid Rocket","Liquid Rocket","Ramjet"}, 0);
    m_rocketThrustMain    = makeDSpin(0, 1000000, 80000, 0, "N");
    m_rocketThrustBooster = makeDSpin(0, 500000,  20000, 0, "N");
    m_rocketBurnTime      = makeDSpin(0, 120, 12, 1, "s");
    pf->addRow("Propulsion:",     m_rocketPropulsion);
    pf->addRow("Main Thrust:",    m_rocketThrustMain);
    pf->addRow("Booster Thrust:", m_rocketThrustBooster);
    pf->addRow("Burn Time:",      m_rocketBurnTime);
    vl->addWidget(pg);

    // --- Warhead ---
    QFormLayout* wf; QGroupBox* wg = makeGroup("Warhead & Range", wf);
    m_rocketMaxRange    = makeDSpin(0, 50000, 8000, 0, "m");
    m_rocketBlastRadius = makeDSpin(0, 1000, 60, 1, "m");
    m_rocketDetonation  = makeCombo({"Impact","Proximity","Timed","Command"}, 0);
    wf->addRow("Max Range:",    m_rocketMaxRange);
    wf->addRow("Blast Radius:", m_rocketBlastRadius);
    wf->addRow("Detonation:",   m_rocketDetonation);
    vl->addWidget(wg);

    vl->addStretch();
    return w;
}

// ── 4. TORPEDO ───────────────────────────────────────────────
QWidget* AddWeaponDialog::buildTorpedoPanel()
{
    QWidget* w = new QWidget();
    QVBoxLayout* vl = new QVBoxLayout(w);
    vl->setSpacing(10); vl->setContentsMargins(4,4,4,4);

    // --- Guidance ---
    QFormLayout* gf; QGroupBox* gg = makeGroup("Guidance", gf);
    m_torpedoGuidance = makeCombo({"Fully Active","Semi Active","Passive Infrared",
                                   "Command Guided","Inertial","Unguided"}, 0);
    gf->addRow("Guidance Type:", m_torpedoGuidance);
    vl->addWidget(gg);

    // --- Performance ---
    QFormLayout* pf; QGroupBox* pg = makeGroup("Performance", pf);
    m_torpedoMaxVelocity = makeDSpin(0, 300, 45, 1, "m/s");
    m_torpedoMaxRange    = makeDSpin(0, 100000, 40000, 0, "m");
    m_torpedoMaxAltitude = makeDSpin(0, 1000, 500, 1, "m");  // max depth
    pf->addRow("Max Speed:",  m_torpedoMaxVelocity);
    pf->addRow("Max Range:",  m_torpedoMaxRange);
    pf->addRow("Max Depth:",  m_torpedoMaxAltitude);
    vl->addWidget(pg);

    // --- Warhead ---
    QFormLayout* wf; QGroupBox* wg = makeGroup("Warhead", wf);
    m_torpedoBlastRadius  = makeDSpin(0, 500, 80, 1, "m");
    m_torpedoPayloadMass  = makeDSpin(0, 1000, 200, 1, "kg");
    wf->addRow("Blast Radius:",  m_torpedoBlastRadius);
    wf->addRow("Warhead Mass:",  m_torpedoPayloadMass);
    vl->addWidget(wg);

    vl->addStretch();
    return w;
}

// ── 5. ARTILLERY ─────────────────────────────────────────────
QWidget* AddWeaponDialog::buildArtilleryPanel()
{
    QWidget* w = new QWidget();
    QVBoxLayout* vl = new QVBoxLayout(w);
    vl->setSpacing(10); vl->setContentsMargins(4,4,4,4);

    // --- Ballistics ---
    QFormLayout* bf; QGroupBox* bg = makeGroup("Ballistics", bf);
    m_artilleryMaxRange    = makeDSpin(0, 200000, 30000, 0, "m");
    m_artilleryMaxVelocity = makeDSpin(0, 2000, 800, 1, "m/s");
    m_artilleryMaxG        = makeDSpin(0, 100, 20, 1, "G");
    bf->addRow("Max Range:",    m_artilleryMaxRange);
    bf->addRow("Muzzle Velocity:", m_artilleryMaxVelocity);
    bf->addRow("Max G-force:",  m_artilleryMaxG);
    vl->addWidget(bg);

    // --- Shell / Warhead ---
    QFormLayout* sf; QGroupBox* sg = makeGroup("Shell & Detonation", sf);
    m_artilleryBlastRadius = makeDSpin(0, 2000, 100, 1, "m");
    m_artilleryDetonation  = makeCombo({"Impact","Proximity","Timed","Command"}, 0);
    m_artilleryRearmTime   = new QSpinBox();
    m_artilleryRearmTime->setRange(1, 3600);
    m_artilleryRearmTime->setValue(10);
    m_artilleryRearmTime->setSuffix(" s");
    sf->addRow("Blast Radius:",  m_artilleryBlastRadius);
    sf->addRow("Detonation:",    m_artilleryDetonation);
    sf->addRow("Reload Time:",   m_artilleryRearmTime);
    vl->addWidget(sg);

    vl->addStretch();
    return w;
}

// ══════════════════════════════════════════════════════════════
//  JSON BUILDERS
//  Each maps widget values to the Weapon field names used in
//  Weapon::fromJson() so the caller can pass to updateSubComponent()
// ══════════════════════════════════════════════════════════════

static const QStringList kGuidanceStr = {
    "FullyActive","SemiActive","PassiveInfrared",
    "CommandGuided","InertialGuidance","Unguided"
};
static const QStringList kPropulsionStr = {
    "SolidRocket","LiquidRocket","Turbofan","Ramjet","Turboprop","Gravity"
};
static const QStringList kDetonationStr = {
    "Proximity","Impact","Timed","Command"
};
static const QStringList kDetonationStrBomb = {
    "Impact","Proximity","Timed","Command"
};

QJsonObject AddWeaponDialog::buildMissileJson() const
{
    QJsonObject obj;
    obj["weaponType"]       = "Missile";
    obj["guidanceType"]     = kGuidanceStr.value(m_missileGuidance->currentIndex(), "FullyActive");
    obj["seekerRange"]      = m_missileSeekerRange->value();
    obj["seekerFOV"]        = m_missileSeekerFOV->value();
    obj["propulsionType"]   = kPropulsionStr.value(m_missilePropulsion->currentIndex(), "SolidRocket");
    obj["thrustMain"]       = m_missileThrustMain->value();
    obj["burnTime"]         = m_missileBurnTime->value();
    obj["blastRadius"]      = m_missileBlastRadius->value();
    obj["detonationType"]   = kDetonationStr.value(m_missileDetonation->currentIndex(), "Proximity");
    obj["proximityRange"]   = m_missileProximity->value();
    obj["maxRange"]         = m_missileMaxRange->value();
    obj["maxVelocity"]      = m_missileMaxVelocity->value();
    return obj;
}

QJsonObject AddWeaponDialog::buildBombJson() const
{
    QJsonObject obj;
    obj["weaponType"]       = "Bomb";
    obj["totalMass"]        = m_bombTotalMass->value();
    obj["payloadMass"]      = m_bombPayloadMass->value();
    obj["maxAltitude"]      = m_bombMaxAltitude->value();
    obj["blastRadius"]      = m_bombBlastRadius->value();
    obj["effectiveRadius"]  = m_bombEffectiveRadius->value();
    obj["peakPressure"]     = m_bombPeakPressure->value();
    obj["warheadType"]      = m_bombWarheadType->text();
    obj["detonationType"]   = kDetonationStrBomb.value(m_bombDetonation->currentIndex(), "Impact");
    obj["timerDelay"]       = m_bombTimerDelay->value();
    return obj;
}

QJsonObject AddWeaponDialog::buildGunJson() const
{
    QJsonObject obj;
    obj["weaponType"]         = "Artillery";  // maps to Gun via Artillery enum
    obj["maxVelocity"]        = m_gunMaxVelocity->value();
    obj["maxRange"]           = m_gunMaxRange->value();
    obj["blastRadius"]        = m_gunBlastRadius->value();
    obj["rearmTime"]          = m_gunRearmTime->value();
    obj["preflightCheckTime"] = m_gunPreflightCheck->value();
    obj["armed"]              = m_gunArmed->isChecked();
    return obj;
}

QJsonObject AddWeaponDialog::buildRocketJson() const
{
    QJsonObject obj;
    obj["weaponType"]     = "Rocket";
    obj["propulsionType"] = kPropulsionStr.value(m_rocketPropulsion->currentIndex(), "SolidRocket");
    obj["thrustMain"]     = m_rocketThrustMain->value();
    obj["thrustBooster"]  = m_rocketThrustBooster->value();
    obj["burnTime"]       = m_rocketBurnTime->value();
    obj["maxRange"]       = m_rocketMaxRange->value();
    obj["blastRadius"]    = m_rocketBlastRadius->value();
    obj["detonationType"] = kDetonationStr.value(m_rocketDetonation->currentIndex(), "Proximity");
    return obj;
}

QJsonObject AddWeaponDialog::buildTorpedoJson() const
{
    QJsonObject obj;
    obj["weaponType"]    = "Torpedo";
    obj["guidanceType"]  = kGuidanceStr.value(m_torpedoGuidance->currentIndex(), "FullyActive");
    obj["maxVelocity"]   = m_torpedoMaxVelocity->value();
    obj["maxRange"]      = m_torpedoMaxRange->value();
    obj["maxAltitude"]   = m_torpedoMaxAltitude->value();   // depth stored here
    obj["blastRadius"]   = m_torpedoBlastRadius->value();
    obj["payloadMass"]   = m_torpedoPayloadMass->value();
    return obj;
}

QJsonObject AddWeaponDialog::buildArtilleryJson() const
{
    QJsonObject obj;
    obj["weaponType"]    = "Artillery";
    obj["maxRange"]      = m_artilleryMaxRange->value();
    obj["maxVelocity"]   = m_artilleryMaxVelocity->value();
    obj["maximumG"]      = m_artilleryMaxG->value();
    obj["blastRadius"]   = m_artilleryBlastRadius->value();
    obj["detonationType"]= kDetonationStr.value(m_artilleryDetonation->currentIndex(),"Impact");
    obj["rearmTime"]     = m_artilleryRearmTime->value();
    return obj;
}
