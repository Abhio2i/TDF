#include "sonar.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/Components/transform.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/sonar/sonar_model.h"

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
        root->Platforms[parentEntity->ID]->transform;
    if (!source) return;

    double lat     = source->getLatitude();
    double lon     = source->getLongitude();
    float  heading = source->getHeading();

    if (!m_timerStarted) {
        m_timer.start();
        m_timerStarted = true;
    }

    float simTime = m_timer.elapsed() / 1000.0f;

    qDebug() << "SONAR scan() called simTime:" << simTime
             << "canPing:" << m_activeSonar.canPing(simTime)
             << "lastPingTime:" << m_activeSonar.getLastPingTime()
             << "pingInterval:" << m_pingInterval
             << "queueSize:" << m_echoQueue.size()
             << "lastResults:" << m_lastResults.size();

    // ── Position change detect karo ──
    bool positionChanged = false;
    if (std::abs(lat - m_lastLat) > 0.0001 ||
        std::abs(lon - m_lastLon) > 0.0001)
    {
        positionChanged = true;
        m_lastLat = lat;
        m_lastLon = lon;

        qDebug() << "Position changed — queue reset";
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
        m_echoQueue.clear();
        m_lastResults.clear();
        this->targets.clear();

        // Timer reset — fresh ping
        m_timer.restart();
        m_activeSonar.sendPing(-999.0f, 0.0f); // force next ping
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

    qDebug() << "After sendPing — targets collecting...";

    qDebug() << "Sonar PING sent at t=" << simTime
             << "from" << QString::fromStdString(parentEntity->Name);

    // ── Step 4: Targets collect ──
    std::vector<SonarTarget> sonarTargets =
        collectTargets(lat, lon);

    qDebug() << "collectTargets returned:" << sonarTargets.size()
             << "range:" << (range * 1000.0f)
             << "maxDepth:" << m_maxDepth;

    if (sonarTargets.empty())
    {
        qDebug() << "NO TARGETS — check entity positions";
        return;
    }

    qDebug() << "Targets found:" << sonarTargets.size();

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

        float travelTime  = (2.0f * distance) / m_soundSpeed;
        float arrivalTime = simTime + travelTime;

        PendingEcho echo;
        echo.targetName     = target.name;
        echo.arrivalTime    = arrivalTime;
        echo.distance       = distance;
        echo.bearing        = bearing;
        echo.targetStrength = target.targetStrength;

        m_echoQueue.push_back(echo);

        qDebug() << "Echo scheduled:"
                 << QString::fromStdString(target.name)
                 << "dist:" << (int)distance << "m"
                 << "travel:" << travelTime << "sec"
                 << "arrival:" << arrivalTime << "sec";
    }

    // ← Sort by arrivalTime — paas wala pehle
    m_echoQueue.sort([](const PendingEcho& a, const PendingEcho& b) {
        return a.arrivalTime < b.arrivalTime;
    });
}

void Sonar::processEchoQueue(float simTime)
{
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
        input.noiseLevel         = m_noiseLevel;
        input.detectionThreshold = m_threshold;
        input.absorption         = 0.0001f;
        input.targetStrength     = it->targetStrength;

        float tl  = SonarModel::computeTransmissionLoss(
            it->distance, input.absorption);
        float snr = SonarModel::computeActiveSNR(
            input.sourceLevel, tl,
            it->targetStrength, input.noiseLevel);

        DetectionResult result;
        result.name     = it->targetName;
        result.distance = it->distance;
        result.bearing  = it->bearing;
        result.category = it->category;

        if (snr >= input.detectionThreshold)
        {
            result.detected     = true;
            result.signalExcess = snr - input.detectionThreshold;
            result.confidence   = SonarModel::computeConfidence(
                result.signalExcess);
            result.reason       = "DETECTED";

            Target t;
            t.radius = result.distance;
            t.angle  = result.bearing;
            this->targets.push_back(t);
            anyDetected = true;

            // ← lastResults me add karo — clear nahi
            m_lastResults.push_back(result);

            qDebug() << "ECHO RECEIVED:"
                     << QString::fromStdString(it->targetName)
                     << "t=" << simTime
                     << "dist:" << (int)it->distance;
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
    if (!root ) return result;

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
        t.depth = (altitude < 0) ? (float)(-altitude) : 0.0f;

        // Target strength by category
        t.targetStrength = (entity->category == Entity::Category::Submarine)
                               ? 25.0f   // submarine — louder acoustically
                               : 15.0f;  // ship — quieter

        result.push_back(t);
    }

    return result;
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
