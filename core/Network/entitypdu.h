#ifndef ENTITYPDU_H
#define ENTITYPDU_H

#pragma once

#include <dis7/DataPdu.h>              // Base class for all DIS PDUs
#include <dis7/utils/DataStream.h>     // For binary marshalling/unmarshalling
#include <string>
#include <vector>
#include <cstdint>

/// @brief Represents an Entity Protocol Data Unit (PDU)
/// This PDU carries data about an entity in the simulation world.
/// It supports add/remove/rename operations and binary marshalling via DIS::DataStream.
class EntityPDU : public DIS::DataPdu {
public:
    uint8_t role;              ///< Operation type: 0 = Add Entity, 1 = Remove Entity, 2 = Rename Entity
    uint8_t type;              ///< Entity type/category (e.g., 0 = Unknown, 1 = Vehicle, 2 = Folder, 3 = Profile)

    std::string entityID;      ///< Unique identifier (UUID or GUID) for this entity instance
    std::string parentID;      ///< Identifier of the parent entity/folder (for hierarchical structures)

    std::string name;          ///< Current visible name of the entity
    std::string newName;       ///< New name after renaming (only used if role == 2)

    bool profileFlag;          ///< Indicates if this entity is linked to a profile (true = profile entity)

    std::vector<uint8_t> delta; ///< Optional binary delta payload (e.g., incremental updates or metadata changes)

    /// @brief Default constructor initializing defaults
    EntityPDU();

    /// @brief Serializes this PDU into binary form for network transmission.
    /// @param ds Reference to a DIS::DataStream for writing bytes.
    void marshal(DIS::DataStream& ds) const override;

    /// @brief Deserializes binary PDU data received from the network.
    /// @param ds Reference to a DIS::DataStream for reading bytes.
    void unmarshal(DIS::DataStream& ds) override;
};

#endif // ENTITYPDU_H

