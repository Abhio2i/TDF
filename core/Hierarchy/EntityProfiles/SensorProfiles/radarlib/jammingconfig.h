#pragma once
#ifndef JAMMINGCONFIG_H
#define JAMMINGCONFIG_H

/// Electronic countermeasure — jammer parameters.
/// Shared between Platform (entity system) and RadarModel (simulation).
/// Lives in radar library public headers — ships with radarlib.so
struct JammerConfig
{
    bool   active        = false;
    double power_kW      = 0.0;
    double gain_dBi      = 0.0;
    double bandwidth_Hz  = 1e6;
    double range_m       = 0.0;   ///< Stand-off range; ignored when selfScreening
    bool   selfScreening = false; ///< true = jammer rides on the target itself
};

#endif
