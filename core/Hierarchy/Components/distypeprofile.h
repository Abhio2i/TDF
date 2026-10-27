// =============================================================================
// FILE:        distypeprofile.h
// MODULE:      Tactical Simulation Entity Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the DISTypeProfile component. Holds an explicit,
//              per-entity override of the IEEE 1278.1 / SISO-REF-010
//              Entity Type record (Kind, Domain, Country, Category,
//              SubCategory, Specific, Extra).
//
//              When useOverride is false (the default), the DIS plugin
//              derives Kind/Domain/Category/etc. from the entity's own
//              TDF Category/SubCategory exactly as it always has —
//              this component has zero effect on any entity that does
//              not explicitly opt in.
//
//              When useOverride is true, the DIS plugin uses the values
//              stored here verbatim when building outgoing PDUs.
//
//              Dropdown option lists for every field are sourced at
//              runtime from DISReferenceDataLoader, which reads
//              ~/TDF/DIS/siso_reference_data.json (extracted from the
//              real SISO-REF-010 v35 standard). Kind/Domain/Country and
//              Category are fully populated; SubCategory/Specific/Extra
//              grow as entries are added to that JSON file by hand — no
//              rebuild required. A field with no matching entry in the
//              table simply displays its bare numeric value, and stays
//              editable so a known code can always be entered directly.
//
// AUTHOR:       O2I Development Team
// REVIEWED BY:  [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Sep 2026  Initial implementation — per-entity SISO-REF-010
//                     Entity Type override, opt-in, non-breaking.
//   Rev 2  Sep 2026  Wired to DISReferenceDataLoader — real SISO-REF-010
//                     v35 data (Kind/Domain/Country/Category) replaces
//                     placeholder option lists.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef DISTYPEPROFILE_H
#define DISTYPEPROFILE_H

#include "./component.h"
#include "core/DISPlugin/interface/DISReferenceData.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <cstdint>
#include <vector>

// =============================================================================
// CLASS: DISTypeProfile
//
// DESCRIPTION: Component holding an explicit IEEE 1278.1 Entity Type
//              record (Kind/Domain/Country/Category/SubCategory/
//              Specific/Extra) that can optionally override TDF's
//              automatically-derived DIS entity type for a single entity.
// =============================================================================
class DISTypeProfile : public Component
{
public:
    DISTypeProfile();

    ComponentType Typo() const override { return ComponentType::DISTypeProfile; }

    // =========================================================================
    // SECTION: Override Switch
    // DESCRIPTION: When false, the DIS plugin ignores every field below and
    //              derives the entity type exactly as it did before this
    //              component existed. When true, the fields below are sent
    //              on the wire verbatim.
    // =========================================================================
    bool useOverride = false;

    // =========================================================================
    // SECTION: IEEE 1278.1 / SISO-REF-010 Entity Type Fields
    // DESCRIPTION: The 7-field Entity Type record as defined by the DIS
    //              standard. Defaults are safe/inert values (Other/Platform)
    //              until useOverride is explicitly set true.
    // =========================================================================
    uint8_t  kind        = 1;   // 1 = Platform (SISO-REF-010 Kind table)
    uint8_t  domain      = 0;   // 0 = Other    (SISO-REF-010 Domain table)
    uint16_t country     = 0;   // 0 = Other    (SISO-REF-010 Country table)
    uint8_t  category    = 0;
    uint8_t  subcategory = 0;
    uint8_t  specific    = 0;
    uint8_t  extra       = 0;

    // =========================================================================
    // SECTION: Component Interface Overrides
    // =========================================================================
    void init(Entity* entity) override;
    void Awake() override;
    void Start() override;

    // --- Sub-Component Management (no sub-components — same as Transform) ---
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;

    // --- Serialization ---
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

private:
    // =========================================================================
    // SECTION: Option-list builders — read from DISReferenceDataLoader.
    // DESCRIPTION: Domain/Category/SubCategory/Specific/Extra all depend on
    //              the entity's currently-set Kind/Domain/Category/etc., so
    //              these are instance methods (not static) reading the
    //              component's own current field values.
    // =========================================================================
    QJsonArray kindOptions() const;
    QJsonArray domainOptions() const;
    QJsonArray countryOptions() const;
    QJsonArray categoryOptions() const;
    QJsonArray subcategoryOptions() const;
    QJsonArray specificOptions() const;
    QJsonArray extraOptions() const;

    QString kindName(uint8_t value) const;
    QString domainName(uint8_t value) const;
    QString countryName(uint16_t value) const;
    QString categoryName(uint8_t value) const;
    QString subcategoryName(uint8_t value) const;
    QString specificName(uint8_t value) const;
    QString extraName(uint8_t value) const;

    // Extracts the leading integer from a "NNN - Name" display string, or
    // a plain numeric string. Used to parse dropdown/edited values back
    // into raw SISO codes.
    static uint16_t parseLeadingNumber(const QString& displayValue);

    // Given a reference table and a numeric value, returns "value - Name"
    // if found, or the bare numeric value if not found (e.g. because that
    // branch of siso_reference_data.json hasn't been populated yet).
    static QString formatEntry(const std::vector<DISRefEntry>& table,
                               uint16_t value, int padWidth);
};

#endif // DISTYPEPROFILE_H
