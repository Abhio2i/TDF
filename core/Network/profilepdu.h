#ifndef PROFILEPDU_H
#define PROFILEPDU_H

#pragma once

#include <dis7/DataPdu.h>   // Base class for DIS PDUs (Distributed Interactive Simulation)
#include <string>
#include <cstdint>

/**
 * @class ProfilePDU
 * @brief Represents a custom Profile Protocol Data Unit (PDU) used for
 *        managing profile-related operations such as adding, removing,
 *        or renaming profiles in a distributed simulation.
 *
 * Inherits from DIS::DataPdu to allow integration with the DIS (Distributed
 * Interactive Simulation) networking framework.
 */
class ProfilePDU : public DIS::DataPdu {
public:
    /**
     * @brief Role of the profile operation.
     * Possible values:
     * - 0 → Add a new profile
     * - 1 → Remove an existing profile
     * - 2 → Rename an existing profile
     */
    uint8_t role;

    /// Unique identifier for the profile being added/removed/renamed.
    std::string profileID;

    /// Current name of the profile.
    std::string name;

    /// New name of the profile (used only when renaming).
    std::string newName;

    /// @brief Default constructor initializing role to 0 (add).
    ProfilePDU() : role(0) {}

    /**
     * @brief Serializes (marshals) the ProfilePDU into a DataStream
     * for network transmission.
     * @param ds The DataStream to write to.
     */
    void marshal(DIS::DataStream& ds) const override;

    /**
     * @brief Deserializes (unmarshals) the ProfilePDU from a DataStream
     * received over the network.
     * @param ds The DataStream to read from.
     */
    void unmarshal(DIS::DataStream& ds) override;
};

#endif // PROFILEPDU_H

