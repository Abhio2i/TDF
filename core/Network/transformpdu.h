//Author::Aman Negi
#ifndef TRANSFORMPDU_H
#define TRANSFORMPDU_H

#pragma once

#include <dis7/DataPdu.h>
#include <dis7/utils/DataStream.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdint>

/**
 * @class TransformPDU
 * @brief Represents a network PDU for the Transform component of an entity.
 *
 * This PDU contains all necessary data to synchronize a Transform component
 * over the network, including position, rotation, size, geocords, and
 * component activation state.
 *
 * It also includes entityID and parentID so the receiving client knows which
 * entity and hierarchy node this component belongs to.
 */
class TransformPDU : public DIS::DataPdu {
public:
    std::string entityID;   ///< ID of the entity this component belongs to
    std::string parentID;   ///< ID of the parent (profile category or folder)
    bool active;            ///< Component active state

    // Geocord information
    double latitude;        ///< Latitude of the entity in world coordinates
    double longitude;       ///< Longitude of the entity in world coordinates
    double altitude;        ///< Altitude of the entity in world coordinates
    double heading;         ///< Heading of the entity in degrees

    // Local transform data
    float posX, posY, posZ; ///< Position in local space
    float rotX, rotY, rotZ; ///< Rotation in local space
    float sizeX, sizeY, sizeZ; ///< Scale/size

    /// Default constructor initializing default values
    TransformPDU();

    /// Serialize the TransformPDU into a binary DataStream (full)
    void marshal(DIS::DataStream& ds) const override;

    /// Deserialize the TransformPDU from a binary DataStream (full)
    void unmarshal(DIS::DataStream& ds) override;

    /// --- Compact versions for network optimization ---
    void marshalCompact(DIS::DataStream& ds) const;
    void unmarshalCompact(DIS::DataStream& ds);

private:
    /// Helper: Convert 24-char hex string (Mongo ObjectID) to 12-byte vector
    static std::vector<uint8_t> hexToBytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        bytes.reserve(hex.length() / 2);
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }

    /// Helper: Convert 12-byte vector back to 24-char hex string
    static std::string bytesToHex(const std::vector<uint8_t>& bytes) {
        std::ostringstream oss;
        for (auto b : bytes)
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        return oss.str();
    }
};

#endif
/// -----------------------------------------------------------------------------
/// 🔧 Helper Functions for Compact ID Encoding
/// -----------------------------------------------------------------------------
/// MongoDB-style ObjectIDs are 24-character hex strings (e.g. "186d17c60915b2d2047efedd").
/// To make network PDUs smaller, we convert them to 12 raw bytes before transmission.

/// ▪ Each pair of hex characters (2 chars) represents 1 byte (8 bits).
///   → "186d17" → [0x18, 0x6d, 0x17]

/// ▪ hexToBytes():
///     Converts a 24-character hex string → 12-byte vector<uint8_t>
///     Used during serialization (marshalCompact).

/// ▪ bytesToHex():
///     Converts a 12-byte vector<uint8_t> → 24-character hex string
///     Used during deserialization (unmarshalCompact).

/// ⚙️ Example Conversion
///   Input:  "186d17c60915b2d2047efedd"
///   Binary: [0x18, 0x6d, 0x17, 0xc6, 0x09, 0x15, 0xb2, 0xd2, 0x04, 0x7e, 0xfe, 0xdd]
///   Back:   "186d17c60915b2d2047efedd" (same string reconstructed)

/// ✅ Benefits:
///   - Reduces 24 ASCII chars → 12 bytes binary
///   - Saves bandwidth in network PDUs
///   - Maintains reversible, lossless conversion
/// -----------------------------------------------------------------------------



