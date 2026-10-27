// #include "esm.h"
// #include "core/Hierarchy/Utils/entityutils.h"
// #include "core/Hierarchy/hierarchy.h"
// const float RAD2DEG = 180.0f / M_PI;
// ESM::ESM(Hierarchy* h) : Sensor(h) {
//     subType = SubType::ESM;
//     azimuth = 360;
//     // if(h){
//     //     connect(h,&Hierarchy::broadCast,[](Hierarchy::BroadcastMsg msg){

//     //     }
//     // );
// }

// void ESM::scan(){
//     if(!Active) return;
//     if(!parentEntity) return;
//     if(!root) return;
//     // Static default transform agar ID na mile (example ke taur par)
//     Transform defaultTransform;

//     Transform& source = root->Platforms.count(parentEntity->ID)
//                             ? root->Platforms[parentEntity->ID]->transform
//                             : defaultTransform;
//     // if(!source) return;
//     detect.clear();
//     ewdetects.clear();
//     for (auto& [key, entity] : root->Sensors)
//     {
//         if(!entity) continue;
//         if(!entity->parentEntity) continue;
//         if(!entity->parentEntity->Active) continue;
//         if(entity->subType != Sensor::SubType::Generic) continue;
//         if(!entity->Active) continue;
//         if(entity->parentEntity->ID == parentEntity->ID) continue;
//         auto it = root->Platforms.find(entity->parentEntity->ID);
//         if(it == root->Platforms.end()) continue;
//         Platform* platform = it->second;
//         if(!platform) continue;
//         // if(!platform->transform) continue;
//         float metredis = distanceBetween(
//                              source.getLatitude(),  source.getLongitude(),
//                              platform->transform.getLatitude(), platform->transform.getLongitude()
//                              ) / 1000.0f;
//         float dLon  = platform->transform.getLongitude() - source.getLongitude();
//         float dLat  = platform->transform.getLatitude()  - source.getLatitude();
//         float yAngle = std::atan2(dLon, dLat) * RAD2DEG;
//         bool inRange = (range * 1000.0f) > metredis;
//         if(inRange)
//         {
//             ewdetects.insert(platform);
//             ESMTarget target;
//             target.entity  = platform;
//             target.angle   = yAngle;
//             target.radius  = metredis;
//             detect.append(target);
//         }
//     }
// }

// QJsonObject ESM::toJson() const {
//     QJsonObject obj;
//     obj["id"] = QString::fromStdString(ID);
//     obj["name"] = QString::fromStdString(Name);
//     obj["active"] = Active;
//     obj["SensorType"] = "ESM";
//     QJsonObject defaultObj;
//     defaultObj["type"] = "Section";
//     defaultObj["range"] = toParm(range,"km",  0,   500);
//     defaultObj["frequency"] = toParm(frequency,"Ghz", 0.1, 100);
//     defaultObj["azimuth"] = toParm(azimuth,"deg", 0,   360);
//     obj["default"] = defaultObj;
//     QJsonObject AddParameters = AdditionalParameters;
//     AddParameters["type"] = "Section";
//     obj["AdditionalParameters"] = AddParameters;
//     return obj;
// }


// void ESM::fromJson(const QJsonObject& obj) {
//     if (obj.contains("id")){
//         ID = obj["id"].toString().toStdString();
//     }
//     if (obj.contains("active"))
//         Active = obj["active"].toBool();
//     if (obj.contains("default") && obj["default"].isObject()) {
//         QJsonObject defaultObj = obj["default"].toObject();
//         if (defaultObj.contains("range"))
//             range = valueFromParm(defaultObj["range"].toObject());
//         if (defaultObj.contains("frequency"))
//             frequency = valueFromParm(defaultObj["frequency"].toObject());
//         if (defaultObj.contains("azimuth"))
//             azimuth = valueFromParm(defaultObj["azimuth"].toObject());
//     }
//     if(obj.contains("AdditionalParameters")){
//         AdditionalParameters = obj["AdditionalParameters"].toObject();
//     }
// }



//(Vansh)

#include "esm.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/ESM/esmemitters.h"        #(Vansh)
#include <QDebug>
#include <QHash>
#include <cmath>
#include <typeinfo>  #Vansh

namespace {
constexpr double kDeg2Rad = M_PI / 180.0;
constexpr double kRad2Deg = 180.0 / M_PI;
constexpr double kTransformToMetres = 1000.0;  // same convention as Radar::buildPose (COORD_SCALE)

/* Great-circle initial bearing, deg clockwise from north, (-180,180]. */
double initialBearingDeg(double lat1, double lon1, double lat2, double lon2)
{
    const double p1 = lat1 * kDeg2Rad, p2 = lat2 * kDeg2Rad, dl = (lon2 - lon1) * kDeg2Rad;
    const double y = std::sin(dl) * std::cos(p2);
    const double x = std::cos(p1) * std::sin(p2) - std::sin(p1) * std::cos(p2) * std::cos(dl);
    return std::atan2(y, x) * kRad2Deg;
}

double altitudeMetres(Platform* p)
{
    return std::max(0.0, double(p->transform.matrix.translation().y()) * kTransformToMetres);
}

quint32 deriveEmitterId(const std::string& sensorId)
{
    return static_cast<quint32>(qHash(QString::fromStdString(sensorId))) | 1u;   // never 0
}
} // namespace

ESM::ESM(Hierarchy* h) : Sensor(h)
{
    subType = SubType::ESM;
    azimuth = 360;
}

void ESM::clearOutputs()
{
    detect.clear();
    ewdetects.clear();
    specPsdDbm_.clear();   // never leave a stale spectrum looking live
    specAcc_.reset();
}

void ESM::scan()
{
    if (!Active || !parentEntity || !root || !rx.isValid()) { clearOutputs(); return; }

    auto ownIt = root->Platforms.find(parentEntity->ID);
    if (ownIt == root->Platforms.end() || !ownIt->second) { clearOutputs(); return; }
    Platform* ownPlatform = ownIt->second;
    Transform& source = ownPlatform->transform;

    const double lat1 = source.getLatitude(), lon1 = source.getLongitude();
    if (!std::isfinite(lat1) || !std::isfinite(lon1)) { clearOutputs(); return; }
    const double ownAltM = altitudeMetres(ownPlatform);

    detect.clear();
    ewdetects.clear();
    /*QHash<const Platform*, int> indexByPlatform;*/     // one observation per platform
    QVector<esmrf::SpectrumContribution> specEmitters;

    for (auto& [key, emitter] : root->Sensors)
    {
        if (!emitter || !emitter->parentEntity) continue;
        if (!emitter->parentEntity->Active || !emitter->Active) continue;
        if (emitter->parentEntity->ID == parentEntity->ID) continue;

        // Emitter identification: explicit RF interface (any subtype), or legacy Generic.
        // const IRFEmitter* rfIf = dynamic_cast<const IRFEmitter*>(emitter);
        // const bool legacyGeneric = (emitter->subType == Sensor::SubType::Generic);
        // if (!rfIf && !(legacyGeneric && rx.legacyGenericDetection)) continue;

        //Vansh
        RFEmitterParams ep;
        const EsmEmitterQuery q = esmQueryEmitter(emitter, ep);

        //Vansh
        qDebug() << "[ESM-QUERY]"
                 << "Emitter:" << QString::fromStdString(emitter->Name)
                 << "Query:" << int(q)
                 << "Emitting:" << ep.emitting
                 << "Frequency MHz:" << ep.frequencyMHz
                 << "Receiver band:" << rx.fMinMHz << "-" << rx.fMaxMHz;

        //Vansh
        static int dbgTick = 0;
        const bool dbg = (dbgTick++ % 200) == 0;
        if (dbg) qDebug() << "[ESM] emitter" << QString::fromStdString(emitter->Name)
                     << typeid(*emitter).name() << "query=" << int(q);

        // const bool legacyGeneric = (emitter->subType == Sensor::SubType::Generic);
        // if (q == EsmEmitterQuery::Unavailable) continue;                       // RF sensor, no data
        // if (q == EsmEmitterQuery::NotApplicable && !(legacyGeneric && rx.legacyGenericDetection)) continue;

        // Emitter-level ESM: process only emitters with usable RF parameters.
        // Do not create geometric-only detections for generic platforms. (Vansh)
        if (q != EsmEmitterQuery::Ok)
            continue;

        if (!ep.emitting)
            continue;

        auto pit = root->Platforms.find(emitter->parentEntity->ID);
        if (pit == root->Platforms.end() || !pit->second) continue;
        Platform* platform = pit->second;

        const double lat2 = platform->transform.getLatitude();
        const double lon2 = platform->transform.getLongitude();
        if (!std::isfinite(lat2) || !std::isfinite(lon2)) continue;

        const double distanceKm = distanceBetween(lat1, lon1, lat2, lon2) / 1000.0;  // m -> km
        if (!std::isfinite(distanceKm)) continue;

        const double configuredRangeKm = static_cast<double>(range);

        if (!std::isfinite(configuredRangeKm) ||
            configuredRangeKm <= 0.0 ||
            configuredRangeKm > 500.0) {
            qWarning() << "[ESM] Invalid runtime range:"
                       << configuredRangeKm << "km";
            clearOutputs();
            return;
        }

        if (distanceKm > configuredRangeKm)
            continue;
        // ESMTarget t;
        // t.entity = platform;
        // t.angle  = initialBearingDeg(lat1, lon1, lat2, lon2);
        // t.radius = distanceKm;

        ESMTarget t;
        t.entity = platform;
        t.angle  = initialBearingDeg(lat1, lon1, lat2, lon2);
        t.radius = distanceKm;

        // Preserve the identity of the actual emitter sensor.
        t.emitterSensorId =
            QString::fromStdString(emitter->ID);

        t.emitterSensorName =
            QString::fromStdString(emitter->Name);

        // if (rfIf) {
        //     RFEmitterParams ep;
        //     if (!rfIf->rfEmitterParams(ep) || !ep.emitting) continue;

        //Vansh
        if (q == EsmEmitterQuery::Ok) {
            if (!ep.emitting) continue;

            const double horizonKm = rx.enableHorizon
                                         ? esmrf::losHorizonKm(ownAltM, altitudeMetres(platform), rx.earthRadiusFactor) : -1.0;
            const esmrf::RFLink link = esmrf::computeLink(ep, distanceKm, rx, horizonKm);

            //Vansh
            if (dbg) qDebug() << "[ESM]   reason=" << int(link.reason)
                         << "Pr=" << link.rxPowerDbm
                         << "SNR=" << link.snrDb;

            // Present, in-band, line-of-sight signals shape the spectrum even when too weak to detect.
            if (link.reason != esmrf::LinkReason::InvalidInput &&
                link.reason != esmrf::LinkReason::OutOfBand &&
                link.reason != esmrf::LinkReason::BeyondHorizon)
                specEmitters.append({ ep.frequencyMHz, ep.bandwidthHz, link.rxPowerDbm });

            //Vansh
            qDebug() << "[ESM-SPECTRUM-CONTRIBUTION]"
                     << "Frequency MHz:" << ep.frequencyMHz
                     << "Bandwidth Hz:" << ep.bandwidthHz
                     << "Received power dBm:" << link.rxPowerDbm;
            //Vansh

            if (!link.detected) continue;

            t.emitterValid = true;
            t.emitterid    = ep.emitterId ? ep.emitterId : deriveEmitterId(emitter->ID);
            t.frequencyMHz = ep.frequencyMHz;        // NOMINAL, not measured
            t.emitterType  = ep.typeLabel;
            t.rfValid      = true;
            t.bandwidthHz  = ep.bandwidthHz;
            t.rxPowerDbm   = link.rxPowerDbm;
            t.snrDb        = link.snrDb;
            t.pathLossDb   = link.pathLossDb;
        }
        // else: legacy Generic -> geometric-only target, RF fields stay invalid

        // auto dup = indexByPlatform.find(platform);   // duplicate suppression per platform
        // if (dup == indexByPlatform.end()) {
        //     indexByPlatform.insert(platform, detect.size());
        //     detect.append(t);
        // } else {
        //     ESMTarget& old = detect[dup.value()];
        //     if (t.rfValid && (!old.rfValid || t.snrDb > old.snrDb)) old = t;
        // }

        // Each detected emitter gets its own observation.
        // Do not merge emitters merely because they share a platform.
        detect.append(t);
        ewdetects.insert(platform);
    }

    //Vansh
    qDebug() << "[ESM-SPECTRUM]"
             << "Contributions:" << specEmitters.size()
             << "Points:" << rx.spectrumPoints
             << "Min MHz:" << rx.fMinMHz
             << "Max MHz:" << rx.fMaxMHz;

    QVector<double> f, p;
    if (esmrf::synthesizeSpectrum(rx, specEmitters, f, p)) {
        specAcc_.setWindow(rx.averageWindow);
        specAcc_.update(f, p);
        specPsdDbm_ = specAcc_.psdDbmPerHz();
    } else {
        specAcc_.reset();
        specPsdDbm_.clear();
    }
}

QJsonObject ESM::toJson() const
{
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["name"] = QString::fromStdString(Name);
    obj["active"] = Active;
    obj["SensorType"] = "ESM";

    QJsonObject d;
    d["type"] = "Section";
    d["range"] = toParm(range, "km", 0, 500);
    d["frequency"] = toParm(frequency, "Ghz", 0.1, 100);
    d["azimuth"] = toParm(azimuth, "deg", 0, 360);
    obj["default"] = d;

    QJsonObject r;
    r["type"] = "Section";
    r["freqMin"]        = toParm(float(rx.fMinMHz),         "MHz",    1,     100000, "Receiver lower frequency");
    r["freqMax"]        = toParm(float(rx.fMaxMHz),         "MHz",    1,     100000, "Receiver upper frequency");
    r["bandwidth"]      = toParm(float(rx.bandwidthHz/1e6), "MHz",    0.001, 10000,  "Receiver analysis bandwidth");
    r["noiseFigure"]    = toParm(float(rx.noiseFigureDb),   "dB",     0,     60,     "Receiver noise figure");
    r["sensitivity"]    = toParm(float(rx.sensitivityDbm),  "dBm",   -150,   0,      "Minimum channel power");
    r["minSnr"]         = toParm(float(rx.minSnrDb),        "dB",    -20,    60,     "Minimum detection SNR");
    r["rxGain"]         = toParm(float(rx.rxGainDbi),       "dBi",   -30,    60,     "Receive antenna gain");
    r["rainLoss"]       = toParm(float(rx.rainDbPerKm),     "dB/km",  0,     50,     "Rain attenuation (gamma)");
    r["atmosphericLoss"]= toParm(float(rx.atmosphericDbPerKm),"dB/km",0,     50,     "Atmospheric attenuation");
    r["otherLoss"]      = toParm(float(rx.otherLossDb),     "dB",     0,     100,    "Other fixed losses");
    r["earthRadiusFactor"] = toParm(float(rx.earthRadiusFactor), "", 1,      2,      "Effective earth radius factor");
    r["spectrumPoints"] = toParm(float(rx.spectrumPoints),  "pts",    2,     4096,   "Spectrum points");
    r["averageWindow"]  = toParm(float(rx.averageWindow),   "frames", 1,     1000,   "Average window");
    r["gainMode"]       = static_cast<int>(rx.gainMode);   // 0 main, 1 peak sidelobe, 2 avg sidelobe
    r["useSnrThreshold"] = rx.useSnrThreshold;
    r["enableHorizon"]   = rx.enableHorizon;
    r["legacyGenericDetection"] = rx.legacyGenericDetection;
    obj["receiver"] = r;

    QJsonObject add = AdditionalParameters;
    add["type"] = "Section";
    obj["AdditionalParameters"] = add;
    return obj;
}

void ESM::fromJson(const QJsonObject& obj)
{
    if (obj.contains("id"))     ID = obj["id"].toString().toStdString();
    if (obj.contains("active")) Active = obj["active"].toBool();


    if (obj.contains("default") && obj["default"].isObject()) {
        const QJsonObject d = obj["default"].toObject();

        auto readParameter = [&](const char* key, double& output,
                                 double minValue, double maxValue) -> bool {
            const QString parameterName = QString::fromLatin1(key);

            if (!d.contains(parameterName) ||
                !d[parameterName].isObject()) {
                return true; // Missing field: retain existing value.
            }

            const double value =
                valueFromParm(d[parameterName].toObject());

            if (!std::isfinite(value) ||
                value < minValue || value > maxValue) {
                qWarning() << "[ESM] Invalid parameter:"
                           << parameterName << value
                           << "Allowed:" << minValue << "to" << maxValue;
                return false;
            }

            output = value;
            return true;
        };

        double loadedRange = range;
        double loadedFrequency = frequency;
        double loadedAzimuth = azimuth;

        const bool validRange =
            readParameter("range", loadedRange, 0.0, 500.0);

        const bool validFrequency =
            readParameter("frequency", loadedFrequency, 0.1, 100.0);

        const bool validAzimuth =
            readParameter("azimuth", loadedAzimuth, 0.0, 360.0);

        // Apply only after every supplied parameter passes validation.
        if (validRange && validFrequency && validAzimuth) {
            range = static_cast<float>(loadedRange);
            frequency = static_cast<float>(loadedFrequency);
            azimuth = static_cast<float>(loadedAzimuth);
        } else {
            qWarning() << "[ESM] Default configuration rejected;"
                       << "retaining previous values.";
        }
    }

    // Old scenarios have no "receiver" section -> defaults stay.
    if (obj.contains("receiver") && obj["receiver"].isObject()) {
        const QJsonObject r = obj["receiver"].toObject();
        ESMReceiverConfig c = rx;
        double pts = c.spectrumPoints, win = c.averageWindow;
        auto num = [&](const char* k, double& v, double scale = 1.0) {
            if (r.contains(k) && r[k].isObject()) v = double(valueFromParm(r[k].toObject())) * scale;
        };
        num("freqMin", c.fMinMHz);            num("freqMax", c.fMaxMHz);
        num("bandwidth", c.bandwidthHz, 1e6); // file MHz -> internal Hz
        num("noiseFigure", c.noiseFigureDb);  num("sensitivity", c.sensitivityDbm);
        num("minSnr", c.minSnrDb);            num("rxGain", c.rxGainDbi);
        num("rainLoss", c.rainDbPerKm);       num("atmosphericLoss", c.atmosphericDbPerKm);
        num("otherLoss", c.otherLossDb);      num("earthRadiusFactor", c.earthRadiusFactor);
        num("spectrumPoints", pts);           num("averageWindow", win);

        if (!std::isfinite(pts) ||
            !std::isfinite(win) ||
            std::floor(pts) != pts ||
            std::floor(win) != win ||
            pts < 2.0 || pts > 4096.0 ||
            win < 1.0 || win > 1000.0) {
            qWarning() << "[ESM] Invalid integer receiver parameters:"
                       << "spectrumPoints =" << pts
                       << "averageWindow =" << win;
            return;
        }

        c.spectrumPoints = static_cast<int>(pts);
        c.averageWindow = static_cast<int>(win);        if (r.contains("gainMode"))        c.gainMode = static_cast<GainMode>(r["gainMode"].toInt(int(c.gainMode)));
        if (r.contains("useSnrThreshold")) c.useSnrThreshold = r["useSnrThreshold"].toBool(c.useSnrThreshold);
        if (r.contains("enableHorizon"))   c.enableHorizon   = r["enableHorizon"].toBool(c.enableHorizon);
        if (r.contains("legacyGenericDetection"))
            c.legacyGenericDetection = r["legacyGenericDetection"].toBool(c.legacyGenericDetection);

        if (c.isValid()) { rx = c; resetSpectrum(); }
        else qWarning() << "ESM: invalid receiver configuration rejected, keeping previous values";
    }
    if (obj.contains("AdditionalParameters"))
        AdditionalParameters = obj["AdditionalParameters"].toObject();
}