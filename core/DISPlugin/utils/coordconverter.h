// =============================================================================
// FILE:        CoordConverter.h
// MODULE:      DIS Network Plugin — Utils
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Converts between your engine's coordinate system and
//              DIS standard ECEF (Earth-Centered Earth-Fixed) coordinates.
//
// YOUR ENGINE uses:
//   geocord->latitude   degrees  (e.g. 28.6139 for Delhi)
//   geocord->longitude  degrees  (e.g. 77.2090 for Delhi)
//   geocord->altitude   FEET     (from your transform.cpp)
//   geocord->Heading    degrees
//
// DIS ECEF uses:
//   X, Y, Z in METERS from Earth center
//   Orientation as Euler angles psi/theta/phi in RADIANS
//
// USES your existing functions from entityutils.h:
//   xyzToGeo()      ← ECEF → lat/lon/alt (already exists)
//   geoToFlatXYZ()  ← lat/lon/alt → local XYZ (already exists)
//
// =============================================================================

#ifndef COORDCONVERTER_H
#define COORDCONVERTER_H

#include <cmath>

// =============================================================================
// ECEFPosition
// DIS wire format position — meters from Earth center
// Uses double precision — critical for accuracy at large coordinates
// =============================================================================
struct ECEFPosition {
    double x = 0.0;  // meters
    double y = 0.0;  // meters
    double z = 0.0;  // meters
};

// =============================================================================
// DISOrientation
// DIS wire format orientation — Euler angles in RADIANS
// =============================================================================
struct DISOrientation {
    float psi   = 0.0f;  // yaw   (radians) — from heading degrees
    float theta = 0.0f;  // pitch (radians) — from pitch degrees
    float phi   = 0.0f;  // roll  (radians) — from roll degrees
};

// =============================================================================
// DISVelocity
// DIS wire format velocity — meters per second in ECEF frame
// =============================================================================
struct DISVelocity {
    float x = 0.0f;  // m/s east component mapped to ECEF X
    float y = 0.0f;  // m/s north component mapped to ECEF Y
    float z = 0.0f;  // m/s vertical component mapped to ECEF Z
};

// =============================================================================
// CoordConverter
// Static utility class — no instance needed
// All methods are stateless and thread safe
// =============================================================================
class CoordConverter {
public:

    // ── WGS84 Earth constants ─────────────────────────────────────────────────
    static constexpr double WGS84_A  = 6378137.0;          // semi-major axis (meters)
    static constexpr double WGS84_F  = 1.0 / 298.257223563;// flattening
    static constexpr double WGS84_B  = WGS84_A * (1.0 - WGS84_F); // semi-minor axis
    static constexpr double WGS84_E2 = 2.0 * WGS84_F - WGS84_F * WGS84_F; // eccentricity squared

    // ── Unit conversions ──────────────────────────────────────────────────────
    static constexpr double DEG_TO_RAD = M_PI / 180.0;
    static constexpr double RAD_TO_DEG = 180.0 / M_PI;
    static constexpr double FEET_TO_METERS = 0.3048;
    static constexpr double METERS_TO_FEET = 1.0 / FEET_TO_METERS;

    // =========================================================================
    // PRIMARY FUNCTION — geocord → DIS ECEF
    // Call this to convert your entity position to DIS wire format
    //
    // Input:
    //   lat_deg   — geocord->latitude  (degrees)
    //   lon_deg   — geocord->longitude (degrees)
    //   alt_feet  — geocord->altitude  (FEET — matches your transform.cpp)
    //
    // Output:
    //   ECEFPosition with X,Y,Z in meters
    // =========================================================================
    static ECEFPosition geocordToECEF(double lat_deg,
                                      double lon_deg,
                                      double alt_feet);

    // =========================================================================
    // REVERSE FUNCTION — DIS ECEF → geocord
    // Call this when receiving EntityState PDU from network
    // Fills your geocord struct fields
    //
    // Input:
    //   ecef — X,Y,Z in meters from DIS PDU
    //
    // Output:
    //   lat_deg  → geocord->latitude  (degrees)
    //   lon_deg  → geocord->longitude (degrees)
    //   alt_feet → geocord->altitude  (FEET)
    // =========================================================================
    static void ecefToGeocord(const ECEFPosition& ecef,
                              double& lat_deg,
                              double& lon_deg,
                              double& alt_feet);

    // =========================================================================
    // ORIENTATION — degrees → DIS Euler radians
    // Input:  heading, pitch, roll in DEGREES (from your geocord/dynamicModel)
    // Output: DISOrientation with psi, theta, phi in RADIANS
    // =========================================================================
    static DISOrientation headingToEuler(float heading_deg,
                                         float pitch_deg,
                                         float roll_deg);

    // =========================================================================
    // ORIENTATION REVERSE — DIS Euler radians → degrees
    // Input:  DISOrientation from received PDU
    // Output: heading, pitch, roll in degrees for your geocord
    // =========================================================================
    static void eulerToHeading(const DISOrientation& euler,
                               float& heading_deg,
                               float& pitch_deg,
                               float& roll_deg);

    // =========================================================================
    // VELOCITY — NED velocity → DIS ECEF velocity
    // Input:  northVel, eastVel, vertVel in m/s (from dynamicModel)
    //         lat_deg, lon_deg — needed for rotation matrix
    // Output: DISVelocity in ECEF frame
    // =========================================================================
    static DISVelocity nedToECEFVelocity(float northVel_ms,
                                         float eastVel_ms,
                                         float vertVel_ms,
                                         double lat_deg,
                                         double lon_deg);

    // =========================================================================
    // VELOCITY REVERSE — DIS ECEF velocity → NED
    // =========================================================================
    static void ecefToNEDVelocity(const DISVelocity& ecefVel,
                                  double lat_deg,
                                  double lon_deg,
                                  float& northVel_ms,
                                  float& eastVel_ms,
                                  float& vertVel_ms);

    // =========================================================================
    // VALIDATION — sanity check before sending
    // Returns false if coordinates are clearly wrong
    // =========================================================================
    static bool isValidGeocord(double lat_deg,
                               double lon_deg,
                               double alt_feet);

private:
    // Internal helper — compute N (radius of curvature in prime vertical)
    static double computeN(double lat_rad);
};

#endif // COORDCONVERTER_H
