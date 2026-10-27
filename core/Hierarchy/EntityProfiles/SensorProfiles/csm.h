// #ifndef CSM_H
// #define CSM_H
// #include "core/Hierarchy/EntityProfiles/sensor.h"
// #include "core/Hierarchy/EntityProfiles/SensorProfiles/CSM/csmtypes.h"
// #include "core/Hierarchy/EntityProfiles/SensorProfiles/CSM/csmspectrum.h"
// #include "core/Hierarchy/EntityProfiles/SensorProfiles/CSM/csmmonitor.h"
// #include <limits>
// #include <map>
// #include <memory>
// #include <string>
// class CSM: public Sensor
// {
//     Q_OBJECT
// public:
//     explicit CSM(Hierarchy* h);
//     void scan() override;
//     QJsonObject toJson() const override;
//     void fromJson(const QJsonObject& obj) override;
//     QJsonObject AdditionalParameters;   //!< User-defined extension parameters

//     //! Optional CSM detection threshold [dBm]. NaN = unset (no gating).
//     //! Not the emitter radio's receiver sensitivity. Persisted (see toJson).
//     float sensitivityDbm = std::numeric_limits<float>::quiet_NaN();

//     //! Minimum SNR [dB] over the signal's occupied bandwidth for a detection.
//     //! Default 0 dB = signal power >= noise power in that bandwidth. NaN = off.
//     float minSnrDb = 0.0f;

//     // --- Monitoring configuration (see CSM/csmmonitor.h and CSM_README.md) ---
//     //! true (default): automatic receiver range (100 MHz - 6 GHz capability).
//     //! false: manual range from frequency [GHz] and bandwidth [MHz].
//     bool autoMonitor = false;
//     //! false: MONITOR (whole range at once). true: SCAN (stepped window).
//     bool scanMode = false;
//     float scanStepMhz = 100.0f;   //!< SCAN window width [MHz]
//     float scanDwellS = 0.5f;      //!< SCAN dwell per step [s]

//     //! Monitoring window used by the most recent scan (valid flag + reason).
//     const csm::monitor::Window& lastMonitor() const
//     {
//         static const csm::monitor::Window kNone;
//         return Active ? lastMonitor_ : kNone;
//     }

//     //! Result of the most recent scan (empty if the sensor is inactive).
//     const csm::ScanResult& lastScanResult() const
//     {
//         static const csm::ScanResult kNone;
//         return Active ? lastScan_ : kNone;
//     }

//     //! Modelled spectrum of the most recent scan; null if the sensor is inactive
//     //! or has not scanned yet. Shared and immutable: safe to hold across scans.
//     std::shared_ptr<const csm::spectrum::Result> lastSpectrum() const
//     {
//         return Active ? spectrumCache_.result()
//                       : std::shared_ptr<const csm::spectrum::Result>();
//     }

// private:
//     void clearScanState();
//     void dropPlatformTargets();
//     csm::ScanResult lastScan_;
//     csm::spectrum::Cache spectrumCache_;
//     csm::monitor::Window lastMonitor_;
//     // SCAN mode only: last detection per emitter, kept for one sweep so the
//     // table does not flicker between dwells. Cleared in MONITOR mode.
//     std::map<std::string, csm::Observation> held_;
//     double lastScanTimeS_ = -1.0;
// };

// #endif // CSM_H














#ifndef CSM_H
#define CSM_H
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/CSM/csmtypes.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/CSM/csmspectrum.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/CSM/csmmonitor.h"
#include <limits>
#include <map>
#include <memory>
#include <string>
class CSM: public Sensor
{
    Q_OBJECT
public:
    explicit CSM(Hierarchy* h);
    void scan() override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
    QJsonObject AdditionalParameters;   //!< User-defined extension parameters

    //! Optional CSM detection threshold [dBm]. NaN = unset (no gating).
    //! Not the emitter radio's receiver sensitivity. Persisted (see toJson).
    float sensitivityDbm = std::numeric_limits<float>::quiet_NaN();

    //! Minimum SNR [dB] over the signal's occupied bandwidth for a detection.
    //! Default 0 dB = signal power >= noise power in that bandwidth. NaN = off.
    float minSnrDb = 0.0f;

    // --- Monitoring configuration (see CSM/csmmonitor.h and CSM_README.md) ---
    //! true (default): automatic receiver range (100 MHz - 6 GHz capability).
    //! false: manual range from frequency [GHz] and bandwidth [MHz].
    bool autoMonitor = false;
    //! false: MONITOR (whole range at once). true: SCAN (stepped window).
    bool scanMode = false;
    float scanStepMhz = 100.0f;   //!< SCAN window width [MHz]
    float scanDwellS = 0.5f;      //!< SCAN dwell per step [s]

    // --- Operating mode (RECEIVE = existing ES receiver; TRANSMIT = jammer) ---
    //! The CSM is ONE entity with an explicit mode. Default RECEIVE keeps all
    //! pre-existing behaviour. Use setOperatingMode() so stale receive state
    //! is dropped on a switch.
    csm::OperatingMode operatingMode() const { return opMode_; }
    void setOperatingMode(csm::OperatingMode m);

    // TRANSMIT-mode source configuration (single source of truth; the antenna
    // gain is the CSM's existing antennaGain [dBi]).
    bool  txEnabled = true;          //!< transmission on/off (TRANSMIT mode only)
    float txFrequencyGhz = 0.15f;    //!< interference centre frequency [GHz]
    float txBandwidthMhz = 10.0f;    //!< interference bandwidth [MHz], > 0
    float txPowerDbm = 30.0f;        //!< transmit power [dBm]

    //! True only while this CSM really emits: Active && TRANSMIT && txEnabled.
    bool isTransmitting() const { return Active && opMode_ == csm::OperatingMode::Transmit && txEnabled; }

    //! Interference source description for receivers (no path loss, no identity).
    //! Returns false when not transmitting or when the configuration is invalid.
    bool interferenceSource(csm::InterfererInput& out) const;

    //! Monitoring window used by the most recent scan (valid flag + reason).
    const csm::monitor::Window& lastMonitor() const
    {
        static const csm::monitor::Window kNone;
        return Active ? lastMonitor_ : kNone;
    }

    //! Result of the most recent scan (empty if the sensor is inactive).
    const csm::ScanResult& lastScanResult() const
    {
        static const csm::ScanResult kNone;
        return Active ? lastScan_ : kNone;
    }

    //! Modelled spectrum of the most recent scan; null if the sensor is inactive
    //! or has not scanned yet. Shared and immutable: safe to hold across scans.
    std::shared_ptr<const csm::spectrum::Result> lastSpectrum() const
    {
        return Active ? spectrumCache_.result()
                      : std::shared_ptr<const csm::spectrum::Result>();
    }

private:
    void clearScanState();
    void dropPlatformTargets();
    csm::OperatingMode opMode_ = csm::OperatingMode::Receive;
    csm::ScanResult lastScan_;
    csm::spectrum::Cache spectrumCache_;
    csm::monitor::Window lastMonitor_;
    // SCAN mode only: last detection per emitter, kept for one sweep so the
    // table does not flicker between dwells. Cleared in MONITOR mode.
    std::map<std::string, csm::Observation> held_;
    double lastScanTimeS_ = -1.0;
};

#endif // CSM_H
