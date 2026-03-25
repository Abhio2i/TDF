
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
// #include "core/Hierarchy/EntityProfiles/iff.h"
// #include "core/Hierarchy/EntityProfiles/radio.h"
// #include "core/Hierarchy/EntityProfiles/sensor.h"
class Hierarchy;
class AttachedEnitities;
class Radio;
class Weapon;
class Sensor;
class IFF;
class Entity: public QObject
{
    Q_OBJECT

public:
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
        RedTeam,
        BlueTeam,
        GreenTeam,
        YellowTeam,
        GreyTeam,
        AlphaTeam,
        BetaTeam,
        GammaTeam
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
    const std::string TeamNames[8] = { "RedTeam",
                                      "BlueTeam",
                                      "GreenTeam",
                                      "YellowTeam",
                                      "GreyTeam",
                                      "AlphaTeam",
                                      "BetaTeam",
                                      "GammaTeam"};
    enum Category{
        Aircraft,
        Helicopter,
        Ship,
        Submarine,
        Tank/*,
        Missile,
        Bomb*/
    };
    const std::string CategoryNames[5] = {"Aircraft",
                                          "Helicopter",
                                          "Ship",
                                          "Submarine",
                                          "Tank"/*,
                                          "Missile",
                                          "Bomb"*/};
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
    Category category = Entity::Category::Aircraft;
    Country country = Entity::Country::INDIA;
    Team team = Entity::Team::GreyTeam;
    float Health = 100;
    float fuel = 100;
    bool isVictom = false;
    bool engaged = false;
    float detectionCount = 0;
    float weaponcount = 0;
    float hitcount = 0;
    std::unordered_map<std::string, std::shared_ptr<Parameter>> parameters;
    std::vector<Radio*> radioList;
    std::vector<Sensor*> sensorList;
    std::vector<IFF*> iffList;
    void addRadio(Radio* radio);
    void removeRadio(Radio* radio);
    void clearRadios();
    void addSensor(Sensor* sensor);
    void removeSensor(Sensor* sensor);
    void clearSensors();
    void addIFF(IFF* iff);
    void removeIFF(IFF* iff);
    void clearIFFs();

    std::vector<Weapon*> weaponList;
    void addWeapon(Weapon* weapon);
    void removeWeapon(Weapon* weapon);
    void clearWeapons();

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
