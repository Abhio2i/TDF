#pragma once
#ifndef RADARANTENNA_AESA_H
#define RADARANTENNA_AESA_H
// =============================================================================
// radarantenna_aesa.h  —  Electronic beam steering + FIX-13 beam spoiling
// =============================================================================

#include "radarmodel_aesa.h"

namespace aesa {

class RadarAntenna_AESA
{
public:
    RadarAntenna_AESA() = default;

    void reset();

    // FIX-13: spoilFactor > 1 widens effective beam
    void   pointBeam(double az_deg, double el_deg,
                     const RadarConfig& cfg,
                     float spoilFactor = 1.0f);

    bool   isReachable(double az_deg, double el_deg,
                       const RadarConfig& cfg) const;

    double computeSteeringAngle(double beamAz, double beamEl,
                                double targetAz, double targetEl) const;

    // FIX-13: spoilFactor reduces gain by 1/sf²
    double computeArrayGain(double steeringAngle_deg,
                            const RadarConfig& cfg,
                            float spoilFactor = 1.0f) const;

    double currentAzimuth()       const { return currentAzimuth_;      }
    double currentElevation()     const { return currentElevation_;     }
    float  currentSpoilFactor()   const { return currentSpoilFactor_;   }
    double effectiveBeamWidth()   const { return effectiveBeamWidth_;   }
    bool   scanBoundaryOccurred() const { return scanBoundaryOccurred_; }

    void setScanBoundary()   { scanBoundaryOccurred_ = true;  }
    void clearScanBoundary() { scanBoundaryOccurred_ = false; }

private:
    double currentAzimuth_       = 0.0;
    double currentElevation_     = 0.0;
    float  currentSpoilFactor_   = 1.0f;
    double effectiveBeamWidth_   = 0.0;
    bool   scanBoundaryOccurred_ = false;
};

} // namespace aesa
#endif