#pragma once

#include <dis7/EntityManagementFamilyPdu.h>
#include <dis7/EntityID.h>
#include <dis7/utils/DataStream.h>
#include <dis7/opendis7_export.h>

namespace DIS
{

// Transfer Ownership PDU — IEEE 1278.1-2012 Section 7.8.2
// PDU Type 35, Protocol Family 7 (Entity Management)
// Used to transfer ownership of an entity from one simulation
// application to another.
//
// Wire layout (40 bytes total):
//   [0-11]  PDU Header (from Pdu base)
//   [12-17] Originating Entity ID (site/app/entity)
//   [18-23] Receiving Entity ID (site/app/entity)
//   [24-27] Request ID (uint32)
//   [28]    Required Reliability Service (uint8)
//   [29]    Transfer Type (uint8)
//   [30-35] Transfer Entity ID (site/app/entity)
//   [36-37] Number of Record Sets (uint16)
//   [38-39] Padding (2 bytes, align to 4-byte boundary)
//
// Copyright (c) 2007-2009, MOVES Institute, Naval Postgraduate School.
// All rights reserved.

class OPENDIS7_EXPORT TransferOwnershipPdu : public EntityManagementFamilyPdu
{
protected:
    /** Entity that is transferring ownership */
    EntityID _originatingEntityID;

    /** Entity that is receiving ownership */
    EntityID _receivingEntityID;

    /** Identifies the specific transfer request. Used to match Acknowledge. */
    unsigned int _requestID;

    /** Level of reliability required. 0 = Acknowledged. */
    unsigned char _requiredReliabilityService;

    /** Type of transfer. IEEE 1278.1 Table 70. 2 = owner initiates. */
    unsigned char _transferType;

    /** The entity being transferred */
    EntityID _transferEntityID;

    /** Number of record sets. 0 for basic transfer. */
    unsigned short _numberOfRecordSets;

public:
    TransferOwnershipPdu();
    virtual ~TransferOwnershipPdu();

    virtual void marshal  (DataStream& dataStream) const;
    virtual void unmarshal(DataStream& dataStream);

    // Originating Entity ID — who is sending the transfer
    EntityID&       getOriginatingEntityID();
    const EntityID& getOriginatingEntityID() const;
    void            setOriginatingEntityID(const EntityID& pX);

    // Receiving Entity ID — who should take ownership
    EntityID&       getReceivingEntityID();
    const EntityID& getReceivingEntityID() const;
    void            setReceivingEntityID(const EntityID& pX);

    // Request ID — must match Acknowledge response
    unsigned int getRequestID() const;
    void         setRequestID(unsigned int pX);

    // Required Reliability Service — 0 = Acknowledged
    unsigned char getRequiredReliabilityService() const;
    void          setRequiredReliabilityService(unsigned char pX);

    // Transfer Type — IEEE 1278.1 Table 70
    // 1=Other 2=ControllingAppRequests 3=RequestingAppDesires
    // 4=MutualConsent 7=Automatic
    unsigned char getTransferType() const;
    void          setTransferType(unsigned char pX);

    // Transfer Entity ID — the entity being transferred
    EntityID&       getTransferEntityID();
    const EntityID& getTransferEntityID() const;
    void            setTransferEntityID(const EntityID& pX);

    // Number of record sets
    unsigned short getNumberOfRecordSets() const;
    void           setNumberOfRecordSets(unsigned short pX);

    virtual int getMarshalledSize() const;

    bool operator==(const TransferOwnershipPdu& rhs) const;
};

} // namespace DIS
