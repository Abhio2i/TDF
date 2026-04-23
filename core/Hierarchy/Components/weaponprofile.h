// =============================================================================
// FILE:        weaponprofile.h
// MODULE:      Weapon Profile Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the WeaponProfile class, which acts as a container for
//              weapon configurations (missiles, bombs, torpedoes, rockets,
//              flares, chaff) attached to a Platform entity. Manages a
//              collection of Weapon objects, supports lookup by ID, and
//              provides JSON serialization. Follows the same pattern as
//              SensorProfile and RadioProfile.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//   Rev 2  [Date]  Added auto-component signals and weapon type support.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef WEAPONPROFILE_H
#define WEAPONPROFILE_H

#include "./component.h"
#include "core/Hierarchy/EntityProfiles/weapon.h"
#include <QObject>
#include <QJsonObject>

class Hierarchy;

// =============================================================================
// CLASS: WeaponProfile
//
// DESCRIPTION: Represents a weapon configuration profile that can be attached
//              to a Platform entity. Holds a collection of Weapon objects of
//              various types (Missile, Bomb, Torpedo, Artillery, Rocket,
//              Flare, Chaff). Provides JSON serialization for saving/loading
//              profiles and supports automatic signal emission for UI/network.
// =============================================================================
class WeaponProfile : public QObject, public Component
{
    Q_OBJECT
public:
    WeaponProfile(Hierarchy* h);
    ComponentType Typo() const override { return ComponentType::WeaponProfile; }

    // =========================================================================
    // SECTION: Weapon Profile Properties
    // DESCRIPTION: Core state and reference to global weapon registry.
    // =========================================================================
    bool Active = true;                                   //!< Whether this weapon profile is active
    std::unordered_map<std::string, Weapon*> *weapons;    //!< Pointer to map of Weapon entries

    Weapon* getWeapon(const std::string& id) const;       //!< Retrieves a weapon by its ID

    // =========================================================================
    // SECTION: Custom Parameters
    // DESCRIPTION: Extensible key-value store for user-defined weapon data.
    // =========================================================================
    QJsonObject AdditionalParameters;                          //!< Custom key-value parameters

    // =========================================================================
    // SECTION: Subcomponent Management
    // DESCRIPTION: Overloaded methods to add, remove, and update weapons.
    //              data1 = weapon type: "Missile" | "Bomb" | "Torpedo" |
    //                      "Artillery" | "Rocket" | "Flare" | "Chaff"
    //              data2 = source weapon ID to copy config from (optional)
    //              data3 = unused (reserved)
    // =========================================================================
    void addSubComponent(std::string name,
                         QString data1 = "",
                         QString data2 = "",
                         QJsonObject data3 = QJsonObject()) override;

    void removeSubComponent(std::string ID)                             override;
    QJsonObject getsubComponentData(std::string ID)              const  override;
    void        updateSubComponent (std::string ID,
                            const QJsonObject& obj)             override;
    void        renameSubComponent (std::string ID, QString newName)    override;

    // Serialization
    QJsonObject toJson()                                          const  override;
    void        fromJson(const QJsonObject& obj)                        override;

private:
    // Helper to emit signals when a weapon is added (UI/network sync)
    static void emitAutoComponentSignals(Hierarchy* parent, Weapon* weapon);
};

#endif // WEAPONPROFILE_H
