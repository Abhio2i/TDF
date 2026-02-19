//Author::Aman Negi
#include "folderpdu.h"
#include <cstring> // for strncpy, strnlen

/**
 * @brief Serializes (marshals) the FolderPDU data into a DIS::DataStream.
 *
 * The function writes all member fields to the provided DataStream in a
 * fixed-size format. This ensures consistent packet structure across the
 * network and compatibility with the DIS protocol.
 *
 * Fields are stored as follows:
 * - role (1 byte)
 * - folderID (36 bytes)
 * - parentID (36 bytes)
 * - name (64 bytes)
 * - newName (64 bytes)
 */
void FolderPDU::marshal(DIS::DataStream& ds) const {
    // Write the role field (0=add, 1=remove, 2=rename)
    ds << role;

    // --- Serialize folderID (max 36 bytes) ---
    char buf[36] = {};
    strncpy(buf, folderID.c_str(), 36);
    for (size_t i = 0; i < 36; ++i)
        ds << buf[i];

    // --- Serialize parentID (max 36 bytes) ---
    strncpy(buf, parentID.c_str(), 36);
    for (size_t i = 0; i < 36; ++i)
        ds << buf[i];

    // --- Serialize name (max 64 bytes) ---
    char nbuf[64] = {};
    strncpy(nbuf, name.c_str(), 64);
    for (size_t i = 0; i < 64; ++i)
        ds << nbuf[i];

    // --- Serialize newName (max 64 bytes) ---
    strncpy(nbuf, newName.c_str(), 64);
    for (size_t i = 0; i < 64; ++i)
        ds << nbuf[i];
}

/**
 * @brief Deserializes (unmarshals) FolderPDU data from a DIS::DataStream.
 *
 * The function reconstructs a FolderPDU object from the raw binary stream
 * by reading each field in the same order as written by marshal().
 *
 * It ensures string safety by using `strnlen()` to avoid reading beyond
 * buffer bounds.
 */
void FolderPDU::unmarshal(DIS::DataStream& ds) {
    // Read the role field
    ds >> role;

    // --- Deserialize folderID (max 36 bytes) ---
    char buf[36];
    for (size_t i = 0; i < 36; ++i)
        ds >> buf[i];
    folderID = std::string(buf, strnlen(buf, 36));

    // --- Deserialize parentID (max 36 bytes) ---
    for (size_t i = 0; i < 36; ++i)
        ds >> buf[i];
    parentID = std::string(buf, strnlen(buf, 36));

    // --- Deserialize name (max 64 bytes) ---
    char nbuf[64];
    for (size_t i = 0; i < 64; ++i)
        ds >> nbuf[i];
    name = std::string(nbuf, strnlen(nbuf, 64));

    // --- Deserialize newName (max 64 bytes) ---
    for (size_t i = 0; i < 64; ++i)
        ds >> nbuf[i];
    newName = std::string(nbuf, strnlen(nbuf, 64));
}

