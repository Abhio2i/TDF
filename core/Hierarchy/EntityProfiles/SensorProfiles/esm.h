// // #ifndef ESM_H
// // #define ESM_H
// // #include "core/Hierarchy/EntityProfiles/sensor.h"


// /* One ESM detection produced by ESM::scan().
//  *
//  * scan() currently fills ONLY: entity, angle, radius.
//  *   angle  : degrees, atan2(dLon, dLat) -> clockwise from north,
//  *            range (-180, 180]  (UI normalises to 0..360)
//  *   radius : km (platform-to-platform distance computed by scan())
//  *
//  * All other fields are reserved for the future ESM processing modules.
//  * They are now default-initialised (previously indeterminate) and are
//  * only meaningful when signalValid == true. Zero is NOT a valid
//  * "missing" marker, so the UI checks signalValid instead of testing
//  * for zero. */
// struct ESMTarget {
//     Platform *entity = nullptr;
//     double angle = 0.0;
//     double radius = 0.0;
//     quint32 timesec = 0;
//     quint32 time_frac = 0;
//     quint32 emitterid = 0;
//     char emitter_name = 0;
//     quint32 pri = 0;
//     quint16 frequency = 0;
//     quint8 frequencytype = 0;
//     quint8 pritype = 0;
//     quint32 pw = 0;
//     quint16 platformed = 0;
//     bool signalValid = false;   //!< true only when emitterid/frequency/... hold real measurements
// };
// class ESM: public Sensor
// {
//     Q_OBJECT
// public:
//     explicit ESM(Hierarchy* h);
//     // float frequency = 8;//ghz
//     // float azimuth = 60;//deg
//     // float range = 100;//km
//     QVector<ESMTarget> detect;
//     void scan() override;
//     QJsonObject toJson() const override;
//     void fromJson(const QJsonObject& obj) override;
//     QJsonObject AdditionalParameters;   //!< User-defined extension parameters

// };

// #endif // ESM_H

//(Vansh)
#ifndef ESM_H
#define ESM_H
#include <QString>
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/ESM/esmrf.h"

/* One ESM detection.
 *  GEOMETRY (always valid): entity, angle [deg, (-180,180], cw from north],
 *           radius [km, GEOMETRIC platform distance, not an RF range estimate].
 *  EMITTER (emitterValid): emitterid, frequencyMHz = NOMINAL (emitter-configured),
 *           emitterType.
 *  RF LINK (rfValid): SIMULATED link-budget result: bandwidthHz, rxPowerDbm, snrDb, pathLossDb.
 *  Legacy fields are untouched; signalValid stays false (no real measurement exists). */
struct ESMTarget {
    Platform *entity = nullptr;
    double angle = 0.0;
    double radius = 0.0;

    // Identity of the actual emitting sensor, not just its host platform. (Vansh)
    QString emitterSensorId;
    QString emitterSensorName;

    quint32 timesec = 0;
    quint32 time_frac = 0;
    quint32 emitterid = 0;
    char emitter_name = 0;
    quint32 pri = 0;
    quint16 frequency = 0;
    quint8 frequencytype = 0;
    quint8 pritype = 0;
    quint32 pw = 0;
    quint16 platformed = 0;
    bool signalValid = false;

    bool    emitterValid = false;
    double  frequencyMHz = 0.0;
    QString emitterType;
    bool    rfValid = false;
    double  bandwidthHz = 0.0;
    double  rxPowerDbm = 0.0;
    double  snrDb = 0.0;
    double  pathLossDb = 0.0;
};

class ESM : public Sensor
{
    Q_OBJECT
public:
    explicit ESM(Hierarchy* h);

    QVector<ESMTarget> detect;
    ESMReceiverConfig rx;

    void scan() override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
    QJsonObject AdditionalParameters;

    // Simulated spectrum (from the link model, NOT from waveform samples)
    void setSpectrumMode(esmrf::SpectrumMode m) { specAcc_.setMode(m); specPsdDbm_.clear(); }
    esmrf::SpectrumMode spectrumMode() const { return specAcc_.mode(); }
    void resetSpectrum() { specAcc_.reset(); specPsdDbm_.clear(); }
    bool hasSpectrum() const { return !specPsdDbm_.isEmpty(); }
    const QVector<double>& spectrumFreqMHz() const { return specAcc_.freqMHz(); }
    const QVector<double>& spectrumPsdDbmPerHz() const { return specPsdDbm_; }

private:
    void clearOutputs();
    esmrf::SpectrumAccumulator specAcc_;
    QVector<double> specPsdDbm_;
};
#endif