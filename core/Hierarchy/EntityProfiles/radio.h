
// =============================================================================
// FILE:        radio.h
// MODULE:      Tactical Simulation Communication Systems
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Defines the Radio class, representing communication hardware
//               in the simulation. Manages transmission/reception parameters,
//               propagation models, modulation schemes, and signal encryption.
//
// AUTHOR:       Raj Singh
// REVIEWED BY:  [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Sep 2025  Initial implementation for TDF project.
//   Rev 2  Dec 2025  Added PropagationModel and Radiolib integration.
//   Rev 3  Mar 2026  Integrated RadioTarget tracking and scanning logic.
//   Rev 4  Apr 2026  Aligned with DO-178C documentation standards.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

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
#include "core/DISPlugin/interface/DISTransmitterSnapshot.h"
class CanvasWidget;

// =============================================================================
// CLASS: Radio
//
// DESCRIPTION: Concrete implementation of a tactical communication entity.
//              Handles signal propagation, frequency management, and target
//              detection via radio scanning.
// =============================================================================
class Radio : public Entity
{
    Q_OBJECT
public:
    explicit Radio(Hierarchy* h);
    ~Radio();

    // =========================================================================
    // SECTION: Communication Enumerations
    // DESCRIPTION: Definitions for hardware types, modulation, and encryption.
    // =========================================================================
    enum class RadioType { Transmitter, Receiver, Transceiver };
    enum class SpreadSpectrum { FHSS, DSSS, None };
    enum class MajorModulation { AM, FM, PSK, QAM };
    enum class EncryptionType { AES, DES, None };

    const std::string RadioTypeNames[3] = { "RECEIVER_ONLY", "TRANSMITTER_ONLY", "TRANSCEIVER"};
    const std::string CommsModeTypeNames[4] = { "LINE_OF_SIGHT", "BEYOND_LINE_OF_SIGHT", "SATCOM", "TROPOSCATTER"};
    const std::string SpreadSpectrumTypeNames[3] = {"NONE", "FHSS", "DSSS"};
    const std::string ModulationClassTypeNames[4] = { "AM", "FM", "PSK", "QAM"};
    const std::string ModulationSchemeTypeNames[10] = { "AM", "FM", "BPSK", "QPSK", "PSK8", "QAM16", "QAM64", "FSK2", "FSK4", "GMSK"};
    const std::string EncryptionTypeNames[3] = {"NONE", "AES", "DES"};
    const std::string PolarizationTypeNames[4] = { "VERTICAL", "HORIZONTAL", "CIRCULAR_LEFT", "CIRCULAR_RIGHT"};
    const std::string DuplexModeTypeNames[3] = {"SIMPLEX", "HALF_DUPLEX", "FULL_DUPLEX"};
    const std::string ScanTypeNames[3] = {"FIXED", "SECTOR_SCAN", "CONICAL_SCAN"};

    // =========================================================================
    // SECTION: Hardware & Propagation
    // DESCRIPTION: Physical radio attributes and signal propagation configuration.
    // =========================================================================
    Entity* parentEntity = nullptr;
    RadioType radioType = RadioType::Transceiver;

    radio::PropagationModelConfig model_cfg;
    static radio::PropagationModel* model;
    radio::Radiolib* lib_radio;

    // --- Frequency & Power ---
    float minFrequency = 8;
    float maxFrequency = 12;
    float power = 0;
    float powerDegradation = 0;

    // --- Envelope / Coverage ---
    float minAzimuth = 0;   // deg
    float maxAzimuth = 60;  // deg
    float minElevation = 0; // deg
    float maxElevation = 60;// deg

    // --- Signal Processing ---
    float spreadSpecturm = 0;
    float majorModulation = 0;
    float detail = 0;
    float detailModulation = 0;
    float pulseWidth = 0;

    // --- Antenna Configuration ---
    float AntennaGain = 1;
    float AntennaBandwidth = 1;
    float beamWidth = 1;
    int scanType = 0;
    int scanTime1 = 0;
    int scanTime2 = 0;
    float peakSideLobLevel = 1;
    float avgSideLobLevel = 1;

    // --- Operational State ---
    float Range = 10.0f;
    float bandwidth;
    float msgTimeStamp = 0;
    std::string msg = "";

    // =========================================================================
    // SECTION: Tracking Data Structures
    // DESCRIPTION: Structures for managing detected targets within radio range.
    // =========================================================================
    struct RadioTarget {
        Platform* entity;
        std::string name;
        float radius = 0.0f;
        float angle = 0.0f;
        float range = 0.0f;
        float frequency = 1.0f;
        double distance_m = 0.0;

        double radius_m = 0.0;
        double azimuth_deg = 0.0;

        double scanner_max_range_m = 0.0;
        double scanner_frequency_hz = 0.0;

        double path_loss_db = 0.0;
        double rx_power_dbm = 0.0;
        double noise_floor_dbm = 0.0;
        double snr_db = 0.0;
        double rain_attenuation_db = 0.0;
        double wind_attenuation_db_per_km = 0.0;
        double los_horizon_distance_m = 0.0;
        double polarization_loss_db = 0.0;
        double required_snr_threshold_db = 0.0;

        bool network_match = false;
        bool frequency_match = false;
        bool range_ok = false;
        bool beam_ok = false;
        bool sensitivity_ok = false;
        bool squelch_ok = false;
        bool snr_ok = false;

        // New: transmitter-only scan result can return a footprint record
        bool is_footprint = false;
        double heading_deg = 0.0;
        double beamwidth_deg = 0.0;


    };
    static void resetModel() {
        if (model) {
            delete model;
            model = nullptr;
        }
    }
    std::unordered_set<Platform*> detects;
    QVector<RadioTarget> targets;

    // =========================================================================
    // SECTION: Functional API
    // DESCRIPTION: Logic for scanning environment and message transmission.
    // =========================================================================
    void scan();
    void sendMsg(std::string msg);
    int getRadioTargetCount() const;

    bool getRadioTarget(
        int index,
        std::string& outName,
        float& outRadius,
        float& outAngle,
        float& outRange,
        float& outFrequency
        ) const;
    ECS::radioComponent& getRadioData() const;
    // =========================================================================
    // SECTION: Virtual Interface Overrides
    // DESCRIPTION: Entity lifecycle and component management.
    // =========================================================================
    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addSupportedcomponent() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;
    QJsonObject AdditionalParameters;   //!< User-defined extension parameters

    // =========================================================================
    // SECTION: Serialization
    // DESCRIPTION: Persistence logic for Radio state and configuration.
    // =========================================================================
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
    int radioIndex = 0;   // 0 = unassigned; RadioProfile assigns a real, per-platform-unique value at attachment

signals:
    void transmitterSnapshotReady(DISTransmitterSnapshot snap);

};

#endif // RADIO_H
