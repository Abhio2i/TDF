// =============================================================================
// FILE:        PDUVersionAdapter.h
// MODULE:      DIS Network Plugin — Version
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Handles DIS version compatibility between V6 and V7.
//
// THE PROBLEM:
//   Your engine uses DIS 7 (IEEE 1278.1-2012)
//   DRDO STAGE 22.0 is configured for DIS 6 (IEEE 1278.1-1995)
//
//   DIS PDU header byte[0] = protocol version
//     V6 = 6
//     V7 = 7
//
//   STAGE setting "Ignore other DIS versions" = unchecked
//   meaning STAGE WILL accept both V6 and V7 PDUs.
//
//   But to be safe and fully compatible, we:
//     1. Send V6 bytes to known V6 peers (DRDO machines)
//     2. Send V7 bytes to V7 peers (other TDF instances)
//     3. Accept both V6 and V7 when receiving
//
// HOW IT WORKS:
//   PDU header is 12 bytes. Byte[0] is protocol version.
//   Everything else is identical between V6 and V7 for the 8 PDU types we use.
//   So version adaptation is literally ONE byte change.
//
// PEER REGISTRY:
//   PeerVersionRegistry tracks which peers are V6 vs V7
//   based on the version byte of PDUs we receive from them.
// =============================================================================

#ifndef PDUVERSIONADAPTER_H
#define PDUVERSIONADAPTER_H

#include <QByteArray>
#include <QHostAddress>
#include <QReadWriteLock>
#include <QDateTime>
#include <unordered_map>
#include <string>
#include <cstdint>

// =============================================================================
// DIS Protocol Version constants
// =============================================================================
namespace DISVersion {
constexpr uint8_t V5 = 5;
constexpr uint8_t V6 = 6;
constexpr uint8_t V7 = 7;

// DIS PDU header offsets
constexpr int PROTOCOL_VERSION_OFFSET = 0;  // byte 0
constexpr int EXERCISE_ID_OFFSET      = 1;  // byte 1
constexpr int PDU_TYPE_OFFSET         = 2;  // byte 2
constexpr int PROTOCOL_FAMILY_OFFSET  = 3;  // byte 3
constexpr int TIMESTAMP_OFFSET        = 4;  // bytes 4-7
constexpr int PDU_LENGTH_OFFSET       = 8;  // bytes 8-9
constexpr int PADDING_OFFSET          = 10; // bytes 10-11
constexpr int HEADER_SIZE             = 12;
}

// =============================================================================
// PeerInfo
// Tracks what we know about a remote DIS node
// =============================================================================
struct PeerInfo {
    QString     address;
    uint8_t     disVersion  = DISVersion::V7;
    uint16_t    siteID      = 0;
    uint16_t    appID       = 0;
    QString     name        = "";
    QDateTime   lastSeen;
    int         pduCount    = 0;
    bool        active      = true;
};

// =============================================================================
// PDUVersionAdapter
// Handles version byte patching for outgoing PDUs
// =============================================================================
class PDUVersionAdapter {
public:

    // =========================================================================
    // adaptForPeer
    // Takes a V7 PDU byte array and patches the version byte
    // if the target peer requires V6.
    //
    // If targetVersion == 7: returns pduBytes unchanged
    // If targetVersion == 6: returns copy with byte[0] = 6
    //
    // This is the only change needed — V6 and V7 PDU structure
    // is identical for EntityState, Fire, Detonation, StartResume,
    // StopFreeze, RemoveEntity, CreateEntity, ElectromagneticEmission
    // =========================================================================
    static QByteArray adaptForPeer(const QByteArray& pduBytes,
                                   uint8_t targetVersion);

    // =========================================================================
    // extractVersion
    // Read the protocol version byte from a received PDU
    // Returns 0 if PDU is too short to be valid
    // =========================================================================
    static uint8_t extractVersion(const QByteArray& pduBytes);

    // =========================================================================
    // extractPDUType
    // Read the PDU type byte from a received PDU
    // =========================================================================
    static uint8_t extractPDUType(const QByteArray& pduBytes);

    // =========================================================================
    // extractExerciseID
    // Read the exercise ID byte from a received PDU
    // =========================================================================
    static uint8_t extractExerciseID(const QByteArray& pduBytes);

    // =========================================================================
    // isValidDISPDU
    // Basic header validation before processing
    // Checks minimum length, version range, non-zero PDU type
    // =========================================================================
    static bool isValidDISPDU(const QByteArray& pduBytes,
                              uint8_t expectedExerciseID = 0);

    // =========================================================================
    // normalizeToV7
    // Takes any received PDU (V5/V6/V7) and sets version byte to 7
    // so our internal processing always sees V7
    // =========================================================================
    static QByteArray normalizeToV7(const QByteArray& pduBytes);
    // =========================================================================
    // extractTimestamp
    // Read the 32-bit timestamp field from PDU header bytes 4-7 (big-endian)
    // Bit 0 of the value indicates timestamp type:
    //   0 = relative (sender-defined epoch, cannot validate)
    //   1 = absolute (microseconds since top of hour, IEEE 1278.1)
    // =========================================================================
    static uint32_t extractTimestamp(const QByteArray& pduBytes);

    // =========================================================================
    // isLateMessage
    // Returns true if the PDU should be discarded as stale.
    // Only absolute timestamps (bit 0 = 1) are validated.
    // Zero or relative timestamps are always accepted.
    // thresholdMs — discard if PDU is older than this many milliseconds
    // =========================================================================
    static bool isLateMessage(const QByteArray& pduBytes, int thresholdMs);
};

// =============================================================================
// PeerVersionRegistry
// Thread-safe registry of all known DIS peers and their versions
// Updated every time we receive a PDU from a new or known peer
// =============================================================================
class PeerVersionRegistry {
public:
    PeerVersionRegistry() = default;
    ~PeerVersionRegistry() = default;

    // ── Update ────────────────────────────────────────────────────────────────

    // Called when a PDU is received from a peer
    // Updates version, site/app IDs, last seen time
    void updatePeer(const QString& address,
                    uint8_t  disVersion,
                    uint16_t siteID,
                    uint16_t appID);

    // ── Query ────────────────────────────────────────────────────────────────

    // Get the DIS version used by a specific peer
    // Returns V7 as default if peer is unknown
    uint8_t getPeerVersion(const QString& address) const;

    // Check if a specific peer is known
    bool hasPeer(const QString& address) const;

    // Get all currently active peers
    QList<PeerInfo> getActivePeers() const;

    // Get peer count
    int peerCount() const;

    // ── Maintenance ───────────────────────────────────────────────────────────

    // Remove peers not seen for more than timeoutSeconds
    // Call this periodically (e.g. every 30 seconds)
    void removeStale(int timeoutSeconds = 30);

    // Clear all peers
    void clear();

private:
    // address string → PeerInfo
    std::unordered_map<std::string, PeerInfo> m_peers;
    mutable QReadWriteLock m_lock;
};

#endif // PDUVERSIONADAPTER_H
