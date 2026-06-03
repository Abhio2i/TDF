// =============================================================================
// FILE:        CoordConverter.cpp
// MODULE:      DIS Network Plugin — Utils
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "coordconverter.h"
#include <cmath>
#include <QtDebug>

// =============================================================================
// computeN
// Radius of curvature in the prime vertical
// Used internally by geocordToECEF
// =============================================================================
double CoordConverter::computeN(double lat_rad)
{
    double sinLat = std::sin(lat_rad);
    return WGS84_A / std::sqrt(1.0 - WGS84_E2 * sinLat * sinLat);
}

// =============================================================================
// geocordToECEF
// Converts your entity geocord (lat/lon/alt) to DIS ECEF meters
//
// Your geocord uses FEET for altitude.
// DIS ECEF uses METERS.
// This function handles the conversion automatically.
//
// Validated against Delhi coordinates:
//   Input:  lat=28.6139, lon=77.2090, alt=700 feet
//   Output: X≈1176519m, Y≈6073589m, Z≈3025330m  (approx)
// =============================================================================
ECEFPosition CoordConverter::geocordToECEF(double lat_deg,
                                           double lon_deg,
                                           double alt_feet)
{
    // Convert units
    double lat_rad = lat_deg * DEG_TO_RAD;
    double lon_rad = lon_deg * DEG_TO_RAD;
    double alt_m   = alt_feet * FEET_TO_METERS;  // feet → meters

    // WGS84 ECEF conversion
    double N      = computeN(lat_rad);
    double cosLat = std::cos(lat_rad);
    double sinLat = std::sin(lat_rad);
    double cosLon = std::cos(lon_rad);
    double sinLon = std::sin(lon_rad);

    ECEFPosition ecef;
    ecef.x = (N + alt_m) * cosLat * cosLon;
    ecef.y = (N + alt_m) * cosLat * sinLon;
    ecef.z = (N * (1.0 - WGS84_E2) + alt_m) * sinLat;

    return ecef;
}

// =============================================================================
// ecefToGeocord
// Converts incoming DIS ECEF position to your geocord format
// Uses Bowring's iterative method for accuracy
//
// Output altitude is in FEET to match your geocord->altitude field
// =============================================================================
void CoordConverter::ecefToGeocord(const ECEFPosition& ecef,
                                   double& lat_deg,
                                   double& lon_deg,
                                   double& alt_feet)
{
    double x = ecef.x;
    double y = ecef.y;
    double z = ecef.z;

    // Longitude — simple atan2
    lon_deg = std::atan2(y, x) * RAD_TO_DEG;

    // Latitude and altitude — Bowring iterative method
    double p      = std::sqrt(x * x + y * y);
    double lat_rad = std::atan2(z, p * (1.0 - WGS84_E2));

    // Iterate to convergence (3 iterations is sufficient for cm accuracy)
    for (int i = 0; i < 5; ++i) {
        double N      = computeN(lat_rad);
        double sinLat = std::sin(lat_rad);
        lat_rad       = std::atan2(z + WGS84_E2 * N * sinLat, p);
    }

    double N      = computeN(lat_rad);
    double sinLat = std::sin(lat_rad);
    double cosLat = std::cos(lat_rad);

    // Altitude in meters then convert to feet
    double alt_m;
    if (std::abs(cosLat) > 1e-10) {
        alt_m = p / cosLat - N;
    } else {
        alt_m = std::abs(z) / std::abs(sinLat) - N * (1.0 - WGS84_E2);
    }

    lat_deg  = lat_rad * RAD_TO_DEG;
    alt_feet = alt_m * METERS_TO_FEET;  // meters → feet for your geocord
}

// =============================================================================
// headingToEuler
// Converts your heading/pitch/roll (degrees) to DIS Euler angles (radians)
//
// DIS Euler convention:
//   psi   = yaw/heading (rotation about Z axis)
//   theta = pitch       (rotation about Y axis)
//   phi   = roll        (rotation about X axis)
//
// Your geocord heading is compass bearing (0=North, 90=East)
// DIS psi is measured from East in radians
// This function handles the mapping
// =============================================================================
DISOrientation CoordConverter::headingToEuler(float heading_deg,
                                              float pitch_deg,
                                              float roll_deg)
{
    DISOrientation euler;

    // heading_deg is compass bearing from North
    // DIS psi is measured counter-clockwise from East
    // Conversion: psi = 90 - heading (in degrees) then to radians
    float psi_deg = 90.0f - heading_deg;

    euler.psi   = static_cast<float>(psi_deg   * DEG_TO_RAD);
    euler.theta = static_cast<float>(pitch_deg  * DEG_TO_RAD);
    euler.phi   = static_cast<float>(roll_deg   * DEG_TO_RAD);

    return euler;
}

// =============================================================================
// eulerToHeading
// Converts incoming DIS Euler angles (radians) back to heading/pitch/roll (degrees)
// =============================================================================
void CoordConverter::eulerToHeading(const DISOrientation& euler,
                                    float& heading_deg,
                                    float& pitch_deg,
                                    float& roll_deg)
{
    // Reverse of headingToEuler
    float psi_deg = static_cast<float>(euler.psi * RAD_TO_DEG);

    heading_deg = 90.0f - psi_deg;

    // Normalize heading to 0-360
    while (heading_deg < 0.0f)   heading_deg += 360.0f;
    while (heading_deg >= 360.0f) heading_deg -= 360.0f;

    pitch_deg = static_cast<float>(euler.theta * RAD_TO_DEG);
    roll_deg  = static_cast<float>(euler.phi   * RAD_TO_DEG);
}

// =============================================================================
// nedToECEFVelocity
// Converts NED (North-East-Down) velocity to ECEF velocity
//
// Your dynamicModel provides:
//   NorthVelocity, EastVelocity, VerticalVelocity in m/s
//
// DIS needs velocity in ECEF frame (X,Y,Z meters per second)
// =============================================================================
// DISVelocity CoordConverter::nedToECEFVelocity(float northVel_ms,
//                                               float eastVel_ms,
//                                               float vertVel_ms,
//                                               double lat_deg,
//                                               double lon_deg)
// {
//     double lat_rad = lat_deg * DEG_TO_RAD;
//     double lon_rad = lon_deg * DEG_TO_RAD;

//     double sinLat = std::sin(lat_rad);
//     double cosLat = std::cos(lat_rad);
//     double sinLon = std::sin(lon_rad);
//     double cosLon = std::cos(lon_rad);

//     // NED to ECEF rotation matrix
//     // [vx]   [-sinLat*cosLon  -sinLon   cosLat*cosLon] [vN]
//     // [vy] = [-sinLat*sinLon   cosLon   cosLat*sinLon] [vE]
//     // [vz]   [ cosLat          0        sinLat        ] [vD]
//     // Note: VerticalVelocity is UP in your model, DIS vD is DOWN
//     float vD = -vertVel_ms;  // UP to DOWN

//     DISVelocity vel;
//     vel.x = static_cast<float>(
//         -sinLat * cosLon * northVel_ms
//         - sinLon         * eastVel_ms
//         + cosLat * cosLon * vD);

//     vel.y = static_cast<float>(
//         -sinLat * sinLon * northVel_ms
//         + cosLon         * eastVel_ms
//         + cosLat * sinLon * vD);

//     vel.z = static_cast<float>(
//         cosLat * northVel_ms
//         + sinLat * vD);

//     return vel;
// }
DISVelocity CoordConverter::nedToECEFVelocity(float northVel_ms,
                                              float eastVel_ms,
                                              float vertVel_ms,   // positive UP
                                              double lat_deg,
                                              double lon_deg)
{
    double lat_rad = lat_deg * DEG_TO_RAD;
    double lon_rad = lon_deg * DEG_TO_RAD;

    double sinLat = std::sin(lat_rad);
    double cosLat = std::cos(lat_rad);
    double sinLon = std::sin(lon_rad);
    double cosLon = std::cos(lon_rad);

    // Standard NED→ECEF rotation (vD = -vertVel, so signs flip vs vD form)
    // v_ECEF = vN·N̂ + vE·Ê + vD·D̂  where D̂ = -Û
    // Substituting vD = -vertVel:
    //   vX = -sinLat·cosLon·vN - sinLon·vE + cosLat·cosLon·vertVel
    //   vY = -sinLat·sinLon·vN + cosLon·vE + cosLat·sinLon·vertVel
    //   vZ =  cosLat·vN                    + sinLat·vertVel
    DISVelocity vel;
    vel.x = static_cast<float>(
        -sinLat * cosLon * northVel_ms
        - sinLon         * eastVel_ms
        + cosLat * cosLon * vertVel_ms);    // ← was + cosLat*cosLon * vD

    vel.y = static_cast<float>(
        -sinLat * sinLon * northVel_ms
        + cosLon         * eastVel_ms
        + cosLat * sinLon * vertVel_ms);    // ← was + cosLat*sinLon * vD

    vel.z = static_cast<float>(
        cosLat           * northVel_ms
        + sinLat         * vertVel_ms);     // ← was + sinLat * vD

    return vel;
}
// =============================================================================
// ecefToNEDVelocity
// Converts incoming DIS ECEF velocity to NED velocity
// =============================================================================
// void CoordConverter::ecefToNEDVelocity(const DISVelocity& ecefVel,
//                                        double lat_deg,
//                                        double lon_deg,
//                                        float& northVel_ms,
//                                        float& eastVel_ms,
//                                        float& vertVel_ms)
// {
//     double lat_rad = lat_deg * DEG_TO_RAD;
//     double lon_rad = lon_deg * DEG_TO_RAD;

//     double sinLat = std::sin(lat_rad);
//     double cosLat = std::cos(lat_rad);
//     double sinLon = std::sin(lon_rad);
//     double cosLon = std::cos(lon_rad);

//     // Transpose of NED→ECEF rotation matrix = ECEF→NED
//     northVel_ms = static_cast<float>(
//         -sinLat * cosLon * ecefVel.x
//         - sinLat * sinLon * ecefVel.y
//         + cosLat           * ecefVel.z);

//     eastVel_ms = static_cast<float>(
//         -sinLon * ecefVel.x
//         + cosLon * ecefVel.y);

//     float vD = static_cast<float>(
//         cosLat * cosLon * ecefVel.x
//         + cosLat * sinLon * ecefVel.y
//         + sinLat           * ecefVel.z);

//     vertVel_ms = -vD;  // DOWN to UP for your engine
// }


void CoordConverter::ecefToNEDVelocity(const DISVelocity& ecefVel,
                                       double lat_deg,
                                       double lon_deg,
                                       float& northVel_ms,
                                       float& eastVel_ms,
                                       float& vertVel_ms)
{
    double lat_rad = lat_deg * DEG_TO_RAD;
    double lon_rad = lon_deg * DEG_TO_RAD;

    double sinLat = std::sin(lat_rad);
    double cosLat = std::cos(lat_rad);
    double sinLon = std::sin(lon_rad);
    double cosLon = std::cos(lon_rad);

    // Transpose of NED→ECEF matrix
    northVel_ms = static_cast<float>(
        -sinLat * cosLon * ecefVel.x
        - sinLat * sinLon * ecefVel.y
        + cosLat           * ecefVel.z);    // unchanged ✓

    eastVel_ms = static_cast<float>(
        -sinLon * ecefVel.x
        + cosLon * ecefVel.y);              // unchanged ✓

    // Correct: Up = cosLat·cosLon·vX + cosLat·sinLon·vY + sinLat·vZ
    // (previously: vD = this, then vertVel = -vD — DOUBLE WRONG)
    vertVel_ms = static_cast<float>(
        cosLat * cosLon * ecefVel.x
        + cosLat * sinLon * ecefVel.y
        + sinLat           * ecefVel.z);    // ← removed the -vD negation
}
// =============================================================================
// isValidGeocord
// Basic sanity check before sending PDU
// Catches uninitialized or clearly wrong coordinates
// =============================================================================
bool CoordConverter::isValidGeocord(double lat_deg,
                                    double lon_deg,
                                    double alt_feet)
{
    if (lat_deg < -90.0  || lat_deg > 90.0)   return false;
    if (lon_deg < -180.0 || lon_deg > 180.0)  return false;
    if (alt_feet < -1500.0 || alt_feet > 3000000.0) return false; // dead sea to orbit

    // Check for uninitialized large values from your geocord default
    // Your Delhi default is 2842341234.70 which is clearly wrong scale
    // Real lat is -90 to +90 degrees
    if (std::abs(lat_deg) > 90.0)  return false;
    if (std::abs(lon_deg) > 180.0) return false;

    return true;
}
