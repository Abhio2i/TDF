

#ifndef RADIO_H
#define RADIO_H

#include "core/Hierarchy/EntityProfiles/Radio/include/radio/radio_interface.h"
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

    const std::string RadioTypeNames[3] = { "RECEIVER_ONLY",
                                            "TRANSMITTER_ONLY",
                                            "TRANSCEIVER"};

    const std::string CommsModeTypeNames[4] = { "LINE_OF_SIGHT",
                                            "BEYOND_LINE_OF_SIGHT",
                                            "SATCOM",
                                            "TROPOSCATTER"};

    const std::string SpreadSpectrumTypeNames[3] = {"NONE",
                                                    "FHSS",
                                                    "DSSS"};

    const std::string ModulationClassTypeNames[4] = { "AM",
                                                      "FM",
                                                      "PSK",
                                                      "QAM"};

    const std::string ModulationSchemeTypeNames[10] = { "AM",
                                                      "FM",
                                                      "BPSK",
                                                      "QPSK",
                                                      "PSK8",
                                                      "QAM16",
                                                      "QAM64",
                                                      "FSK2",
                                                      "FSK4",
                                                      "GMSK",
                                                      };

    const std::string EncryptionTypeNames[3] = {"NONE",
                                                "AES",
                                                "DES"};

    const std::string PolarizationTypeNames[4] = { "VERTICAL",
                                                    "HORIZONTAL",
                                                    "CIRCULAR_LEFT",
                                                    "CIRCULAR_RIGHT"};

    const std::string DuplexModeTypeNames[3] = {"SIMPLEX",
                                                "HALF_DUPLEX",
                                                "FULL_DUPLEX"};

    const std::string ScanTypeNames[3] = {"FIXED",
                                        "SECTOR_SCAN",
                                        "CONICAL_SCAN"};

    Entity* parentEntity = nullptr;
    RadioType radioType = RadioType::Transceiver;

    radio::PropagationModelConfig model_cfg;
    static radio::PropagationModel* model;
    radio::Radiolib* lib_radio;
    //Radio Transmitter
    float minFrequency = 8;
    float maxFrequency = 12;
    float power = 0;
    float powerDegradation = 0;

    //Envolope
    float minAzimuth = 0;//deg
    float maxAzimuth = 60;//deg
    float minElevation = 0;//deg
    float maxElevation = 60;//deg

    //Modulation
    float spreadSpecturm = 0;
    float majorModulation = 0;
    float detail = 0;
    float detailModulation = 0;

    //Radio Pulse
    float pulseWidth = 0;

    //Radio Antenna
    float AntennaGain = 1;
    float AntennaBandwidth = 1;
    float beamWidth = 1;
    int scanType = 0;
    int scanTime1 = 0;
    int scanTime2 = 0;
    float peakSideLobLevel = 1;
    float avgSideLobLevel = 1;

    float Range=10.0f;
    float bandwidth;
    float msgTimeStamp = 0;
    std::string msg = "hello";

    struct RadioTarget {
        Platform* entity;
        std::string name;     // Target platform name
        float radius = 0.0f;  // Distance from this radio (meters)
        float angle = 0.0f;   // Angle relative to this radio (degrees)
        float range = 0.0f;   // This radio's max range (meters)
        float frequency = 1.0f; // Frequency used (MHz)
    };
    void scan();
    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;
    // ---- RadioTarget access for ScriptEngine ----
    int getRadioTargetCount() const;
    void sendMsg(std::string msg);

    bool getRadioTarget(
        int index,
        std::string& outName,
        float& outRadius,
        float& outAngle,
        float& outRange,
        float& outFrequency
        ) const;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
    std::unordered_set<Platform*> detects;
    QVector<RadioTarget> targets;
};

#endif // RADIO_H
