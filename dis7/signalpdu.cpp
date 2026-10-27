#include <dis7/signalpdu.h>

using namespace DIS;

SignalPdu::SignalPdu() : RadioCommunicationsFamilyPdu(),
    _encodingScheme(0), _tdlType(0),
    _sampleRate(0), _dataLength(0), _samples(0)
{
    setPduType( 26 );
}

SignalPdu::~SignalPdu() { _data.clear(); }

EntityID& SignalPdu::getEntityID() { return getEntityId(); }
const EntityID& SignalPdu::getEntityID() const { return getEntityId(); }
void SignalPdu::setEntityID(const EntityID &pX) { setEntityId(pX); }

unsigned short SignalPdu::getRadioID() const { return getRadioId(); }
void SignalPdu::setRadioID(unsigned short pX) { setRadioId(pX); }
unsigned short SignalPdu::getEncodingScheme() const { return _encodingScheme; }
void SignalPdu::setEncodingScheme(unsigned short pX) { _encodingScheme = pX; }

unsigned short SignalPdu::getTdlType() const { return _tdlType; }
void SignalPdu::setTdlType(unsigned short pX) { _tdlType = pX; }

unsigned int SignalPdu::getSampleRate() const { return _sampleRate; }
void SignalPdu::setSampleRate(unsigned int pX) { _sampleRate = pX; }

unsigned short SignalPdu::getDataLength() const
{
    return static_cast<unsigned short>(_data.size() * 8);
}

unsigned short SignalPdu::getSamples() const { return _samples; }
void SignalPdu::setSamples(unsigned short pX) { _samples = pX; }

std::vector<uint8_t>& SignalPdu::getData() { return _data; }
const std::vector<uint8_t>& SignalPdu::getData() const { return _data; }
void SignalPdu::setData(const std::vector<uint8_t> &pX) { _data = pX; }

void SignalPdu::marshal(DataStream& dataStream) const
{
    RadioCommunicationsFamilyPdu::marshal(dataStream);
    // _entityID.marshal(dataStream);   ← DELETE
    // dataStream << _radioID;          ← DELETE
    dataStream << _encodingScheme;
    dataStream << _tdlType;
    dataStream << _sampleRate;
    dataStream << getDataLength();
    dataStream << _samples;
    for (size_t idx = 0; idx < _data.size(); idx++)
        dataStream << _data[idx];
    size_t remainder = _data.size() % 4;
    if (remainder != 0)
        for (size_t i = 0; i < 4 - remainder; i++)
            dataStream << static_cast<uint8_t>(0);
}

void SignalPdu::unmarshal(DataStream& dataStream)
{
    RadioCommunicationsFamilyPdu::unmarshal(dataStream);

    dataStream >> _encodingScheme;
    dataStream >> _tdlType;
    dataStream >> _sampleRate;
    dataStream >> _dataLength;
    dataStream >> _samples;

    unsigned short dataLengthBytes = _dataLength / 8;
    _data.clear();
    for (unsigned short idx = 0; idx < dataLengthBytes; idx++) {
        uint8_t b;
        dataStream >> b;
        _data.push_back(b);
    }

    size_t remainder = dataLengthBytes % 4;
    if (remainder != 0)
        for (size_t i = 0; i < 4 - remainder; i++) {
            uint8_t discard;
            dataStream >> discard;
        }
}

bool SignalPdu::operator==(const SignalPdu& rhs) const
{
    bool ivarsEqual = RadioCommunicationsFamilyPdu::operator==(rhs);

    if (!(_encodingScheme == rhs._encodingScheme)) ivarsEqual = false;
    if (!(_tdlType == rhs._tdlType)) ivarsEqual = false;
    if (!(_sampleRate == rhs._sampleRate)) ivarsEqual = false;
    if (!(_samples == rhs._samples)) ivarsEqual = false;
    if (_data != rhs._data) ivarsEqual = false;
    return ivarsEqual;
}

int SignalPdu::getMarshalledSize() const
{
    int marshalSize = RadioCommunicationsFamilyPdu::getMarshalledSize();
    marshalSize += 2 + 2 + 2 + 4 + 2 + 2;
    marshalSize += static_cast<int>(_data.size());
    size_t remainder = _data.size() % 4;
    if (remainder != 0) marshalSize += static_cast<int>(4 - remainder);
    return marshalSize;
}
