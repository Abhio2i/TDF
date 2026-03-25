// /*
//    bomb.cpp

//    Call chain:
//      Platform::update()
//        └─ alt >= 300ft → Platform::launchBombs()
//              └─ Bomb::launch()
//                    └─ Bomb::bombStart()
//                          └─ Weapon::startFlightMonitor()   [QTimer 100ms]
//                                └─ Bomb::checkFlightState() [100ms tick]
//                                      └─ Bomb::bombUpdate(dt)
//                                            └─ Bomb::checkDetonation()
//                                                  └─ Bomb::bombEnd()    [on impact]
// */

// #include "bomb.h"
// #include "core/Hierarchy/EntityProfiles/platform.h"
// #include <core/Hierarchy/hierarchy.h>
// #include <core/GlobalRegistry.h>
// #include <core/Debug/console.h>
// #include <QtMath>
// #include <QDebug>
// #include <QTimer>

// static constexpr double EARTH_RADIUS_M = 6371000.0;
// static constexpr float  GRAVITY_MS2    = 9.81f;

// // =============================================================================
// // Bomb()
// // =============================================================================
// Bomb::Bomb(Hierarchy* h) : Weapon(h)
// {
//     weaponType     = WeaponType::Bomb;
//     designation    = "GP-Bomb";
//     length         = 2.21f;
//     diameter       = 0.273f;
//     totalMass      = 227.0f;
//     payloadMass    = 87.0f;
//     fuelMass       = 0.0f;
//     maxVelocity    = 350.0f;
//     maxRange       = 15000.0f;
//     detonationType = DetonationType::Impact;

//     // ── Auto-attach bomb sprite ───────────────────────────────────────────────
//     // Weapon base constructor already called addComponent("bitmap") which
//     // created meshRenderer2d with a default AUV sprite. We replace it here
//     // so the user sees the correct bomb icon as soon as the weapon is added.
//     if (meshRenderer2d && meshRenderer2d->Sprite) {
//         meshRenderer2d->Sprite->clear();
//         meshRenderer2d->Sprite->append(":/texture/images/Texture/bomb.png");
//     }
// }

// // =============================================================================
// // launch()
// // Called by Platform::launchBombs() when aircraft crosses 300 ft (91.44 m).
// // Copies aircraft geocoord, inherits forward speed, then calls bombStart().
// // =============================================================================
// void Bomb::launch()
// {
//     if (isLaunched) return;

//     Platform* aircraft = dynamic_cast<Platform*>(parentEntity);
//     if (!aircraft || !aircraft->transform || !aircraft->transform->geocord) {
//         Console::error("Bomb::launch() — no parent aircraft or transform: " + Name);
//         return;
//     }
//     if (!transform || !transform->geocord) {
//         Console::error("Bomb::launch() — bomb has no transform: " + Name);
//         return;
//     }

//     // Copy aircraft position exactly — bomb starts at aircraft location
//     transform->geocord->latitude  = aircraft->transform->geocord->latitude;
//     transform->geocord->longitude = aircraft->transform->geocord->longitude;
//     transform->geocord->altitude  = aircraft->transform->geocord->altitude;
//     transform->geocord->Heading   = aircraft->transform->geocord->Heading;

//     releaseAltitude = static_cast<float>(aircraft->transform->geocord->altitude);

//     // Inherit aircraft forward speed (DynamicModel stores km/h)
//     float speedMs = 200.0f;
//     if (aircraft->dynamicModel)
//         speedMs = aircraft->dynamicModel->moveSpeed / 3.6f;

//     m_horizontalVelocity = speedMs;
//     m_verticalVelocity   = 0.0f;
//     m_fuzeTimer          = 0.0f;
//     m_airborneTime       = 0.0f;

//     safed      = false;
//     armed      = true;
//     isLaunched = true;

//     Console::log("Bomb::launch() — released: " + Name +
//                  "  alt=" + std::to_string(static_cast<int>(releaseAltitude)) + "m" +
//                  "  speed=" + std::to_string(static_cast<int>(speedMs)) + "m/s");

//     Hierarchy* par = GlobalRegistry::getParentHierarchy(this);
//     if (par) {
//         emit par->bombLaunched(
//             QString::fromStdString(ID),
//             QString::fromStdString(aircraft->ID),
//             transform->geocord->latitude,
//             transform->geocord->longitude,
//             static_cast<double>(transform->geocord->altitude)
//             );
//     }

//     bombStart();
// }

// // =============================================================================
// // bombStart()
// // Starts the 100ms flight-monitor. The Weapon base fires QTimer(100ms)
// // which calls checkFlightState() → bombUpdate(dt).
// // NO missileStart() is called anywhere on Bomb.
// // =============================================================================
// void Bomb::bombStart()
// {
//     if (!isLaunched) return;
//     m_flightActive = true;
//     m_dtClock.start();
//     startFlightMonitor();
//     Console::log("Bomb::bombStart() — flight monitor started: " + Name);
// }

// // =============================================================================
// // checkFlightState()  — the ONLY Weapon virtual override on Bomb
// // Redirects the 100ms timer tick to bombUpdate(dt).
// // =============================================================================
// void Bomb::checkFlightState()
// {
//     if (!m_flightActive || !isLaunched || isDead) return;

//     float dt = static_cast<float>(m_dtClock.elapsed()) / 1000.0f;
//     m_dtClock.restart();
//     if (dt <= 0.0f || dt > 0.5f) dt = 0.1f;

//     bombUpdate(dt);
// }

// // =============================================================================
// // bombUpdate(float dt)
// // Ballistic physics every 100ms.
// //   Vertical  : v_y -= 9.81 * dt   (gravity)
// //               alt += v_y * dt
// //   Horizontal: v_h *= (1 - drag*dt)
// //               geocoord advances along heading by (v_h * dt) metres
// // =============================================================================
// void Bomb::bombUpdate(float dt)
// {
//     if (!transform || !transform->geocord || isDead) return;

//     m_airborneTime += dt;

//     // ── Vertical ─────────────────────────────────────────────────────────────
//     m_verticalVelocity -= GRAVITY_MS2 * dt;
//     double newAlt = transform->geocord->altitude +
//                     static_cast<double>(m_verticalVelocity * dt);

//     // ── Horizontal ───────────────────────────────────────────────────────────
//     m_horizontalVelocity *= (1.0f - dragCoefficient * dt);
//     float  horizDist  = m_horizontalVelocity * dt;
//     double headingRad = qDegreesToRadians(transform->geocord->Heading);
//     double latRad     = qDegreesToRadians(transform->geocord->latitude);
//     double dLat = (horizDist * qCos(headingRad)) / EARTH_RADIUS_M;
//     double dLon = (horizDist * qSin(headingRad)) / (EARTH_RADIUS_M * qCos(latRad));

//     transform->geocord->latitude  += qRadiansToDegrees(dLat);
//     transform->geocord->longitude += qRadiansToDegrees(dLon);
//     transform->geocord->altitude   = (newAlt < 0.0) ? 0.0 : newAlt;

//     m_fuzeTimer += dt;

//     Hierarchy* par = GlobalRegistry::getParentHierarchy(this);
//     if (par) emit par->entityUpdate(QString::fromStdString(ID));

//     qDebug() << "[Bomb]" << QString::fromStdString(Name)
//              << "t=" << m_airborneTime << "s"
//              << "alt=" << transform->geocord->altitude << "m"
//              << "vVel=" << m_verticalVelocity << "m/s";

//     checkDetonation();
// }

// // =============================================================================
// // checkDetonation()
// // Fuse logic at end of every bombUpdate().
// // m_airborneTime guard prevents instant detonation when aircraft spawns at alt=0.
// // =============================================================================
// void Bomb::checkDetonation()
// {
//     if (isDead || !transform || !transform->geocord) return;

//     // Must be airborne ≥ 0.5 s before any detonation check
//     if (m_airborneTime < 0.5f) return;

//     double  alt            = transform->geocord->altitude;
//     bool    shouldDetonate = false;
//     QString reason;

//     switch (detonationType) {
//     case DetonationType::Impact:
//         if (alt <= 0.0) {
//             if (hasDelayFuze && m_fuzeTimer < fuzeDelaySeconds) break;
//             shouldDetonate = true;
//             reason = "Impact (ground)";
//         }
//         break;
//     case DetonationType::Timed:
//         if (m_fuzeTimer >= fuzeDelaySeconds) {
//             shouldDetonate = true;
//             reason = QString("Timed %1s").arg(fuzeDelaySeconds);
//         }
//         break;
//     case DetonationType::Proximity:
//         if (alt <= static_cast<double>(proximityRange)) {
//             shouldDetonate = true;
//             reason = QString("Proximity alt=%1m").arg(alt);
//         }
//         break;
//     default:
//         if (alt <= 0.0) { shouldDetonate = true; reason = "Ground"; }
//         break;
//     }

//     if (!shouldDetonate && m_fuzeTimer > flightTimeMax) {
//         shouldDetonate = true; reason = "Max flight time";
//     }

//     if (shouldDetonate) {
//         Console::log("Bomb::checkDetonation() — DETONATED [" +
//                      reason.toStdString() + "]: " + Name);
//         bombEnd();
//     }
// }

// // =============================================================================
// // bombEnd()
// // Impact handler:
// //   1. Stop flight monitor.
// //   2. Swap bomb sprite → blast image at exact impact location.
// //   3. Emit bombDetonated(id, lat, lon, alt) signal.
// //   4. After BLAST_DISPLAY_MS (1500ms), hide the entity.
// // =============================================================================
// void Bomb::bombEnd()
// {
//     if (isDead) return;

//     isDead         = true;
//     m_flightActive = false;
//     stopFlightMonitor();

//     if (transform && transform->geocord)
//         transform->geocord->altitude = 0.0;

//     Hierarchy* par = GlobalRegistry::getParentHierarchy(this);

//     // ── Swap bomb icon → blast image ──────────────────────────────────────────
//     if (meshRenderer2d && meshRenderer2d->Sprite && par) {

//         // 1. Remove the old bomb icon so canvas stops drawing it
//         emit par->entityMeshRemoved(QString::fromStdString(ID));

//         // 2. Swap sprite — Sprite is QStringList*, use clear()+append(), NOT assign()
//         meshRenderer2d->Sprite->clear();
//         meshRenderer2d->Sprite->append(":/texture/images/Texture/blast.png");

//         // 3. Force canvas to reload and draw the new blast image.
//         //    entityUpdate alone is NOT enough — canvas only re-renders
//         //    the sprite when meshRenderer2DisAdded is emitted.
//         emit par->meshRenderer2DisAdded(QString::fromStdString(ID), meshRenderer2d);
//         emit par->entityMeshAdded(QString::fromStdString(parentID), this);
//         emit par->entityUpdate(QString::fromStdString(ID));

//         // 4. After 1500ms hide the blast
//         m_blastTimer = new QTimer(this);
//         m_blastTimer->setSingleShot(true);
//         connect(m_blastTimer, &QTimer::timeout, this, [this]() {
//             Active = false;
//             Hierarchy* p = GlobalRegistry::getParentHierarchy(this);
//             if (p) {
//                 emit p->entityMeshRemoved(QString::fromStdString(ID));
//                 emit p->entityUpdate(QString::fromStdString(ID));
//             }
//             Console::log("Bomb::bombEnd() — blast cleared: " + Name);
//         });
//         m_blastTimer->start(BLAST_DISPLAY_MS);
//     }

//     // ── Emit detonation signal ────────────────────────────────────────────────
//     if (par) {
//         emit par->bombDetonated(
//             QString::fromStdString(ID),
//             transform ? transform->geocord->latitude  : 0.0,
//             transform ? transform->geocord->longitude : 0.0,
//             transform ? transform->geocord->altitude  : 0.0
//             );
//     }

//     Console::log("Bomb::bombEnd() — detonated: " + Name);
// }

// // =============================================================================
// // flyToTarget — no-op for unguided bomb
// // =============================================================================
// void Bomb::flyToTarget() {}

// // =============================================================================
// // Enum helpers
// // =============================================================================
// QString Bomb::releaseModeToString() const {
//     switch (releaseMode) {
//     case ReleaseMode::CCIP:    return "CCIP";
//     case ReleaseMode::CCRP:    return "CCRP";
//     case ReleaseMode::Manual:  return "Manual";
//     case ReleaseMode::Lofting: return "Lofting";
//     default:                   return "CCIP";
//     }
// }
// void Bomb::setReleaseModeFromString(const QString& s) {
//     if      (s == "CCRP")    releaseMode = ReleaseMode::CCRP;
//     else if (s == "Manual")  releaseMode = ReleaseMode::Manual;
//     else if (s == "Lofting") releaseMode = ReleaseMode::Lofting;
//     else                      releaseMode = ReleaseMode::CCIP;
// }
// QString Bomb::guidanceTypeToString() const {
//     return (guidanceType == GuidanceType::InertialGuidance) ? "InertialGuidance" : "Unguided";
// }
// void Bomb::setGuidanceTypeFromString(const QString& s) {
//     guidanceType = (s == "InertialGuidance") ? GuidanceType::InertialGuidance
//                                              : GuidanceType::Unguided;
// }

// // =============================================================================
// // toJson / fromJson
// // =============================================================================
// QJsonObject Bomb::toJson() const
// {
//     QJsonObject obj = Weapon::toJson();
//     obj["weaponTypeName"]   = "Bomb";
//     obj["guidanceType"]     = guidanceTypeToString();
//     obj["hasPrecisionKit"]  = hasPrecisionKit;
//     obj["cep"]              = static_cast<double>(cep);
//     obj["releaseMode"]      = releaseModeToString();
//     obj["dragCoefficient"]  = static_cast<double>(dragCoefficient);
//     obj["terminalVelocity"] = static_cast<double>(terminalVelocity);
//     obj["releaseAltitude"]  = static_cast<double>(releaseAltitude);
//     obj["hasDelayFuze"]     = hasDelayFuze;
//     obj["fuzeDelaySeconds"] = static_cast<double>(fuzeDelaySeconds);
//     return obj;
// }

// void Bomb::fromJson(const QJsonObject& obj)
// {
//     Weapon::fromJson(obj);
//     setGuidanceTypeFromString(obj.value("guidanceType").toString());
//     hasPrecisionKit  = obj.value("hasPrecisionKit").toBool(hasPrecisionKit);
//     cep              = static_cast<float>(obj.value("cep").toDouble(cep));
//     setReleaseModeFromString(obj.value("releaseMode").toString());
//     dragCoefficient  = static_cast<float>(obj.value("dragCoefficient").toDouble(dragCoefficient));
//     terminalVelocity = static_cast<float>(obj.value("terminalVelocity").toDouble(terminalVelocity));
//     releaseAltitude  = static_cast<float>(obj.value("releaseAltitude").toDouble(releaseAltitude));
//     hasDelayFuze     = obj.value("hasDelayFuze").toBool(hasDelayFuze);
//     fuzeDelaySeconds = static_cast<float>(obj.value("fuzeDelaySeconds").toDouble(fuzeDelaySeconds));
// }
/*
   bomb.cpp

   Call chain:
     Platform::update()
       └─ alt >= 1000ft → Platform::launchBombs()
             └─ Bomb::launch()
                   └─ Bomb::bombStart()
                         └─ Weapon::startFlightMonitor()   [QTimer 100ms]
                               └─ Bomb::checkFlightState() [100ms tick]
                                     └─ Bomb::bombUpdate(dt)
                                           └─ Bomb::checkDetonation()
                                                 └─ Bomb::bombEnd()    [on impact]
*/

#include "bomb.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/GlobalRegistry.h>
#include <core/Debug/console.h>
#include <QtMath>
#include <QDebug>
#include <QTimer>

static constexpr double EARTH_RADIUS_M = 6371000.0;
static constexpr float  GRAVITY_MS2    = 9.81f;

// =============================================================================
// Bomb()
// =============================================================================
Bomb::Bomb(Hierarchy* h) : Weapon(h)
{
    weaponType     = WeaponType::Bomb;
    designation    = "GP-Bomb";
    length         = 2.21f;
    diameter       = 0.273f;
    totalMass      = 227.0f;
    payloadMass    = 87.0f;
    fuelMass       = 0.0f;
    maxVelocity    = 350.0f;
    maxRange       = 15000.0f;
    detonationType = DetonationType::Impact;

    // ── Auto-attach bomb sprite ───────────────────────────────────────────────
    // Weapon base constructor already called addComponent("bitmap") which
    // created meshRenderer2d with a default AUV sprite. We replace it here
    // so the user sees the correct bomb icon as soon as the weapon is added.
    if (meshRenderer2d && meshRenderer2d->Sprite) {
        meshRenderer2d->Sprite->clear();
        meshRenderer2d->Sprite->append(":/texture/images/Texture/bomb.png");
    }
}

// =============================================================================
// launch()
// Called by Platform::launchBombs() when aircraft crosses 1000 ft (304.8 m).
// Copies aircraft geocoord (converts ft→m), inherits forward speed, then calls bombStart().
// =============================================================================
void Bomb::launch()
{
    if (isLaunched) return;

    Platform* aircraft = dynamic_cast<Platform*>(parentEntity);
    if (!aircraft || !aircraft->transform || !aircraft->transform->geocord) {
        Console::error("Bomb::launch() — no parent aircraft or transform: " + Name);
        return;
    }
    if (!transform || !transform->geocord) {
        Console::error("Bomb::launch() — bomb has no transform: " + Name);
        return;
    }

    // Copy aircraft position — bomb starts at aircraft location.
    // NOTE: aircraft geocord->altitude is in FEET (FTtoKM = 1/3281).
    //       bombUpdate() physics uses metres (gravity 9.81 m/s²), so
    //       convert feet → metres here for correct ballistic calculations.
    transform->geocord->latitude  = aircraft->transform->geocord->latitude;
    transform->geocord->longitude = aircraft->transform->geocord->longitude;
    transform->geocord->altitude  = aircraft->transform->geocord->altitude * 0.3048; // ft → m
    transform->geocord->Heading   = aircraft->transform->geocord->Heading;

    releaseAltitude = static_cast<float>(transform->geocord->altitude); // metres

    // Inherit aircraft forward speed (DynamicModel stores km/h)
    float speedMs = 200.0f;
    if (aircraft->dynamicModel)
        speedMs = aircraft->dynamicModel->moveSpeed / 3.6f;

    m_horizontalVelocity = speedMs;
    m_verticalVelocity   = 0.0f;
    m_fuzeTimer          = 0.0f;
    m_airborneTime       = 0.0f;

    safed      = false;
    armed      = true;
    isLaunched = true;

    Console::log("Bomb::launch() — released: " + Name +
                 "  alt=" + std::to_string(static_cast<int>(releaseAltitude)) + "m" +
                 "  speed=" + std::to_string(static_cast<int>(speedMs)) + "m/s");

    Hierarchy* par = GlobalRegistry::getParentHierarchy(this);
    if (par) {
        emit par->bombLaunched(
            QString::fromStdString(ID),
            QString::fromStdString(aircraft->ID),
            transform->geocord->latitude,
            transform->geocord->longitude,
            static_cast<double>(transform->geocord->altitude)
            );
    }

    bombStart();
}

// =============================================================================
// bombStart()
// Starts the 100ms flight-monitor. The Weapon base fires QTimer(100ms)
// which calls checkFlightState() → bombUpdate(dt).
// NO missileStart() is called anywhere on Bomb.
// =============================================================================
void Bomb::bombStart()
{
    if (!isLaunched) return;
    m_flightActive = true;
    m_dtClock.start();
    startFlightMonitor();
    Console::log("Bomb::bombStart() — flight monitor started: " + Name);
}

// =============================================================================
// checkFlightState()  — the ONLY Weapon virtual override on Bomb
// Redirects the 100ms timer tick to bombUpdate(dt).
// =============================================================================
void Bomb::checkFlightState()
{
    if (!m_flightActive || !isLaunched || isDead) return;

    float dt = static_cast<float>(m_dtClock.elapsed()) / 1000.0f;
    m_dtClock.restart();
    if (dt <= 0.0f || dt > 0.5f) dt = 0.1f;

    bombUpdate(dt);
}

// =============================================================================
// bombUpdate(float dt)
// Ballistic physics every 100ms.
//   Vertical  : v_y -= 9.81 * dt   (gravity)
//               alt += v_y * dt
//   Horizontal: v_h *= (1 - drag*dt)
//               geocoord advances along heading by (v_h * dt) metres
// =============================================================================
void Bomb::bombUpdate(float dt)
{
    if (!transform || !transform->geocord || isDead) return;

    m_airborneTime += dt;

    // ── Vertical ─────────────────────────────────────────────────────────────
    m_verticalVelocity -= GRAVITY_MS2 * dt;
    double newAlt = transform->geocord->altitude +
                    static_cast<double>(m_verticalVelocity * dt);

    // ── Horizontal ───────────────────────────────────────────────────────────
    m_horizontalVelocity *= (1.0f - dragCoefficient * dt);
    float  horizDist  = m_horizontalVelocity * dt;
    double headingRad = qDegreesToRadians(transform->geocord->Heading);
    double latRad     = qDegreesToRadians(transform->geocord->latitude);
    double dLat = (horizDist * qCos(headingRad)) / EARTH_RADIUS_M;
    double dLon = (horizDist * qSin(headingRad)) / (EARTH_RADIUS_M * qCos(latRad));

    transform->geocord->latitude  += qRadiansToDegrees(dLat);
    transform->geocord->longitude += qRadiansToDegrees(dLon);
    transform->geocord->altitude   = (newAlt < 0.0) ? 0.0 : newAlt;

    m_fuzeTimer += dt;

    Hierarchy* par = GlobalRegistry::getParentHierarchy(this);
    if (par) emit par->entityUpdate(QString::fromStdString(ID));

    qDebug() << "[Bomb]" << QString::fromStdString(Name)
             << "t=" << m_airborneTime << "s"
             << "alt=" << transform->geocord->altitude << "m"
             << "vVel=" << m_verticalVelocity << "m/s";

    checkDetonation();
}

// =============================================================================
// checkDetonation()
// Fuse logic at end of every bombUpdate().
// m_airborneTime guard prevents instant detonation when aircraft spawns at alt=0.
// =============================================================================
void Bomb::checkDetonation()
{
    if (isDead || !transform || !transform->geocord) return;

    // Must be airborne ≥ 0.5 s before any detonation check
    if (m_airborneTime < 0.5f) return;

    double  alt            = transform->geocord->altitude;
    bool    shouldDetonate = false;
    QString reason;

    switch (detonationType) {
    case DetonationType::Impact:
        if (alt <= 0.0) {
            if (hasDelayFuze && m_fuzeTimer < fuzeDelaySeconds) break;
            shouldDetonate = true;
            reason = "Impact (ground)";
        }
        break;
    case DetonationType::Timed:
        if (m_fuzeTimer >= fuzeDelaySeconds) {
            shouldDetonate = true;
            reason = QString("Timed %1s").arg(fuzeDelaySeconds);
        }
        break;
    case DetonationType::Proximity:
        if (alt <= static_cast<double>(proximityRange)) {
            shouldDetonate = true;
            reason = QString("Proximity alt=%1m").arg(alt);
        }
        break;
    default:
        if (alt <= 0.0) { shouldDetonate = true; reason = "Ground"; }
        break;
    }

    if (!shouldDetonate && m_fuzeTimer > flightTimeMax) {
        shouldDetonate = true; reason = "Max flight time";
    }

    if (shouldDetonate) {
        Console::log("Bomb::checkDetonation() — DETONATED [" +
                     reason.toStdString() + "]: " + Name);
        bombEnd();
    }
}

// =============================================================================
// bombEnd()
// Impact handler:
//   1. Stop flight monitor.
//   2. Swap bomb sprite → blast image at exact impact location.
//   3. Emit bombDetonated(id, lat, lon, alt) signal.
//   4. After BLAST_DISPLAY_MS (1500ms), hide the entity.
// =============================================================================
void Bomb::bombEnd()
{
    if (isDead) return;

    isDead         = true;
    m_flightActive = false;
    stopFlightMonitor();

    if (transform && transform->geocord)
        transform->geocord->altitude = 0.0;

    Hierarchy* par = GlobalRegistry::getParentHierarchy(this);

    // ── Swap bomb icon → blast image ──────────────────────────────────────────
    if (meshRenderer2d && meshRenderer2d->Sprite && par) {

        // 1. Remove the old bomb icon so canvas stops drawing it
        emit par->entityMeshRemoved(QString::fromStdString(ID));

        // 2. Swap sprite — Sprite is QStringList*, use clear()+append(), NOT assign()
        meshRenderer2d->Sprite->clear();
        meshRenderer2d->Sprite->append(":/texture/images/Texture/blast.png");

        // 3. Force canvas to reload and draw the new blast image.
        //    entityUpdate alone is NOT enough — canvas only re-renders
        //    the sprite when meshRenderer2DisAdded is emitted.
        emit par->meshRenderer2DisAdded(QString::fromStdString(ID), meshRenderer2d);
        emit par->entityMeshAdded(QString::fromStdString(parentID), this);
        emit par->entityUpdate(QString::fromStdString(ID));

        // 4. After 1500ms hide the blast
        m_blastTimer = new QTimer(this);
        m_blastTimer->setSingleShot(true);
        connect(m_blastTimer, &QTimer::timeout, this, [this]() {
            Active = false;
            Hierarchy* p = GlobalRegistry::getParentHierarchy(this);
            if (p) {
                emit p->entityMeshRemoved(QString::fromStdString(ID));
                emit p->entityUpdate(QString::fromStdString(ID));
            }
            Console::log("Bomb::bombEnd() — blast cleared: " + Name);
        });
        m_blastTimer->start(BLAST_DISPLAY_MS);
    }

    // ── Emit detonation signal ────────────────────────────────────────────────
    if (par) {
        emit par->bombDetonated(
            QString::fromStdString(ID),
            transform ? transform->geocord->latitude  : 0.0,
            transform ? transform->geocord->longitude : 0.0,
            transform ? transform->geocord->altitude  : 0.0
            );
    }

    Console::log("Bomb::bombEnd() — detonated: " + Name);
}

// =============================================================================
// flyToTarget — no-op for unguided bomb
// =============================================================================
void Bomb::flyToTarget() {}

// =============================================================================
// Enum helpers
// =============================================================================
QString Bomb::releaseModeToString() const {
    switch (releaseMode) {
    case ReleaseMode::CCIP:    return "CCIP";
    case ReleaseMode::CCRP:    return "CCRP";
    case ReleaseMode::Manual:  return "Manual";
    case ReleaseMode::Lofting: return "Lofting";
    default:                   return "CCIP";
    }
}
void Bomb::setReleaseModeFromString(const QString& s) {
    if      (s == "CCRP")    releaseMode = ReleaseMode::CCRP;
    else if (s == "Manual")  releaseMode = ReleaseMode::Manual;
    else if (s == "Lofting") releaseMode = ReleaseMode::Lofting;
    else                      releaseMode = ReleaseMode::CCIP;
}
QString Bomb::guidanceTypeToString() const {
    return (guidanceType == GuidanceType::InertialGuidance) ? "InertialGuidance" : "Unguided";
}
void Bomb::setGuidanceTypeFromString(const QString& s) {
    guidanceType = (s == "InertialGuidance") ? GuidanceType::InertialGuidance
                                             : GuidanceType::Unguided;
}

// =============================================================================
// toJson / fromJson
// =============================================================================
QJsonObject Bomb::toJson() const
{
    QJsonObject obj = Weapon::toJson();
    obj["weaponTypeName"]   = "Bomb";
    obj["guidanceType"]     = guidanceTypeToString();
    obj["hasPrecisionKit"]  = hasPrecisionKit;
    obj["cep"]              = static_cast<double>(cep);
    obj["releaseMode"]      = releaseModeToString();
    obj["dragCoefficient"]  = static_cast<double>(dragCoefficient);
    obj["terminalVelocity"] = static_cast<double>(terminalVelocity);
    obj["releaseAltitude"]  = static_cast<double>(releaseAltitude);
    obj["hasDelayFuze"]     = hasDelayFuze;
    obj["fuzeDelaySeconds"] = static_cast<double>(fuzeDelaySeconds);
    return obj;
}

void Bomb::fromJson(const QJsonObject& obj)
{
    Weapon::fromJson(obj);
    setGuidanceTypeFromString(obj.value("guidanceType").toString());
    hasPrecisionKit  = obj.value("hasPrecisionKit").toBool(hasPrecisionKit);
    cep              = static_cast<float>(obj.value("cep").toDouble(cep));
    setReleaseModeFromString(obj.value("releaseMode").toString());
    dragCoefficient  = static_cast<float>(obj.value("dragCoefficient").toDouble(dragCoefficient));
    terminalVelocity = static_cast<float>(obj.value("terminalVelocity").toDouble(terminalVelocity));
    releaseAltitude  = static_cast<float>(obj.value("releaseAltitude").toDouble(releaseAltitude));
    hasDelayFuze     = obj.value("hasDelayFuze").toBool(hasDelayFuze);
    fuzeDelaySeconds = static_cast<float>(obj.value("fuzeDelaySeconds").toDouble(fuzeDelaySeconds));
}
