#include "disdatumregistry.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/EntityProfiles/specialzone.h"
#include "core/Hierarchy/hierarchy.h"
#include <QDataStream>
#include <algorithm>

bool DISDatumRegistry::getFixed(uint32_t datumID, Platform* platform,
                                uint32_t currentSimTimeSeconds, uint32_t& outValue)
{
    if (!platform) return false;

    switch (datumID) {
    case DISDatumID::WeaponSystemStatus: {
        uint32_t armedCount = 0;
        if (platform->weapons.weapons) {
            for (const auto& [id, weapon] : *platform->weapons.weapons) {
                if (weapon && weapon->armed) armedCount++;
            }
        }
        outValue = armedCount;
        return true;
    }
    case DISDatumID::EngagementData: {
        outValue = static_cast<uint32_t>(platform->roi);
        return true;
    }
    case DISDatumID::TimeSynchronization: {
        // Sourced from the caller — DISNetworkPlugin.cpp already legitimately
        // depends on Simulation; this file intentionally does not.
        outValue = currentSimTimeSeconds;
        return true;
    }
    default:
        return false;
    }
}

bool DISDatumRegistry::setFixed(uint32_t datumID, Platform* platform, uint32_t value)
{
    if (!platform) return false;

    switch (datumID) {
    case DISDatumID::EngagementData: {
        // Bounds-check against the real enum range (7 values, 0-6 —
        // Platform::ROI in platform.h). Reject out-of-range rather than
        // silently truncating into an unintended enum value.
        if (value > static_cast<uint32_t>(Platform::COMMAND_AUTHORIZATION_REQUIRED))
            return false;
        platform->roi = static_cast<Platform::ROI>(value);
        return true;
    }
    default:
        return false; // read-only or unknown — value left unchanged
    }
}

bool DISDatumRegistry::getVariable(uint32_t datumID, Platform* platform,
                                   Hierarchy* hierarchy, QByteArray& outData)
{
    if (!platform) return false;

    switch (datumID) {
    case DISDatumID::EntityTrackIdentity: {
        outData = QByteArray::fromStdString(platform->Name);
        return true;
    }
    case DISDatumID::EnvironmentalData: {
        if (!hierarchy) return false;

        // Same 4 atmosphere fields already synced to AESARadar in
        // Specialzone::Update() (REQ-AESA-071) — reusing those exact
        // fields, formula, and clear-air defaults rather than inventing
        // a separate environmental model, so the DIS answer matches
        // what the engine's own sensors already experience locally.
        float temperatureC   = 30.0f;
        float humidityPct    = 30.0f;
        float rainRateMmph   = 0.0f;
        float fogVisibilityM = 0.0f;

        for (const auto& [key, zone] : hierarchy->Specialzones) {
            if (!zone) continue;
            float distance = zone->transform.matrix.translation()
                                 .distanceToPoint(platform->transform.matrix.translation()) * 1000.0f;
            float alt = platform->dynamicModel.currentAltitude * 1.0f; // matches Specialzone's own check
            if (distance < zone->collider.CollideRadius &&
                alt > (zone->MinAltitude - 100.0f) &&
                alt < zone->MaxAltitude) {
                temperatureC = zone->Temprature;
                humidityPct  = zone->humidity;
                rainRateMmph = zone->rain;
                // Verbatim copy of Specialzone's own fog->visibility formula,
                // including its known quirk at fog=100 — intentional, see
                // session notes: never silently "fix" it here.
                fogVisibilityM = (zone->fog > 0.0f)
                                     ? std::clamp((1.0f - zone->fog / 100.0f) * 10000.0f, 0.0f, 10000.0f)
                                     : 0.0f;
                break; // first matching zone wins, same as Specialzone's own per-frame order
            }
        }

        // Documented byte layout for any receiving developer:
        // 4 x 32-bit float, in order: temperature_C, humidity_pct,
        // rainRate_mmph, fogVisibility_m. Written via QDataStream default
        // (big-endian), matching network byte order used elsewhere.
        QDataStream ds(&outData, QIODevice::WriteOnly);
        ds << temperatureC << humidityPct << rainRateMmph << fogVisibilityM;
        return true;
    }
    default:
        return false;
    }
}
