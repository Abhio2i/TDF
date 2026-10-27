#include "aircraft.h"

// ============================================================================
// CONSTRUCTION / LIFECYCLE
// ============================================================================

Aircraft::Aircraft() = default;

void Aircraft::Initialize()
{
    if (initialized_)
        return;

    initialized_ = true;

    aircraftState.currentSpeed = NonNegative(aircraftState.currentSpeed);
    aircraftState.currentAcceleration = 0.0f;
    commandYawRate_ = 0.0f;
    engineCommand_ = 0.0f;
    sinkSpeed_ = 0.0f;

    SynchronizeStateFromTransform();

    // Respect a flat ground plane only inside Aircraft's SI-space.
    if (transform.position.y <= flightPhysics.groundLevel + GroundEpsilon)
    {
        transform.position.y = flightPhysics.groundLevel;
        flightState.isGrounded = true;

        aircraftState.currentPitch = 0.0f;
        aircraftState.currentRoll = 0.0f;
        transform.pitch = 0.0f;
        transform.roll = 0.0f;
    }
    else
    {
        flightState.isGrounded = false;
    }

    // Clamp an externally supplied initial position to the configured ceiling.
    const float ceiling = Finite(maximums.maxAltitude);
    if (ceiling > flightPhysics.groundLevel &&
        transform.position.y > ceiling)
    {
        transform.position.y = ceiling;
    }

    RebuildVelocity();
    RefreshAerodynamicState();

    previousGroundVelocity_ = velocityState.groundVelocity;
    previousPitch_ = aircraftState.currentPitch;
    previousRoll_ = aircraftState.currentRoll;
    previousHeading_ = aircraftState.currentHeading;

    PublishMotionTelemetry(0.0f);

    aircraftState.currentPitchRate = 0.0f;
    aircraftState.currentRollRate = 0.0f;
    aircraftState.currentYawRate = 0.0f;
    aircraftState.currentTurnRate = 0.0f;
    aircraftState.horizonPitchRate = 0.0f;
    aircraftState.horizonRollRate = 0.0f;
    aircraftState.horizonRatesValid = true;
}

void Aircraft::ResetRuntimeState()
{
    initialized_ = true;

    sinkSpeed_ = 0.0f;
    engineCommand_ = 0.0f;
    commandYawRate_ = 0.0f;

    SynchronizeStateFromTransform();

    flightState.isGrounded =
        transform.position.y <= flightPhysics.groundLevel + GroundEpsilon;

    if (flightState.isGrounded)
    {
        transform.position.y = flightPhysics.groundLevel;
        transform.pitch = 0.0f;
        transform.roll = 0.0f;

        aircraftState.currentPitch = 0.0f;
        aircraftState.currentRoll = 0.0f;
    }

    RebuildVelocity();
    RefreshAerodynamicState();

    previousGroundVelocity_ = velocityState.groundVelocity;
    previousPitch_ = aircraftState.currentPitch;
    previousRoll_ = aircraftState.currentRoll;
    previousHeading_ = aircraftState.currentHeading;

    aircraftState.currentAcceleration = 0.0f;
    aircraftState.currentPitchRate = 0.0f;
    aircraftState.currentRollRate = 0.0f;
    aircraftState.currentYawRate = 0.0f;
    aircraftState.currentTurnRate = 0.0f;
    aircraftState.horizonPitchRate = 0.0f;
    aircraftState.horizonRollRate = 0.0f;
    aircraftState.horizonRatesValid = true;

    accelerationState.acceleration = {};
    accelerationState.bodyAcceleration = {};
}

void Aircraft::FixedUpdate(float dt)
{
    if (!IsFinite(dt) || dt <= 0.0f)
        return;

    if (!initialized_)
        Initialize();

    // Simulation copies the entity Transform into Aircraft::transform before
    // calling FixedUpdate(). Keep that external state authoritative at the
    // start of the frame, then Aircraft owns the state for this physics step.
    SynchronizeStateFromTransform();

    aircraftState.currentSpeed = NonNegative(aircraftState.currentSpeed);

    // Handle external teleport below/above the configured flat ground level.
    if (transform.position.y < flightPhysics.groundLevel)
    {
        transform.position.y = flightPhysics.groundLevel;
        flightState.isGrounded = true;
        sinkSpeed_ = 0.0f;
    }
    else if (flightState.isGrounded &&
             transform.position.y > flightPhysics.groundLevel + GroundEpsilon)
    {
        flightState.isGrounded = false;
        sinkSpeed_ = 0.0f;
        commandYawRate_ = 0.0f;
    }

    const float oldPitch = aircraftState.currentPitch;
    const float oldRoll = aircraftState.currentRoll;
    const float oldHeading = aircraftState.currentHeading;

    // 1) Propulsion / speed controller.
    UpdatePropulsionAndSpeed(dt);

    // 2) Current aerodynamic state before applying new attitude commands.
    RebuildVelocity();
    RefreshAerodynamicState();

    // 3) Navigation command controller: speed/heading/altitude are already
    // extracted from the trajectory by Simulation/DynamicModel.
    UpdateCommandControl(dt);

    // 4) Automatic takeoff transition for command-navigation operation.
    if (flightState.isGrounded)
    {
        const float effectiveTakeoffSpeed =
            std::max(NonNegative(flightPhysics.minimumTakeoffSpeed),
                     NonNegative(flightPhysics.stallSpeed));

        const float takeoffPitch =
            std::max(0.1f, NonNegative(flightPhysics.takeoffPitchThreshold));

        const float targetAltitude = CommandedWorldAltitude();
        const float forwardAir = ForwardAirSpeed();

        const bool speedReady =
            aircraftState.currentSpeed > SpeedEpsilon &&
            forwardAir >= effectiveTakeoffSpeed;

        const bool pitchReady =
            aircraftState.currentPitch + 0.00001f >= takeoffPitch &&
            Forward().y > 0.0f;

        const bool altitudeRequested =
            targetAltitude > flightPhysics.groundLevel + GroundEpsilon;

        const bool belowCeiling =
            maximums.maxAltitude <= flightPhysics.groundLevel ||
            transform.position.y < maximums.maxAltitude;

        const float upwardVelocity =
            forwardAir * Forward().y + Finite(wind.windVelocity.y);

        if (speedReady && pitchReady && altitudeRequested &&
            belowCeiling && upwardVelocity > 0.0f)
        {
            flightState.isGrounded = false;
            aircraftState.currentSpeed = forwardAir;
            sinkSpeed_ = 0.0f;
            commandYawRate_ = 0.0f;
        }
    }

    // 5) Gravity/lift/stall approximation.
    UpdateFlightPhysics(dt);

    // 6) Respect hard altitude ceiling before integrating position.
    ApplyCeilingToVelocity(dt);

    // 7) Integrate SI-space world position.
    transform.position += velocityState.groundVelocity * dt;

    // 8) Resolve flat-ground contact.
    ResolveGroundContact();

    // 9) Publish final telemetry after all corrections.
    SynchronizeStateFromTransform();
    RefreshAerodynamicState();
    PublishMotionTelemetry(dt);
    PublishAngularTelemetry(oldPitch, oldRoll, oldHeading, dt);

    previousPitch_ = aircraftState.currentPitch;
    previousRoll_ = aircraftState.currentRoll;
    previousHeading_ = aircraftState.currentHeading;
}


// ============================================================================
// COMMAND API
// ============================================================================

void Aircraft::SetNavigationCommand(float speedMS,
                                    float headingDeg,
                                    float altitudeM,
                                    AltitudeReference reference)
{
    commands.altitudeReference = reference;
    commands.commandedSpeed = NonNegative(speedMS);
    commands.commandedHeading = Normalize360(Finite(headingDeg));

    const float fallback =
        reference == AltitudeReference::GroundRelative
            ? 0.0f
            : flightPhysics.groundLevel;

    commands.commandedAltitude = Finite(altitudeM, fallback);
}

float Aircraft::CommandedWorldAltitude() const
{
    const bool relative =
        commands.altitudeReference == AltitudeReference::GroundRelative;

    float target =
        Finite(commands.commandedAltitude,
               relative ? 0.0f : flightPhysics.groundLevel);

    if (relative)
        target += flightPhysics.groundLevel;

    target = std::max(flightPhysics.groundLevel, target);

    const float ceiling = Finite(maximums.maxAltitude);
    if (ceiling > flightPhysics.groundLevel)
        target = std::min(target, ceiling);

    return target;
}


// ============================================================================
// SYNCHRONIZATION
// ============================================================================

void Aircraft::SynchronizeStateFromTransform()
{
    transform.position.x = Finite(transform.position.x);
    transform.position.y = Finite(transform.position.y);
    transform.position.z = Finite(transform.position.z);

    transform.pitch = Clamp(
        Finite(transform.pitch),
        -89.9f,
        89.9f);

    transform.heading = Normalize360(Finite(transform.heading));
    transform.roll = Normalize180(Finite(transform.roll));

    aircraftState.currentPitch = transform.pitch;
    aircraftState.currentHeading = transform.heading;
    aircraftState.currentRoll = transform.roll;
    aircraftState.currentYaw = transform.heading;

    aircraftState.currentAltitude = transform.position.y;
    aircraftState.currentHeightAboveGround =
        transform.position.y - flightPhysics.groundLevel;
}


// ============================================================================
// PROPULSION / SPEED
// ============================================================================

float Aircraft::NaturalDragAcceleration() const
{
    const float airspeed = ForwardAirSpeed();

    const float referenceSpeed =
        std::max(1.0f, NonNegative(aerodynamicDrag.referenceSpeed));

    const float referenceRatio = airspeed / referenceSpeed;

    const float highRatio =
        maximums.maxSpeed > SpeedEpsilon
            ? Clamp01(airspeed / maximums.maxSpeed)
            : 0.0f;

    const float baseDrag =
        NonNegative(aerodynamicDrag.zeroThrottleDrag) *
            referenceRatio * referenceRatio +
        NonNegative(aerodynamicDrag.highSpeedDrag) *
            highRatio * highRatio;

    const float induced =
        flightState.isGrounded
            ? 0.0f
            : NonNegative(aerodynamicDrag.inducedDrag) *
                  Clamp01(flightState.liftFactor) *
                  highRatio * highRatio;

    return NonNegative(baseDrag + induced);
}

void Aircraft::UpdatePropulsionAndSpeed(float dt)
{
    const float oldSpeed = NonNegative(aircraftState.currentSpeed);

    const float maxAcceleration =
        NonNegative(maximums.maxAcceleration);

    const float maxDeceleration =
        NonNegative(maximums.maxDeceleration);

    float targetSpeed = NonNegative(commands.commandedSpeed);

    // minSpeed is an airborne command floor, not a forced physical speed.
    if (!flightState.isGrounded && targetSpeed > 0.0f)
        targetSpeed = std::max(targetSpeed, NonNegative(maximums.minSpeed));

    if (maximums.maxSpeed > 0.0f)
        targetSpeed = std::min(targetSpeed, maximums.maxSpeed);

    float error = targetSpeed - oldSpeed;

    // Predict some of the speed that will be added/removed while the engine
    // command is spooling down. This reduces command overshoot.
    const float spoolRelease =
        NonNegative(responses.timeToReachMaxDecel);

    const float currentAcceleration =
        Finite(aircraftState.currentAcceleration);

    const float jerk =
        (spoolRelease > 0.0f)
            ? maxAcceleration / spoolRelease
            : 0.0f;

    if (jerk > 0.0f && error * currentAcceleration > 0.0f)
    {
        const float predictedChange =
            (currentAcceleration * currentAcceleration) /
            (2.0f * jerk);

        const float correction =
            std::min(std::abs(error), predictedChange);

        error -= (currentAcceleration >= 0.0f ? 1.0f : -1.0f) *
                 correction;
    }

    const float accErrorScale =
        std::max(0.001f,
                 NonNegative(responses.deltaSpdCommandMaxAcc));

    const float decErrorScale =
        std::max(0.001f,
                 NonNegative(responses.deltaSpdCommandMaxDecel));

    const float desiredAcceleration =
        error >= 0.0f
            ? Clamp01(error / accErrorScale) * maxAcceleration
            : -Clamp01(-error / decErrorScale) * maxDeceleration;

    const float drag = NaturalDragAcceleration();

    // Positive nose-up flight sees gravity as a forward deceleration.
    const float gravityAlongFlight =
        flightState.isGrounded
            ? 0.0f
            : -NonNegative(flightPhysics.gravity) * Forward().y;

    // Feed-forward drag/gravity compensation means a steady-speed command can
    // settle without continually losing speed.
    const float requiredEngine =
        desiredAcceleration + drag - gravityAlongFlight;

    const float requestedEngine =
        maxAcceleration > 0.0f
            ? Clamp01(requiredEngine / maxAcceleration)
            : 0.0f;

    const float spoolTime =
        requestedEngine > engineCommand_
            ? NonNegative(responses.timeToReachMaxAcc)
            : NonNegative(responses.timeToReachMaxDecel);

    if (spoolTime <= 0.0f)
    {
        engineCommand_ = requestedEngine;
    }
    else
    {
        engineCommand_ =
            MoveTowards(engineCommand_,
                        requestedEngine,
                        dt / std::max(0.0001f, spoolTime));
    }

    // Command mode is allowed to use aerodynamic braking when the target speed
    // requires more deceleration than natural drag alone provides.
    const float brakeSpeedRatio =
        Clamp01(ForwardAirSpeed() /
                std::max(1.0f,
                         NonNegative(aerodynamicDrag.referenceSpeed)));

    const float availableBrake =
        flightState.isGrounded
            ? maxDeceleration
            : maxDeceleration *
                  brakeSpeedRatio * brakeSpeedRatio;

    const float brakeDemand =
        (availableBrake > 0.0f && requiredEngine < 0.0f)
            ? Clamp01(-requiredEngine / availableBrake)
            : 0.0f;

    float actualBrake = brakeDemand * availableBrake;

    // A zero-speed command on the ground acts like a full stop request.
    if (flightState.isGrounded && targetSpeed <= SpeedEpsilon)
    {
        engineCommand_ = 0.0f;
        actualBrake = maxDeceleration;
    }

    const float thrustAcceleration =
        engineCommand_ * maxAcceleration;

    float netAcceleration =
        thrustAcceleration -
        drag -
        actualBrake +
        gravityAlongFlight;

    netAcceleration =
        Clamp(netAcceleration,
              -maxDeceleration,
              maxAcceleration);

    aircraftState.currentSpeed =
        std::max(0.0f,
                 oldSpeed + netAcceleration * dt);

    if (maximums.maxSpeed > 0.0f)
    {
        aircraftState.currentSpeed =
            std::min(aircraftState.currentSpeed,
                     maximums.maxSpeed);
    }

    // Actual measured longitudinal acceleration.
    aircraftState.currentAcceleration =
        (aircraftState.currentSpeed - oldSpeed) / dt;
}


// ============================================================================
// CONTROL AUTHORITY
// ============================================================================

float Aircraft::ForwardAirSpeed() const
{
    if (!flightState.isGrounded)
        return NonNegative(aircraftState.currentSpeed);

    const vec3 windVelocity{
        Finite(wind.windVelocity.x),
        Finite(wind.windVelocity.y),
        Finite(wind.windVelocity.z)
    };

    return std::max(
        0.0f,
        NonNegative(aircraftState.currentSpeed) -
            vec3::Dot(windVelocity, HorizontalForward()));
}

float Aircraft::PitchRollAuthority() const
{
    // A rapid vertical fall must not restore pitch/roll authority.
    if (aircraftState.currentSpeed <= SpeedEpsilon)
        return 0.0f;

    const float fullSpeed =
        std::max(
            SpeedEpsilon,
            std::max(NonNegative(flightPhysics.minimumTakeoffSpeed),
                     NonNegative(flightPhysics.stallSpeed)));

    return Clamp01(ForwardAirSpeed() / fullSpeed);
}

float Aircraft::AirYawAuthority() const
{
    if (aircraftState.currentSpeed <= SpeedEpsilon)
        return 0.0f;

    const float minimum =
        NonNegative(maximums.minimumTurnSpeed);

    const float full =
        std::max(minimum,
                 NonNegative(maximums.fullTurnAuthoritySpeed));

    const float speed = ForwardAirSpeed();

    if (speed <= minimum)
        return 0.0f;

    if (full <= minimum)
        return 1.0f;

    return Clamp01((speed - minimum) / (full - minimum));
}


// ============================================================================
// COMMAND CONTROL
// ============================================================================

void Aircraft::UpdateCommandControl(float dt)
{
    const float authority = PitchRollAuthority();

    const float yawTarget =
        Normalize360(Finite(commands.commandedHeading));

    const float yawError =
        DeltaAngle(aircraftState.currentHeading,
                   yawTarget);

    const float headingScale =
        std::max(
            0.001f,
            NonNegative(responses.deltaHdgCommandMaxRot));

    const float turnDemand =
        Clamp(yawError / headingScale,
              -1.0f,
              1.0f);

    const float maximumYawRate =
        NonNegative(maximums.turnRate);

    const float availableYaw =
        maximumYawRate * AirYawAuthority();

    if (availableYaw <= 0.0f || maximumYawRate <= 0.0f)
    {
        commandYawRate_ = 0.0f;
    }
    else
    {
        const float targetYawRate =
            Clamp(turnDemand * availableYaw,
                  -availableYaw,
                  availableYaw);

        const float responseTime =
            NonNegative(responses.timeToReachMaxRot);

        if (responseTime <= 0.0f)
        {
            commandYawRate_ = targetYawRate;
        }
        else
        {
            const float rateStep =
                maximumYawRate /
                std::max(0.0001f, responseTime) *
                dt;

            commandYawRate_ =
                MoveTowards(commandYawRate_,
                            targetYawRate,
                            rateStep);

            commandYawRate_ =
                Clamp(commandYawRate_,
                      -availableYaw,
                      availableYaw);
        }
    }

    float yawStep = commandYawRate_ * dt;

    // Do not orbit around the requested heading.
    if (yawStep * yawError >= 0.0f &&
        std::abs(yawStep) >= std::abs(yawError))
    {
        yawStep = yawError;
        commandYawRate_ = 0.0f;
    }

    aircraftState.currentHeading =
        Normalize360(aircraftState.currentHeading + yawStep);

    float targetPitch = NavigationPitchTarget();

    const float maxBank =
        Clamp(NonNegative(maximums.maxBankAngle),
              0.0f,
              180.0f);

    float targetRoll = 0.0f;

    if (!flightState.isGrounded &&
        availableYaw > SpeedEpsilon)
    {
        // Same sign convention as the C# model:
        // positive heading turn -> negative roll (right bank).
        targetRoll =
            -commandYawRate_ /
            availableYaw *
            maxBank *
            AirYawAuthority();
    }

    const float pitchLimit =
        Clamp(NonNegative(maximums.maxPitchAngle),
              0.0f,
              89.0f);

    if (flightState.isGrounded)
    {
        const float effectiveTakeoffSpeed =
            std::max(
                NonNegative(flightPhysics.minimumTakeoffSpeed),
                NonNegative(flightPhysics.stallSpeed));

        const bool rotationAllowed =
            aircraftState.currentSpeed > SpeedEpsilon &&
            ForwardAirSpeed() >= effectiveTakeoffSpeed;

        targetPitch =
            rotationAllowed
                ? Clamp(targetPitch, 0.0f, pitchLimit)
                : 0.0f;

        targetRoll = 0.0f;
    }
    else if (stallRecovery.autoStallRecovery &&
             flightState.isStalling)
    {
        // Nose-down recovery command is allowed to override an altitude climb
        // command until usable airflow returns.
        targetPitch =
            std::min(
                targetPitch,
                Clamp(Finite(stallRecovery.stallRecoveryPitch),
                      -pitchLimit,
                      pitchLimit));
    }

    float pitchRate =
        NonNegative(responses.maximumPitchRate) *
        authority;

    if (flightState.isGrounded)
    {
        const float effectiveTakeoffSpeed =
            std::max(
                NonNegative(flightPhysics.minimumTakeoffSpeed),
                NonNegative(flightPhysics.stallSpeed));

        if (ForwardAirSpeed() < effectiveTakeoffSpeed)
            pitchRate = 0.0f;
    }

    aircraftState.currentPitch =
        MoveTowards(
            aircraftState.currentPitch,
            Clamp(targetPitch, -pitchLimit, pitchLimit),
            pitchRate * dt);

    if (flightState.isGrounded)
    {
        aircraftState.currentRoll = 0.0f;
    }
    else
    {
        aircraftState.currentRoll =
            MoveTowardsAngle(
                aircraftState.currentRoll,
                Clamp(targetRoll, -maxBank, maxBank),
                NonNegative(responses.maximumRollRate) *
                    authority *
                    dt);
    }

    transform.pitch = aircraftState.currentPitch;
    transform.heading = aircraftState.currentHeading;
    transform.roll = aircraftState.currentRoll;

    aircraftState.currentYaw =
        aircraftState.currentHeading;
}

float Aircraft::NavigationPitchTarget() const
{
    const float targetAltitude =
        CommandedWorldAltitude();

    const float error =
        targetAltitude - transform.position.y;

    float verticalCommand = 0.0f;

    if (error >= 0.0f)
    {
        const float scale =
            std::max(
                0.001f,
                NonNegative(responses.deltaToReachMaxROC));

        verticalCommand =
            Clamp01(error / scale) *
            NonNegative(maximums.climbRate);
    }
    else
    {
        const float scale =
            std::max(
                0.001f,
                NonNegative(responses.deltaToReachMaxROD));

        verticalCommand =
            -Clamp01(-error / scale) *
            NonNegative(maximums.diveRate);
    }

    const float pitchLimit =
        Clamp(NonNegative(maximums.maxPitchAngle),
              0.0f,
              89.0f);

    if (flightState.isGrounded)
    {
        const float effectiveTakeoffSpeed =
            std::max(
                NonNegative(flightPhysics.minimumTakeoffSpeed),
                NonNegative(flightPhysics.stallSpeed));

        const bool rotationAllowed =
            aircraftState.currentSpeed > SpeedEpsilon &&
            ForwardAirSpeed() >= effectiveTakeoffSpeed;

        if (!rotationAllowed ||
            targetAltitude <=
                flightPhysics.groundLevel + GroundEpsilon)
        {
            return 0.0f;
        }

        const float rotationPitch =
            std::max(
                0.1f,
                NonNegative(flightPhysics.takeoffPitchThreshold));

        const float ratio =
            Clamp(
                verticalCommand /
                    std::max(SpeedEpsilon,
                             ForwardAirSpeed()),
                -1.0f,
                1.0f);

        const float requestedPitch =
            std::asin(ratio) * Rad2Deg;

        return std::min(
            pitchLimit,
            std::max(rotationPitch,
                     requestedPitch));
    }

    // Compensate gravity sink and vertical wind while trying to achieve the
    // commanded ground-relative climb/descent rate.
    const float requiredForwardVertical =
        verticalCommand +
        sinkSpeed_ -
        Finite(wind.windVelocity.y);

    const float ratio =
        Clamp(
            requiredForwardVertical /
                std::max(SpeedEpsilon,
                         NonNegative(aircraftState.currentSpeed)),
            -1.0f,
            1.0f);

    return Clamp(
        std::asin(ratio) * Rad2Deg,
        -pitchLimit,
        pitchLimit);
}


// ============================================================================
// VELOCITY / AERODYNAMIC STATE
// ============================================================================

void Aircraft::RebuildVelocity()
{
    const vec3 safeWind{
        Finite(wind.windVelocity.x),
        Finite(wind.windVelocity.y),
        Finite(wind.windVelocity.z)
    };

    if (flightState.isGrounded)
    {
        velocityState.groundVelocity =
            HorizontalForward() *
            NonNegative(aircraftState.currentSpeed);

        velocityState.airVelocity =
            velocityState.groundVelocity -
            safeWind;
    }
    else
    {
        velocityState.airVelocity =
            Forward() *
                NonNegative(aircraftState.currentSpeed) +
            vec3{0.0f, -NonNegative(sinkSpeed_), 0.0f};

        velocityState.groundVelocity =
            velocityState.airVelocity +
            safeWind;
    }

    UpdateVelocityFrames();
}

void Aircraft::UpdateVelocityFrames()
{
    velocityState.bodyAirVelocity =
        WorldToBody(velocityState.airVelocity);

    velocityState.bodyGroundVelocity =
        WorldToBody(velocityState.groundVelocity);

    speedState.trueAirSpeed =
        velocityState.airVelocity.Magnitude();

    const float gx = velocityState.groundVelocity.x;
    const float gz = velocityState.groundVelocity.z;

    speedState.groundSpeed =
        std::sqrt(gx * gx + gz * gz);

    flightState.verticalSpeed =
        velocityState.groundVelocity.y;

    const float ax = velocityState.airVelocity.x;
    const float az = velocityState.airVelocity.z;

    const float horizontalAir =
        std::sqrt(ax * ax + az * az);

    flightState.flightPathAngle =
        std::atan2(
            velocityState.airVelocity.y,
            horizontalAir) *
        Rad2Deg;

    const vec3 body =
        velocityState.bodyAirVelocity;

    if (body.SqrMagnitude() <= 0.000001f)
    {
        flightState.angleOfAttack = 0.0f;
        flightState.sideslipAngle = 0.0f;
    }
    else
    {
        // Reverse flow is not folded into a fake "forward" AoA.
        flightState.angleOfAttack =
            std::atan2(
                -body.y,
                body.z) *
            Rad2Deg;

        flightState.sideslipAngle =
            std::atan2(
                body.x,
                std::sqrt(
                    body.y * body.y +
                    body.z * body.z)) *
            Rad2Deg;
    }
}

void Aircraft::RefreshAerodynamicState()
{
    UpdateVelocityFrames();

    if (flightState.isGrounded)
    {
        flightState.isStalling = false;
        flightState.liftFactor = 0.0f;
        return;
    }

    const float speed =
        NonNegative(aircraftState.currentSpeed);

    const float stallSpeed =
        NonNegative(flightPhysics.stallSpeed);

    const float ratio =
        stallSpeed > SpeedEpsilon
            ? speed / stallSpeed
            : (speed > SpeedEpsilon ? 1.0f : 0.0f);

    const float criticalAoA =
        Clamp(
            NonNegative(flightPhysics.criticalAngleOfAttack),
            0.0f,
            180.0f);

    const bool aoaStall =
        std::abs(flightState.angleOfAttack) >
        criticalAoA;

    flightState.isStalling =
        speed <= SpeedEpsilon ||
        speed < stallSpeed ||
        aoaStall;

    // Upward wing support decreases with bank/inversion.
    const float upwardProjection =
        Clamp01(Up().y);

    flightState.liftFactor =
        Clamp01(
            ratio * ratio *
            upwardProjection) *
        (aoaStall
             ? Clamp01(flightPhysics.stalledLiftFactor)
             : 1.0f);
}

void Aircraft::UpdateFlightPhysics(float dt)
{
    RebuildVelocity();
    RefreshAerodynamicState();

    if (flightState.isGrounded)
    {
        sinkSpeed_ = 0.0f;
    }
    else
    {
        const float upwardSupport =
            Clamp01(flightState.liftFactor);

        sinkSpeed_ +=
            NonNegative(flightPhysics.gravity) *
            (1.0f - upwardSupport) *
            dt;

        const float stallSpeed =
            NonNegative(flightPhysics.stallSpeed);

        const bool recoveringFlow =
            !flightState.isStalling ||
            (aircraftState.currentSpeed >= stallSpeed &&
             aircraftState.currentPitch < 0.0f);

        const bool noseDownRecovery =
            aircraftState.currentSpeed >= stallSpeed &&
            aircraftState.currentPitch < 0.0f &&
            Up().y > 0.95f;

        if (recoveringFlow &&
            (upwardSupport > 0.95f ||
             noseDownRecovery))
        {
            sinkSpeed_ =
                MoveTowards(
                    sinkSpeed_,
                    0.0f,
                    NonNegative(
                        flightPhysics.stallRecoveryRate) *
                        dt);
        }

        sinkSpeed_ =
            Clamp(
                sinkSpeed_,
                0.0f,
                NonNegative(
                    flightPhysics.maxSinkSpeed));
    }

    RebuildVelocity();
    RefreshAerodynamicState();
}

void Aircraft::ApplyCeilingToVelocity(float dt)
{
    const float ceiling =
        Finite(maximums.maxAltitude);

    if (ceiling <= flightPhysics.groundLevel ||
        velocityState.groundVelocity.y <= 0.0f)
    {
        return;
    }

    const float allowedVertical =
        std::max(
            0.0f,
            ceiling - transform.position.y) /
        dt;

    if (velocityState.groundVelocity.y <= allowedVertical)
        return;

    velocityState.groundVelocity.y =
        allowedVertical;

    const vec3 safeWind{
        Finite(wind.windVelocity.x),
        Finite(wind.windVelocity.y),
        Finite(wind.windVelocity.z)
    };

    velocityState.airVelocity =
        velocityState.groundVelocity -
        safeWind;

    UpdateVelocityFrames();
}

void Aircraft::ResolveGroundContact()
{
    if (transform.position.y >
        flightPhysics.groundLevel)
    {
        return;
    }

    const bool touchdown =
        !flightState.isGrounded;

    transform.position.y =
        flightPhysics.groundLevel;

    if (touchdown)
    {
        aircraftState.currentSpeed =
            std::max(
                0.0f,
                vec3::Dot(
                    velocityState.groundVelocity,
                    HorizontalForward()));

        commandYawRate_ = 0.0f;
    }

    flightState.isGrounded = true;
    flightState.isStalling = false;
    sinkSpeed_ = 0.0f;

    if (touchdown)
    {
        aircraftState.currentPitch = 0.0f;
        aircraftState.currentRoll = 0.0f;

        transform.pitch = 0.0f;
        transform.roll = 0.0f;
    }
    else
    {
        // Stay level on the runway until takeoff rotation is permitted.
        const float effectiveTakeoffSpeed =
            std::max(
                NonNegative(flightPhysics.minimumTakeoffSpeed),
                NonNegative(flightPhysics.stallSpeed));

        if (ForwardAirSpeed() < effectiveTakeoffSpeed)
        {
            aircraftState.currentPitch = 0.0f;
            transform.pitch = 0.0f;
        }

        aircraftState.currentRoll = 0.0f;
        transform.roll = 0.0f;
    }

    RebuildVelocity();
    RefreshAerodynamicState();
}


// ============================================================================
// TELEMETRY
// ============================================================================

void Aircraft::PublishMotionTelemetry(float dt)
{
    aircraftState.currentAltitude =
        transform.position.y;

    aircraftState.currentHeightAboveGround =
        transform.position.y -
        flightPhysics.groundLevel;

    UpdateVelocityFrames();

    if (dt > 0.0f)
    {
        accelerationState.acceleration =
            (velocityState.groundVelocity -
             previousGroundVelocity_) /
            dt;

        accelerationState.bodyAcceleration =
            WorldToBody(
                accelerationState.acceleration);
    }
    else
    {
        accelerationState.acceleration = {};
        accelerationState.bodyAcceleration = {};
    }

    previousGroundVelocity_ =
        velocityState.groundVelocity;
}

void Aircraft::PublishAngularTelemetry(float oldPitch,
                                       float oldRoll,
                                       float oldHeading,
                                       float dt)
{
    aircraftState.currentPitchRate = 0.0f;
    aircraftState.currentRollRate = 0.0f;
    aircraftState.currentYawRate = 0.0f;
    aircraftState.horizonPitchRate = 0.0f;
    aircraftState.horizonRollRate = 0.0f;
    aircraftState.currentTurnRate = 0.0f;
    aircraftState.horizonRatesValid = false;

    if (dt <= 0.0f)
        return;

    const float pitchDot =
        DeltaAngle(oldPitch,
                   aircraftState.currentPitch) /
        dt;

    const float rollDot =
        DeltaAngle(oldRoll,
                   aircraftState.currentRoll) /
        dt;

    const float headingDot =
        DeltaAngle(oldHeading,
                   aircraftState.currentHeading) /
        dt;

    // Reconstruct the old body frame. Positive aircraft pitch is nose-up,
    // which is a negative right-hand rotation around local +X.
    const float h = oldHeading * Deg2Rad;
    const float p = oldPitch * Deg2Rad;
    const float r = oldRoll * Deg2Rad;

    const float sh = std::sin(h);
    const float ch = std::cos(h);
    const float sp = std::sin(p);
    const float cp = std::cos(p);
    const float sr = std::sin(r);
    const float cr = std::cos(r);

    const vec3 oldForward{
        sh * cp,
        sp,
        ch * cp
    };

    const vec3 unbankedRight{
        ch,
        0.0f,
        -sh
    };

    const vec3 unbankedUp =
        vec3::Cross(oldForward,
                    unbankedRight);

    const vec3 oldRight =
        unbankedRight * cr +
        unbankedUp * sr;

    const vec3 oldUp =
        unbankedUp * cr -
        unbankedRight * sr;

    // Approximate the net angular velocity vector in world space from the
    // Euler derivatives, then resolve it back into the old aircraft axes.
    const vec3 worldUp{0.0f, 1.0f, 0.0f};

    const vec3 omegaWorld =
        worldUp * headingDot +
        oldRight * (-pitchDot) +
        oldForward * rollDot;

    aircraftState.currentPitchRate =
        Finite(-vec3::Dot(omegaWorld,
                          oldRight));

    aircraftState.currentYawRate =
        Finite(vec3::Dot(omegaWorld,
                         oldUp));

    aircraftState.currentRollRate =
        Finite(vec3::Dot(omegaWorld,
                         oldForward));

    const vec3 newForward = Forward();

    const vec3 beforeHorizontal{
        oldForward.x,
        0.0f,
        oldForward.z
    };

    const vec3 afterHorizontal{
        newForward.x,
        0.0f,
        newForward.z
    };

    const bool nearPole =
        std::abs(oldForward.y) > 0.999f ||
        std::abs(newForward.y) > 0.999f;

    const bool crossedPole =
        vec3::Dot(beforeHorizontal,
                  afterHorizontal) < 0.0f &&
        std::abs(oldForward.y) > 0.9f &&
        std::abs(newForward.y) > 0.9f;

    aircraftState.horizonRatesValid =
        !nearPole && !crossedPole;

    if (!aircraftState.horizonRatesValid)
        return;

    aircraftState.horizonPitchRate =
        pitchDot;

    aircraftState.horizonRollRate =
        rollDot;

    // Legacy/current navigation turn rate means world heading change.
    aircraftState.currentTurnRate =
        headingDot;
}


// ============================================================================
// ORIENTATION / FRAME HELPERS
// ============================================================================

Aircraft::vec3 Aircraft::Forward() const
{
    const float heading =
        Finite(transform.heading) *
        Deg2Rad;

    const float pitch =
        Finite(transform.pitch) *
        Deg2Rad;

    const float sh = std::sin(heading);
    const float ch = std::cos(heading);
    const float sp = std::sin(pitch);
    const float cp = std::cos(pitch);

    return {
        sh * cp,
        sp,
        ch * cp
    };
}

Aircraft::vec3 Aircraft::Right() const
{
    const vec3 forward = Forward();

    const float heading =
        Finite(transform.heading) *
        Deg2Rad;

    const float roll =
        Finite(transform.roll) *
        Deg2Rad;

    const vec3 unbankedRight{
        std::cos(heading),
        0.0f,
        -std::sin(heading)
    };

    const vec3 unbankedUp =
        vec3::Cross(forward,
                    unbankedRight);

    return (
               unbankedRight * std::cos(roll) +
               unbankedUp * std::sin(roll)
               ).Normalized();
}

Aircraft::vec3 Aircraft::Up() const
{
    const vec3 forward = Forward();
    const vec3 right = Right();

    return vec3::Cross(forward,
                       right).Normalized();
}

Aircraft::vec3 Aircraft::HorizontalForward() const
{
    const float heading =
        Finite(transform.heading) *
        Deg2Rad;

    return {
        std::sin(heading),
        0.0f,
        std::cos(heading)
    };
}

Aircraft::vec3 Aircraft::WorldToBody(
    const vec3& worldVector) const
{
    const vec3 right = Right();
    const vec3 up = Up();
    const vec3 forward = Forward();

    return {
        vec3::Dot(worldVector, right),
        vec3::Dot(worldVector, up),
        vec3::Dot(worldVector, forward)
    };
}


// ============================================================================
// MATH
// ============================================================================

bool Aircraft::IsFinite(float value)
{
    return std::isfinite(value);
}

float Aircraft::Finite(float value,
                       float fallback)
{
    return IsFinite(value)
    ? value
    : fallback;
}

float Aircraft::NonNegative(float value)
{
    return std::max(
        0.0f,
        Finite(value));
}

float Aircraft::Clamp(float value,
                      float minimum,
                      float maximum)
{
    if (minimum > maximum)
        std::swap(minimum, maximum);

    return std::max(
        minimum,
        std::min(value, maximum));
}

float Aircraft::Clamp01(float value)
{
    return Clamp(value, 0.0f, 1.0f);
}

float Aircraft::Lerp(float a,
                     float b,
                     float t)
{
    return a +
           (b - a) *
               Clamp01(t);
}

float Aircraft::MoveTowards(float current,
                            float target,
                            float maxDelta)
{
    maxDelta = NonNegative(maxDelta);

    const float delta =
        target - current;

    if (std::abs(delta) <= maxDelta)
        return target;

    return current +
           (delta > 0.0f
                ? maxDelta
                : -maxDelta);
}

float Aircraft::MoveTowardsAngle(float current,
                                 float target,
                                 float maxDelta)
{
    const float delta =
        DeltaAngle(current, target);

    const float result =
        MoveTowards(
            current,
            current + delta,
            maxDelta);

    return Normalize180(result);
}

float Aircraft::Normalize360(float angle)
{
    angle = Finite(angle);

    angle =
        std::fmod(angle, 360.0f);

    if (angle < 0.0f)
        angle += 360.0f;

    return angle;
}

float Aircraft::Normalize180(float angle)
{
    angle = Normalize360(angle);

    if (angle > 180.0f)
        angle -= 360.0f;

    return angle;
}

float Aircraft::DeltaAngle(float current,
                           float target)
{
    return Normalize180(
        target - current);
}
