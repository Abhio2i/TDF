#include "transferownershippdu.h"

using namespace DIS;

// =============================================================================
// Constructor
// Sets PDU type to 35 (Transfer Ownership) and protocol family to 7
// (Entity Management) per IEEE 1278.1-2012 Section 7.8.2
// =============================================================================
TransferOwnershipPdu::TransferOwnershipPdu()
    : EntityManagementFamilyPdu(),
    _requestID(0),
    _requiredReliabilityService(0),
    _transferType(0),
    _numberOfRecordSets(0)
{
    setPduType(35);        // PDU_TRANSFER_OWNERSHIP
    setProtocolFamily(7);  // Entity Management Family
}

TransferOwnershipPdu::~TransferOwnershipPdu()
{
}

// =============================================================================
// Originating Entity ID
// =============================================================================
EntityID& TransferOwnershipPdu::getOriginatingEntityID()
{
    return _originatingEntityID;
}

const EntityID& TransferOwnershipPdu::getOriginatingEntityID() const
{
    return _originatingEntityID;
}

void TransferOwnershipPdu::setOriginatingEntityID(const EntityID& pX)
{
    _originatingEntityID = pX;
}

// =============================================================================
// Receiving Entity ID
// =============================================================================
EntityID& TransferOwnershipPdu::getReceivingEntityID()
{
    return _receivingEntityID;
}

const EntityID& TransferOwnershipPdu::getReceivingEntityID() const
{
    return _receivingEntityID;
}

void TransferOwnershipPdu::setReceivingEntityID(const EntityID& pX)
{
    _receivingEntityID = pX;
}

// =============================================================================
// Request ID
// =============================================================================
unsigned int TransferOwnershipPdu::getRequestID() const
{
    return _requestID;
}

void TransferOwnershipPdu::setRequestID(unsigned int pX)
{
    _requestID = pX;
}

// =============================================================================
// Required Reliability Service
// =============================================================================
unsigned char TransferOwnershipPdu::getRequiredReliabilityService() const
{
    return _requiredReliabilityService;
}

void TransferOwnershipPdu::setRequiredReliabilityService(unsigned char pX)
{
    _requiredReliabilityService = pX;
}

// =============================================================================
// Transfer Type
// =============================================================================
unsigned char TransferOwnershipPdu::getTransferType() const
{
    return _transferType;
}

void TransferOwnershipPdu::setTransferType(unsigned char pX)
{
    _transferType = pX;
}

// =============================================================================
// Transfer Entity ID
// =============================================================================
EntityID& TransferOwnershipPdu::getTransferEntityID()
{
    return _transferEntityID;
}

const EntityID& TransferOwnershipPdu::getTransferEntityID() const
{
    return _transferEntityID;
}

void TransferOwnershipPdu::setTransferEntityID(const EntityID& pX)
{
    _transferEntityID = pX;
}

// =============================================================================
// Number of Record Sets
// =============================================================================
unsigned short TransferOwnershipPdu::getNumberOfRecordSets() const
{
    return _numberOfRecordSets;
}

void TransferOwnershipPdu::setNumberOfRecordSets(unsigned short pX)
{
    _numberOfRecordSets = pX;
}

// =============================================================================
// marshal
// Serializes all fields to the DataStream in big-endian order.
// Base class marshals the 12-byte PDU header first.
// =============================================================================
void TransferOwnershipPdu::marshal(DataStream& dataStream) const
{
    EntityManagementFamilyPdu::marshal(dataStream); // 12-byte header

    _originatingEntityID.marshal(dataStream);       // 6 bytes
    _receivingEntityID.marshal(dataStream);         // 6 bytes
    dataStream << _requestID;                       // 4 bytes
    dataStream << _requiredReliabilityService;      // 1 byte
    dataStream << _transferType;                    // 1 byte
    _transferEntityID.marshal(dataStream);          // 6 bytes
    dataStream << _numberOfRecordSets;              // 2 bytes
    dataStream << static_cast<unsigned short>(0);   // 2 bytes padding
}

// =============================================================================
// unmarshal
// Deserializes all fields from the DataStream in big-endian order.
// Base class unmarshals the 12-byte PDU header first.
// =============================================================================
void TransferOwnershipPdu::unmarshal(DataStream& dataStream)
{
    EntityManagementFamilyPdu::unmarshal(dataStream);

    _originatingEntityID.unmarshal(dataStream);
    _receivingEntityID.unmarshal(dataStream);
    dataStream >> _requestID;
    dataStream >> _requiredReliabilityService;
    dataStream >> _transferType;
    _transferEntityID.unmarshal(dataStream);
    dataStream >> _numberOfRecordSets;

    unsigned short pad = 0;
    dataStream >> pad;  // consume padding bytes
}

// =============================================================================
// operator==
// =============================================================================
bool TransferOwnershipPdu::operator==(const TransferOwnershipPdu& rhs) const
{
    bool ivarsEqual = EntityManagementFamilyPdu::operator==(rhs);

    if (!(_originatingEntityID      == rhs._originatingEntityID))      ivarsEqual = false;
    if (!(_receivingEntityID        == rhs._receivingEntityID))        ivarsEqual = false;
    if (!(_requestID                == rhs._requestID))                ivarsEqual = false;
    if (!(_requiredReliabilityService == rhs._requiredReliabilityService)) ivarsEqual = false;
    if (!(_transferType             == rhs._transferType))             ivarsEqual = false;
    if (!(_transferEntityID         == rhs._transferEntityID))         ivarsEqual = false;
    if (!(_numberOfRecordSets       == rhs._numberOfRecordSets))       ivarsEqual = false;

    return ivarsEqual;
}

// =============================================================================
// getMarshalledSize
// Header(12) + originatingID(6) + receivingID(6) + requestID(4)
// + reliabilityService(1) + transferType(1) + transferEntityID(6)
// + numberOfRecordSets(2) + padding(2) = 40 bytes
// =============================================================================
int TransferOwnershipPdu::getMarshalledSize() const
{
    int marshalSize = EntityManagementFamilyPdu::getMarshalledSize();
    marshalSize += _originatingEntityID.getMarshalledSize();  // 6
    marshalSize += _receivingEntityID.getMarshalledSize();    // 6
    marshalSize += 4;  // _requestID
    marshalSize += 1;  // _requiredReliabilityService
    marshalSize += 1;  // _transferType
    marshalSize += _transferEntityID.getMarshalledSize();     // 6
    marshalSize += 2;  // _numberOfRecordSets
    marshalSize += 2;  // padding
    return marshalSize;
}

// Copyright (c) 1995-2009 held by the author(s). All rights reserved.
// IEEE 1278.1-2012 Transfer Ownership PDU — implemented by O2I
// following OpenDIS library conventions.
