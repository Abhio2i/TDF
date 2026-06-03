// =============================================================================
// FILE:        PDUVersionAdapter.cpp
// MODULE:      DIS Network Plugin — Version
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "pduversionadapter.h"
#include <QDebug>
#include <QReadLocker>
#include <QWriteLocker>
#include <QTime>    // ADD — for absolute timestamp validation

// =============================================================================
// PDUVersionAdapter — static methods
// =============================================================================

// =============================================================================
// adaptForPeer
// Patches version byte if peer requires V6
// Called just before sendMulticast() or sendUnicast()
// =============================================================================
QByteArray PDUVersionAdapter::adaptForPeer(const QByteArray& pduBytes,
                                           uint8_t targetVersion)
{
    if (pduBytes.size() < DISVersion::HEADER_SIZE) return pduBytes;

    uint8_t currentVersion = static_cast<uint8_t>(pduBytes[DISVersion::PROTOCOL_VERSION_OFFSET]);

    // Already correct version — return unchanged (no copy)
    if (currentVersion == targetVersion) return pduBytes;

    // Need to patch — make a copy and change byte 0
    QByteArray adapted = pduBytes;
    adapted[DISVersion::PROTOCOL_VERSION_OFFSET] = static_cast<char>(targetVersion);
    return adapted;
}

// =============================================================================
// extractVersion
// Read protocol version byte from PDU header
// =============================================================================
uint8_t PDUVersionAdapter::extractVersion(const QByteArray& pduBytes)
{
    if (pduBytes.size() < DISVersion::HEADER_SIZE) return 0;
    return static_cast<uint8_t>(pduBytes[DISVersion::PROTOCOL_VERSION_OFFSET]);
}

// =============================================================================
// extractPDUType
// Read PDU type byte from PDU header
// =============================================================================
uint8_t PDUVersionAdapter::extractPDUType(const QByteArray& pduBytes)
{
    if (pduBytes.size() < DISVersion::HEADER_SIZE) return 0;
    return static_cast<uint8_t>(pduBytes[DISVersion::PDU_TYPE_OFFSET]);
}

// =============================================================================
// extractExerciseID
// Read exercise ID byte from PDU header
// =============================================================================
uint8_t PDUVersionAdapter::extractExerciseID(const QByteArray& pduBytes)
{
    if (pduBytes.size() < DISVersion::HEADER_SIZE) return 0;
    return static_cast<uint8_t>(pduBytes[DISVersion::EXERCISE_ID_OFFSET]);
}

// =============================================================================
// isValidDISPDU
// Quick sanity check before processing a received datagram
//
// Checks:
//   1. Minimum size (12 bytes for header)
//   2. Protocol version is 5, 6, or 7
//   3. PDU type is non-zero
//   4. Exercise ID matches if expectedExerciseID != 0
// =============================================================================
bool PDUVersionAdapter::isValidDISPDU(const QByteArray& pduBytes,
                                      uint8_t expectedExerciseID)
{
    // Must be at least header size
    if (pduBytes.size() < DISVersion::HEADER_SIZE) {
        return false;
    }

    // Version must be 5, 6, or 7
    uint8_t version = extractVersion(pduBytes);
    if (version < DISVersion::V5 || version > DISVersion::V7) {
        qWarning() << "[PDUVersionAdapter] Invalid DIS version:" << version;
        return false;
    }

    // PDU type must be non-zero
    uint8_t pduType = extractPDUType(pduBytes);
    if (pduType == 0) {
        qWarning() << "[PDUVersionAdapter] PDU type is zero — invalid";
        return false;
    }

    // Exercise ID check (if configured)
    if (expectedExerciseID != 0) {
        uint8_t rxExerciseID = extractExerciseID(pduBytes);
        if (rxExerciseID != expectedExerciseID) {
            // Different exercise — silently ignore
            // (not an error — just a different exercise on the network)
            return false;
        }
    }

    return true;
}

// =============================================================================
// normalizeToV7
// Sets version byte to 7 on any received PDU
// So our internal processing code always works with V7
// =============================================================================
QByteArray PDUVersionAdapter::normalizeToV7(const QByteArray& pduBytes)
{
    return adaptForPeer(pduBytes, DISVersion::V7);
}
// =============================================================================
// extractTimestamp
// Reads bytes 4-7 from PDU header as big-endian uint32
// =============================================================================
uint32_t PDUVersionAdapter::extractTimestamp(const QByteArray& pduBytes)
{
    if (pduBytes.size() < DISVersion::HEADER_SIZE) return 0;

    return (static_cast<uint32_t>(static_cast<uint8_t>(
                pduBytes[DISVersion::TIMESTAMP_OFFSET]))     << 24) |
           (static_cast<uint32_t>(static_cast<uint8_t>(
                pduBytes[DISVersion::TIMESTAMP_OFFSET + 1])) << 16) |
           (static_cast<uint32_t>(static_cast<uint8_t>(
                pduBytes[DISVersion::TIMESTAMP_OFFSET + 2])) << 8)  |
           static_cast<uint32_t>(static_cast<uint8_t>(
               pduBytes[DISVersion::TIMESTAMP_OFFSET + 3]));
}

// =============================================================================
// isLateMessage
// Validates absolute DIS timestamps against current wall clock.
//
// DIS absolute timestamp format (IEEE 1278.1):
//   Bit 0 = 1 (absolute), upper 31 bits = time units since top of hour
//   1 unit = 3600 seconds / 2^31 ≈ 1.676 microseconds
//
// Acceptance rules:
//   timestamp == 0           → accept (sender did not set timestamp)
//   bit 0 == 0 (relative)   → accept (cannot validate without sender epoch)
//   bit 0 == 1 (absolute)   → check delta against thresholdMs
// =============================================================================
bool PDUVersionAdapter::isLateMessage(const QByteArray& pduBytes, int thresholdMs)
{
    uint32_t timestamp = extractTimestamp(pduBytes);

    // Zero → sender did not set timestamp → accept
    if (timestamp == 0) return false;

    // Bit 0 = 0 → relative timestamp → cannot validate → accept
    if ((timestamp & 0x01) == 0) return false;

    // Absolute timestamp — upper 31 bits are time units since top of hour
    // Convert to milliseconds:
    //   ms = units * 3600 * 1000 / 2^31
    // Use 64-bit arithmetic to avoid overflow
    uint32_t units = timestamp >> 1;
    uint64_t pduMs = (static_cast<uint64_t>(units) * 3600000ULL)
                     / 2147483648ULL;

    // Current milliseconds since top of hour
    QTime now = QTime::currentTime();
    uint64_t nowMs = static_cast<uint64_t>(
        now.hour()   * 3600000 +
        now.minute() * 60000   +
        now.second() * 1000    +
        now.msec());

    // Delta — handle hour boundary wraparound
    // (PDU sent at 59:59.999 arrives at 00:00.001 of next hour)
    int64_t deltaMs;
    if (nowMs >= pduMs) {
        deltaMs = static_cast<int64_t>(nowMs - pduMs);
    } else {
        // PDU timestamp is from previous hour
        deltaMs = static_cast<int64_t>(nowMs + 3600000ULL - pduMs);
    }

    return deltaMs > static_cast<int64_t>(thresholdMs);
}

// =============================================================================
// PeerVersionRegistry — implementation
// =============================================================================
// =============================================================================
// PeerVersionRegistry — implementation
// =============================================================================

// =============================================================================
// updatePeer
// Called every time we receive a PDU from any address
// Tracks DIS version, site/app IDs, last seen time
// Emits no signals — caller checks getActivePeers() for UI updates
// =============================================================================
void PeerVersionRegistry::updatePeer(const QString& address,
                                     uint8_t  disVersion,
                                     uint16_t siteID,
                                     uint16_t appID)
{
    QWriteLocker lock(&m_lock);

    std::string key = address.toStdString();
    auto it = m_peers.find(key);

    if (it == m_peers.end()) {
        // New peer
        PeerInfo info;
        info.address    = address;
        info.disVersion = disVersion;
        info.siteID     = siteID;
        info.appID      = appID;
        info.lastSeen   = QDateTime::currentDateTime();
        info.pduCount   = 1;
        info.active     = true;

        m_peers[key] = info;

        qDebug() << "[PeerVersionRegistry] New peer discovered:"
                 << address
                 << "DIS v" << disVersion
                 << "site="  << siteID
                 << "app="   << appID;
    } else {
        // Known peer — update
        it->second.disVersion = disVersion;
        it->second.siteID     = siteID;
        it->second.appID      = appID;
        it->second.lastSeen   = QDateTime::currentDateTime();
        it->second.pduCount++;
        it->second.active     = true;
    }
}

// =============================================================================
// getPeerVersion
// Returns the DIS version used by a specific peer
// Defaults to V7 if peer unknown (safe — V7 is our native format)
// =============================================================================
uint8_t PeerVersionRegistry::getPeerVersion(const QString& address) const
{
    QReadLocker lock(&m_lock);

    auto it = m_peers.find(address.toStdString());
    if (it != m_peers.end()) {
        return it->second.disVersion;
    }

    // Unknown peer — assume V7 (our native format)
    // If they are V6 we will find out when we receive their PDUs
    return DISVersion::V7;
}

// =============================================================================
// hasPeer
// =============================================================================
bool PeerVersionRegistry::hasPeer(const QString& address) const
{
    QReadLocker lock(&m_lock);
    return m_peers.find(address.toStdString()) != m_peers.end();
}

// =============================================================================
// getActivePeers
// Returns a snapshot of all active peers for UI display
// =============================================================================
QList<PeerInfo> PeerVersionRegistry::getActivePeers() const
{
    QReadLocker lock(&m_lock);

    QList<PeerInfo> result;
    for (const auto& pair : m_peers) {
        if (pair.second.active) {
            result.append(pair.second);
        }
    }
    return result;
}

// =============================================================================
// peerCount
// =============================================================================
int PeerVersionRegistry::peerCount() const
{
    QReadLocker lock(&m_lock);
    return static_cast<int>(m_peers.size());
}

// =============================================================================
// removeStale
// Called periodically to remove peers not seen recently
// A peer is stale if lastSeen > timeoutSeconds ago
// =============================================================================
void PeerVersionRegistry::removeStale(int timeoutSeconds)
{
    QWriteLocker lock(&m_lock);

    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-timeoutSeconds);
    auto it = m_peers.begin();

    while (it != m_peers.end()) {
        if (it->second.lastSeen < cutoff) {
            qDebug() << "[PeerVersionRegistry] Peer timed out:"
                     << it->second.address;
            it = m_peers.erase(it);
        } else {
            ++it;
        }
    }
}

// =============================================================================
// clear
// =============================================================================
void PeerVersionRegistry::clear()
{
    QWriteLocker lock(&m_lock);
    m_peers.clear();
    qDebug() << "[PeerVersionRegistry] Cleared all peers";
}
