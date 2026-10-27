#include "seamodel.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kRad = kPi / 180.0f;
constexpr float kDeg = 180.0f / kPi;
constexpr float kEps = 0.00001f;
constexpr float kG = 9.80665f;
float sane(float x, float fallback = 0.0f) { return std::isfinite(x) ? x : fallback; }
float nonneg(float x) { return std::max(0.0f, sane(x)); }
float limit(float v, float lo, float hi) { return std::clamp(v, lo, hi); }
float wrap360(float a) {
    a = std::fmod(sane(a), 360.0f);
    return (a < 0.0f) ? a + 360.0f : a;
}
float wrap180(float a) {
    a = std::fmod(sane(a) + 180.0f, 360.0f);
    if (a < 0.0f) a += 360.0f;
    return a - 180.0f;
}
float approach(float current, float target, float maxChange) {
    return current + limit(target - current, -nonneg(maxChange), nonneg(maxChange));
}
float magHorizontal(const SeaModel::Vec3& v) { return std::hypot(v.x, v.z); }
SeaModel::Vec3 toBody(const SeaModel::Vec3& v, float heading, float pitch) {
    // Navigation frame with pitch but ignoring visual roll in velocity projection.
    const float h = heading * kRad, p = pitch * kRad;
    const float sh = std::sin(h), ch = std::cos(h);
    const float sp = std::sin(p), cp = std::cos(p);
    // right = (cos h, 0, -sin h)
    // forward = (sin h cos p, sin p, cos h cos p)
    // up = (-sin h sin p, cos p, -cos h sin p)
    return {v.x * ch - v.z * sh,
            -v.x * sh * sp + v.y * cp - v.z * ch * sp,
            v.x * sh * cp + v.y * sp + v.z * ch * cp};
}
} // namespace

void SeaModel::SetNavigationCommand(float speedMps, float headingDegrees,
                                    float altitudeMetres, AltitudeReference reference) {
    commands.speed = nonneg(speedMps);
    commands.heading = wrap360(headingDegrees);
    commands.altitude = sane(altitudeMetres);
    commands.altitudeReference = reference;
}

void SeaModel::SetDepthCommand(float speedMps, float headingDegrees, float depthMetres) {
    SetNavigationCommand(speedMps, headingDegrees, nonneg(depthMetres),
                         AltitudeReference::DepthBelowSurface);
}

float SeaModel::minAllowedAltitude() const {
    const float surface = sane(environment.seaSurfaceY);
    float floorY = surface - nonneg(limits.maxOperatingDepth);
    if (environment.hasSeafloor) {
        floorY = std::max(floorY, sane(environment.seafloorY, surface) +
                                      nonneg(environment.seabedClearance));
    }
    return std::min(surface, floorY);
}

float SeaModel::CommandedWorldAltitude() const {
    const float surface = sane(environment.seaSurfaceY);
    if (vesselType == VesselType::SurfaceShip) return surface;
    float altitude = sane(commands.altitude);
    switch (commands.altitudeReference) {
    case AltitudeReference::WorldY: break;
    case AltitudeReference::SeaSurfaceRelative: altitude += surface; break;
    case AltitudeReference::DepthBelowSurface: altitude = surface - nonneg(altitude); break;
    }
    return limit(altitude, minAllowedAltitude(), surface);
}

void SeaModel::applyDepthBounds() {
    const float surface = sane(environment.seaSurfaceY);
    if (vesselType == VesselType::SurfaceShip) {
        transform.position.y = surface;
        state.verticalSpeed = 0.0f;
        transform.pitch = 0.0f;
    } else {
        const float minY = minAllowedAltitude();
        if (transform.position.y >= surface) {
            transform.position.y = surface;
            if (state.verticalSpeed > 0.0f) state.verticalSpeed = 0.0f;
        } else if (transform.position.y <= minY) {
            transform.position.y = minY;
            if (state.verticalSpeed < 0.0f) state.verticalSpeed = 0.0f;
        }
    }
}

void SeaModel::Initialize() {
    if (initialized_) return;
    initialized_ = true;
    transform.position.x = sane(transform.position.x);
    transform.position.y = sane(transform.position.y, sane(environment.seaSurfaceY));
    transform.position.z = sane(transform.position.z);
    transform.heading = wrap360(transform.heading);
    transform.pitch = sane(transform.pitch);
    transform.roll = sane(transform.roll);
    state.speedThroughWater = limit(nonneg(state.speedThroughWater), 0.0f,
                                    nonneg(limits.maxSpeed));
    state.acceleration = 0.0f;
    state.headingRate = 0.0f;
    state.verticalSpeed = 0.0f;
    state.pitchRate = 0.0f;
    state.rollRate = 0.0f;
    applyDepthBounds();
    publishTelemetry(0.0f, {}, transform.pitch, transform.roll);
    lastGroundVelocity_ = telemetry.groundVelocity;
}

void SeaModel::ResetRuntimeState() {
    initialized_ = false;
    Initialize();
}

void SeaModel::step(float dt) {
    const float oldSpeed = state.speedThroughWater;
    const float targetSpeed = limit(nonneg(commands.speed), 0.0f, nonneg(limits.maxSpeed));
    const float speedError = targetSpeed - oldSpeed;
    const float speedT = std::max(0.1f, sane(responses.speedResponseTime, 2.0f));
    const float targetAcceleration = limit(speedError / speedT,
                                           -nonneg(limits.maxDeceleration),
                                           nonneg(limits.maxAcceleration));
    state.acceleration = approach(state.acceleration, targetAcceleration,
                                  nonneg(limits.maxJerk) * dt);
    state.speedThroughWater = limit(oldSpeed + state.acceleration * dt,
                                    0.0f, nonneg(limits.maxSpeed));
    if ((speedError >= 0.0f && state.speedThroughWater >= targetSpeed) ||
        (speedError <= 0.0f && state.speedThroughWater <= targetSpeed)) {
        state.speedThroughWater = targetSpeed;
        state.acceleration = 0.0f;
    }

    // Rudder authority depends on speed through water, not speed over ground.
    float turnAuthority = 0.0f;
    if (limits.canTurnInPlace) {
        turnAuthority = 1.0f;
    } else {
        const float lo = nonneg(limits.minSteerageSpeed);
        const float hi = std::max(lo + 0.01f, nonneg(limits.fullSteerageSpeed));
        turnAuthority = limit((state.speedThroughWater - lo) / (hi - lo), 0.0f, 1.0f);
    }
    const float availableYawRate = nonneg(limits.maxTurnRate) * turnAuthority;
    const float headingError = wrap180(commands.heading - transform.heading);
    const float headingT = std::max(0.1f, sane(responses.headingResponseTime, 3.0f));
    const float requestedYawRate = limit(headingError / headingT,
                                         -availableYawRate, availableYawRate);
    state.headingRate = approach(state.headingRate, requestedYawRate,
                                 nonneg(limits.maxTurnAcceleration) * dt);
    state.headingRate = limit(state.headingRate, -availableYawRate, availableYawRate);
    float deltaHeading = state.headingRate * dt;
    if (headingError * deltaHeading > 0.0f && std::abs(deltaHeading) >= std::abs(headingError)) {
        deltaHeading = headingError;
        state.headingRate = 0.0f;
    }
    transform.heading = wrap360(transform.heading + deltaHeading);

    const float oldY = transform.position.y;
    if (vesselType == VesselType::SurfaceShip) {
        state.verticalSpeed = 0.0f;
        transform.position.y = sane(environment.seaSurfaceY);
    } else {
        const float heightError = CommandedWorldAltitude() - oldY;
        const float depthT = std::max(0.1f, sane(responses.depthResponseTime, 6.0f));
        const float requestedVerticalSpeed = limit(heightError / depthT,
                                                   -nonneg(limits.maxDiveRate),
                                                   nonneg(limits.maxRiseRate));
        state.verticalSpeed = approach(state.verticalSpeed, requestedVerticalSpeed,
                                       nonneg(limits.maxVerticalAcceleration) * dt);
        float dy = (state.verticalSpeed + sane(environment.waterCurrent.y)) * dt;
        // Limit target overshoot when the environment doesn't force us beyond it.
        if (heightError * dy > 0.0f && std::abs(dy) >= std::abs(heightError)) {
            dy = heightError;
            state.verticalSpeed = -sane(environment.waterCurrent.y);
        }
        transform.position.y += dy;
        applyDepthBounds();
    }

    // Horizontal through-water motion plus advecting water current.
    const float headingRad = transform.heading * kRad;
    const Vec3 current = {sane(environment.waterCurrent.x),
                          sane(environment.waterCurrent.y), sane(environment.waterCurrent.z)};
    const float moveX = state.speedThroughWater * std::sin(headingRad) + current.x;
    const float moveZ = state.speedThroughWater * std::cos(headingRad) + current.z;
    transform.position.x += moveX * dt;
    transform.position.z += moveZ * dt;

    const float oldPitch = transform.pitch;
    const float oldRoll = transform.roll;
    float targetPitch = 0.0f;
    if (vesselType == VesselType::Submarine && state.speedThroughWater > 0.05f) {
        targetPitch = std::atan2(state.verticalSpeed, state.speedThroughWater) * kDeg;
    }
    targetPitch = limit(targetPitch, -nonneg(limits.maxPitch), nonneg(limits.maxPitch));
    const float pitchBlend = 1.0f - std::exp(-dt / std::max(0.1f, sane(limits.pitchResponseTime, 2.5f)));
    transform.pitch = oldPitch + (targetPitch - oldPitch) * pitchBlend;
    // Visual heel/bank approximation from centripetal acceleration, not wave physics.
    const float heelDegrees = std::atan2(state.speedThroughWater * state.headingRate * kRad, kG) * kDeg;
    const float targetRoll = limit(-heelDegrees, -nonneg(limits.maxRoll), nonneg(limits.maxRoll));
    const float rollBlend = 1.0f - std::exp(-dt / std::max(0.1f, sane(limits.rollResponseTime, 3.0f)));
    transform.roll = oldRoll + (targetRoll - oldRoll) * rollBlend;
}

void SeaModel::publishTelemetry(float dt, const Vec3& previousGroundVelocity,
                                float oldPitch, float oldRoll) {
    const float headingRad = transform.heading * kRad;
    const Vec3 current = {sane(environment.waterCurrent.x),
                          sane(environment.waterCurrent.y), sane(environment.waterCurrent.z)};
    Vec3 throughWater = {state.speedThroughWater * std::sin(headingRad),
                         state.verticalSpeed,
                         state.speedThroughWater * std::cos(headingRad)};
    if (vesselType == VesselType::SurfaceShip) throughWater.y = 0.0f;
    Vec3 ground = throughWater + current;
    if (vesselType == VesselType::SurfaceShip) ground.y = 0.0f;
    const float surface = sane(environment.seaSurfaceY);
    telemetry.altitude = transform.position.y;
    telemetry.depth = std::max(0.0f, surface - telemetry.altitude);
    telemetry.submerged = vesselType == VesselType::Submarine && telemetry.depth > 0.01f;
    telemetry.heightAboveSeabed = environment.hasSeafloor
                                      ? transform.position.y - sane(environment.seafloorY)
                                      : std::numeric_limits<float>::quiet_NaN();
    telemetry.heading = transform.heading;
    telemetry.pitch = transform.pitch;
    telemetry.roll = transform.roll;
    telemetry.speedThroughWater = state.speedThroughWater;
    telemetry.speedOverGround = magHorizontal(ground);
    telemetry.courseOverGround = telemetry.speedOverGround > kEps
                                     ? wrap360(std::atan2(ground.x, ground.z) * kDeg)
                                     : transform.heading;
    telemetry.driftAngle = wrap180(telemetry.courseOverGround - transform.heading);
    telemetry.verticalSpeed = ground.y;
    telemetry.waterRelativeVelocity = throughWater;
    telemetry.groundVelocity = ground;
    telemetry.bodyWaterVelocity = toBody(throughWater, transform.heading, transform.pitch);
    telemetry.bodyGroundVelocity = toBody(ground, transform.heading, transform.pitch);
    state.pitchRate = dt > kEps ? (transform.pitch - oldPitch) / dt : 0.0f;
    state.rollRate = dt > kEps ? (transform.roll - oldRoll) / dt : 0.0f;
    telemetry.acceleration = dt > kEps
                                 ? (ground - previousGroundVelocity) * (1.0f / dt)
                                 : Vec3{};
    telemetry.bodyAcceleration = toBody(telemetry.acceleration, transform.heading, transform.pitch);
}

void SeaModel::FixedUpdate(float deltaSeconds) {
    if (!(deltaSeconds > 0.0f) || !std::isfinite(deltaSeconds)) return;
    if (!initialized_) Initialize();

    // Snapshot telemetry source once; substepping does not multiply measured accel.
    const Vec3 previousGroundVelocity = lastGroundVelocity_;
    const float oldPitch = transform.pitch;
    const float oldRoll = transform.roll;
    const float oldHeading = transform.heading;

    // Large dt remains stable and can handle fast-forward / time scaling.
    constexpr float kMaxStep = 0.05f;
    float remaining = deltaSeconds;
    while (remaining > 0.0f) {
        const float stepSize = std::min(remaining, kMaxStep);
        step(stepSize);
        remaining -= stepSize;
    }
    publishTelemetry(deltaSeconds, previousGroundVelocity, oldPitch, oldRoll);
    state.headingRate = wrap180(transform.heading - oldHeading) / deltaSeconds;
    lastGroundVelocity_ = telemetry.groundVelocity;
}
