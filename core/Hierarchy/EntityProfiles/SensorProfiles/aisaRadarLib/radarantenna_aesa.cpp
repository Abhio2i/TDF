// radarantenna_aesa.cpp  —  Rev 3
#include "radarantenna_aesa.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace aesa {

void RadarAntenna_AESA::reset()
{
    currentAzimuth_ = currentElevation_ = 0.0;
    currentSpoilFactor_   = 1.0f;
    effectiveBeamWidth_   = 0.0;
    scanBoundaryOccurred_ = false;
}

void RadarAntenna_AESA::pointBeam(double az_deg, double el_deg,
                                   const RadarConfig& cfg,
                                   float spoilFactor)
{
    if (!std::isfinite(az_deg)) az_deg = 0.0;
    if (!std::isfinite(el_deg)) el_deg = 0.0;
    // ADD — normalise to -180..+180 before reachability check
    while (az_deg >  180.0) az_deg -= 360.0;
    while (az_deg < -180.0) az_deg += 360.0;

    if (!isReachable(az_deg, el_deg, cfg))
    {
        double maxAng = static_cast<double>(cfg.maxSteeringAngle_deg);
        az_deg = std::clamp(az_deg, -maxAng, maxAng);
        el_deg = std::clamp(el_deg,
                            static_cast<double>(cfg.minElevation),
                            static_cast<double>(cfg.maxElevation));
    }
    currentAzimuth_     = az_deg;
    currentElevation_   = el_deg;
    currentSpoilFactor_ = std::max(1.0f, spoilFactor);
    effectiveBeamWidth_ = static_cast<double>(cfg.beamWidth) * currentSpoilFactor_;
}

bool RadarAntenna_AESA::isReachable(double az_deg, double el_deg,
                                     const RadarConfig& cfg) const
{
    double azRad = az_deg * M_PI / 180.0;
    double elRad = el_deg * M_PI / 180.0;
    double bx    = std::cos(elRad) * std::cos(azRad);
    double steer = std::acos(std::clamp(bx, -1.0, 1.0)) * 180.0 / M_PI;
    return steer <= static_cast<double>(cfg.maxSteeringAngle_deg);
}

double RadarAntenna_AESA::computeSteeringAngle(double beamAz, double beamEl,
                                                double targetAz, double targetEl) const
{
    double a1 = beamAz   * M_PI / 180.0, e1 = beamEl   * M_PI / 180.0;
    double a2 = targetAz * M_PI / 180.0, e2 = targetEl * M_PI / 180.0;
    double dot = std::cos(e1)*std::cos(e2)*std::cos(a1-a2) + std::sin(e1)*std::sin(e2);
    return std::acos(std::clamp(dot, -1.0, 1.0)) * 180.0 / M_PI;
}

double RadarAntenna_AESA::computeArrayGain(double steeringAngle_deg,
                                           const RadarConfig& cfg,
                                           float spoilFactor) const
{
    int    active = std::max(0, cfg.numElements - cfg.failedModules);

    // ADD — dead array has zero gain, full stop
    if (active <= 0) return 0.0;

    //double ratio = static_cast<double>(active) / cfg.numElements;

    double ratio  = (cfg.numElements > 0)
                       ? static_cast<double>(active) / cfg.numElements : 1.0;

    double theta  = steeringAngle_deg * M_PI / 180.0;
    double G_bore = std::pow(10.0, static_cast<double>(cfg.antennaGain) / 10.0);
    double sf     = std::max(1.0, static_cast<double>(spoilFactor));

    // Taylor aperture weighting — more realistic than cos(theta)
    // Element pattern: cos^1.5(theta) — standard for patch elements
    double elementPattern = std::pow(std::max(0.0, std::cos(theta)), 1.5);

    // Array factor rolls off faster than element pattern
    // At max steering angle (typically 60°), gain drops ~3dB vs boresight
    double steerLoss = 1.0;
    double maxSteer  = static_cast<double>(cfg.maxSteeringAngle_deg) * M_PI / 180.0;
    if (theta > 0.0 && maxSteer > 0.0)
    {
        // Cosine^2 roll-off for phased array
        double relTheta = theta / maxSteer;
        steerLoss = std::pow(std::cos(theta), 2.0)
                    * std::exp(-2.0 * relTheta * relTheta);
        steerLoss = std::max(steerLoss, 1e-4); // floor at -40dB
    }

    // Failed module gain degradation
    // Each failed module reduces gain by ratio² (coherent loss)
    double moduleLoss = ratio * ratio;

    return G_bore * elementPattern * steerLoss * moduleLoss / (sf * sf);
}
// double RadarAntenna_AESA::computeArrayGain(double steeringAngle_deg,
//                                             const RadarConfig& cfg,
//                                             float spoilFactor) const
// {
//     int    active = std::max(0, cfg.numElements - cfg.failedModules);
//     double ratio  = (cfg.numElements > 0) ? static_cast<double>(active) / cfg.numElements : 1.0;
//     double theta  = steeringAngle_deg * M_PI / 180.0;
//     double G_bore = std::pow(10.0, static_cast<double>(cfg.antennaGain) / 10.0);
//     double sf     = std::max(1.0, static_cast<double>(spoilFactor));
//     return G_bore * std::max(0.0, std::cos(theta)) * ratio / (sf * sf);
// }

}

