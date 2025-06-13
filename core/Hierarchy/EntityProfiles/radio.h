
#ifndef RADIO_H
#define RADIO_H

#include <core/Hierarchy/entity.h>
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <string>
#include <vector>

class Radio : public Entity
{
    Q_OBJECT
public:
    explicit Radio(Hierarchy* h);

    // Radio attributes
    enum class RadioType { Transmitter, Receiver, Transceiver };
    enum class SpreadSpectrum { FHSS, DSSS, None };
    enum class MajorModulation { AM, FM, PSK, QAM };
    enum class EncryptionType { AES, DES, None };

    struct Modulation {
        SpreadSpectrum spreadSpectrum = SpreadSpectrum::None;
        MajorModulation majorModulation = MajorModulation::AM;
        std::string detailModulation;
    };

    struct Message {
        std::string timeStamp; // Assumed as string, can be changed to QDateTime or other type
        std::string source; // Entity Reference
        std::string destination; // Entity Reference
        std::string content; // Message Content (string or reference)
    };

    RadioType radioType = RadioType::Transceiver;
    Modulation modulation;
    float frequencyMin = 0.0f; // MHz
    float frequencyMax = 0.0f; // MHz
    float emittingPower = 0.0f; // Watts
    float bandwidth = 0.0f; // kHz
    float dataRate = 0.0f; // kbps
    EncryptionType encryptionType = EncryptionType::None;
    int channelCount = 1;
    bool jammingResistance = false;
    float antennaGain = 0.0f; // dBi
    float noiseFigure = 0.0f; // dB
    std::vector<Message> messages;
    float frequencyUsed = 0.0f; // MHz

    void spawn() override;
    std::vector<Component*> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

private:
    QString radioTypeToString(RadioType rt) const;
    RadioType stringToRadioType(const QString& str) const;
    QString spreadSpectrumToString(SpreadSpectrum ss) const;
    SpreadSpectrum stringToSpreadSpectrum(const QString& str) const;
    QString majorModulationToString(MajorModulation mm) const;
    MajorModulation stringToMajorModulation(const QString& str) const;
    QString encryptionTypeToString(EncryptionType et) const;
    EncryptionType stringToEncryptionType(const QString& str) const;
};

#endif // RADIO_H
