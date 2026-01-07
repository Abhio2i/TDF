#include "transformpdu.h"
#include <cstring> // for strncpy, strnlen

// -------------------------------------------------------------
// Constructor: Initialize default values
// -------------------------------------------------------------
TransformPDU::TransformPDU()
    : active(true),
    latitude(0), longitude(0), altitude(0), heading(0),
    posX(0), posY(0), posZ(0),
    rotX(0), rotY(0), rotZ(0),
    sizeX(1), sizeY(1), sizeZ(1) {}

// -------------------------------------------------------------
// Full (readable) serialization — for debug or logs
// -------------------------------------------------------------
void TransformPDU::marshal(DIS::DataStream& ds) const {
    char buf[36] = {};

    // entityID (36 chars, padded)
    strncpy(buf, entityID.c_str(), 36);
    for (int i = 0; i < 36; ++i) ds << buf[i];

    // parentID (36 chars, padded)
    strncpy(buf, parentID.c_str(), 36);
    for (int i = 0; i < 36; ++i) ds << buf[i];

    ds << static_cast<uint8_t>(active);

    // Geographical info
    ds << latitude << longitude << altitude << heading;

    // Transform
    ds << posX << posY << posZ;
    ds << rotX << rotY << rotZ;
    ds << sizeX << sizeY << sizeZ;
}

// -------------------------------------------------------------
// Full (readable) deserialization — for debug or logs
// -------------------------------------------------------------
void TransformPDU::unmarshal(DIS::DataStream& ds) {
    char buf[36];

    // entityID
    for (int i = 0; i < 36; ++i) ds >> buf[i];
    entityID = std::string(buf, strnlen(buf, 36));

    // parentID
    for (int i = 0; i < 36; ++i) ds >> buf[i];
    parentID = std::string(buf, strnlen(buf, 36));

    uint8_t act;
    ds >> act;
    active = (act != 0);

    // Geographical info
    ds >> latitude >> longitude >> altitude >> heading;

    // Transform
    ds >> posX >> posY >> posZ;
    ds >> rotX >> rotY >> rotZ;
    ds >> sizeX >> sizeY >> sizeZ;
}
/*
--------------------------------------------------------------------------------
🧱 Compact TransformPDU Binary Layout (marshalCompact)
--------------------------------------------------------------------------------
This compact format reduces size and allows direct binary transmission.
All fields are fixed-size and tightly packed — total = **73 bytes per PDU**.

 Byte Layout:
 ┌────────────┬───────────────────────┬──────────┬──────────────┐
 │ Field      │ Type                  │ Size (B) │ Offset (hex) │
 ├────────────┼───────────────────────┼──────────┼──────────────┤
 │ entityID   │ uint8_t[12]           │ 12       │ 00–0B        │
 │ parentID   │ uint8_t[12]           │ 12       │ 0C–17        │
 │ active     │ uint8_t               │ 1        │ 18           │
 │ latitude   │ float (32-bit)        │ 4        │ 19–1C        │
 │ longitude  │ float (32-bit)        │ 4        │ 1D–20        │
 │ altitude   │ float (32-bit)        │ 4        │ 21–24        │
 │ heading    │ float (32-bit)        │ 4        │ 25–28        │
 │ posX,posY,Z│ float ×3              │ 12       │ 29–34        │
 │ rotX,rotY,Z│ float ×3              │ 12       │ 35–40        │
 │ sizeX,Y,Z  │ float ×3              │ 12       │ 41–4C        │
 └────────────┴───────────────────────┴──────────┴──────────────┘

✅ Total = 12 + 12 + 1 + (12 floats × 4 bytes) = **73 bytes**
✅ Compact, fixed-length, endian-safe (DIS::BIG)
✅ Uses 12-byte binary ObjectIDs (MongoDB style)
✅ Ideal for high-frequency network updates
by removing the geocords we reduces the pdu to 60 bytes
--------------------------------------------------------------------------------
*/

// -------------------------------------------------------------
// Compact (binary-optimized) serialization
// -------------------------------------------------------------
void TransformPDU::marshalCompact(DIS::DataStream& ds) const {
    // Convert 24-char MongoDB Type ObjectIDs → 12-byte binary form
    auto eBytes = hexToBytes(entityID);
    auto pBytes = hexToBytes(parentID);

    // Write 12 + 12 bytes (Entity + Parent)
    for (auto b : eBytes) ds << b;
    for (auto b : pBytes) ds << b;

    // ds << static_cast<uint8_t>(active);

    // // Use float instead of double for reduced bandwidth
    // ds << static_cast<float>(latitude);
    // ds << static_cast<float>(longitude);
    // ds << static_cast<float>(altitude);
    // ds << static_cast<float>(heading);

    // Local Transform
    ds << posX << posY << posZ;
    ds << rotX << rotY << rotZ;
    ds << sizeX << sizeY << sizeZ;
}

// -------------------------------------------------------------
// Compact (binary-optimized) deserialization
// -------------------------------------------------------------
void TransformPDU::unmarshalCompact(DIS::DataStream& ds) {
    std::vector<uint8_t> eBytes(12), pBytes(12);

    // Read 12 + 12 bytes (Entity + Parent)
    for (auto& b : eBytes) ds >> b;
    for (auto& b : pBytes) ds >> b;

    entityID = bytesToHex(eBytes);
    parentID = bytesToHex(pBytes);

    //uint8_t act;
    //ds >> act;
    //active = (act != 0);

    // float latF, lonF, altF, headF;
    // ds >> latF >> lonF >> altF >> headF;
    // latitude = latF;
    // longitude = lonF;
    // altitude = altF;
    // heading = headF;

    ds >> posX >> posY >> posZ;
    ds >> rotX >> rotY >> rotZ;
    ds >> sizeX >> sizeY >> sizeZ;
}

