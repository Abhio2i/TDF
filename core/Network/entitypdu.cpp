#include "entitypdu.h"
#include <dis7/utils/DataStream.h>
#include <cstring>

/**
 * @file EntityPDU.cpp
 * @brief Implements serialization (marshal) and deserialization (unmarshal)
 *        logic for the EntityPDU class, which represents entity-level
 *        operations in a distributed simulation system.
 */

/**
 * @brief Default constructor initializing all members to safe defaults.
 *
 * - role = 0 (add)
 * - type = 0 (undefined entity type)
 * - profileFlag = false (no associated profile data)
 */
EntityPDU::EntityPDU() {
    role = 0;
    type = 0;
    profileFlag = false;
}

/**
 * @brief Serializes (marshals) the EntityPDU object into a DataStream
 *        for network transmission.
 *
 * The order of serialization must match the unmarshalling order exactly.
 * Strings are copied into fixed-length buffers to maintain consistent
 * data size and structure across the network.
 *
 * @param ds The DIS::DataStream to which the object data will be written.
 */
void EntityPDU::marshal(DIS::DataStream& ds) const {
    // Write basic fields
    ds << role;           // Operation type (add/remove/rename)
    ds << type;           // Entity type (custom-defined elsewhere)

    // Serialize entityID as a fixed 36-byte buffer
    char buf[36] = {};
    strncpy(buf, entityID.c_str(), 36);
    for (size_t i = 0; i < 36; ++i) ds << buf[i];

    // Serialize parentID as a fixed 36-byte buffer
    strncpy(buf, parentID.c_str(), 36);
    for (size_t i = 0; i < 36; ++i) ds << buf[i];

    // Serialize name as a fixed 64-byte buffer
    char nbuf[64] = {};
    strncpy(nbuf, name.c_str(), 64);
    for (size_t i = 0; i < 64; ++i) ds << nbuf[i];

    // Serialize newName as a fixed 64-byte buffer
    strncpy(nbuf, newName.c_str(), 64);
    for (size_t i = 0; i < 64; ++i) ds << nbuf[i];

    // Serialize profile flag (converted to uint8_t)
    ds << static_cast<uint8_t>(profileFlag);

    // Serialize delta vector (variable length binary data)
    uint32_t deltaSize = static_cast<uint32_t>(delta.size());
    ds << deltaSize;                     // Write size first
    for (uint8_t b : delta) ds << b;     // Write each byte
}

/**
 * @brief Deserializes (unmarshals) data from a DataStream into
 *        an EntityPDU object.
 *
 * The read order must exactly match the marshal() order.
 * Fixed-size buffers are used to ensure predictable parsing.
 *
 * @param ds The DIS::DataStream to read from.
 */
void EntityPDU::unmarshal(DIS::DataStream& ds) {
    // Read basic fields
    ds >> role;
    ds >> type;

    // Read entityID (fixed 36 bytes)
    char buf[36];
    for (size_t i = 0; i < 36; ++i) ds >> buf[i];
    entityID = std::string(buf, strnlen(buf, 36));

    // Read parentID (fixed 36 bytes)
    for (size_t i = 0; i < 36; ++i) ds >> buf[i];
    parentID = std::string(buf, strnlen(buf, 36));

    // Read name (fixed 64 bytes)
    char nbuf[64];
    for (size_t i = 0; i < 64; ++i) ds >> nbuf[i];
    name = std::string(nbuf, strnlen(nbuf, 64));

    // Read newName (fixed 64 bytes)
    for (size_t i = 0; i < 64; ++i) ds >> nbuf[i];
    newName = std::string(nbuf, strnlen(nbuf, 64));

    // Read and convert profile flag
    uint8_t pf;
    ds >> pf;
    profileFlag = (pf != 0);

    // Read delta vector (variable size)
    uint32_t deltaSize;
    ds >> deltaSize;
    delta.resize(deltaSize);
    for (uint32_t i = 0; i < deltaSize; ++i) ds >> delta[i];
}

