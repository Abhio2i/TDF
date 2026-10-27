// =============================================================================
// FILE:        DISReferenceData.h
// MODULE:      DIS Network Plugin — Interface
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Plain data struct holding the SISO-REF-010 reference tables
//              (Kind, Domain, Country, Category, SubCategory, Specific,
//              Extra) once loaded from siso_reference_data.json by
//              DISReferenceDataLoader. No Qt types — matches the plain
//              std::string/std::vector style of DISConfig.h.
// =============================================================================
#ifndef DISREFERENCEDATA_H
#define DISREFERENCEDATA_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

// =============================================================================
// DISRefEntry
// One (value, name) row from a SISO-REF-010 enumeration table — e.g.
// {1, "Platform"} or {164, "India (IND)"}.
// =============================================================================
struct DISRefEntry {
    uint16_t    value = 0;
    std::string name;
};

// =============================================================================
// DISReferenceData
// In-memory representation of siso_reference_data.json. Nesting mirrors
// the JSON file exactly:
//   domain[kind]                                  -> list
//   category[kind][domain]                        -> list
//   subcategory[kind][domain][category]           -> list
//   specific[kind][domain][category][subcategory] -> list
//   extra[kind][domain][category][subcategory]    -> list  (same depth as
//     specific — Extra is rarely populated in the real standard; if a
//     project genuinely needs Extra keyed by Specific value too, extend
//     this one more level when that need actually arises, same
//     self-service philosophy as everything else in this file)
// =============================================================================
struct DISReferenceData {
    std::vector<DISRefEntry> kind;
    std::vector<DISRefEntry> country;

    std::map<uint8_t, std::vector<DISRefEntry>> domain;

    std::map<uint8_t, std::map<uint8_t,
                               std::vector<DISRefEntry>>> category;

    std::map<uint8_t, std::map<uint8_t, std::map<uint8_t,
                                                 std::vector<DISRefEntry>>>> subcategory;

    std::map<uint8_t, std::map<uint8_t, std::map<uint8_t, std::map<uint8_t,
                                                                   std::vector<DISRefEntry>>>>> specific;

    std::map<uint8_t, std::map<uint8_t, std::map<uint8_t, std::map<uint8_t,
                                                                   std::vector<DISRefEntry>>>>> extra;

    bool loaded = false;
};

#endif // DISREFERENCEDATA_H
