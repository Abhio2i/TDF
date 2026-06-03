#include "sonar.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/Components/transform.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/sonar/sonar_model.h"
#include "core/Hierarchy/Components/dynamicmodel.h"
#include <cstdlib>
#include "core/Simulation/simulation.h"

Sonar::Sonar(Hierarchy* h) : Sensor(h) {
    subType = SubType::
        Sonar;azimuth = 20;
    range   = 100;
    qDebug()<<"i am sonar";

    m_activeSonar.setSoundSpeed(m_soundSpeed);
    m_activeSonar.setMaxRange(range * 1000.0f);
    m_activeSonar.setMaxDepth(m_maxDepth);
    m_activeSonar.setBeamWidth(azimuth);
    m_activeSonar.setPingInterval(m_pingInterval);
    m_activeSonar.setFalseDetectionRate(0.0f);

}


// ==========================================================
// scan()
// Main active sonar scan cycle.

// Workflow:
// 1. Validate sonar/platform state
// 2. Update sonar position and environmental parameters
// 3. Process previously scheduled echoes
// 4. Check ping interval
// 5. Transmit new ping
// 6. Collect visible targets
// 7. Apply occlusion filtering
// 8. Schedule echo arrivals
// ==========================================================
void Sonar::scan(){
    if (!Active)
        return;
    if (!parentEntity)
        return;

    Transform* source = (root->Platforms)[parentEntity->ID]->transform;
    if (!source) return;

    double lat     = source->getLatitude();
    double lon     = source->getLongitude();
    float  heading = source->getHeading();

    // if (!m_timerStarted) {
    //     m_timer.start();
    //     m_timerStarted = true;
    // }

    // float simTime = m_timer.elapsed() / 1000.0f;

    float simTime = Simulation::simulationTime;

    // Detect platform movement.
    // Position changes may require refreshing sonar state.
    bool positionChanged = false;

    if (std::abs(lat - m_lastLat) > 0.0001 ||
        std::abs(lon - m_lastLon) > 0.0001)
    {
        positionChanged = true;
        m_lastLat = lat;
        m_lastLon = lon;
    }

    // ── Config ──
    m_activeSonar.setEntityPosition(lat, lon);
    m_activeSonar.setHeading(heading);
    m_activeSonar.setMaxRange(range * 1000.0f);
    m_activeSonar.setBeamWidth(azimuth);
    m_activeSonar.setPingInterval(m_pingInterval);
    //m_activeSonar.setSoundSpeed(m_soundSpeed);
    float sonarDepth = source->getAltitude();

    sonarDepth = (sonarDepth < 0) ? -sonarDepth : 0.0f;

    // Calculate sound speed using Mackenzie equation.
    // Sound speed varies with temperature, salinity and depth.
    float dynamicC = SonarModel::computeMackenzieSoundSpeed(m_waterTemperature, m_salinity, sonarDepth);

    m_activeSonar.setSoundSpeed(dynamicC);
    m_activeSonar.setMaxDepth(m_maxDepth);

    // ── Position change hone pe queue + results clear ──
    if (positionChanged)
    {
        // m_echoQueue.clear();
        // m_lastResults.clear();
        // this->targets.clear();

        // // Timer reset — fresh ping
        // m_timer.restart();
        // m_activeSonar.sendPing(-999.0f, 0.0f); // force next ping

        m_lastLat = lat;
        m_lastLon = lon;
    }

    // ── Step 1: Check whether sonar is allowed to transmit a new ping. ──
    processEchoQueue(simTime);

    // ── Step 2: Ping interval check ──
    if (!m_activeSonar.canPing(simTime)) return;

    // ── Step 3: Start a fresh ping cycle.
    // Previous detections are cleared but pending echoes remain. ──
    m_lastResults.clear();
    this->targets.clear();
    // ← m_echoQueue.clear() HATA DIYA — echoes lose nahi hone chahiye

    m_activeSonar.sendPing(simTime, 0.0f);

    // ── Step 4: ather all potential sonar targets from hierarchy. ──
    std::vector<SonarTarget> sonarTargets = collectTargets(lat, lon);

    qDebug() << "BEFORE OCCLUSION:" << sonarTargets.size();

    // Remove targets hidden behind nearer objects.
    sonarTargets = applyOcclusionFilter(sonarTargets, lat, lon);

    // qDebug() << "After OCCLUSION:" << sonarTargets.size();

    if (sonarTargets.empty())
    {
        qDebug() << "NO TARGETS — check entity positions";
        return;
    }

    //qDebug() << "Targets found:" << sonarTargets.size();

    // ── Step 5: Echo queue me schedule karo ──
    for (const auto& target : sonarTargets)
    {
        float distance = SonarModel::geoDistance(lat, lon,
                                                 target.lat,
                                                 target.lon);
        float bearing  = SonarModel::computeBearing(lat, lon,
                                                   target.lat,
                                                   target.lon);

        if (distance > range * 1000.0f) continue;
        if (target.depth > m_maxDepth)  continue;

        // Apply sonar directional response.
        // Rear sectors receive lower sensitivity.
        float dirFactor = computeDirectionalFactor(bearing, heading);

        // Baffle zone — detect nahi karo
        if (dirFactor <= 0.05f)  // only ignore extremely weak
        {
            // qDebug() << "BAFFLE ZONE — skip:"
            //          << QString::fromStdString(target.name);
            continue;
        }

        // float travelTime  = (2.0f * distance) / m_soundSpeed;
        float avgC = (dynamicC + SonarModel::computeMackenzieSoundSpeed(m_waterTemperature, m_salinity, target.depth)) * 0.5f;

        // Calculate two-way acoustic travel time.
        // Echo will be processed when arrivalTime is reached.
        float travelTime =(2.0f * distance) / avgC;
        float arrivalTime = simTime + travelTime;

        PendingEcho echo;
        echo.targetName     = target.name;
        echo.arrivalTime    = arrivalTime;
        echo.distance       = distance;
        echo.bearing        = bearing;
        echo.targetLat = target.lat;
        echo.targetLon = target.lon;
        echo.targetId = target.id;
        echo.category    = target.category;
        echo.subCategory = target.subCategory;
        // echo.targetStrength = target.targetStrength * dirFactor;
        echo.targetStrength = target.targetStrength * (0.5f + 0.5f * dirFactor);
        echo.targetDepth = target.depth;
        m_echoQueue.push_back(echo);  // Store pending echo for future processing.
    }

    // ← Sort by arrivalTime — paas wala pehle
    m_echoQueue.sort([](const PendingEcho& a, const PendingEcho& b) {
        return a.arrivalTime < b.arrivalTime;
    });

}

// ==========================================================
// processEchoQueue()
//
// Processes echoes that have reached the sonar.

// Performs:
// - Transmission loss calculation
// - Noise estimation
// - Target strength modelling
// - Doppler analysis
// - Reverberation modelling
// - Signal excess calculation
// - Probability of detection
// - Detection generation
// ==========================================================
void Sonar::processEchoQueue(float simTime){
    Platform* platform = (root->Platforms)[parentEntity->ID];
    Transform* source = platform->transform;

    DynamicModel* model = platform->dynamicModel;

    // Obtain sonar platform position and heading
    double lat = source->getLatitude();
    double lon = source->getLongitude();

    bool anyDetected = false;

    auto it = m_echoQueue.begin();
    while (it != m_echoQueue.end())
    {
        if (simTime < it->arrivalTime)
        {
            ++it;
            continue;
        }

        SonarInput input;
        input.sourceLevel        = m_sourceLevel;
        //input.noiseLevel         = m_noiseLevel;
        // input.detectionThreshold = m_threshold;
        input.detectionThreshold = SonarModel::validateDetectionThreshold(m_threshold);
        input.absorption =SonarModel::computeThorpAbsorption(frequency);
        //input.targetStrength     = it->targetStrength;

        float newDistance = SonarModel::geoDistance(
            lat, lon,
            it->targetLat,
            it->targetLon
            );

        float selfSpeed = model->velocity.length();

        float speedKnots;

        if (selfSpeed < 0.1f)
            speedKnots = model->moveSpeed / 1.852f;
        else
            speedKnots = selfSpeed * 1.94384f;

        float ambientNoise = SonarModel::computeAmbientNoise(frequency);

        float flowNoise = SonarModel::computeFlowNoise(speedKnots);

        float windNoise = SonarModel::computeWindNoise(frequency, m_windSpeed);

        float shippingNoise = SonarModel::computeShippingNoise(frequency, m_shippingDensity);

        std::vector<float> noiseSources =
            {
                m_noiseLevel,
                flowNoise,
                ambientNoise,
                windNoise,
                shippingNoise
            };

        // Estimate total environmental noise.
        // Includes:
        //   - User configured noise
        //   - Flow noise
        //   - Ambient ocean noise
        //   - Wind noise
        //   - Shipping noise
        input.noiseLevel = SonarModel::computeTotalNoise(noiseSources);


        qDebug() << "[FLOW NOISE]"
                 << "Speed(knots):" << speedKnots
                 << "NL:" << flowNoise;

        // // 🌊 ambient ocean noise (dynamic, not hardcoded logic)
        // float seaState = 1.5f; // later config se lena
        // float ambientNoise = 20.0f + 2.0f * seaState;

        // qDebug() << "[SONAR] Noise level :" << input.noiseLevel
        //          << "Base:" << m_noiseLevel;

        // Compute one-way acoustic transmission loss.
        float tl  = SonarModel::computeTransmissionLoss(
            newDistance, input.absorption);

        float baseBearing = SonarModel::computeBearing(
            lat, lon,
            it->targetLat,
            it->targetLon
            );

        float relBearing = baseBearing - source->getHeading();

        // normalize
        while (relBearing > 180.0f) relBearing -= 360.0f;
        while (relBearing < -180.0f) relBearing += 360.0f;

        // LOS projection
        float losFactor = cos(relBearing * M_PI / 180.0f);

        //float depth1 = 0.0f;              // sonar (surface)
        float depth1 = platform->transform->getAltitude();

        depth1 = (depth1 < 0) ? -depth1 : 0.0f;
        float depth2 = it->targetDepth;     // target ka depth

        float c1 = SonarModel::computeMackenzieSoundSpeed(m_waterTemperature, m_salinity, depth1);

        float c2 = SonarModel::computeMackenzieSoundSpeed(m_waterTemperature, m_salinity, depth2);

        float avgC = (c1 + c2) * 0.5f;

        // Compute Time Difference Of Arrival (TDOA)
        // for bearing estimation using hydrophone array.
        float tdoa =SonarModel::computeTDOA(m_arrayLength,relBearing,avgC);

        //float noisyTDOA = tdoa + (((rand() % 100) / 10000.0f) - 0.005f);
        float noisyTDOA = tdoa;

        // Noise only when angle sufficiently large
        if (std::abs(relBearing) > 10.0f)
        {
            noisyTDOA += (((rand() % 100) / 1000000.0f) - 0.00005f);
        }

        float estimatedBearing =SonarModel::computeBearingFromTDOA(noisyTDOA , m_arrayLength,avgC);

        QString subCategory = QString::fromStdString(it->subCategory);

        float lambda = c1 / (frequency * 1000.0f);

        // Estimate target strength based on platform type.
        if (subCategory == "Submarine")
        {
            float modelTS =SonarModel::computeSubmarineTargetStrength( 100.0f, lambda);
            input.targetStrength = it->targetStrength + modelTS;
        }
        else if (subCategory == "Ship")
        {
            float modelTS = SonarModel::computeCylinderTargetStrength(5.0f, 120.0f, lambda, newDistance);
            input.targetStrength = it->targetStrength + modelTS;
        }
        else if (subCategory == "Frigate")
        {
            float modelTS = SonarModel::computeCylinderTargetStrength( 4.0f, 140.0f, lambda, newDistance);
            input.targetStrength = it->targetStrength + modelTS;
        }
        else
        {
            input.targetStrength =SonarModel::computeTargetStrength( 10.0f);
        }

        float PRI = m_pingInterval;  //already present in sonar

        //float PRI = (2.0f * range * 1000.0f) / c1;

        // Calculate maximum range before range ambiguity occurs.
        float maxRange = SonarModel::computeMaxUnambiguousRange(c1, PRI);

        qDebug() << "[TL TEST]"
                 << "Distance:" << newDistance
                 << "TL:" << tl;

        if (newDistance > maxRange)
        {
            qDebug() << "[AMBIGUOUS WARNING] Range folding possible at"
                     << newDistance << "m (limit:" << maxRange << "m)";

            // it = m_echoQueue.erase(it);
            ++it;
            continue;
        }

        qDebug() << "[MAX RANGE]"
                 << "PRI:" << PRI
                 << "MaxRange:" << maxRange;

        // Doppler Loss
        float pulseDuration = m_pulseDuration;

        float rangeRes = SonarModel::computeRangeResolution(c1, pulseDuration);

        float f0 = frequency * 1000.0f;

        // array length (assume ya config se lo)
        float arrayLength = m_arrayLength;  // meters (later config)

        // bearing resolution (radians)
        float bearingRes = SonarModel::computeBearingResolution(lambda, arrayLength);

        // degrees me convert
        float bearingResDeg = bearingRes * 180.0f / M_PI;

        qDebug() << "[BEARING RES]"
                 << "lambda:" << lambda
                 << "L:" << arrayLength
                 << "rad:" << bearingRes
                 << "deg:" << bearingResDeg;

        float vmax = SonarModel::computeMaxUnambiguousSpeed(lambda, PRI);

        qDebug() << "[MAX SPEED]"
                 << "lambda:" << lambda
                 << "PRI:" << PRI
                 << "vmax(m/s):" << vmax
                 << "vmax(kts):" << vmax * 1.94384f;

        float newBearing = baseBearing;

        // float tl  = SonarModel::computeTransmissionLoss(
        //     newDistance, input.absorption);

        float depthDiff = fabs(depth1 - depth2);

        // thermocline-like attenuation
        float depthFactor = exp(-depthDiff / 200.0f);

        tl += (1.0f - depthFactor) * 4.0f;

        float boundaryLoss = 0.0f;

        if (depth2 < 20.0f) boundaryLoss += 2.0f;
        if (depth2 > m_maxDepth * 0.8f) boundaryLoss += 3.0f;

        tl += boundaryLoss;

        // CZ apply
        //tl = SonarModel::applyConvergenceZone(newDistance, tl);

        // CZ sirf deep ocean me hota hai
        if (m_maxDepth >= 300.0f)
        {
            tl = SonarModel::applyConvergenceZone(newDistance, tl);
        }

        const float MIN_DETECTION_RANGE = 50.0f;

        if (newDistance < MIN_DETECTION_RANGE)
        {
            qDebug() << "[SONAR] Target too close / overlap ignored:"
                     << newDistance << "m"
                     << "TL:" << tl;

            it = m_echoQueue.erase(it);
            continue;
        }

        float effectiveTS = input.targetStrength;

        float elements = (2.0f * m_arrayLength) / lambda;

        // Compute sonar array directivity index.
        // Larger arrays provide better spatial filtering.
        float DI = SonarModel::computeDirectivityIndex(elements);
        DI = std::clamp(DI, 0.0f, 30.0f);

        qDebug() << "[DI]" << "arrayLength:" << m_arrayLength
                 << "lambda:" << lambda
                 << "DI:" << DI;

        float snr = SonarModel::computeActiveSNR(input.sourceLevel, tl, effectiveTS, input.noiseLevel,  DI);

        float nPulses = m_integrationPulses;  // ping accumulation

        float integrationGain = SonarModel::computeIncoherentIntegrationGain(nPulses);

        // SNR boost
        snr += integrationGain;

        qDebug() << "[INTEGRATION]"
                 << "Pulses:" << nPulses
                 << "Gain(dB):" << integrationGain;

        // TARGET SPEED (ID se)
        auto itPlatform = root->Platforms.find(it->targetId);

        if (itPlatform == root->Platforms.end())
        {
            qDebug() << "[ERROR] targetId not found:"
                     << QString::fromStdString(it->targetId);

            it = m_echoQueue.erase(it);
            continue;
        }

        Platform* targetPlatform = itPlatform->second;

        if (!targetPlatform || !targetPlatform->dynamicModel)
        {
            qDebug() << "[ERROR] Null platform/dynamicModel";

            it = m_echoQueue.erase(it);
            continue;
        }        DynamicModel* targetModel = targetPlatform->dynamicModel;

        float targetSpeed = targetModel->velocity.length();

        if (targetSpeed < 0.1f)
        {
            targetSpeed = targetModel->moveSpeed / 3.6f;
        }

        // transmitter = target, receiver = self sonar
        // target velocity projected towards sonar
        float v_tx = targetSpeed * losFactor;

        // receiver velocity projected towards target (opposite direction)
        float v_rx = -selfSpeed * losFactor;

        float relVel = v_tx + v_rx;

        // optional clamp (realistic limits)
        v_tx = std::clamp(v_tx, -15.0f, 15.0f);
        v_rx = std::clamp(v_rx, -15.0f, 15.0f);

        // Calculate Doppler shifted echo frequency caused by
        // relative target and observer motion.
        float f_echo = SonarModel::computeDopplerFrequency(f0, v_tx, v_rx, avgC);

        // Δf (optional — for logging / loss)
        float deltaF = f_echo - f0;

        float estimatedSpeed = SonarModel::computeVelocityFromDoppler(deltaF, f0, avgC);

        float dopplerLoss = SonarModel::computeDopplerLoss(deltaF, pulseDuration);

        dopplerLoss = std::clamp(dopplerLoss, 0.5f, 1.0f);

        // Apply Doppler mismatch loss to effective SNR.
        snr += 10.0f * log10(dopplerLoss);

        // DEBUG
        qDebug() << "[DOPPLER]"
                 << "TargetSpeed:" << targetSpeed
                 << "self:" << selfSpeed
                 << "speed:" << estimatedSpeed
                 << "Frequency:" << f0
                 << "f_echo:" << f_echo
                 << "deltaF:" << deltaF
                 << "loss(L):" << dopplerLoss
                 << "T:" << pulseDuration;

        float theta = azimuth * M_PI/180.0f;
        float psi = theta * theta;

        float V = SonarModel::computeReverbVolume(c1, pulseDuration, newDistance, psi);   // use 0.01 temporarily

        // Estimate volume reverberation level.
        float SNR_vol = SonarModel::computeVolumeReverbSNR(input.sourceLevel, tl, -70.0f, V);

        float area = SonarModel::computeEnsonifiedArea(c1, pulseDuration, newDistance, psi);

        // Estimate bottom reverberation level.
        float SNR_bot = SonarModel::computeBottomReverbSNR(input.sourceLevel, tl, m_bottomBackscatter, area);

        //float effectiveSNR = std::min(snr, std::max(SNR_vol, SNR_bot));
        float effectiveSNR = snr;

        // TDOA decorrelation loss
        float tdoaPenalty = std::exp(-std::abs(tdoa) * 50.0f);

        effectiveSNR += 10.0f * log10(tdoaPenalty);

        // apply reverb penalty only if worse
        float reverbLimit = std::max(SNR_vol, SNR_bot);

        if (reverbLimit > snr)
        {
            effectiveSNR -= (reverbLimit - snr);
        }


        // multipath / ocean fluctuation (real sonar effect)
        float multipath = ((rand() % 100) / 100.0f - 0.5f) * 1.0f;  // ±1 dB
        effectiveSNR += multipath;

        // Final detection test.
        // Target is detected when effective SNR exceeds threshold.
        bool detected = (effectiveSNR >= input.detectionThreshold);

        qDebug() << "[SNR CHECK]"
                 << "target:" << QString::fromStdString(it->targetName)
                 << "dist:" << newDistance
                 << "TL:" << tl
                 << "TS:" << effectiveTS
                 << "NL:" << input.noiseLevel
                 << "DI:" << DI
                 << "SNR:" << effectiveSNR
                 << "threshold:" << input.detectionThreshold
                 << "detected:" << detected;

        float dt = input.detectionThreshold;

        // Calculate detection and false alarm probabilities.
        float pfa = SonarModel::computeFalseAlarmProbability(dt);
        float pd = SonarModel::computeProbabilityOfDetection(effectiveSNR, pfa);

        float requiredSNR = SonarModel::computeRequiredSNR(pd, pfa);

        qDebug() << "Pd:" << pd
                 << "Pfa:" << pfa
                 << "Required SNR:" << requiredSNR
                 << "Actual SNR:" << effectiveSNR;

        // Reverb sirf debug ke liye log karo
        qDebug() << "[REVERB INFO] SNR:" << snr
                 << "VOL:" << SNR_vol
                 << "BOT:" << SNR_bot;

        float FOM = SonarModel::computeFOM(input.sourceLevel, input.noiseLevel, DI, dt);

        qDebug() << "[FOM]" << FOM;

        if (std::isnan(effectiveSNR))
        {
            qDebug() << "[SE ERROR] SNR is NaN";
            it = m_echoQueue.erase(it);
            continue;
        }

        DetectionResult result;
        result.name     = it->targetName;
        result.distance = newDistance;
        //result.bearing  =  newBearing + (estimatedBearing - relBearing);
        float measuredBearing = newBearing;

        // Apply bearing error
        if (std::abs(relBearing) > 10.0f)
        {
            float bearingError = estimatedBearing - relBearing;

            // Limit error to sonar bearing resolution
            bearingError = std::clamp(
                bearingError,
                -bearingResDeg,
                bearingResDeg);

            measuredBearing = newBearing + bearingError;

            while (measuredBearing < 0.0f)
                measuredBearing += 360.0f;

            while (measuredBearing >= 360.0f)
                measuredBearing -= 360.0f;
        }

        result.bearing = measuredBearing;

        result.category = it->category;
        result.subCategory = it->subCategory;

        // Pfa calculate
        //float pfa = SonarModel::computeFalseAlarmProbability(dt);

        // random value
        float randVal = rand() / (float)RAND_MAX;

        // detection conditions
        bool realDetection  = (randVal < pd);   // probabilistic detection
        bool falseDetection = (randVal < pfa);

        float maxSNR   = 50.0f;
        float snrNorm  = std::clamp(effectiveSNR / maxSNR, 0.0f, 1.0f);
        float distNorm = 1.0f - std::clamp(newDistance / (range * 1000.0f), 0.0f, 1.0f);

        if (realDetection || falseDetection)
        {
            result.detected     = true;
            result.signalExcess = effectiveSNR - dt;
            result.confidence   = SonarModel::computeConfidence(result.signalExcess);
            result.intensity = (snrNorm * 0.6f) + (distNorm * 0.4f);

            if (realDetection)
                result.reason = "DETECTED";
            else
                result.reason = "FALSE ALARM";

            Target t;
            t.radius = newDistance;
            t.angle  = newBearing;
            this->targets.push_back(t);
            anyDetected = true;

            // duplicate check
            bool exists = false;

            for (auto& r : m_lastResults)
            {
                float distDiff = fabs(r.distance - newDistance);
                float bearingDiff = fabs(r.bearing - newBearing);

                if (bearingDiff > 180.0f)
                    bearingDiff = 360.0f - bearingDiff;

                if (distDiff < rangeRes && bearingDiff < bearingResDeg)
                {
                    exists = true;
                    break;
                }
            }

            float signalExcess = effectiveSNR - dt;

            float safeDistance = m_safeObstacleDistance;

            bool warning = SonarModel::computeObstacleWarning(newDistance, safeDistance, relVel, result.detected);

            if (warning)
            {
                qDebug() << "[WARNING] Obstacle closing!"
                         << "Dist:" << newDistance
                         << "RelVel:" << relVel;
            }

            if (!exists)
            {
                m_lastResults.push_back(result);
            }

            // DEBUG
            qDebug() << "[PD + PFA]"
                     << "SNR:" << effectiveSNR
                     << "DT:" << dt
                     << "Pfa:" << pfa
                     << "Pd:" << pd
                     << "SE:" << signalExcess
                     << "Rand:" << randVal
                     << "Real:" << realDetection
                     << "False:" << falseDetection;
        }

        else
        {
            result.detected = false;
            result.intensity = 0.0f;
            result.reason   = "WEAK SIGNAL";
            m_lastResults.push_back(result);
        }

        it = m_echoQueue.erase(it);
    }

    if (anyDetected)
        emit enemyDetected();

}

// ==========================================================
// collectTargets()
//
// Finds all marine entities that can act as sonar targets.
// Converts entity information into SonarTarget objects.
// ==========================================================
std::vector<SonarTarget> Sonar::collectTargets(
    double selfLat, double selfLon) const
{

    qDebug() << "[COLLECT TARGETS CALLED]";

    std::vector<SonarTarget> result;
    if (!root || root->Entities.empty()) return result;

    for (auto& [id, entity] : root->Entities)
    {
        qDebug() << "[ENTITY LOOP]";
        if (!entity || id == parentEntity->ID) continue;

        // Only Marine entities
        if (entity->category != Entity::Category::Marine)
        {
            continue;
        }

        QJsonObject transformJson = entity->getComponent("transform");
        if (transformJson.isEmpty()) continue;

        QJsonObject geocord = transformJson["geocord"].toObject();
        double tLat     = geocord["latitude"].toDouble();
        double tLon     = geocord["longitude"].toDouble();
        double altitude = geocord["altitude"].toDouble();

        if (tLat == 0.0 && tLon == 0.0) continue;

        SonarTarget t;
        t.name  = entity->Name;
        t.lat   = tLat;
        t.lon   = tLon;
        t.id   = id;
        t.category = entity->category;

        QJsonObject entityJson = entity->toJson();

        QString subCat =
            entityJson["SubCategory"]
                .toObject()["value"]
                .toString();

        t.subCategory = subCat.toStdString();

        qDebug() << "[TARGET SUBCATEGORY]"
                 << subCat;

        switch (entity->marineCategory)
        {
        case Entity::SubMarineCategory::Submarine:
            t.targetStrength = 20.0f;
            break;

        case Entity::SubMarineCategory::Ship:
            t.targetStrength = 15.0f;
            break;

        case Entity::SubMarineCategory::Frigate:
            t.targetStrength = 18.0f;
            break;

        default:
            t.targetStrength = 10.0f;
            break;
        }

        qDebug() << entity->Name.c_str()
                 << "Category:" << entity->category
                 << "MarineCategory:" << entity->marineCategory;

        t.depth = (altitude < 0) ? (float)(-altitude) : 0.0f;

        result.push_back(t);
    }

    return result;

}

// ==========================================================
// applyOcclusionFilter()
//
// Removes targets shadowed by nearer objects located along
// approximately the same bearing and depth.
//
// Simulates acoustic shadowing effects.
// ==========================================================
std::vector<SonarTarget> Sonar::applyOcclusionFilter(
    const std::vector<SonarTarget>& input,
    double selfLat, double selfLon)
{
    std::vector<SonarTarget> sorted = input;
    // Step 1: nearest first
    std::sort(sorted.begin(), sorted.end(),
              [&](const SonarTarget& a, const SonarTarget& b)
              {
                  float da = SonarModel::geoDistance(selfLat, selfLon, a.lat, a.lon);
                  float db = SonarModel::geoDistance(selfLat, selfLon, b.lat, b.lon);
                  return da < db;
              });

    std::vector<SonarTarget> visible;

    for (const auto& target : sorted)
    {
        float dist = SonarModel::geoDistance(selfLat, selfLon, target.lat, target.lon);
        float bearing = SonarModel::computeBearing(selfLat, selfLon, target.lat, target.lon);

        bool occluded = false;

        for (const auto& prev : visible)
        {
            float prevDist = SonarModel::geoDistance(selfLat, selfLon, prev.lat, prev.lon);
            float prevBearing = SonarModel::computeBearing(selfLat, selfLon, prev.lat, prev.lon);

            //  Bearing difference
            float diff = fabs(bearing - prevBearing);
            if (diff > 180) diff = 360 - diff;

            float angularSpread = atan2(20.0f, prevDist) * 180.0f / M_PI;

            bool sameLine = (diff < angularSpread);

            float depthTolerance = std::max(20.0f, 0.02f * prevDist);

            bool sameDepth = fabs(target.depth - prev.depth) < depthTolerance;

            if (sameLine && sameDepth && dist > prevDist)
            {
                occluded = true;
                break;
            }
        }

        if (!occluded)
        {
            visible.push_back(target);
        }
    }

    return visible;
}


// ==========================================================
// computeDirectionalFactor()
//
// Models sonar directional sensitivity.
//
// Forward sector:
//     High sensitivity
//
// Rear sector:
//     Reduced sensitivity
//
// Baffle zone:
//     Minimal detection capability
// ==========================================================
float Sonar::computeDirectionalFactor(float bearing,float heading) const{

    // Relative angle
    float rel =std::fmod(bearing - heading + 360.0f,360.0f);

    // Mirror to 0-180
    if (rel > 180.0f)
        rel = 360.0f - rel;

    // Safety
    rel = std::abs(rel);

    float factor = 1.0f;

    // Forward lobe
    if (rel <= 60.0f)
    {
        factor = 1.0f;
    }

    // 60 → 90
    else if (rel <= 90.0f)
    {
        float t = (rel - 60.0f) / 30.0f;

        factor = 1.0f - t * 0.25f;
    }

    // 90 → 120
    else if (rel <= 120.0f)
    {
        float t = (rel - 90.0f) / 30.0f;

        factor = 0.75f - t * 0.25f;
    }

    // 120 → 140
    else if (rel <= 140.0f)
    {
        float t = (rel - 120.0f) / 20.0f;

        factor = 0.5f - t * 0.225f;
    }

    // 140 → 160
    else if (rel <= 160.0f)
    {
        float t = (rel - 140.0f) / 20.0f;

        factor = 0.275f - t * 0.225f;
    }

    // Rear blind zone
    else
    {
        factor = 0.05f;
    }

    factor = std::clamp(factor,0.05f, 1.0f);

    qDebug() << "[DIR FACTOR]"
             << "bearing:" << bearing
             << "heading:" << heading
             << "relAngle:" << rel
             << "factor:" << factor;

    return factor;

}

QJsonObject Sonar::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["name"] = QString::fromStdString(Name);
    obj["active"] = Active;obj["SensorType"] = "Sonar";

    QJsonObject defaultObj;
    defaultObj["type"] = "Section";

    // ── Base params ──
    defaultObj["range"]     = toParm(range,     "km",  0,   500);
    defaultObj["frequency"] = toParm(frequency, "Khz", 0, 1000);
    defaultObj["azimuth"]   = toParm(azimuth,   "deg", 0,   360);

    // ── Sonar specific params ──
    defaultObj["soundSpeed"]   = toParm(m_soundSpeed,   "m/s", 1400, 1600);
    defaultObj["maxDepth"]     = toParm(m_maxDepth,     "m",   0,    1000);
    defaultObj["pingInterval"] = toParm(m_pingInterval, "sec", 0,    100);
    defaultObj["sourceLevel"]  = toParm(m_sourceLevel,  "dB",  0,  300);
    defaultObj["noiseLevel"]   = toParm(m_noiseLevel,   "dB",  0,    150);
    defaultObj["windSpeed"] = toParm(m_windSpeed,"kts", 0, 100);
    defaultObj["threshold"]    = toParm(m_threshold,    "dB",  0,    50);
    defaultObj["arrayLength"] = toParm(m_arrayLength, "m", 0, 20);
    defaultObj["pulseDuration"] = toParm(m_pulseDuration, "sec", 0.0001, 1.0);
    defaultObj["integrationPulses"] = toParm(m_integrationPulses, "pulses", 1, 1000);
    defaultObj["safeObstacleDistance"] = toParm(m_safeObstacleDistance,"m", 0, 10000);
    defaultObj["bottomBackscatter"] = toParm(m_bottomBackscatter, "dB",-100,20);
    defaultObj["shippingDensity"] = toParm(m_shippingDensity, "density", 0, 100);
    defaultObj["waterTemperature"] = toParm(m_waterTemperature, "°C", -5, 40);
    defaultObj["salinity"] = toParm(m_salinity, "ppt", 0, 40);

    obj["default"] = defaultObj;
    return obj;

}

void Sonar::fromJson(const QJsonObject& obj) {
    if (obj.contains("id")){
        ID = obj["id"].toString().toStdString();
    }
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    if (obj.contains("default") && obj["default"].isObject()) {
        QJsonObject defaultObj = obj["default"].toObject();
        if (defaultObj.contains("range"))
            range = valueFromParm(defaultObj["range"].toObject());
        if (defaultObj.contains("frequency"))
            frequency = valueFromParm(defaultObj["frequency"].toObject());

        if (defaultObj.contains("azimuth"))
            azimuth = valueFromParm(defaultObj["azimuth"].toObject());

        if (obj.contains("default") && obj["default"].isObject())
        {
            QJsonObject d = obj["default"].toObject();

            // Base
            if (d.contains("range"))
                range     = valueFromParm(d["range"].toObject());
            if (d.contains("frequency"))
                frequency = valueFromParm(d["frequency"].toObject());
            if (d.contains("azimuth"))
                azimuth   = valueFromParm(d["azimuth"].toObject());


            // Sonar specific
            if (d.contains("soundSpeed"))
                m_soundSpeed   = valueFromParm(d["soundSpeed"].toObject());
            if (d.contains("maxDepth"))
                m_maxDepth     = valueFromParm(d["maxDepth"].toObject());
            if (d.contains("pingInterval"))
                m_pingInterval = valueFromParm(d["pingInterval"].toObject());
            if (d.contains("sourceLevel"))
                m_sourceLevel  = valueFromParm(d["sourceLevel"].toObject());
            if (d.contains("noiseLevel"))
                m_noiseLevel   = valueFromParm(d["noiseLevel"].toObject());
            if (d.contains("windSpeed"))
                m_windSpeed   = valueFromParm(d["windSpeed"].toObject());
            if (d.contains("threshold"))
                m_threshold    = valueFromParm(d["threshold"].toObject());

            if (d.contains("arrayLength"))
                m_arrayLength = valueFromParm(d["arrayLength"].toObject());

            if (d.contains("pulseDuration"))
                m_pulseDuration = valueFromParm(d["pulseDuration"].toObject());

            if (d.contains("integrationPulses"))
                m_integrationPulses = valueFromParm(d["integrationPulses"].toObject());

            if (d.contains("safeObstacleDistance"))
                m_safeObstacleDistance = valueFromParm(d["safeObstacleDistance"].toObject());

            if (d.contains("bottomBackscatter"))
                m_bottomBackscatter = valueFromParm(d["bottomBackscatter"].toObject());

            if (d.contains("shippingDensity"))
                m_shippingDensity = valueFromParm(d["shippingDensity"].toObject());

            if (d.contains("waterTemperature"))
                m_waterTemperature = valueFromParm(d["waterTemperature"].toObject());

            if (d.contains("salinity"))
                m_salinity = valueFromParm(d["salinity"].toObject());
        }
    }

}
