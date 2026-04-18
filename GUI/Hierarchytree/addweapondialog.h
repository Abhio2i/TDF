#ifndef ADDWEAPONDIALOG_H
#define ADDWEAPONDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QStackedWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QScrollArea>
#include <QJsonObject>
#include <QCompleter>
#include <QStringListModel>
#include <QMap>
#include <QVariantList>

#include "core/Hierarchy/EntityProfiles/weapon.h"

class Hierarchy;

class AddWeaponDialog : public QDialog
{
    Q_OBJECT

public:

    explicit AddWeaponDialog(QWidget* parent = nullptr,
                             Hierarchy* dbHierarchy = nullptr,
                             bool isDatabaseEditor = true);

    // ── results ──────────────────────────────────────────────
    QString            weaponName()        const;
    Weapon::WeaponType weaponType()        const;
    QString            weaponTypeStr()     const;
    QJsonObject        configJson()        const;
    QString            selectedEntityId()  const { return m_selectedEntityId; }

private slots:
    void onTypeChanged(int index);

private:
    // ── Search Entity (scenario/runtime only) ─────────────────
    Hierarchy*          m_dbHierarchy    = nullptr;
    bool                m_isDatabaseEditor = true;
    QLineEdit*          m_searchEdit     = nullptr;
    QCompleter*         m_completer      = nullptr;
    QString             m_selectedEntityId;
    QMap<QString, QVariantList> m_entityMap;   // displayName → {id, profileName}

    void buildSearchSection(QVBoxLayout* root);
    void populateWeaponEntities();
    void applyEntityConfig(const QString& entityId);

    // ── top bar ──────────────────────────────────────────────
    QLineEdit*      m_nameEdit       = nullptr;
    QComboBox*      m_typeCombo      = nullptr;

    // ── stacked panels ───────────────────────────────────────
    QStackedWidget* m_stack          = nullptr;

    enum PanelIndex { PanelMissile=0, PanelBomb, PanelGun,
                      PanelRocket,    PanelTorpedo, PanelArtillery };

    // ── Missile widgets ──────────────────────────────────────
    QComboBox*      m_missileGuidance      = nullptr;
    QDoubleSpinBox* m_missileSeekerRange   = nullptr;
    QDoubleSpinBox* m_missileSeekerFOV     = nullptr;
    QComboBox*      m_missilePropulsion    = nullptr;
    QDoubleSpinBox* m_missileThrustMain    = nullptr;
    QDoubleSpinBox* m_missileBurnTime      = nullptr;
    QDoubleSpinBox* m_missileBlastRadius   = nullptr;
    QDoubleSpinBox* m_missileMaxRange      = nullptr;
    QDoubleSpinBox* m_missileMaxVelocity   = nullptr;
    QComboBox*      m_missileDetonation    = nullptr;
    QDoubleSpinBox* m_missileProximity     = nullptr;

    // ── Bomb widgets ─────────────────────────────────────────
    QDoubleSpinBox* m_bombTotalMass        = nullptr;
    QDoubleSpinBox* m_bombPayloadMass      = nullptr;
    QDoubleSpinBox* m_bombBlastRadius      = nullptr;
    QDoubleSpinBox* m_bombEffectiveRadius  = nullptr;
    QDoubleSpinBox* m_bombPeakPressure     = nullptr;
    QComboBox*      m_bombDetonation       = nullptr;
    QDoubleSpinBox* m_bombTimerDelay       = nullptr;
    QLineEdit*      m_bombWarheadType      = nullptr;
    QDoubleSpinBox* m_bombMaxAltitude      = nullptr;

    // ── Gun widgets ──────────────────────────────────────────
    QDoubleSpinBox* m_gunMaxVelocity       = nullptr;
    QDoubleSpinBox* m_gunMaxRange          = nullptr;
    QDoubleSpinBox* m_gunBlastRadius       = nullptr;
    QSpinBox*       m_gunRearmTime         = nullptr;
    QDoubleSpinBox* m_gunPreflightCheck    = nullptr;
    QCheckBox*      m_gunArmed             = nullptr;

    // ── Rocket widgets ───────────────────────────────────────
    QComboBox*      m_rocketPropulsion     = nullptr;
    QDoubleSpinBox* m_rocketThrustMain     = nullptr;
    QDoubleSpinBox* m_rocketThrustBooster  = nullptr;
    QDoubleSpinBox* m_rocketBurnTime       = nullptr;
    QDoubleSpinBox* m_rocketMaxRange       = nullptr;
    QDoubleSpinBox* m_rocketBlastRadius    = nullptr;
    QComboBox*      m_rocketDetonation     = nullptr;

    // ── Torpedo widgets ──────────────────────────────────────
    QDoubleSpinBox* m_torpedoMaxVelocity   = nullptr;
    QDoubleSpinBox* m_torpedoMaxRange      = nullptr;
    QDoubleSpinBox* m_torpedoMaxAltitude   = nullptr;
    QComboBox*      m_torpedoGuidance      = nullptr;
    QDoubleSpinBox* m_torpedoBlastRadius   = nullptr;
    QDoubleSpinBox* m_torpedoPayloadMass   = nullptr;

    // ── Artillery widgets ────────────────────────────────────
    QDoubleSpinBox* m_artilleryMaxRange    = nullptr;
    QDoubleSpinBox* m_artilleryMaxVelocity = nullptr;
    QDoubleSpinBox* m_artilleryBlastRadius = nullptr;
    QDoubleSpinBox* m_artilleryMaxG        = nullptr;
    QSpinBox*       m_artilleryRearmTime   = nullptr;
    QComboBox*      m_artilleryDetonation  = nullptr;

    // ── helpers ───────────────────────────────────────────────
    QWidget*    buildMissilePanel();
    QWidget*    buildBombPanel();
    QWidget*    buildGunPanel();
    QWidget*    buildRocketPanel();
    QWidget*    buildTorpedoPanel();
    QWidget*    buildArtilleryPanel();

    QWidget*    wrapScroll(QWidget* panel);

    static QGroupBox* makeGroup(const QString& title, QFormLayout*& outForm);
    static void       applyDialogStyle(QWidget* w);

    QJsonObject buildMissileJson()    const;
    QJsonObject buildBombJson()       const;
    QJsonObject buildGunJson()        const;
    QJsonObject buildRocketJson()     const;
    QJsonObject buildTorpedoJson()    const;
    QJsonObject buildArtilleryJson()  const;
};

#endif // ADDWEAPONDIALOG_H
