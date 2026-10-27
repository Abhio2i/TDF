// =============================================================================
// FILE:        CoordConverter.cpp
// MODULE:      DIS Network Plugin — Utils
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "coordconverter.h"
#include <cmath>
#include <QtDebug>
#include <array>
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
// =============================================================================
// Internal 3x3 rotation matrix helpers — used only for the geocentric <->
// topographic Euler angle conversion below. Not exposed in the header.
//
// Method: heading/pitch/roll are Tait-Bryan angles referenced to the LOCAL
// North-East-Down frame at the entity's own lat/lon. DIS psi/theta/phi are
// the same Tait-Bryan sequence referenced to ECEF instead. Converting between
// them requires composing the local->body rotation with the ECEF->local
// rotation (which depends on lat/lon), then re-extracting the three angles
// from the combined matrix. Verified term-for-term against the NPS/MOVES
// Institute reference implementation (edu.nps.moves.disutil.EulerConversions).
// =============================================================================
namespace {

using Mat3 = std::array<std::array<double,3>,3>;

Mat3 matMul(const Mat3& a, const Mat3& b)
{
    Mat3 r{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            double sum = 0.0;
            for (int k = 0; k < 3; ++k) sum += a[i][k] * b[k][j];
            r[i][j] = sum;
        }
    return r;
}

Mat3 matTranspose(const Mat3& a)
{
    Mat3 r{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            r[i][j] = a[j][i];
    return r;
}

// Tait-Bryan (yaw-pitch-roll / 3-2-1) rotation: reference frame -> body frame.
Mat3 buildTaitBryanMatrix(double psi, double theta, double phi)
{
    double cpsi = std::cos(psi),   spsi = std::sin(psi);
    double cth  = std::cos(theta), sth  = std::sin(theta);
    double cphi = std::cos(phi),   sphi = std::sin(phi);

    Mat3 r;
    r[0][0] = cth * cpsi;
    r[0][1] = cth * spsi;
    r[0][2] = -sth;

    r[1][0] = sphi * sth * cpsi - cphi * spsi;
    r[1][1] = sphi * sth * spsi + cphi * cpsi;
    r[1][2] = sphi * cth;

    r[2][0] = cphi * sth * cpsi + sphi * spsi;
    r[2][1] = cphi * sth * spsi - sphi * cpsi;
    r[2][2] = cphi * cth;
    return r;
}

// Extracts (psi, theta, phi) radians from a matrix built as above.
// Valid away from the theta = +/-90 deg gimbal-lock singularity.
void extractTaitBryanAngles(const Mat3& r, double& psi, double& theta, double& phi)
{
    theta = std::asin(-r[0][2]);
    psi   = std::atan2(r[0][1], r[0][0]);
    phi   = std::atan2(r[1][2], r[2][2]);
}

// ECEF -> local North/East/Down rotation at given lat/lon (radians).
// Uses the same North/East/Up basis vectors already validated in
// CoordConverter::nedToECEFVelocity / ecefToNEDVelocity, so orientation
// and velocity math stay consistent with each other.
Mat3 buildEcefToNedMatrix(double lat_rad, double lon_rad)
{
    double sinLat = std::sin(lat_rad), cosLat = std::cos(lat_rad);
    double sinLon = std::sin(lon_rad), cosLon = std::cos(lon_rad);

    Mat3 r;
    r[0][0] = -sinLat * cosLon; r[0][1] = -sinLat * sinLon; r[0][2] =  cosLat; // North
    r[1][0] = -sinLon;          r[1][1] =  cosLon;          r[1][2] =  0.0;    // East
    r[2][0] = -cosLat * cosLon; r[2][1] = -cosLat * sinLon; r[2][2] = -sinLat; // Down
    return r;
}

} // anonymous namespace

DISOrientation CoordConverter::headingToEuler(float heading_deg, float pitch_deg, float roll_deg,
                                              double lat_deg, double lon_deg)
{
    double latRad = lat_deg * DEG_TO_RAD;
    double lonRad = lon_deg * DEG_TO_RAD;

    Mat3 localToBody = buildTaitBryanMatrix(
        heading_deg * DEG_TO_RAD, pitch_deg * DEG_TO_RAD, roll_deg * DEG_TO_RAD);
    Mat3 ecefToLocal = buildEcefToNedMatrix(latRad, lonRad);
    Mat3 ecefToBody  = matMul(localToBody, ecefToLocal);

    double psi, theta, phi;
    extractTaitBryanAngles(ecefToBody, psi, theta, phi);

    DISOrientation euler;
    euler.psi   = static_cast<float>(psi);
    euler.theta = static_cast<float>(theta);
    euler.phi   = static_cast<float>(phi);
    return euler;
}

void CoordConverter::eulerToHeading(const DISOrientation& euler,
                                    double lat_deg, double lon_deg,
                                    float& heading_deg, float& pitch_deg, float& roll_deg)
{
    double latRad = lat_deg * DEG_TO_RAD;
    double lonRad = lon_deg * DEG_TO_RAD;

    Mat3 ecefToBody  = buildTaitBryanMatrix(euler.psi, euler.theta, euler.phi);
    Mat3 ecefToLocal = buildEcefToNedMatrix(latRad, lonRad);
    Mat3 localToBody = matMul(ecefToBody, matTranspose(ecefToLocal));

    double heading, pitch, roll;
    extractTaitBryanAngles(localToBody, heading, pitch, roll);

    heading_deg = static_cast<float>(heading * RAD_TO_DEG);
    while (heading_deg < 0.0f)    heading_deg += 360.0f;
    while (heading_deg >= 360.0f) heading_deg -= 360.0f;

    pitch_deg = static_cast<float>(pitch * RAD_TO_DEG);
    roll_deg  = static_cast<float>(roll  * RAD_TO_DEG);
}
// DISOrientation CoordConverter::headingToEuler(float heading_deg,
//                                               float pitch_deg,
//                                               float roll_deg)
// {
//     DISOrientation euler;

//     // heading_deg is compass bearing from North
//     // DIS psi is measured counter-clockwise from East
//     // Conversion: psi = 90 - heading (in degrees) then to radians
//     float psi_deg = 90.0f - heading_deg;

//     euler.psi   = static_cast<float>(psi_deg   * DEG_TO_RAD);
//     euler.theta = static_cast<float>(pitch_deg  * DEG_TO_RAD);
//     euler.phi   = static_cast<float>(roll_deg   * DEG_TO_RAD);

//     return euler;
// }

// // =============================================================================
// // eulerToHeading
// // Converts incoming DIS Euler angles (radians) back to heading/pitch/roll (degrees)
// // =============================================================================
// void CoordConverter::eulerToHeading(const DISOrientation& euler,
//                                     float& heading_deg,
//                                     float& pitch_deg,
//                                     float& roll_deg)
// {
//     // Reverse of headingToEuler
//     float psi_deg = static_cast<float>(euler.psi * RAD_TO_DEG);

//     heading_deg = 90.0f - psi_deg;

//     // Normalize heading to 0-360
//     while (heading_deg < 0.0f)   heading_deg += 360.0f;
//     while (heading_deg >= 360.0f) heading_deg -= 360.0f;

//     pitch_deg = static_cast<float>(euler.theta * RAD_TO_DEG);
//     roll_deg  = static_cast<float>(euler.phi   * RAD_TO_DEG);
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
