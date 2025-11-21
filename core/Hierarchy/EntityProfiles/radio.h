

#ifndef RADIO_H
#define RADIO_H

#include "core/Hierarchy/EntityProfiles/platform.h"
#include <core/Hierarchy/entity.h>
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <string>
#include <vector>
#include <unordered_set>
class CanvasWidget;
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
    float Range=100.0f;
    float frequencyMin = 1.0f; // MHz
    float frequencyMax = 1.0f; // MHz
    float emittingPower = 1.0f; // Watts
    float bandwidth = 0.0f; // kHz
    float dataRate = 0.0f; // kbps
    EncryptionType encryptionType = EncryptionType::None;
    int channelCount = 1;
    bool jammingResistance = false;
    float antennaGain = 0.0f; // dBi
    float noiseFigure = 0.0f; // dB
    std::vector<Message> messages;
    float frequencyUsed = 1.0f; // MHz
    float receiverSensitivity = -100.0f;   // dBm, typical small radio
    float systemLoss = 2.0f;               // dB, cable/connectors
    float fadeMargin = 10.0f;              // dB, reliability buffer
    float receiverAntennaGain = -1.0f;     // dBi, -1 = use antennaGain automatically
    float pathLossExponent = 2.0f;         // Free space default
    struct RadioTarget {
        Platform* entity;
        std::string name;     // Target platform name
        float radius = 0.0f;  // Distance from this radio (meters)
        float angle = 0.0f;   // Angle relative to this radio (degrees)
        float range = 0.0f;   // This radio's max range (meters)
        float frequency = 1.0f; // Frequency used (MHz)
    };
    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
    float calculateRange() const;
    // --- Auto connections ---
    void updateAvailableConnections(Transform* source); // Scans hierarchy for compatible radios
    std::vector<RadioTarget> targets;
signals:
    void availableConnectionsUpdated(const QJsonArray& connArray); // <--- Add this
private:
    QString radioTypeToString(RadioType rt) const;
    RadioType stringToRadioType(const QString& str) const;
    QString spreadSpectrumToString(SpreadSpectrum ss) const;
    SpreadSpectrum stringToSpreadSpectrum(const QString& str) const;
    QString majorModulationToString(MajorModulation mm) const;
    MajorModulation stringToMajorModulation(const QString& str) const;
    QString encryptionTypeToString(EncryptionType et) const;
    EncryptionType stringToEncryptionType(const QString& str) const;
    // std::vector<RadioTarget> targets;
    static std::unordered_set<std::string> radioSeen;
};

#endif // RADIO_H
