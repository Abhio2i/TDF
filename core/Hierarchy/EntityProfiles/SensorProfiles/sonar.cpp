#include "sonar.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/Components/transform.h"
#include "core/Hierarchy/hierarchy.h"

Sonar::Sonar(Hierarchy* h) : Sensor(h) {
    subType = SubType::Sonar;
    azimuth = 360;
    qDebug()<<"i am sonar";

    m_activeSonar.setSoundSpeed(m_soundSpeed);
    m_activeSonar.setMaxRange(range * 1000.0f);
    m_activeSonar.setMaxDepth(m_maxDepth);
    m_activeSonar.setBeamWidth(azimuth);
    m_activeSonar.setPingInterval(m_pingInterval);
    m_activeSonar.setFalseDetectionRate(0.0f);
}
void Sonar::scan(){
    if(!Active)return;
    // qDebug() << "[Sensor::ewscan] called for ID:" << QString::fromStdString(id)
    if(!parentEntity) return;
    Transform* source = (*root->Platforms)[parentEntity->ID]->transform;
    if(!source) return;

    double lat     = source->getLatitude();
    double lon     = source->getLongitude();
    float  heading = source->getHeading();

    // qDebug() << "Sonar scan — entity:"
    //          << QString::fromStdString(parentEntity->Name)
    //          << "lat:" << lat
    //          << "lon:" << lon
    //          << "heading:" << heading;

    // ── Timer ──
    if (!m_timerStarted) {
        m_timer.start();
        m_timerStarted = true;
    }

    float simTime = m_timer.elapsed() / 1000.0f;

    // ──  ActiveSonar config — Inspector values ──
    m_activeSonar.setSoundSpeed(m_soundSpeed);
    m_activeSonar.setMaxRange(range * 1000.0f);   // km → meters
    m_activeSonar.setMaxDepth(m_maxDepth);
    m_activeSonar.setBeamWidth(azimuth);
    m_activeSonar.setPingInterval(m_pingInterval);
    m_activeSonar.setEntityPosition(lat, lon);    // real position
    m_activeSonar.setHeading(heading);            // real heading

    // ── Ping interval check ──
    if (!m_activeSonar.canPing(simTime)) return;

    m_activeSonar.sendPing(simTime, 0.0f); // last pingTime

    // ── Targets collect ──
    std::vector<SonarTarget> sonarTargets =
        collectTargets(lat, lon);

    if (sonarTargets.empty()) return;

    // ── Acoustic input — Inspector se ──
    SonarInput input;
    input.sourceLevel        = m_sourceLevel;
    input.noiseLevel         = m_noiseLevel;
    input.detectionThreshold = m_threshold;
    input.absorption         = 0.0001f;
    input.targetStrength     = 0.0f;

    // ── Scan ──
    m_lastResults = m_activeSonar.scan(sonarTargets, input);

    // ── Base class targets update ──
    this->targets.clear();

    for (const auto& r : m_lastResults)
    {
        if (!r.detected) continue;

        Target t;
        t.radius = r.distance;
        t.angle  = r.bearing;
        t.lat    = 0.0f;
        t.lon    = 0.0f;
        this->targets.push_back(t);
    }

    if (!this->targets.isEmpty())
        emit enemyDetected();
    else
        emit enemyNotFound();
}

std::vector<SonarTarget> Sonar::collectTargets(
    double lat, double lon) const
{
    std::vector<SonarTarget> result;

    if (!root || !root->Entities) return result;

    for (auto& [id, entity] : *root->Entities)
    {
        if (!entity)                    continue;
        if (id == parentEntity->ID)     continue;  // khud skip

        QJsonObject transformJson =
            entity->getComponent("transform");
        if (transformJson.isEmpty()) continue;

        QJsonObject geocord = transformJson["geocord"].toObject();
        if (geocord.isEmpty()) continue;

        double lat      = geocord["latitude"].toDouble();
        double lon      = geocord["longitude"].toDouble();
        double altitude = geocord["altitude"].toDouble();

        if (lat == 0.0 && lon == 0.0) continue;

        QJsonObject entityJson = entity->toJson();
        QString     entityType = entityJson["Type"].toString();


        // Aircraft skip — sonar underwater hai
        if (entityType.contains("Aircraft",   Qt::CaseInsensitive) ||
            entityType.contains("Helicopter", Qt::CaseInsensitive) ||
            entityType.contains("UAV",        Qt::CaseInsensitive) ||
            entityType.contains("Airplane",   Qt::CaseInsensitive))
            continue;

        SonarTarget t;
        t.name           = entity->Name;
        t.lat            = lat;
        t.lon            = lon;
        t.depth          = (altitude < 0) ? (float)(-altitude) : 0.0f;
        t.targetStrength = 20.0f;

        // Target strength by type
        if (entityType.contains("Submarine", Qt::CaseInsensitive))
            t.targetStrength = 25.0f;
        if (entityType.contains("Ship",      Qt::CaseInsensitive) ||
            entityType.contains("Destroyer", Qt::CaseInsensitive) ||
            entityType.contains("Frigate",   Qt::CaseInsensitive))
            t.targetStrength = 15.0f;

        // qDebug() << "Entity:" << QString::fromStdString(entity->Name)
        //          << "Type:" << entityType
        //          << "depth:" << altitude;

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

