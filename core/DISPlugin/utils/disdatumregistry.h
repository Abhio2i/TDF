#ifndef DISDATUMREGISTRY_H
#define DISDATUMREGISTRY_H

#include <cstdint>
#include <QByteArray>

class Platform;
class Hierarchy;

// =============================================================================
// Real SISO-REF-010 / EBV-DOC Datum IDs this engine answers for
// DataQuery(18)/SetData(19)/Data(20). The Datum ID *meaning* is
// standardized by DIS; the byte layout of multi-field Variable Datum
// values is implementation-defined by the standard's own design — the
// layout used here is documented inline in the .cpp for any receiving
// developer on the other end of the wire.
// =============================================================================
namespace DISDatumID {
constexpr uint32_t WeaponSystemStatus  = 100200; // Fixed    — read-only
constexpr uint32_t EntityTrackIdentity = 100500; // Variable — read-only
constexpr uint32_t EngagementData      = 100600; // Fixed    — read/write (ROI)
constexpr uint32_t EnvironmentalData   = 101100; // Variable — read-only
constexpr uint32_t TimeSynchronization = 101400; // Fixed    — read-only
}

// =============================================================================
// DISDatumRegistry
// Maps a Datum ID to real engine state on a given Platform. Stateless,
// static — safe to call from anywhere already holding valid pointers.
// =============================================================================
class DISDatumRegistry {
public:
    // Fixed Datum — false if datumID is unknown or platform is null.
    // Fixed Datum — false if datumID is unknown or platform is null.
    // currentSimTimeSeconds is supplied by the caller (only used by
    // TimeSynchronization) — kept out of this file to avoid pulling in
    // a Simulation dependency here; see .cpp for why.
    static bool getFixed(uint32_t datumID, Platform* platform,
                         uint32_t currentSimTimeSeconds, uint32_t& outValue);
    // false if datumID is read-only or unknown — value is left unchanged.
    static bool setFixed(uint32_t datumID, Platform* platform, uint32_t value);

    // Variable Datum — hierarchy is only used by EnvironmentalData (needs
    // to search Specialzones); pass nullptr for IDs that don't need it.
    static bool getVariable(uint32_t datumID, Platform* platform,
                            Hierarchy* hierarchy, QByteArray& outData);
};

#endif // DISDATUMREGISTRY_H
