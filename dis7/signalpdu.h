#pragma once

#include <dis7/EntityID.h>
#include <dis7/RadioCommunicationsFamilyPdu.h>
#include <dis7/opendis7_export.h>
#include <dis7/utils/DataStream.h>
#include <vector>
#include <cstdint>
namespace DIS {
// Detailed information about the signal data being transmitted by a radio.
// Section 7.7.3. Hand-written for dis7 — missing from this vendored copy
// (same category of gap as TransmitterPdu, upstream open-dis-cpp issue #54).
// Modeled on the dis7 IntercomSignalPdu scaffold already present in this
// codebase, with real Signal PDU fields.
//
// Copyright (c) 2007-2009, MOVES Institute, Naval Postgraduate School. All
// rights reserved.
//
// @author DMcG, jkg
class OPENDIS7_EXPORT SignalPdu : public RadioCommunicationsFamilyPdu {
protected:
    // EntityID _entityID;
    // unsigned short _radioID;
    unsigned short _encodingScheme;
    unsigned short _tdlType;
    unsigned int _sampleRate;
    unsigned short _dataLength;   // wire field, IN BITS — derived, see getDataLength()
    unsigned short _samples;
    std::vector<uint8_t> _data;

public:
    SignalPdu();
    virtual ~SignalPdu();

    virtual void marshal(DataStream &dataStream) const;
    virtual void unmarshal(DataStream &dataStream);

    EntityID &getEntityID();
    const EntityID &getEntityID() const;
    void setEntityID(const EntityID &pX);

    unsigned short getRadioID() const;
    void setRadioID(unsigned short pX);

    unsigned short getEncodingScheme() const;
    void setEncodingScheme(unsigned short pX);

    unsigned short getTdlType() const;
    void setTdlType(unsigned short pX);

    unsigned int getSampleRate() const;
    void setSampleRate(unsigned int pX);

    unsigned short getDataLength() const;   // bits, derived from _data.size()

    unsigned short getSamples() const;
    void setSamples(unsigned short pX);

    std::vector<uint8_t> &getData();
    const std::vector<uint8_t> &getData() const;
    void setData(const std::vector<uint8_t> &pX);

    virtual int getMarshalledSize() const;
    bool operator==(const SignalPdu &rhs) const;
};
} // namespace DIS
