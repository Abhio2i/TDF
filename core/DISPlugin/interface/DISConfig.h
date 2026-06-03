// =============================================================================
// FILE:        DISConfig.h
// MODULE:      DIS Network Plugin — Interface
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Configuration structs for the DIS network plugin.
//              These are pure data structures — no Qt, no engine types,
//              no DIS library internals. Any engine can fill these.
//              Loaded from dis_config.json at startup.
//
// AUTHOR:      O2I Engineering
// STANDARD:    IEEE 1278.1-2012 (DIS7), backward compatible with DIS6
// =============================================================================

#ifndef DISCONFIG_H
#define DISCONFIG_H

#include <string>
#include <vector>
#include <cstdint>

// =============================================================================
// PDU type identifiers — matches DIS standard PDU type numbers
// Used in DISPDUConfig to enable/disable per PDU type
// =============================================================================
namespace DISPDUType {
    constexpr uint8_t EntityState              = 1;
    constexpr uint8_t Fire                     = 2;
    constexpr uint8_t Detonation               = 3;
    constexpr uint8_t Collision                = 4;
    constexpr uint8_t StartResume              = 13;
    constexpr uint8_t StopFreeze               = 14;
    constexpr uint8_t Acknowledge              = 15;
    constexpr uint8_t ElectromagneticEmission  = 23;
    constexpr uint8_t Designator               = 24;
    constexpr uint8_t Transmitter              = 25;
    constexpr uint8_t Signal                   = 26;
    constexpr uint8_t IFF                      = 28;
    constexpr uint8_t RemoveEntity             = 12;
    constexpr uint8_t CreateEntity             = 11;
    constexpr uint8_t DataQuery                = 18;
    constexpr uint8_t Data                     = 20;
    constexpr uint8_t SetData                  = 19;
    constexpr uint8_t TransferOwnership        = 35;
}

// =============================================================================
// DISPDUConfig
// Per-PDU send/receive/trace settings
// Mirrors the STAGE Messages tab exactly
// =============================================================================
struct DISPDUConfig {
    uint8_t     pduType     = 0;
    std::string pduName;            // human readable "EntityState", "Fire" etc
    bool        send        = true;
    bool        receive     = true;
    std::string traceLevel  = "Error"; // None / Error / Warning / Basic / Full
};

// =============================================================================
// DISPeerOverride
// Per-peer DIS version and domain config
// Loaded from dis_config.json peer_overrides array
// =============================================================================
struct DISPeerOverride {
    std::string address;        // IP address of peer e.g. "192.168.10.50"
    std::string name;           // human label e.g. "DRDO_LAND_SIM"
    uint8_t     disVersion = 6; // 6 or 7
    std::vector<uint8_t> domains; // domains this peer owns [1]=Land [2]=Air etc
};

// =============================================================================
// DISConfig
// Master configuration struct for the entire DIS plugin
// Loaded once at startup from dis_config.json
// Can be reloaded at runtime via DISUIBridge
// =============================================================================
struct DISConfig {

    // ── Connection ────────────────────────────────────────────────────────────
    std::string multicastGroup  = "239.255.0.1"; // standard DIS multicast
    //std::string unicastPeerAddress = "";   // empty = multicast (default), set IP = unicast mode
    // Unicast peer list — used when connectionMode is "Unicast"
    struct UnicastPeer {
        std::string ip;
        uint16_t    port = 3000;
    };
    std::vector<UnicastPeer> unicastPeers;
    uint16_t    port            = 3000;           // standard DIS port
    uint8_t     ttl                 = 1;
    int         receiveBufferSize   = 65536;
    bool        loopbackEnabled     = true;
    float       positionThresholdMeters  = 1.0f;
    float       heartbeatIntervalSeconds = 5.0f;
    std::string interfaceAddr   = "";             // bind to specific NIC, empty=any
    int         receiveBuffer   = 60000;          // UDP receive buffer bytes
    std::string connectionMode  = "Multicast";   // Multicast or Unicast

    // ── DIS Identifier ───────────────────────────────────────────────────────
    uint16_t    siteID          = 1;
    uint16_t    applicationID   = 1;  // application identifier (1-65535)
    uint8_t     exerciseID      = 1;   // exercise identifier (1-255)
    uint16_t    countryCode     = 164;  // SISO-REF-010: 164=India
    // ── Version ───────────────────────────────────────────────────────────────
    uint8_t     defaultVersion  = 7;              // 6 or 7, applied to all PDUs
    bool        ignoreOtherVersions = false;      // if true, drop non-matching versions

    // ── Domain Ownership ─────────────────────────────────────────────────────
    // Which DIS domains THIS machine owns and sends EntityState PDUs for
    // 1=Land  2=Air  3=Surface  4=Subsurface  5=Space
    std::vector<uint8_t> ownedDomains = {1, 2, 3, 4}; // default owns all

    // ── Peer Overrides ────────────────────────────────────────────────────────
    // Per-peer DIS version override (e.g. DRDO machine uses DIS6)
    std::vector<DISPeerOverride> peerOverrides;

    // ── Send Rates ───────────────────────────────────────────────────────────
    int         entitySendRateHz    = 5;    // EntityState PDU rate for platforms
    int         missileSendRateHz   = 20;   // EntityState PDU rate for weapons

    // ── Dead Reckoning Thresholds ─────────────────────────────────────────────
    float       positionThresholdM  = 1.0f; // meters, send if exceeded
    float       angleThresholdDeg   = 3.0f; // degrees, send if exceeded
    float       heartbeatSec        = 5.0f; // send anyway after this many seconds
    uint8_t     defaultDRAlgorithm  = 2;    // 2=FPW (most common, STAGE default)
    // 1=Static 3=RPW 4=RVW 5=FVW 6=FPB 9=FVB

    // ── Behaviour ────────────────────────────────────────────────────────────
    bool        useMarkings         = true;  // send entity marking (name)
    bool        useAbsTimestamp     = false; // absolute vs relative timestamp
    bool        discardLateMessages = true;  // drop PDUs older than threshold
    int         lateMessageThresholdMs = 2000;  // discard if older than 2 seconds
    int         ackTimeoutSec       = 30;    // ownership transfer ack timeout
    int         autoTimeoutSec      = 30;    // ownership transfer auto timeout

    // ── Per-PDU Settings ─────────────────────────────────────────────────────
    // Which PDUs to send and receive
    // Populated with defaults matching STAGE configuration
    std::vector<DISPDUConfig> pduConfigs;
    std::string networkInterface = "";
    // ── Trace / Logging ───────────────────────────────────────────────────────
    std::string globalTraceLevel    = "None"; // None/Error/Warning/Basic/Full

    // ── Config file path (runtime, not serialized) ────────────────────────────
    std::string configFilePath      = "dis_config.json";

};

// =============================================================================
// DISDefaultConfig
// Factory function — returns a DISConfig pre-populated with
// defaults that match STAGE DIS6 configuration exactly
// Call this once then customise
// =============================================================================
inline DISConfig DISDefaultConfig() {
    DISConfig cfg;

    // Populate PDU configs matching STAGE Messages tab defaults
    // (Send=Yes, Receive=Yes, Trace=Error) unless STAGE shows otherwise
    auto addPDU = [&](uint8_t type, const std::string& name,
                      bool send, bool receive) {
        DISPDUConfig p;
        p.pduType    = type;
        p.pduName    = name;
        p.send       = send;
        p.receive    = receive;
        p.traceLevel = "Error";
        cfg.pduConfigs.push_back(p);
    };

    // Phase 1 PDUs — matches STAGE Messages tab
    addPDU(DISPDUType::EntityState,             "EntityState",             true,  true);
    addPDU(DISPDUType::Fire,                    "Fire",                    true,  false); // STAGE recv=No
    addPDU(DISPDUType::Detonation,              "Detonation",              true,  true);
    addPDU(DISPDUType::Collision,               "Collision",               false, false);
    addPDU(DISPDUType::StartResume,             "Start/Resume",            true,  true);
    addPDU(DISPDUType::StopFreeze,              "Stop/Freeze",             true,  true);
    addPDU(DISPDUType::ElectromagneticEmission, "Emission",                true,  true);
    addPDU(DISPDUType::TransferOwnership,       "Transfer Ownership",      true,  true);
    addPDU(DISPDUType::Acknowledge,             "Acknowledge",             true,  true);
    addPDU(DISPDUType::RemoveEntity,            "Remove Entity",           true,  true);
    addPDU(DISPDUType::CreateEntity,            "Create Entity",           true,  true);
    addPDU(DISPDUType::DataQuery,               "Data Query",              true,  true);
    addPDU(DISPDUType::Data,                    "Data",                    true,  true);
    addPDU(DISPDUType::SetData,                 "Set Data",                true,  true);
    addPDU(DISPDUType::Transmitter,             "Transmitter",             true,  true);
    addPDU(DISPDUType::Signal,                  "Signal",                  true,  true);
    addPDU(DISPDUType::IFF,                     "IFF",                     false, false);
    addPDU(DISPDUType::Designator,              "Designator",              false, false);

    return cfg;
}

#endif // DISCONFIG_H
