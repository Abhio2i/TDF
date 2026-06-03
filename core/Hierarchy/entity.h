// =============================================================================
// FILE:        entity.h
// MODULE:      Tactical Simulation Entity Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Defines the abstract Entity base class. This is the fundamental
//               building block for all simulation objects (Aircraft, Ships, etc.).
//               It manages common attributes like Team, Country, Health, and
//               provides an interface for dynamic component attachment.
//
// AUTHOR:       Pankaj Chauhan
// REVIEWED BY:  [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Sep 2025  Initial implementation for TDF project.
//   Rev 2  Mar 2026  Integrated Radio, Sensor, and IFF tracking.
//   Rev 3  Apr 2026  Added DO-178C compliant documentation and health metrics.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef ENTITY_H
#define ENTITY_H

#include "core/Hierarchy/Components/attachedenitities.h"
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <core/Hierarchy/Components/transform.h>
#include <core/Hierarchy/Components/trajectory.h>
#include <core/Hierarchy/Components/rigidbody.h>
#include <core/Hierarchy/Components/collider.h>
#include <core/Hierarchy/Components/networkobject.h>
#include <core/Hierarchy/Components/meshrenderer2d.h>
#include <core/Hierarchy/Components/component.h>
#include <core/Hierarchy/Components/dynamicmodel.h>
#include <core/Hierarchy/Components/mission.h>

// Forward declarations to reduce compile time
class Hierarchy;
class AttachedEnitities;
class Radio;
class Weapon;
class Sensor;
class IFF;

// =============================================================================
// CLASS: Entity (Abstract)
//
// DESCRIPTION: Base class for all simulation actors. Defines the interface for
//              spawning, component management, and state serialization.
// =============================================================================
class Entity: public QObject
{
    Q_OBJECT

public:
    // =========================================================================
    // SECTION: Enumerations & Constants
    // DESCRIPTION: Definitions for Country, Team, and Category classifications.
    // =========================================================================
    enum Country {
        AFGHANISTAN, ALBANIA, ALGERIA, ANDORRA, ANGOLA, ANTIGUA_AND_BARBUDA, ARGENTINA, ARMENIA, AUSTRALIA, AUSTRIA,
        AZERBAIJAN, BAHAMAS, BAHRAIN, BANGLADESH, BARBADOS, BELARUS, BELGIUM, BELIZE, BENIN, BHUTAN,
        BOLIVIA, BOSNIA_AND_HERZEGOVINA, BOTSWANA, BRAZIL, BRUNEI, BULGARIA, BURKINA_FASO, BURUNDI, CABO_VERDE, CAMBODIA,
        CAMEROON, CANADA, CENTRAL_AFRICAN_REPUBLIC, CHAD, CHILE, CHINA, COLOMBIA, COMOROS, CONGO_BRAZZAVILLE, CONGO_KINSHASA,
        COSTA_RICA, COTE_D_IVOIRE, CROATIA, CUBA, CYPRUS, CZECH_REPUBLIC, DENMARK, DJIBOUTI, DOMINICA, DOMINICAN_REPUBLIC,
        ECUADOR, EGYPT, EL_SALVADOR, EQUATORIAL_GUINEA, ERITREA, ESTONIA, ESWATINI, ETHIOPIA, FIJI, FINLAND,
        FRANCE, GABON, GAMBIA, GEORGIA, GERMANY, GHANA, GREECE, GRENADA, GUATEMALA, GUINEA,
        GUINEA_BISSAU, GUYANA, HAITI, HOLY_SEE, HONDURAS, HUNGARY, ICELAND, INDIA, INDONESIA, IRAN,
        IRAQ, IRELAND, ISRAEL, ITALY, JAMAICA, JAPAN, JORDAN, KAZAKHSTAN, KENYA, KIRIBATI,
        KUWAIT, KYRGYZSTAN, LAOS, LATVIA, LEBANON, LESOTHO, LIBERIA, LIBYA, LIECHTENSTEIN, LITHUANIA,
        LUXEMBOURG, MADAGASCAR, MALAWI, MALAYSIA, MALDIVES, MALI, MALTA, MARSHALL_ISLANDS, MAURITANIA, MAURITIUS,
        MEXICO, MICRONESIA, MOLDOVA, MONACO, MONGOLIA, MONTENEGRO, MOROCCO, MOZAMBIQUE, MYANMAR, NAMIBIA,
        NAURU, NEPAL, NETHERLANDS, NEW_ZEALAND, NICARAGUA, NIGER, NIGERIA, NORTH_KOREA, NORTH_MACEDONIA, NORWAY,
        OMAN, PAKISTAN, PALAU, PALESTINE_STATE, PANAMA, PAPUA_NEW_GUINEA, PARAGUAY, PERU, PHILIPPINES, POLAND,
        PORTUGAL, QATAR, ROMANIA, RUSSIA, RWANDA, SAINT_KITTS_AND_NEVIS, SAINT_LUCA, SAINT_VINCENT_AND_THE_GRENADINES, SAMOA, SAN_MARINO,
        SAO_TOME_AND_PRINCIPE, SAUDI_ARABIA, SENEGAL, SERBIA, SEYCHELLES, SIERRA_LEONE, SINGAPORE, SLOVAKIA, SLOVENIA, SOLOMON_ISLANDS,
        SOMALIA, SOUTH_AFRICA, SOUTH_KOREA, SOUTH_SUDAN, SPAIN, SRI_LANKA, SUDAN, SURINAME, SWEDEN, SWITZERLAND,
        SYRIA, TAJIKISTAN, TANZANIA, THAILAND, TIMOR_LESTE, TOGO, TONGA, TRINIDAD_AND_TOBAGO, TUNISIA, TURKEY,
        TURKMENISTAN, TUVALU, UGANDA, UKRAINE, UNITED_ARAB_EMIRATES, UNITED_KINGDOM, UNITED_STATES_OF_AMERICA, URUGUAY, UZBEKISTAN, VANUATU,
        VENEZUELA, VIETNAM, YEMEN, ZAMBIA, ZIMBABWE
    };

    enum Team{
        RedTeam, BlueTeam, GreenTeam, YellowTeam, GreyTeam, AlphaTeam, BetaTeam, GammaTeam
    };

    const std::string CountryNames[195] = {
        "AFGHANISTAN", "ALBANIA", "ALGERIA", "ANDORRA", "ANGOLA", "ANTIGUA_AND_BARBUDA", "ARGENTINA", "ARMENIA", "AUSTRALIA", "AUSTRIA",
        "AZERBAIJAN", "BAHAMAS", "BAHRAIN", "BANGLADESH", "BARBADOS", "BELARUS", "BELGIUM", "BELIZE", "BENIN", "BHUTAN",
        "BOLIVIA", "BOSNIA_AND_HERZEGOVINA", "BOTSWANA", "BRAZIL", "BRUNEI", "BULGARIA", "BURKINA_FASO", "BURUNDI", "CABO_VERDE", "CAMBODIA",
        "CAMEROON", "CANADA", "CENTRAL_AFRICAN_REPUBLIC", "CHAD", "CHILE", "CHINA", "COLOMBIA", "COMOROS", "CONGO_BRAZZAVILLE", "CONGO_KINSHASA",
        "COSTA_RICA", "COTE_D_IVOIRE", "CROATIA", "CUBA", "CYPRUS", "CZECH_REPUBLIC", "DENMARK", "DJIBOUTI", "DOMINICA", "DOMINICAN_REPUBLIC",
        "ECUADOR", "EGYPT", "EL_SALVADOR", "EQUATORIAL_GUINEA", "ERITREA", "ESTONIA", "ESWATINI", "ETHIOPIA", "FIJI", "FINLAND",
        "FRANCE", "GABON", "GAMBIA", "GEORGIA", "GERMANY", "GHANA", "GREECE", "GRENADA", "GUATEMALA", "GUINEA",
        "GUINEA_BISSAU", "GUYANA", "HAITI", "HOLY_SEE", "HONDURAS", "HUNGARY", "ICELAND", "INDIA", "INDONESIA", "IRAN",
        "IRAQ", "IRELAND", "ISRAEL", "ITALY", "JAMAICA", "JAPAN", "JORDAN", "KAZAKHSTAN", "KENYA", "KIRIBATI",
        "KUWAIT", "KYRGYZSTAN", "LAOS", "LATVIA", "LEBANON", "LESOTHO", "LIBERIA", "LIBYA", "LIECHTENSTEIN", "LITHUANIA",
        "LUXEMBOURG", "MADAGASCAR", "MALAWI", "MALAYSIA", "MALDIVES", "MALI", "MALTA", "MARSHALL_ISLANDS", "MAURITANIA", "MAURITIUS",
        "MEXICO", "MICRONESIA", "MOLDOVA", "MONACO", "MONGOLIA", "MONTENEGRO", "MOROCCO", "MOZAMBIQUE", "MYANMAR", "NAMIBIA",
        "NAURU", "NEPAL", "NETHERLANDS", "NEW_ZEALAND", "NICARAGUA", "NIGER", "NIGERIA", "NORTH_KOREA", "NORTH_MACEDONIA", "NORWAY",
        "OMAN", "PAKISTAN", "PALAU", "PALESTINE_STATE", "PANAMA", "PAPUA_NEW_GUINEA", "PARAGUAY", "PERU", "PHILIPPINES", "POLAND",
        "PORTUGAL", "QATAR", "ROMANIA", "RUSSIA", "RWANDA", "SAINT_KITTS_AND_NEVIS", "SAINT_LUCA", "SAINT_VINCENT_AND_THE_GRENADINES", "SAMOA", "SAN_MARINO",
        "SAO_TOME_AND_PRINCIPE", "SAUDI_ARABIA", "SENEGAL", "SERBIA", "SEYCHELLES", "SIERRA_LEONE", "SINGAPORE", "SLOVAKIA", "SLOVENIA", "SOLOMON_ISLANDS",
        "SOMALIA", "SOUTH_AFRICA", "SOUTH_KOREA", "SOUTH_SUDAN", "SPAIN", "SRI_LANKA", "SUDAN", "SURINAME", "SWEDEN", "SWITZERLAND",
        "SYRIA", "TAJIKISTAN", "TANZANIA", "THAILAND", "TIMOR_LESTE", "TOGO", "TONGA", "TRINIDAD_AND_TOBAGO", "TUNISIA", "TURKEY",
        "TURKMENISTAN", "TUVALU", "UGANDA", "UKRAINE", "UNITED_ARAB_EMIRATES", "UNITED_KINGDOM", "UNITED_STATES_OF_AMERICA", "URUGUAY", "UZBEKISTAN", "VANUATU",
        "VENEZUELA", "VIETNAM", "YEMEN", "ZAMBIA", "ZIMBABWE"
    };

    const std::string TeamNames[8] = {
        "RedTeam", "BlueTeam", "GreenTeam", "YellowTeam", "GreyTeam", "AlphaTeam", "BetaTeam", "GammaTeam"
    };

    enum Category{
        Air, Ground, Marine
    };

    const std::string CategoryNames[3] = {
        "Air", "Ground", "Marine"
    };

    enum SubAirCategory{
        Aircraft, Helicopter, UAV
    };

    const std::string AirCategoryNames[3] = {
        "Aircraft", "Helicopter", "UAV"
    };

    enum SubGroundCategory{
        Tank, GroundRadar, Human
    };

    const std::string GroundCategoryNames[3] = {
        "Tank", "GroundRadar", "Human"
    };

    enum SubMarineCategory{
        Ship, Frigate, Submarine
    };

    const std::string MarineCategoryNames[3] = {
        "Ship", "Frigate", "Submarine"
    };

    // =========================================================================
    // SECTION: Lifecycle & Hierarchy
    // DESCRIPTION: Constructor, destructor, and basic tree connectivity.
    // =========================================================================
    Entity(Hierarchy* h);
    ~Entity();
    Hierarchy* root;
    std::string Name;
    bool Active = true;
    bool isDestroy = false;
    bool collisionWarning = false;
    std::string ID;
    std::string parentID;
    Constants::EntityType type;

    // =========================================================================
    // SECTION: State & Metrics
    // DESCRIPTION: Physical status, tactical affiliation, and health variables.
    // =========================================================================
    Category category = Entity::Category::Air;
    SubAirCategory airCategory = Entity::SubAirCategory::Aircraft;
    SubGroundCategory groundCategory = Entity::SubGroundCategory::Tank;
    SubMarineCategory marineCategory = Entity::SubMarineCategory::Ship;

    Country country = Entity::Country::INDIA;
    Team team = Entity::Team::GreyTeam;
    double illumination = 1.0;  // day/night factor
    double glintFactor  = 1.0;  // reflection boost
    float Health = 100;
    float fuel = 100;
    bool isVictom = false;
    bool engaged = false;
    float detectionCount = 0;
    float weaponcount = 0;
    float hitcount = 0;
    bool isRemoteDISEntity = false;  // true = owned by remote DIS node, no physics

    // For EO/IR Sensor By Himanshu
    float surfaceTemp = 40;            // Celsius
    float specularReflectivity = 0.05;   // 0.0 to 1.0 (e.g., 0.05 for matte paint, 0.9 for glass/polished metal)

    // =========================================================================
    // SECTION: Sub-System Management (Radios, Sensors, Weapons)
    // DESCRIPTION: Containers and methods for managing attached tactical equipment.
    // =========================================================================
    std::unordered_map<std::string, std::shared_ptr<Parameter>> parameters;
    std::vector<Radio*> radioList;
    std::vector<Sensor*> sensorList;
    std::vector<IFF*> iffList;
    std::vector<Weapon*> weaponList;

    void addRadio(Radio* radio);
    void removeRadio(Radio* radio);
    void clearRadios();
    void addSensor(Sensor* sensor);
    void removeSensor(Sensor* sensor);
    void clearSensors();
    void addIFF(IFF* iff);
    void removeIFF(IFF* iff);
    void clearIFFs();
    void addWeapon(Weapon* weapon);
    void removeWeapon(Weapon* weapon);
    void clearWeapons();

    // =========================================================================
    // SECTION: Virtual Interface (DO-178C Compliance)
    // DESCRIPTION: Mandatory methods for derived simulation objects.
    // =========================================================================
    virtual void spawn() = 0;
    virtual std::vector<std::string> getSupportedComponents() = 0;
    virtual void addComponent(std::string name) = 0;
    virtual void removeComponent(std::string name) = 0;
    virtual QJsonObject getComponent(std::string name) = 0;
    virtual void updateComponent(QString name, const QJsonObject& obj) = 0;

    virtual QJsonObject toJson() const = 0;
    virtual void fromJson(const QJsonObject& obj) = 0;
};

#endif // ENTITY_H
