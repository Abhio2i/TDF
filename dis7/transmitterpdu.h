#pragma once

#include <dis7/EntityID.h>
#include <dis7/RadioType.h>
#include <dis7/ModulationType.h>
#include <dis7/Vector3Double.h>
#include <dis7/Vector3Float.h>
#include <dis7/RadioCommunicationsFamilyPdu.h>
#include <dis7/opendis7_export.h>
#include <dis7/utils/DataStream.h>
#include <cstdint>
namespace DIS {
// Detailed information about a radio transmitter. Section 7.7.2.
// Hand-written for dis7 (confirmed missing upstream — open-dis-cpp issue
// #54). Uses this codebase's existing dis7 RadioType/ModulationType, which
// resolves the RadioEntityType->RadioType change the issue flagged.
//
// NOTE: antennaPatternCount and modulationParameterCount are kept at 0 in
// this implementation — a fully IEEE-1278.1-valid state representing an
// omnidirectional-pattern radio with no extra modulation parameters. The
// variable-length antenna-pattern and modulation-parameter records
// themselves are not yet implemented; see unmarshal() for the receive-side
// implication if a peer ever sends non-zero counts.
//
// Copyright (c) 2007-2009, MOVES Institute, Naval Postgraduate School. All
// rights reserved.
//
// @author DMcG, jkg
class OPENDIS7_EXPORT TransmitterPdu : public RadioCommunicationsFamilyPdu {
protected:
    // EntityID _entityID;
    // unsigned short _radioID;
    RadioType _radioType;
    unsigned char _transmitState;
    unsigned char _inputSource;
    unsigned short _padding1;
    Vector3Double _antennaLocation;
    Vector3Float _relativeAntennaLocation;
    unsigned short _antennaPatternType;
    unsigned short _antennaPatternCount;
    uint64_t _frequency;
    float _transmitFrequencyBandwidth;
    float _power;
    ModulationType _modulationType;
    unsigned short _cryptoSystem;
    unsigned short _cryptoKeyId;
    unsigned char _modulationParameterCount;
    unsigned short _padding2;
    unsigned char _padding3;

public:
    TransmitterPdu();
    virtual ~TransmitterPdu();

    virtual void marshal(DataStream &dataStream) const;
    virtual void unmarshal(DataStream &dataStream);

    EntityID &getEntityID();
    const EntityID &getEntityID() const;
    void setEntityID(const EntityID &pX);

    unsigned short getRadioID() const;
    void setRadioID(unsigned short pX);

    RadioType &getRadioType();
    const RadioType &getRadioType() const;
    void setRadioType(const RadioType &pX);

    unsigned char getTransmitState() const;
    void setTransmitState(unsigned char pX);

    unsigned char getInputSource() const;
    void setInputSource(unsigned char pX);

    Vector3Double &getAntennaLocation();
    const Vector3Double &getAntennaLocation() const;
    void setAntennaLocation(const Vector3Double &pX);

    Vector3Float &getRelativeAntennaLocation();
    const Vector3Float &getRelativeAntennaLocation() const;
    void setRelativeAntennaLocation(const Vector3Float &pX);

    unsigned short getAntennaPatternType() const;
    void setAntennaPatternType(unsigned short pX);
    unsigned short getAntennaPatternCount() const;

    uint64_t getFrequency() const;
    void setFrequency(uint64_t pX);

    float getTransmitFrequencyBandwidth() const;
    void setTransmitFrequencyBandwidth(float pX);

    float getPower() const;
    void setPower(float pX);

    ModulationType &getModulationType();
    const ModulationType &getModulationType() const;
    void setModulationType(const ModulationType &pX);

    unsigned short getCryptoSystem() const;
    void setCryptoSystem(unsigned short pX);

    unsigned short getCryptoKeyId() const;
    void setCryptoKeyId(unsigned short pX);

    unsigned char getModulationParameterCount() const;

    virtual int getMarshalledSize() const;
    bool operator==(const TransmitterPdu &rhs) const;
};
} // namespace DIS
