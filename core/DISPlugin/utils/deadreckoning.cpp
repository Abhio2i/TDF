// =============================================================================
// FILE:        DeadReckoning.cpp
// MODULE:      DIS Network Plugin — Utils
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "deadreckoning.h"
#include "coordconverter.h"

#include <cmath>
#include <chrono>
#include <QDebug>

// =============================================================================
// millisecondsSinceEpoch
// Current time in milliseconds — used for timestamping PDU receipt
// =============================================================================
int64_t DeadReckoning::millisecondsSinceEpoch()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(
               system_clock::now().time_since_epoch()
               ).count();
}

// =============================================================================
// updateState
// Called when a new EntityState PDU arrives for a remote entity
// Stores all PDU fields into the entity's DeadReckoningState
// =============================================================================
void DeadReckoning::updateState(DeadReckoningState& state,
                                double posX, double posY, double posZ,
                                float  velX, float  velY, float  velZ,
                                float  psi,  float  theta, float phi,
                                DRAlgorithm algorithm)
{
    state.posX = posX;
    state.posY = posY;
    state.posZ = posZ;

    state.velX = velX;
    state.velY = velY;
    state.velZ = velZ;

    state.psi   = psi;
    state.theta = theta;
    state.phi   = phi;

    state.algorithm    = algorithm;
    state.lastUpdateMs = millisecondsSinceEpoch();
    state.initialized  = true;
}

// =============================================================================
// predict
// Main entry point — called every frame for each remote entity
// Selects algorithm based on state.algorithm
// Returns position/orientation ready to apply to your entity transform
// =============================================================================
PredictedState DeadReckoning::predict(const DeadReckoningState& state,
                                      double dtSeconds)
{
    if (!state.initialized) {
        // No PDU received yet — return zeros
        return PredictedState{};
    }

    // Cap dt to 10 seconds — if PDU is older than that
    // something is wrong and we should not extrapolate wildly
    double dt = dtSeconds;
    if (dt > 10.0) dt = 10.0;
    if (dt < 0.0)  dt = 0.0;

    PredictedState predicted;

    switch (state.algorithm) {
    case DRAlgorithm::Static:
        // No movement — just return last known state
        predicted.posX  = state.posX;
        predicted.posY  = state.posY;
        predicted.posZ  = state.posZ;
        predicted.psi   = state.psi;
        predicted.theta = state.theta;
        predicted.phi   = state.phi;
        break;

    case DRAlgorithm::FPW:
    case DRAlgorithm::FVW:
        predicted = predictFPW(state, dt);
        break;

    case DRAlgorithm::RPW:
    case DRAlgorithm::RVW:
        predicted = predictRPW(state, dt);
        break;

    case DRAlgorithm::FPB:
    case DRAlgorithm::FVB:
        predicted = predictFVB(state, dt);
        break;

    default:
        predicted = predictFPW(state, dt);
        break;
    }

    // Convert ECEF predicted position to geocord for engine
    ECEFPosition ecef;
    ecef.x = predicted.posX;
    ecef.y = predicted.posY;
    ecef.z = predicted.posZ;

    double lat, lon, alt_feet;
    CoordConverter::ecefToGeocord(ecef, lat, lon, alt_feet);

    predicted.latitude  = lat;
    predicted.longitude = lon;
    predicted.altitude  = alt_feet;  // FEET for your geocord

    // Convert Euler angles to heading/pitch/roll degrees
    DISOrientation euler;
    euler.psi   = predicted.psi;
    euler.theta = predicted.theta;
    euler.phi   = predicted.phi;

    CoordConverter::eulerToHeading(euler,
                                   predicted.heading,
                                   predicted.pitch,
                                   predicted.roll);

    return predicted;
}

// =============================================================================
// predictFPW — Algorithm 2
// Fixed Position/Velocity, World coordinates
// Most common algorithm. STAGE uses this by default.
//
// Simple Newtonian extrapolation:
//   pos(t) = pos(t0) + vel * dt
//   orientation = constant (no rotation prediction)
// =============================================================================
PredictedState DeadReckoning::predictFPW(const DeadReckoningState& state,
                                         double dt)
{
    PredictedState predicted;

    // Position extrapolation — p = p0 + v*t
    predicted.posX = state.posX + state.velX * dt;
    predicted.posY = state.posY + state.velY * dt;
    predicted.posZ = state.posZ + state.velZ * dt;

    // Orientation — constant (FPW does not predict rotation)
    predicted.psi   = state.psi;
    predicted.theta = state.theta;
    predicted.phi   = state.phi;

    return predicted;
}

// =============================================================================
// predictRPW — Algorithm 3
// Rate/Position/Velocity, World coordinates
// Adds acceleration term for more accurate prediction of maneuvering entities
//
// Kinematic extrapolation:
//   pos(t) = pos(t0) + vel*dt + 0.5*acc*dt²
// =============================================================================
PredictedState DeadReckoning::predictRPW(const DeadReckoningState& state,
                                         double dt)
{
    PredictedState predicted;

    double dt2 = 0.5 * dt * dt;

    // Position with acceleration
    predicted.posX = state.posX + state.velX * dt + state.accX * dt2;
    predicted.posY = state.posY + state.velY * dt + state.accY * dt2;
    predicted.posZ = state.posZ + state.velZ * dt + state.accZ * dt2;

    // Orientation — constant
    predicted.psi   = state.psi;
    predicted.theta = state.theta;
    predicted.phi   = state.phi;

    return predicted;
}

// =============================================================================
// predictFVB — Algorithm 9
// Fixed Velocity, Body coordinates
// Used for aircraft where velocity is expressed in body frame
//
// Transforms body-frame velocity to world-frame first
// then applies Newtonian extrapolation
// =============================================================================
PredictedState DeadReckoning::predictFVB(const DeadReckoningState& state,
                                         double dt)
{
    // Body to world rotation using Euler angles
    // Standard aerospace rotation matrix (ZYX convention)
    double cosPsi   = std::cos(state.psi);
    double sinPsi   = std::sin(state.psi);
    double cosTheta = std::cos(state.theta);
    double sinTheta = std::sin(state.theta);
    double cosPhi   = std::cos(state.phi);
    double sinPhi   = std::sin(state.phi);

    // Row 1 of rotation matrix
    double r11 = cosTheta * cosPsi;
    double r12 = sinPhi * sinTheta * cosPsi - cosPhi * sinPsi;
    double r13 = cosPhi * sinTheta * cosPsi + sinPhi * sinPsi;

    // Row 2
    double r21 = cosTheta * sinPsi;
    double r22 = sinPhi * sinTheta * sinPsi + cosPhi * cosPsi;
    double r23 = cosPhi * sinTheta * sinPsi - sinPhi * cosPsi;

    // Row 3
    double r31 = -sinTheta;
    double r32 = sinPhi * cosTheta;
    double r33 = cosPhi * cosTheta;

    // Body velocity (velX = forward, velY = right, velZ = down in body frame)
    double vbX = state.velX;
    double vbY = state.velY;
    double vbZ = state.velZ;

    // Transform to world velocity
    double vwX = r11 * vbX + r12 * vbY + r13 * vbZ;
    double vwY = r21 * vbX + r22 * vbY + r23 * vbZ;
    double vwZ = r31 * vbX + r32 * vbY + r33 * vbZ;

    PredictedState predicted;

    // Position extrapolation with world-frame velocity
    predicted.posX = state.posX + vwX * dt;
    predicted.posY = state.posY + vwY * dt;
    predicted.posZ = state.posZ + vwZ * dt;

    // Orientation with angular velocity (if available)
    predicted.psi   = state.psi   + state.psiDot   * static_cast<float>(dt);
    predicted.theta = state.theta + state.thetaDot  * static_cast<float>(dt);
    predicted.phi   = state.phi   + state.phiDot    * static_cast<float>(dt);

    return predicted;
}

// =============================================================================
// shouldSendUpdate
// Determines if we should send a new EntityState PDU for our local entity
//
// DIS standard rule:
//   Send PDU if predicted position differs from actual by > 1 meter
//   OR if predicted orientation differs from actual by > 3 degrees
//   OR if heartbeat timer expired (every 5 seconds regardless)
//
// This is called every frame for locally owned entities.
// Returns true → send PDU now
// Returns false → skip this frame, no PDU needed
// =============================================================================
bool DeadReckoning::shouldSendUpdate(const DeadReckoningState& state,
                                     double actualX, double actualY, double actualZ,
                                     float  actualPsi, float actualTheta, float actualPhi,
                                     float posThresholdMeters,
                                     float oriThresholdRadians)
{
    if (!state.initialized) return true;

    // ── Position threshold check ──────────────────────────────────────────────
    // Get where receivers think we are (dead reckoned position)
    int64_t nowMs = millisecondsSinceEpoch();
    double dtSeconds = (nowMs - state.lastUpdateMs) / 1000.0;

    PredictedState predicted = predictFPW(state, dtSeconds);

    double dx = actualX - predicted.posX;
    double dy = actualY - predicted.posY;
    double dz = actualZ - predicted.posZ;
    double posError = std::sqrt(dx*dx + dy*dy + dz*dz);

    if (posError > posThresholdMeters) return true;

    // ── Orientation threshold check ───────────────────────────────────────────
    float dpsi   = std::abs(actualPsi   - state.psi);
    float dtheta = std::abs(actualTheta - state.theta);
    float dphi   = std::abs(actualPhi   - state.phi);

    if (dpsi   > oriThresholdRadians) return true;
    if (dtheta > oriThresholdRadians) return true;
    if (dphi   > oriThresholdRadians) return true;

    // ── Heartbeat check ───────────────────────────────────────────────────────
    // Always send at least every 5 seconds (DIS standard heartbeat)
    if (dtSeconds >= 5.0) return true;

    return false;
}
