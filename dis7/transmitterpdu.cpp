#include <dis7/transmitterpdu.h>

using namespace DIS;

TransmitterPdu::TransmitterPdu() : RadioCommunicationsFamilyPdu(),
    _radioType(), _transmitState(0), _inputSource(0),
    _padding1(0), _antennaLocation(), _relativeAntennaLocation(),
    _antennaPatternType(0), _antennaPatternCount(0), _frequency(0),
    _transmitFrequencyBandwidth(0.0f), _power(0.0f), _modulationType(),
    _cryptoSystem(0), _cryptoKeyId(0), _modulationParameterCount(0),
    _padding2(0), _padding3(0)
{
    setPduType( 25 );
}
TransmitterPdu::~TransmitterPdu() {}

EntityID& TransmitterPdu::getEntityID() { return getEntityId(); }
const EntityID& TransmitterPdu::getEntityID() const { return getEntityId(); }
void TransmitterPdu::setEntityID(const EntityID &pX) { setEntityId(pX); }

unsigned short TransmitterPdu::getRadioID() const { return getRadioId(); }
void TransmitterPdu::setRadioID(unsigned short pX) { setRadioId(pX); }

RadioType& TransmitterPdu::getRadioType() { return _radioType; }
const RadioType& TransmitterPdu::getRadioType() const { return _radioType; }
void TransmitterPdu::setRadioType(const RadioType &pX) { _radioType = pX; }

unsigned char TransmitterPdu::getTransmitState() const { return _transmitState; }
void TransmitterPdu::setTransmitState(unsigned char pX) { _transmitState = pX; }

unsigned char TransmitterPdu::getInputSource() const { return _inputSource; }
void TransmitterPdu::setInputSource(unsigned char pX) { _inputSource = pX; }

Vector3Double& TransmitterPdu::getAntennaLocation() { return _antennaLocation; }
const Vector3Double& TransmitterPdu::getAntennaLocation() const { return _antennaLocation; }
void TransmitterPdu::setAntennaLocation(const Vector3Double &pX) { _antennaLocation = pX; }

Vector3Float& TransmitterPdu::getRelativeAntennaLocation() { return _relativeAntennaLocation; }
const Vector3Float& TransmitterPdu::getRelativeAntennaLocation() const { return _relativeAntennaLocation; }
void TransmitterPdu::setRelativeAntennaLocation(const Vector3Float &pX) { _relativeAntennaLocation = pX; }

unsigned short TransmitterPdu::getAntennaPatternType() const { return _antennaPatternType; }
void TransmitterPdu::setAntennaPatternType(unsigned short pX) { _antennaPatternType = pX; }
unsigned short TransmitterPdu::getAntennaPatternCount() const { return _antennaPatternCount; }

uint64_t TransmitterPdu::getFrequency() const { return _frequency; }
void TransmitterPdu::setFrequency(uint64_t pX) { _frequency = pX; }

float TransmitterPdu::getTransmitFrequencyBandwidth() const { return _transmitFrequencyBandwidth; }
void TransmitterPdu::setTransmitFrequencyBandwidth(float pX) { _transmitFrequencyBandwidth = pX; }

float TransmitterPdu::getPower() const { return _power; }
void TransmitterPdu::setPower(float pX) { _power = pX; }

ModulationType& TransmitterPdu::getModulationType() { return _modulationType; }
const ModulationType& TransmitterPdu::getModulationType() const { return _modulationType; }
void TransmitterPdu::setModulationType(const ModulationType &pX) { _modulationType = pX; }

unsigned short TransmitterPdu::getCryptoSystem() const { return _cryptoSystem; }
void TransmitterPdu::setCryptoSystem(unsigned short pX) { _cryptoSystem = pX; }

unsigned short TransmitterPdu::getCryptoKeyId() const { return _cryptoKeyId; }
void TransmitterPdu::setCryptoKeyId(unsigned short pX) { _cryptoKeyId = pX; }

unsigned char TransmitterPdu::getModulationParameterCount() const { return _modulationParameterCount; }

void TransmitterPdu::marshal(DataStream& dataStream) const
{
    RadioCommunicationsFamilyPdu::marshal(dataStream);

    _radioType.marshal(dataStream);
    dataStream << _transmitState;
    dataStream << _inputSource;
    dataStream << _padding1;
    _antennaLocation.marshal(dataStream);
    _relativeAntennaLocation.marshal(dataStream);
    dataStream << _antennaPatternType;
    dataStream << _antennaPatternCount;
    dataStream << static_cast<unsigned long long>(_frequency);
    dataStream << _transmitFrequencyBandwidth;
    dataStream << _power;
    _modulationType.marshal(dataStream);
    dataStream << _cryptoSystem;
    dataStream << _cryptoKeyId;
    dataStream << _modulationParameterCount;
    dataStream << _padding2;
    dataStream << _padding3;
    // Variable-length antenna-pattern / modulation-parameter records would
    // be marshalled here if the counts above were non-zero — see class note.
}

void TransmitterPdu::unmarshal(DataStream& dataStream)
{
    RadioCommunicationsFamilyPdu::unmarshal(dataStream);

    _radioType.unmarshal(dataStream);
    dataStream >> _transmitState;
    dataStream >> _inputSource;
    dataStream >> _padding1;
    _antennaLocation.unmarshal(dataStream);
    _relativeAntennaLocation.unmarshal(dataStream);
    dataStream >> _antennaPatternType;
    dataStream >> _antennaPatternCount;

    unsigned long long freqTemp = 0;
    dataStream >> freqTemp;
    _frequency = static_cast<uint64_t>(freqTemp);

    dataStream >> _transmitFrequencyBandwidth;
    dataStream >> _power;
    _modulationType.unmarshal(dataStream);
    dataStream >> _cryptoSystem;
    dataStream >> _cryptoKeyId;
    dataStream >> _modulationParameterCount;
    dataStream >> _padding2;
    dataStream >> _padding3;

    // IMPORTANT: if a peer sends antennaPatternCount or
    // modulationParameterCount > 0, their variable-length records still
    // occupy bytes on the wire that this implementation does not consume —
    // that would desync the DataStream read position for anything after
    // this PDU in the buffer. Safe for the omnidirectional/no-extra-params
    // case; do not enable receipt from a peer known to populate those
    // fields until the variable-length records are implemented.
}

bool TransmitterPdu::operator==(const TransmitterPdu& rhs) const
{
    bool ivarsEqual = RadioCommunicationsFamilyPdu::operator==(rhs);
    if (!(_radioType == rhs._radioType)) ivarsEqual = false;
    if (!(_transmitState == rhs._transmitState)) ivarsEqual = false;
    if (!(_inputSource == rhs._inputSource)) ivarsEqual = false;
    if (!(_antennaLocation == rhs._antennaLocation)) ivarsEqual = false;
    if (!(_relativeAntennaLocation == rhs._relativeAntennaLocation)) ivarsEqual = false;
    if (!(_antennaPatternType == rhs._antennaPatternType)) ivarsEqual = false;
    if (!(_frequency == rhs._frequency)) ivarsEqual = false;
    if (!(_transmitFrequencyBandwidth == rhs._transmitFrequencyBandwidth)) ivarsEqual = false;
    if (!(_power == rhs._power)) ivarsEqual = false;
    if (!(_modulationType == rhs._modulationType)) ivarsEqual = false;
    if (!(_cryptoSystem == rhs._cryptoSystem)) ivarsEqual = false;
    if (!(_cryptoKeyId == rhs._cryptoKeyId)) ivarsEqual = false;
    return ivarsEqual;
}
int TransmitterPdu::getMarshalledSize() const
{
    int marshalSize = RadioCommunicationsFamilyPdu::getMarshalledSize();

    marshalSize += _radioType.getMarshalledSize();
    marshalSize += 1 + 1 + 2;
    marshalSize += _antennaLocation.getMarshalledSize();
    marshalSize += _relativeAntennaLocation.getMarshalledSize();
    marshalSize += 2 + 2;
    marshalSize += 8;
    marshalSize += 4 + 4;
    marshalSize += _modulationType.getMarshalledSize();
    marshalSize += 2 + 2;
    marshalSize += 1 + 2 + 1;
    return marshalSize;
}
