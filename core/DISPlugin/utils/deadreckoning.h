// =============================================================================
// FILE:        DeadReckoning.h
// MODULE:      DIS Network Plugin — Utils
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Implements DIS dead reckoning algorithms.
//              Predicts entity position between EntityState PDU updates.
//
// WHY THIS IS NEEDED:
//   EntityState PDUs arrive at 5Hz (every 200ms).
//   Your simulation runs at ~60Hz.
//   Without dead reckoning, remote entities jump every 200ms.
//   With dead reckoning, entities move smoothly using predicted position.
//
// DIS STANDARD (IEEE 1278.1):
//   Algorithm 1: Static (no movement)
//   Algorithm 2: FPW — Fixed velocity, world coordinates
//   Algorithm 3: RPW — Fixed velocity + acceleration, world coordinates
//   Algorithm 4: RVW — Fixed velocity + acceleration, world coordinates
//   Algorithm 5: FVW — Fixed velocity, world coordinates
//
//   For DRDO/STAGE compatibility we implement:
//   Algorithm 2 (FPW) — most common, used by STAGE default
//   Algorithm 9 (FPB) — fixed velocity, body coordinates (aircraft)
//
// THREAD SAFETY:
//   DeadReckoningState is per-entity data.
//   Each entity has its own state struct.
//   No shared state between entities.
//   Thread safe if each entity's state is accessed by one thread.
// =============================================================================

#ifndef DEADRECKONING_H
#define DEADRECKONING_H

#include "coordconverter.h"
#include <cstdint>
#include <chrono>

// =============================================================================
// DRAlgorithm
// DIS dead reckoning algorithm codes (IEEE 1278.1 Table B.2)
// =============================================================================
enum class DRAlgorithm : uint8_t {
    Static  = 1,  // No movement
    FPW     = 2,  // Fixed position/velocity, world coords — default
    RPW     = 3,  // Rate (velocity + acceleration), world coords
    RVW     = 4,  // Rate velocity world
    FVW     = 5,  // Fixed velocity world
    FPB     = 6,  // Fixed position/velocity, body coords — aircraft
    RPB     = 7,  // Rate, body coords
    RVB     = 8,  // Rate velocity body
    FVB     = 9   // Fixed velocity body
};

// =============================================================================
// DeadReckoningState
// Stores the last known state of one remote entity
// Updated every time a new EntityState PDU is received
// =============================================================================
struct DeadReckoningState {
    // Last known position (ECEF meters)
    double posX = 0.0;
    double posY = 0.0;
    double posZ = 0.0;

    // Last known velocity (ECEF m/s)
    float velX = 0.0f;
    float velY = 0.0f;
    float velZ = 0.0f;

    // Last known acceleration (ECEF m/s²) — used by RPW/RVW
    float accX = 0.0f;
    float accY = 0.0f;
    float accZ = 0.0f;

    // Last known orientation (Euler radians)
    float psi   = 0.0f;  // yaw
    float theta = 0.0f;  // pitch
    float phi   = 0.0f;  // roll

    // Angular velocity (radians/sec) — used by body-frame algorithms
    float psiDot   = 0.0f;
    float thetaDot = 0.0f;
    float phiDot   = 0.0f;

    // Algorithm to use for this entity
    DRAlgorithm algorithm = DRAlgorithm::FPW;

    // Timestamp of last PDU (milliseconds since epoch)
    int64_t lastUpdateMs = 0;

    // Whether this state has been initialized
    bool initialized = false;
};

// =============================================================================
// PredictedState
// Output of dead reckoning — where the entity is NOW
// =============================================================================
struct PredictedState {
    // Predicted position (ECEF meters)
    double posX = 0.0;
    double posY = 0.0;
    double posZ = 0.0;

    // Predicted orientation (Euler radians)
    float psi   = 0.0f;
    float theta = 0.0f;
    float phi   = 0.0f;

    // Converted to your engine format
    double latitude  = 0.0;  // degrees
    double longitude = 0.0;  // degrees
    double altitude  = 0.0;  // FEET (for your geocord)
    float  heading   = 0.0f; // degrees compass
    float  pitch     = 0.0f; // degrees
    float  roll      = 0.0f; // degrees
};

// =============================================================================
// DeadReckoning
// Static utility class — all methods are stateless
// Per-entity state is stored in DeadReckoningState structs
// =============================================================================
class DeadReckoning {
public:

    // =========================================================================
    // updateState
    // Call this when a new EntityState PDU is received for an entity
    // Stores the PDU data into the entity's DeadReckoningState
    // =========================================================================
    static void updateState(DeadReckoningState& state,
                            double posX, double posY, double posZ,
                            float  velX, float  velY, float  velZ,
                            float  psi,  float  theta, float phi,
                            DRAlgorithm algorithm);

    // =========================================================================
    // predict
    // Call this every frame to get the entity's current predicted position
    // dt = time since last PDU in seconds
    //
    // Returns PredictedState with both ECEF and geocord values
    // ready to apply to your entity's transform
    // =========================================================================
    static PredictedState predict(const DeadReckoningState& state,
                                  double dtSeconds);

    // =========================================================================
    // shouldSendUpdate
    // Call this to check if we should send a new EntityState PDU
    // Returns true if predicted position differs from actual by more than
    // the threshold (DIS standard: position > 1m or orientation > 3 degrees)
    // =========================================================================
    static bool shouldSendUpdate(const DeadReckoningState& state,
                                 double actualX, double actualY, double actualZ,
                                 float  actualPsi, float actualTheta, float actualPhi,
                                 float posThresholdMeters  = 1.0f,
                                 float oriThresholdRadians = 0.05236f); // 3 degrees

    // =========================================================================
    // millisecondsSinceEpoch
    // Utility — current time in milliseconds
    // =========================================================================
    static int64_t millisecondsSinceEpoch();

private:
    // ── Algorithm implementations ─────────────────────────────────────────────

    // Algorithm 2: FPW — constant velocity, world coordinates
    // Most common — used by STAGE default
    static PredictedState predictFPW(const DeadReckoningState& state,
                                     double dt);

    // Algorithm 3: RPW — velocity + acceleration, world coordinates
    static PredictedState predictRPW(const DeadReckoningState& state,
                                     double dt);

    // Algorithm 9: FVB — fixed velocity, body coordinates (for aircraft)
    static PredictedState predictFVB(const DeadReckoningState& state,
                                     double dt);

    // Convert ECEF predicted position to geocord for engine
    static void ecefToGeocord(const PredictedState& ecefState,
                              PredictedState& out);
};

#endif // DEADRECKONING_H
