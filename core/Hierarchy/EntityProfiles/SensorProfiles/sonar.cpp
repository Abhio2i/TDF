#include "sonar.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/Components/transform.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/sonar/sonar_model.h"
#include "core/Hierarchy/Components/dynamicmodel.h"

Sonar::Sonar(Hierarchy* h) : Sensor(h) {
    subType = SubType::Sonar;
    azimuth = 360;
    range   = 100;
    qDebug()<<"i am sonar";

    m_activeSonar.setSoundSpeed(m_soundSpeed);
    m_activeSonar.setMaxRange(range * 1000.0f);
    m_activeSonar.setMaxDepth(m_maxDepth);
    m_activeSonar.setBeamWidth(azimuth);
    m_activeSonar.setPingInterval(m_pingInterval);
    m_activeSonar.setFalseDetectionRate(0.0f);
}

void Sonar::scan()
{
    if (!Active)       return;
    if (!parentEntity) return;

    Transform* source =
        (root->Platforms)[parentEntity->ID]->transform;
    if (!source) return;

    double lat     = source->getLatitude();
    double lon     = source->getLongitude();
    float  heading = source->getHeading();

    if (!m_timerStarted) {
        m_timer.start();
        m_timerStarted = true;
    }

    float simTime = m_timer.elapsed() / 1000.0f;

    // qDebug() << "SONAR scan() called simTime:" << simTime
    //          << "canPing:" << m_activeSonar.canPing(simTime)
    //          << "lastPingTime:" << m_activeSonar.getLastPingTime()
    //          << "pingInterval:" << m_pingInterval
    //          << "queueSize:" << m_echoQueue.size()
    //          << "lastResults:" << m_lastResults.size();

    // ── Position change detect karo ──
    bool positionChanged = false;
    if (std::abs(lat - m_lastLat) > 0.0001 ||
        std::abs(lon - m_lastLon) > 0.0001)
    {
        positionChanged = true;
        m_lastLat = lat;
        m_lastLon = lon;

        // qDebug() << "Position changed — queue reset";
    }

    // ── Config ──
    m_activeSonar.setEntityPosition(lat, lon);
    m_activeSonar.setHeading(heading);
    m_activeSonar.setMaxRange(range * 1000.0f);
    m_activeSonar.setBeamWidth(azimuth);
    m_activeSonar.setPingInterval(m_pingInterval);
    m_activeSonar.setSoundSpeed(m_soundSpeed);
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

    // ── Step 1: Pending echoes process karo ──
    processEchoQueue(simTime);

    // ── Step 2: Ping interval check ──
    if (!m_activeSonar.canPing(simTime)) return;

    // ── Step 3: New ping ──
    m_lastResults.clear();
    this->targets.clear();
    // ← m_echoQueue.clear() HATA DIYA — echoes lose nahi hone chahiye

    m_activeSonar.sendPing(simTime, 0.0f);

    // qDebug() << "After sendPing — targets collecting...";

    // qDebug() << "Sonar PING sent at t=" << simTime
    //          << "from" << QString::fromStdString(parentEntity->Name);

    // ── Step 4: Targets collect ──
    std::vector<SonarTarget> sonarTargets =
        collectTargets(lat, lon);

    qDebug() << "BEFORE OCCLUSION:" << sonarTargets.size();

    // apply occlusion
    sonarTargets = applyOcclusionFilter(sonarTargets, lat, lon);

    // qDebug() << "After OCCLUSION:" << sonarTargets.size();

    if (sonarTargets.empty())
    {
        qDebug() << "NO TARGETS — check entity positions";
        return;
    }

    // qDebug() << "Targets found:" << sonarTargets.size();

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

        // ── Directional factor ──
        float dirFactor = computeDirectionalFactor(bearing, heading);

        // Baffle zone — detect nahi karo
        if (dirFactor <= 0.05f)  // only ignore extremely weak
        {
            // qDebug() << "BAFFLE ZONE — skip:"
            //          << QString::fromStdString(target.name);
            continue;
        }

        // apply on strength
        // float finalStrength = target.targetStrength * dirFactor;

        float travelTime  = (2.0f * distance) / m_soundSpeed;
        float arrivalTime = simTime + travelTime;

        PendingEcho echo;
        echo.targetName     = target.name;
        echo.arrivalTime    = arrivalTime;
        echo.distance       = distance;
        echo.bearing        = bearing;
        echo.targetLat = target.lat;
        echo.targetLon = target.lon;
        echo.targetId = target.id;
        echo.targetStrength = target.targetStrength * dirFactor;
        echo.targetDepth = target.depth;
        m_echoQueue.push_back(echo);

        // qDebug() << "Echo scheduled:"
        //          << QString::fromStdString(target.name)
        //          << "dist:" << (int)distance << "m"
        //          << "travel:" << travelTime << "sec"
        //          << "arrival:" << arrivalTime << "sec";
    }

    // ← Sort by arrivalTime — paas wala pehle
    m_echoQueue.sort([](const PendingEcho& a, const PendingEcho& b) {
        return a.arrivalTime < b.arrivalTime;
    });
}

void Sonar::processEchoQueue(float simTime)
{
    Platform* platform = (root->Platforms)[parentEntity->ID];
    Transform* source = platform->transform;

    DynamicModel* model = platform->dynamicModel;


    double lat = source->getLatitude();
    double lon = source->getLongitude();

    // ← Clear mat karo yahan — sirf new ping pe clear hoga
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
        float dt = input.detectionThreshold;
        input.absorption =SonarModel::computeThorpAbsorption(frequency);
        input.targetStrength     = it->targetStrength;

        float selfSpeed = model->velocity.length();

        if (selfSpeed < 0.1f)
        {
            selfSpeed = model->moveSpeed / 3.6f;
        }

        float speedKnots = selfSpeed * 1.94384f;
        float flowNoise = 5.0f * log10(speedKnots + 1.0f);

        input.noiseLevel = m_noiseLevel + flowNoise;

        // qDebug() << "[SONAR] Noise level :" << input.noiseLevel
        //          << "Base:" << m_noiseLevel;

        float newDistance = SonarModel::geoDistance(
            lat, lon,
            it->targetLat,
            it->targetLon
            );

        float baseBearing = SonarModel::computeBearing(
            lat, lon,
            it->targetLat,
            it->targetLon
            );

        float depth1 = 0.0f;              // sonar (surface)
        float depth2 = it->targetDepth;     // target ka depth

        float c1 = m_activeSonar.getSoundSpeedAtDepth(depth1);
        float c2 = m_activeSonar.getSoundSpeedAtDepth(depth2);

        // qDebug() << "[SNELL DEBUG]"
        //          << "depth1:" << depth1
        //          << "c1:" << c1
        //          << "depth2:" << depth2
        //          << "c2:" << c2;

        float newBearing = SonarModel::computeSnellAngle(baseBearing, c1, c2);

        float tl  = SonarModel::computeTransmissionLoss(
            newDistance, input.absorption);

        // CZ apply
        tl = SonarModel::applyConvergenceZone(newDistance, tl);

        float effectiveTS = it->targetStrength;
        // optional realism boost
        effectiveTS -= (newDistance / 1000.0f) * 0.5f; // distance decay

        //  DI calculation
        float safeBeam = std::max(azimuth, 1.0f);
        float DI = 10.0f * log10(360.0f / safeBeam);

        float snr = SonarModel::computeActiveSNR(
            input.sourceLevel, tl,
            effectiveTS, input.noiseLevel,  DI);

        // 🔹 SELF SPEED
        //  float selfSpeed = model->velocity.length();

        // 🔹 TARGET SPEED (ID se)
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

        float relativeSpeed = targetSpeed - selfSpeed;

        // 🔹 RELATIVE SPEED
        relativeSpeed = std::clamp(relativeSpeed, -15.0f, 15.0f);

        // 🔹 Doppler Shift
        float doppler = SonarModel::computeDopplerShift(
            relativeSpeed,
            frequency * 1000.0f,
            c1
            );

        // 🔹 Doppler Loss
        float pulseDuration = 0.005f; // 10 ms

        float dopplerLoss = SonarModel::computeDopplerLoss(
            doppler,
            pulseDuration
            );

        dopplerLoss = std::clamp(dopplerLoss, 0.5f, 1.0f);

        // 🔹 APPLY LOSS TO SNR
        snr += 10.0f * log10(dopplerLoss);

        // 🔹 DEBUG
        qDebug() << "[DOPPLER]"
                 << "target:" << targetSpeed
                 << "self:" << selfSpeed
                 << "rel:" << relativeSpeed
                 << "Hz:" << doppler
                 << "loss:" << dopplerLoss;

        float theta = azimuth * M_PI/180.0f;
        float psi = theta * theta;

        float V = SonarModel::computeReverbVolume(
            c1, 0.01f, newDistance, psi);   // use 0.01 temporarily

        float SNR_vol = SonarModel::computeVolumeReverbSNR(
            input.sourceLevel, tl,
            -70.0f, V);   // ✅ realistic]

        float area = newDistance * newDistance * psi;

        float SNR_bot = SonarModel::computeBottomReverbSNR(
            input.sourceLevel, tl,
            -40.0f, area); // ✅ realistic

        float effectiveSNR = std::min({snr, SNR_vol, SNR_bot});

        qDebug() << "[REVERB DEBUG]"
                 << "SNR:" << snr
                 << "VOL:" << SNR_vol
                 << "BOT:" << SNR_bot
                 << "EFF:" << effectiveSNR;

        float FOM = SonarModel::computeFOM(
            input.sourceLevel,
            input.noiseLevel,
            DI,
            dt);

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
        result.bearing  = newBearing;
        result.category = it->category;

        if (effectiveSNR >= dt)
        {
            result.detected     = true;
            result.signalExcess = effectiveSNR - dt;
            result.confidence   = SonarModel::computeConfidence(
                result.signalExcess);
            result.reason       = "DETECTED";

            Target t;
            t.radius = newDistance;
            t.angle  = newBearing;
            this->targets.push_back(t);
            anyDetected = true;

            // ← lastResults me add karo — clear nahi
            // m_lastResults.push_back(result);


            bool exists = false;

            for (auto& r : m_lastResults)
            {
                if (r.name == result.name)
                {
                    exists = true;
                    break;
                }
            }

            if (!exists)
            {
                m_lastResults.push_back(result);
            }

            // qDebug() << "ECHO RECEIVED:"
            //          << QString::fromStdString(it->targetName)
            //          << "t=" << simTime
            //          << "dist:" << (int)newDistance
            //          << "bearing:" << newBearing;
        }
        else
        {
            result.detected = false;
            result.reason   = "WEAK SIGNAL";
            m_lastResults.push_back(result);
        }

        it = m_echoQueue.erase(it);
    }

    if (anyDetected)
        emit enemyDetected();
}

std::vector<SonarTarget> Sonar::collectTargets(
    double selfLat, double selfLon) const
{
    std::vector<SonarTarget> result;
    if (!root || root->Entities.empty()) return result;

    for (auto& [id, entity] : root->Entities)
    {
        if (!entity || id == parentEntity->ID) continue;

        // ── Sirf Ship aur Submarine — baaki sab skip ──
        if (entity->category != Entity::Category::Ship &&
            entity->category != Entity::Category::Submarine)
            continue;

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
        t.depth = (altitude < 0) ? (float)(-altitude) : 0.0f;

        // Target strength by category
        t.targetStrength = (entity->category == Entity::Category::Submarine)
                               ? 25.0f   // submarine — louder acoustically
                               : 15.0f;  // ship — quieter

        result.push_back(t);
    }

    return result;
}

//  Occlusion filter (real sonar shadowing)
std::vector<SonarTarget> Sonar::applyOcclusionFilter(
    const std::vector<SonarTarget>& input,
    double selfLat, double selfLon)
{
    std::vector<SonarTarget> sorted = input;

    // ✅ Step 1: nearest first
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

            bool sameLine = (diff < 5.0f);      // ~5° tolerance
            bool sameDepth = fabs(target.depth - prev.depth) < 20.0f; // 20m tolerance

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

// ── Directional factor calculate karo ──
// Bearing sonar entity ke heading ke relative me
float Sonar::computeDirectionalFactor(float absoluteBearing,
                                      float sonarHeading) const
{
    // Relative angle — sonar heading ke respect me
    float relAngle = absoluteBearing - sonarHeading;

    // Normalize to -180 to +180
    while (relAngle >  180.0f) relAngle -= 360.0f;
    while (relAngle < -180.0f) relAngle += 360.0f;

    float absAngle = std::abs(relAngle);

    // // ── Baffle zone — ship ke peeche ──
    // // Hull-mounted sonar: ~120° to 180° = baffle
    // if (absAngle > 120.0f)
    //     return 0.0f;   // BLIND — detect nahi hoga

    // ── Front cone (0-60°) → full detection ──
    if (absAngle <= 60.0f)
        return 1.0f;   // 100% SNR

    //  SIDE (60–120°)
    if (absAngle <= 120.0f)
    {
        float t = (absAngle - 60.0f) / 60.0f;
        return 1.0f - (t * 0.5f);   // 1 → 0.5
    }

    //  BACK-CORNER (120–160°) → weak but visible
    if (absAngle <= 160.0f)
    {
        float t = (absAngle - 120.0f) / 40.0f;
        return 0.5f - (t * 0.45f);  // 0.5 → 0.05
    }

    // ── Side (60-120°) → gradual reduction ──
    // Linear fade from 1.0 to 0.3
    // float t = (absAngle - 60.0f) / 60.0f;  // 0 to 1
    // return 1.0f - (t * 0.7f);              // 1.0 to 0.3

    //  EXTREME BACK (160–180°) → almost zero (smooth)
    float t = (absAngle - 160.0f) / 20.0f;
    return 0.05f * (1.0f - t);  // 0.05 → 0
}

QJsonObject Sonar::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["name"] = QString::fromStdString(Name);
    obj["active"] = Active;
    obj["SensorType"] = "Sonar";

    QJsonObject defaultObj;
    defaultObj["type"] = "Section";

    // ── Base params ──
    defaultObj["range"]     = toParm(range,     "km",  0,   500);
    defaultObj["frequency"] = toParm(frequency, "Ghz", 0.1, 100);
    defaultObj["azimuth"]   = toParm(azimuth,   "deg", 0,   360);

    // ── Sonar specific params ──
    defaultObj["soundSpeed"]   = toParm(m_soundSpeed,   "m/s", 1400, 1600);
    defaultObj["maxDepth"]     = toParm(m_maxDepth,     "m",   0,    1000);
    defaultObj["pingInterval"] = toParm(m_pingInterval, "sec", 1,    30);
    defaultObj["sourceLevel"]  = toParm(m_sourceLevel,  "dB",  100,  260);
    defaultObj["noiseLevel"]   = toParm(m_noiseLevel,   "dB",  0,    100);
    defaultObj["threshold"]    = toParm(m_threshold,    "dB",  0,    50);

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
            if (d.contains("threshold"))
                m_threshold    = valueFromParm(d["threshold"].toObject());
        }
    }
}
