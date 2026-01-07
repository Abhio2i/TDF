#ifndef FOLDERPDU_H
#define FOLDERPDU_H

#pragma once
#include <dis7/DataPdu.h>         // Base class for all DIS PDUs
#include <dis7/utils/DataStream.h> // Provides marshalling/unmarshalling for binary data
#include <string>
#include <cstdint>

/**
 * @brief FolderPDU represents a Protocol Data Unit (PDU) used for folder operations
 *        in a distributed simulation or networked environment.
 *
 * This class extends the DIS::DataPdu base class and encapsulates
 * operations like adding, removing, or renaming folders in a hierarchical structure.
 */
class FolderPDU : public DIS::DataPdu {
public:
    uint8_t role;          // Operation type: 0 = Add folder, 1 = Remove folder, 2 = Rename folder
    std::string folderID;  // Unique identifier for this folder
    std::string parentID;  // ID of the parent folder (used when creating hierarchical structures)
    std::string name;      // Current name of the folder (used for Add/Rename)
    std::string newName;   // New name to assign (used only when renaming)

    /**
     * @brief Default constructor initializes the role to 0 (Add).
     */
    FolderPDU() : role(0) {}

    /**
     * @brief Serializes (marshals) the folder data into a binary data stream.
     *
     * This method writes the folder's information to the provided DataStream
     * so it can be transmitted over the network or saved in binary form.
     *
     * @param ds The DataStream object used for output.
     */
    void marshal(DIS::DataStream& ds) const override;

    /**
     * @brief Deserializes (unmarshals) folder data from a binary data stream.
     *
     * This method reads the folder's information from the provided DataStream,
     * reconstructing it from binary form after network transmission or file read.
     *
     * @param ds The DataStream object used for input.
     */
    void unmarshal(DIS::DataStream& ds) override;
};

#endif // FOLDERPDU_H

