#include "profilepdu.h"
#include <dis7/utils/DataStream.h>  // Provides binary read/write operations
#include <cstring>                  // For strncpy and strnlen

/**
 * @brief Serializes (marshals) the ProfilePDU data into binary format.
 *
 * The data is written in a fixed-size, byte-aligned structure suitable for
 * DIS (Distributed Interactive Simulation) transmission.
 * Each string field is copied into a fixed-length character buffer to ensure
 * consistent binary size across platforms.
 *
 * @param ds Reference to a DIS::DataStream used for writing bytes.
 */
void ProfilePDU::marshal(DIS::DataStream& ds) const {
    // Write the role (operation type: add, remove, rename)
    ds << role;

    // Write the profileID (UUID or unique identifier) as 36 fixed bytes
    char buf[36] = {};
    strncpy(buf, profileID.c_str(), 36);
    for (size_t i = 0; i < 36; ++i)
        ds << buf[i];

    // Write the name (current display name) as 64 fixed bytes
    char nbuf[64] = {};
    strncpy(nbuf, name.c_str(), 64);
    for (size_t i = 0; i < 64; ++i)
        ds << nbuf[i];

    // Write the newName (used only if renaming) as 64 fixed bytes
    strncpy(nbuf, newName.c_str(), 64);
    for (size_t i = 0; i < 64; ++i)
        ds << nbuf[i];
}

/**
 * @brief Deserializes (unmarshals) binary data into a ProfilePDU object.
 *
 * This reverses the marshalling process, reading a fixed-length binary
 * structure and reconstructing the string and integer fields.
 *
 * @param ds Reference to a DIS::DataStream used for reading bytes.
 */
void ProfilePDU::unmarshal(DIS::DataStream& ds) {
    // Read the role (operation type)
    ds >> role;

    // Read 36-byte profileID and reconstruct as string (trim at null terminator)
    char buf[36];
    for (size_t i = 0; i < 36; ++i)
        ds >> buf[i];
    profileID = std::string(buf, strnlen(buf, 36));

    // Read 64-byte name field
    char nbuf[64];
    for (size_t i = 0; i < 64; ++i)
        ds >> nbuf[i];
    name = std::string(nbuf, strnlen(nbuf, 64));

    // Read 64-byte newName field
    for (size_t i = 0; i < 64; ++i)
        ds >> nbuf[i];
    newName = std::string(nbuf, strnlen(nbuf, 64));
}
