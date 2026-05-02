/* ========================================================================= */
/* File: WeaponTypes/missile.cpp                                             */
/* Purpose: Full flight system — exact logic from original weapon.cpp        */
/* ========================================================================= */

#include "missile.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/GlobalRegistry.h>
#include <core/Debug/console.h>
#include <cmath>
#include <QtMath>
#include <QVector3D>
#include <QDebug>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Missile::Missile(Hierarchy* h) : Weapon(h)
{
    weaponType     = WeaponType::Missile;
    designation    = "AAM";
    length         = 3.65f;
    diameter       = 0.178f;
    totalMass      = 152.0f;
    payloadMass    = 22.7f;
    fuelMass       = 40.0f;
    maxVelocity    = 1360.0f;
    maxRange       = 120000.0f;
    detonationType = DetonationType::Proximity;
    isActive       = false;
}

// launch() — called by platform Decision() to arm + start flight monitor
void Missile::launch()
{
    startFlightMonitor();
}

void Missile::Update(){

}


void Missile::updateGuidance() {}

bool Missile::canEngage(Platform *target)
{
    if (!target) return false;
    return true;
}

// =============================================================================
// 2. checkFlightState()
// Runs every 100 ms. Waits for Active==true, then calls missileStart() once.
// Exact copy of original Weapon::checkFlightState()
// =============================================================================
void Missile::checkFlightState()
{
    if (m_flightActive) {
        m_flightCheckTimer->stop();
        return;
    }
    if (!Active) return;

    Console::log("[Missile] Flight state detected ACTIVE, launching: " + Name);
    m_flightCheckTimer->stop();
    missileStart();
}

// =============================================================================
// 1. missileStart()
// Armed + activated, position snapped to parent, heading computed, timer started.
// Exact copy of original Weapon::missileStart()
// =============================================================================
void Missile::missileStart()
{
    if (m_flightActive) return;

    safed               = false;
    armed               = true;
    isActive            = true;
    Active              = true;   // CRITICAL — canvas renders entity only when Active==true
    m_flightActive      = true;
    m_updateRunning     = true;
    m_distanceTravelled = 0.0f;
    m_flightTime        = 0.0f;
    isLaunched          = true;

    // ── Snap to parent platform position at launch ────────────────────────────
    if (parentEntity) {
        Platform* parentPlatform = dynamic_cast<Platform*>(parentEntity);
        if (parentPlatform && parentPlatform->transform && parentPlatform->transform->geocord
            && transform && transform->geocord) {
            double pLat = parentPlatform->transform->geocord->latitude;
            double pLon = parentPlatform->transform->geocord->longitude;
            double pAlt = parentPlatform->transform->geocord->altitude;
            if (pLat >= -90.0 && pLat <= 90.0 && pLon >= -180.0 && pLon <= 360.0) {
                transform->setGeoCord(
                    static_cast<float>(pLat),
                    static_cast<float>(pLon),
                    static_cast<float>(pAlt),
                    0.0f);
                Console::log("[Missile] Position from parent: lat="
                             + std::to_string(pLat) + " lon=" + std::to_string(pLon));
            }
        }
    }

    // ── Fallback: invalid coords → reset to near target ───────────────────────
    if (transform && transform->geocord) {
        double ownLat = transform->geocord->latitude;
        double ownLon = transform->geocord->longitude;
        if (ownLat < -90.0 || ownLat > 90.0 || ownLon < -180.0 || ownLon > 360.0) {
            Console::log("[Missile] WARNING: invalid coords, resetting near target");
            transform->setGeoCord(
                static_cast<float>(m_targetLat - 1.0),
                static_cast<float>(m_targetLon),
                0.0f, 0.0f);
        }
    }

    // ── Compute great-circle bearing to target ────────────────────────────────
    float parentHeading = 0.0f;
    if (parentEntity) {
        Platform* pp = dynamic_cast<Platform*>(parentEntity);
        if (pp && pp->transform) parentHeading = pp->transform->getHeading();
    } else if (transform) {
        parentHeading = transform->getHeading();
    }

    double launchLat = 0.0, launchLon = 0.0;
    if (transform && transform->geocord) {
        launchLat = static_cast<double>(transform->geocord->latitude);
        launchLon = static_cast<double>(transform->geocord->longitude);
    }
    {
        double lat1 = qDegreesToRadians(launchLat);
        double lat2 = qDegreesToRadians(m_targetLat);
        double dLon = qDegreesToRadians(m_targetLon - launchLon);
        double y    = std::sin(dLon) * std::cos(lat2);
        double x    = std::cos(lat1) * std::sin(lat2)
                   - std::sin(lat1) * std::cos(lat2) * std::cos(dLon);
        m_launchHeading = static_cast<float>(
            std::fmod(qRadiansToDegrees(std::atan2(y, x)) + 360.0, 360.0));
    }
    if (transform) transform->setHeading(m_launchHeading);

    Console::log("[Missile] LAUNCHED: " + Name
                 + "  parentHeading=" + std::to_string((int)parentHeading)
                 + "  targetBearing=" + std::to_string((int)m_launchHeading)
                 + "  targetLat=" + std::to_string(m_targetLat)
                 + "  targetLon=" + std::to_string(m_targetLon)
                 + "  range=" + std::to_string((int)maxRange) + " m");

    if (m_target && m_targetplatform)
        m_targetplatform->isVictom = true;

    // ── Start 50 ms update timer (20 Hz) ─────────────────────────────────────
    if (!m_updateTimer) {
        m_updateTimer = new QTimer(this);
        m_updateTimer->setInterval(50);
        connect(m_updateTimer, &QTimer::timeout, this, [this]() {
            if (!m_updateRunning) { m_updateTimer->stop(); return; }
            float dt = static_cast<float>(m_dtClock.restart()) / 1000.0f;
            dt = qBound(0.001f, dt, 0.2f);
            missileUpdate(dt);
        });
    }
    m_dtClock.start();
    m_updateTimer->start();
}

// =============================================================================
// 3. missileUpdate(deltaTime)
// Great-circle movement each tick. Exact copy of original Weapon::missileUpdate()
// =============================================================================
void Missile::missileUpdate(float deltaTime)
{
    if (!m_updateRunning || !transform || !transform->geocord) return;

    if (m_target) {
        m_targetLat = m_target->getLatitude();
        m_targetLon = m_target->getLongitude();
    }

    float distThisTick   = maxVelocity * deltaTime;
    m_distanceTravelled += distThisTick;
    m_flightTime        += deltaTime;

    double curLat = static_cast<double>(transform->geocord->latitude);
    double curLon = static_cast<double>(transform->geocord->longitude);

    // Recalculate bearing each tick (proportional navigation)
    {
        double lat1 = qDegreesToRadians(curLat);
        double lat2 = qDegreesToRadians(m_targetLat);
        double dLon = qDegreesToRadians(m_targetLon - curLon);
        double y    = std::sin(dLon) * std::cos(lat2);
        double x    = std::cos(lat1) * std::sin(lat2)
                   - std::sin(lat1) * std::cos(lat2) * std::cos(dLon);
        m_launchHeading = static_cast<float>(
            std::fmod(qRadiansToDegrees(std::atan2(y, x)) + 360.0, 360.0));
    }
    transform->setHeading(m_launchHeading);

    double distKm = static_cast<double>(distThisTick) / 1000.0;

    // setGeoCord internally calls geoToFlatXYZ — this is what moves the sprite on canvas
    auto [newLat, newLon] = calculateNewLatLong(curLat, curLon, m_launchHeading, distKm);
    transform->setGeoCord(
        static_cast<float>(newLat),
        static_cast<float>(newLon),
        transform->geocord->altitude,
        m_launchHeading);

    // Haversine distance to target
    double dLatR = qDegreesToRadians(m_targetLat - newLat);
    double dLonR = qDegreesToRadians(m_targetLon  - newLon);
    double a     = std::sin(dLatR/2)*std::sin(dLatR/2)
               + std::cos(qDegreesToRadians(newLat))
                     * std::cos(qDegreesToRadians(m_targetLat))
                     * std::sin(dLonR/2)*std::sin(dLonR/2);
    double distToTarget = 6371000.0 * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0-a));

    // Log every ~5 s
    if (static_cast<int>(m_flightTime * 1000) % 5000 < 55) {
        Console::log("[Missile] " + Name
                     + "  travelled=" + std::to_string((int)m_distanceTravelled) + " m"
                     + "  to_target=" + std::to_string((int)distToTarget) + " m"
                     + "  lat=" + std::to_string(newLat)
                     + "  lon=" + std::to_string(newLon)
                     + "  hdg=" + std::to_string((int)m_launchHeading));
    }

    if (distToTarget <= static_cast<double>(m_detonationRange)) {
        Console::log("[Missile] DETONATION " + Name
                     + " dist=" + std::to_string((int)distToTarget) + " m");
        missileEnd();
    }
}

// =============================================================================
// 4. calcTargetVector()
// =============================================================================
QVector3D Missile::calcTargetVector() const
{
    return QVector3D(m_launchHeading, 0.0f, 0.0f);
}

// =============================================================================
// 5. missileEnd() — exact copy of original Weapon::missileEnd()
// =============================================================================
void Missile::missileEnd()
{
    if (!m_updateRunning && !m_flightActive) return;

    m_updateRunning = false;
    m_flightActive  = false;
    if (m_updateTimer)      m_updateTimer->stop();
    if (m_flightCheckTimer) m_flightCheckTimer->stop();

    isActive = false;
    Active   = false;
    armed    = false;

    double detLat = 0.0, detLon = 0.0, detAlt = 0.0;
    if (transform && transform->geocord) {
        detLat = static_cast<double>(transform->geocord->latitude);
        detLon = static_cast<double>(transform->geocord->longitude);
        detAlt = static_cast<double>(transform->geocord->altitude);
    }

    Console::log("[Missile] DETONATED: " + Name
                 + "  lat=" + std::to_string(detLat)
                 + "  lon=" + std::to_string(detLon));

    double blastLat = m_targetLat;
    double blastLon = m_targetLon;

    emit missileDetonated(QString::fromStdString(ID), blastLat, blastLon, detAlt);
    isDead = true;

    if (m_target && m_targetplatform) {
        m_targetplatform->Health  -= 40;
        m_targetplatform->isVictom = false;
    }

    if(parentEntity){
        parentEntity->hitcount+=1;
    }

    if (m_canvas) {
        QTimer::singleShot(0, m_canvas, [this, blastLat, blastLon]() {
            if (!m_canvas) return;

            static int blastCounter = 0;
            MeshEntry blastEntry;
            blastEntry.name         = QString("Blast_%1_%2")
                                  .arg(QString::fromStdString(Name))
                                  .arg(blastCounter++);
            blastEntry.position     = new QVector3D(
                static_cast<float>(blastLon),
                static_cast<float>(blastLat),
                0.0f);
            blastEntry.rotation     = new QQuaternion();
            float sz                = 0.01f;
            blastEntry.size         = new QVector3D(sz, sz, 1.0f);
            blastEntry.velocity     = new QVector3D(0, 0, 0);
            blastEntry.trajectory   = nullptr;
            blastEntry.collider     = nullptr;
            blastEntry.entity       = nullptr;
            blastEntry.platform     = nullptr;
            blastEntry.dynamicModel = nullptr;
            blastEntry.mesh         = new Mesh();
            blastEntry.mesh->color     = new QColor(Qt::white);
            blastEntry.mesh->lineWidth = 1;
            blastEntry.mesh->closePath = false;
            blastEntry.bitmapPath      = ":/texture/images/Texture/blast.png";

            m_canvas->tempMeshes.push_back(blastEntry);
            m_canvas->update();

            Console::log("[Missile] Blast at TARGET lat=" + std::to_string(blastLat)
                         + " lon=" + std::to_string(blastLon));

            QTimer::singleShot(5000, m_canvas, [this]() {
                if (!m_canvas) return;
                QString prefix = QString("Blast_%1_").arg(QString::fromStdString(Name));
                m_canvas->tempMeshes.erase(
                    std::remove_if(
                        m_canvas->tempMeshes.begin(),
                        m_canvas->tempMeshes.end(),
                        [&prefix](const MeshEntry& e) {
                            return e.name.startsWith(prefix);
                        }),
                    m_canvas->tempMeshes.end());
                m_canvas->update();
            });
        });
    }
}

// =============================================================================
// Enum helpers
// =============================================================================
QString Missile::guidanceTypeToString() const {
    switch (guidanceType) {
    case GuidanceType::Unguided:         return "Unguided";
    case GuidanceType::SemiActive:       return "SemiActive";
    case GuidanceType::FullyActive:      return "FullyActive";
    case GuidanceType::PassiveInfrared:  return "PassiveInfrared";
    case GuidanceType::CommandGuided:    return "CommandGuided";
    case GuidanceType::InertialGuidance: return "InertialGuidance";
    default:                             return "FullyActive";
    }
}
void Missile::setGuidanceTypeFromString(const QString& s) {
    if      (s == "Unguided")         guidanceType = GuidanceType::Unguided;
    else if (s == "SemiActive")       guidanceType = GuidanceType::SemiActive;
    else if (s == "FullyActive")      guidanceType = GuidanceType::FullyActive;
    else if (s == "PassiveInfrared")  guidanceType = GuidanceType::PassiveInfrared;
    else if (s == "CommandGuided")    guidanceType = GuidanceType::CommandGuided;
    else if (s == "InertialGuidance") guidanceType = GuidanceType::InertialGuidance;
}
QString Missile::propulsionTypeToString() const {
    switch (propulsionType) {
    case PropulsionType::SolidRocket:  return "SolidRocket";
    case PropulsionType::LiquidRocket: return "LiquidRocket";
    case PropulsionType::Turbofan:     return "Turbofan";
    case PropulsionType::Ramjet:       return "Ramjet";
    case PropulsionType::Turboprop:    return "Turboprop";
    case PropulsionType::Gravity:      return "Gravity";
    default:                           return "SolidRocket";
    }
}
void Missile::setPropulsionTypeFromString(const QString& s) {
    if      (s == "LiquidRocket") propulsionType = PropulsionType::LiquidRocket;
    else if (s == "Turbofan")     propulsionType = PropulsionType::Turbofan;
    else if (s == "Ramjet")       propulsionType = PropulsionType::Ramjet;
    else if (s == "Turboprop")    propulsionType = PropulsionType::Turboprop;
    else if (s == "Gravity")      propulsionType = PropulsionType::Gravity;
    else                           propulsionType = PropulsionType::SolidRocket;
}

QJsonObject Missile::toJson() const
{
    QJsonObject obj = Weapon::toJson();
    obj["weaponTypeName"]     = "Missile";
    obj["propulsionType"]     = propulsionTypeToString();
    obj["thrustMain"]         = thrustMain;
    obj["thrustBooster"]      = thrustBooster;
    obj["burnTime"]           = burnTime;
    obj["specificImpulse"]    = specificImpulse;
    obj["guidanceType"]       = guidanceTypeToString();
    obj["seekerRange"]        = seekerRange;
    obj["seekerFOV"]          = seekerFOV;
    obj["lockOnRange"]        = lockOnRange;
    obj["seekerTrackingRate"] = seekerTrackingRate;
    obj["seekerLockAccuracy"] = seekerLockAccuracy;
    obj["isLocked"]           = isLocked;
    obj["isActive"]           = isActive;
    return obj;
}

void Missile::fromJson(const QJsonObject& obj)
{
    Weapon::fromJson(obj);
    setPropulsionTypeFromString(obj.value("propulsionType").toString());
    thrustMain         = obj.value("thrustMain").toDouble(thrustMain);
    thrustBooster      = obj.value("thrustBooster").toDouble(thrustBooster);
    burnTime           = obj.value("burnTime").toDouble(burnTime);
    specificImpulse    = obj.value("specificImpulse").toDouble(specificImpulse);
    setGuidanceTypeFromString(obj.value("guidanceType").toString());
    seekerRange        = obj.value("seekerRange").toDouble(seekerRange);
    seekerFOV          = obj.value("seekerFOV").toDouble(seekerFOV);
    lockOnRange        = obj.value("lockOnRange").toDouble(lockOnRange);
    seekerTrackingRate = obj.value("seekerTrackingRate").toDouble(seekerTrackingRate);
    seekerLockAccuracy = obj.value("seekerLockAccuracy").toDouble(seekerLockAccuracy);
    isLocked           = obj.value("isLocked").toBool(isLocked);
    isActive           = obj.value("isActive").toBool(isActive);
}
