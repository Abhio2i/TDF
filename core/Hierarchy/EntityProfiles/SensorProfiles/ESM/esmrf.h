//============================================================================
// FILE: esmrf.h   MODULE: ESM   PROJECT: ISSST
// RF link budget, emitter interface, simulated spectrum. No Qt widgets and no
// Hierarchy dependency -> unit-testable.
// UNITS: frequency MHz, bandwidth Hz, power dBm, gain dBi, loss dB,
//        distance km, altitude m, PSD linear mW/Hz (dBm/Hz for display).
//============================================================================
#ifndef ESMRF_H
#define ESMRF_H

#include <QString>
#include <QVector>
#include <QtGlobal>
#include <limits>

/* Emitter-configured (NOMINAL) transmit parameters. */
struct RFEmitterParams {
    bool    emitting     = false;
    double  frequencyMHz = 0.0;
    double  bandwidthHz  = 0.0;   //!< occupied bandwidth (derived by the emitter)
    double  txPowerDbm   = 0.0;   //!< peak power at antenna port
    double  txGainDbi    = 0.0;   //!< main-beam gain
    double  txLossDb     = 0.0;   //!< feeder/radome loss (>= 0)
    double  sidelobePeakDb = std::numeric_limits<double>::quiet_NaN(); //!< rel. to main beam (<0); NaN = unavailable
    double  sidelobeAvgDb  = std::numeric_limits<double>::quiet_NaN();
    quint32 emitterId    = 0;     //!< 0 = ESM derives one
    QString typeLabel;
    bool isValid() const;
};

class IRFEmitter {
public:
    virtual ~IRFEmitter() = default;
    virtual bool rfEmitterParams(RFEmitterParams& out) const = 0;  // false = no usable RF data
};

enum class GainMode { MainBeam = 0, PeakSidelobe = 1, AverageSidelobe = 2 };

/* Receiver configuration. Defaults are ILLUSTRATIVE simulation defaults,
 * not verified equipment data. Weather/other losses default to 0 (not modelled). */
struct ESMReceiverConfig {
    double fMinMHz = 2000.0, fMaxMHz = 18000.0;
    double bandwidthHz = 1.0e6;
    double noiseFigureDb = 10.0;
    double sensitivityDbm = -90.0;
    double minSnrDb = 6.0;
    bool   useSnrThreshold = true;
    double rxGainDbi = 0.0;
    double rainDbPerKm = 0.0;         //!< gamma, applied over the whole path
    double atmosphericDbPerKm = 0.0;
    double otherLossDb = 0.0;
    bool   enableHorizon = true;
    double earthRadiusFactor = 1.33;
    GainMode gainMode = GainMode::MainBeam;
    int    spectrumPoints = 512;
    int    averageWindow = 16;
    bool   legacyGenericDetection = true;  //!< keep geometric-only detection for non-RF Generic sensors
    bool isValid() const;
};

namespace esmrf {

constexpr double kThermalNoiseDbmHz = -174.0;
constexpr double kMinDistanceKm = 0.001;

double mwToDbm(double mw);
double dbmToMw(double dbm);
double fsplDb(double frequencyMHz, double distanceKm);
double noisePowerDbm(double bandwidthHz, double noiseFigureDb);
/* Radio horizon sum (km) for two heights (m), effective earth radius factor k. */
double losHorizonKm(double h1m, double h2m, double k);

enum class LinkReason { Detected, InvalidInput, OutOfBand, BeyondHorizon, BelowSensitivity, BelowSnr };

struct RFLink {
    bool detected = false;
    LinkReason reason = LinkReason::InvalidInput;
    double pathLossDb = 0.0;       //!< FSPL + rain + atmosphere + other
    double rxPowerDbm = 0.0;       //!< total received signal power
    double channelPowerDbm = 0.0;  //!< part inside receiver bandwidth
    double noiseDbm = 0.0;
    double snrDb = 0.0;
};

/* losHorizonKm <= 0 disables the horizon check. */
RFLink computeLink(const RFEmitterParams& e, double distanceKm,
                   const ESMReceiverConfig& rx, double losHorizonKm = -1.0);

/* Max free-space+weather range (km) at which the emitter is still detectable,
 * ignoring horizon. NaN if invalid / out of band. Bisection (weather is linear in d). */
double maxRangeKm(const RFEmitterParams& e, const ESMReceiverConfig& rx);

struct SpectrumContribution { double freqMHz; double bandwidthHz; double rxPowerDbm; };

bool synthesizeSpectrum(const ESMReceiverConfig& rx,
                        const QVector<SpectrumContribution>& emitters,
                        QVector<double>& freqMHz, QVector<double>& psdMwPerHz);

enum class SpectrumMode { Live = 0, Average = 1, MaxHold = 2 };

class SpectrumAccumulator {
public:
    void setMode(SpectrumMode m) { if (m != mode_) { mode_ = m; reset(); } }
    SpectrumMode mode() const { return mode_; }
    void setWindow(int frames) { window_ = frames < 1 ? 1 : frames; }
    void reset();
    void update(const QVector<double>& freqMHz, const QVector<double>& psdMwPerHz);
    const QVector<double>& freqMHz() const { return freq_; }
    const QVector<double>& psdMwPerHz() const { return psd_; }
    QVector<double> psdDbmPerHz() const;
private:
    SpectrumMode mode_ = SpectrumMode::Live;
    int window_ = 16, count_ = 0;
    QVector<double> freq_, psd_;
};

} // namespace esmrf
#endif