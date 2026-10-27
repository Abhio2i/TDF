// =============================================================================
// FILE:         aesaexampleplugin.cpp
// MODULE:       AESA plugin SDK — example plugin
// DESCRIPTION:  See aesaexampleplugin.h.
// =============================================================================
#include "aesaexampleplugin.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>
#include <algorithm>

namespace
{
constexpr double kSmallRcs_m2      = 0.01;    // "rcs" mode mean RCS
constexpr double kFogLossFactor    = 0.01;    // "fog" mode: -20 dB two-way
constexpr double kFilterRange_km   = 50.0;    // "prefilter" / "filter" modes
constexpr qint64 kLogPeriod_ms     = 2000;
constexpr double kAzimuthLimit_deg = 30.0;    // "config" mode

// Key names of the azimuth limits in AESARadar::toJson() (see P2 PC2).
const char* const kMinAzimuthKey = "minAzimuth";
const char* const kMaxAzimuthKey = "maxAzimuth";

// Returns v with its number replaced by x. Handles a plain number and an
// object with a "value" field. Undefined = unknown format (do not patch).
QJsonValue withNumber(const QJsonValue& v, double x)
{
    if (v.isDouble()) return QJsonValue(x);
    if (v.isObject())
    {
        QJsonObject o = v.toObject();
        if (o.contains(QStringLiteral("value")))
        {
            o[QStringLiteral("value")] = x;
            return o;
        }
    }
    return QJsonValue(QJsonValue::Undefined);
}

// Builds a partial config that sets the azimuth limits to +/-limit_deg,
// keeping the section structure produced by toJson(). Empty = not possible.
QJsonObject makeAzimuthPatch(const QJsonObject& cfg, double limit_deg)
{
    for (auto it = cfg.begin(); it != cfg.end(); ++it)
    {
        if (!it.value().isObject()) continue;
        const QJsonObject section = it.value().toObject();
        if (!section.contains(QLatin1String(kMinAzimuthKey)) ||
            !section.contains(QLatin1String(kMaxAzimuthKey)))
            continue;

        const QJsonValue mn = withNumber(section.value(QLatin1String(kMinAzimuthKey)), -limit_deg);
        const QJsonValue mx = withNumber(section.value(QLatin1String(kMaxAzimuthKey)),  limit_deg);
        if (mn.isUndefined() || mx.isUndefined()) return QJsonObject();

        QJsonObject partialSection;
        partialSection[QLatin1String(kMinAzimuthKey)] = mn;
        partialSection[QLatin1String(kMaxAzimuthKey)] = mx;
        QJsonObject patch;
        patch[it.key()] = partialSection;
        return patch;
    }
    return QJsonObject();
}

QString radarLabel(const aesaplugin::RadarInfo& r)
{
    return QString::fromStdString(r.name) + " [" + QString::fromStdString(r.entityId) + "]";
}
} // namespace

bool AesaExamplePlugin::has(const char* mode) const
{
    return modes_.contains(QString::fromLatin1(mode));
}

void AesaExamplePlugin::init()
{
    const QByteArray env = qgetenv("AESA_EXAMPLE_MODE");
    const QString spec = env.isEmpty() ? QStringLiteral("log") : QString::fromLocal8Bit(env);
    for (const QString& m : spec.split(QLatin1Char(','), Qt::SkipEmptyParts))
        modes_.insert(m.trimmed().toLower());

    qInfo() << "[AesaExample] init, SDK version" << aesaplugin::API_VERSION
            << "modes =" << modes_.values();

    aesaplugin::LogicHooks hooks;

    if (has("rcs"))
    {
        hooks.rcs = [this](const aesaplugin::RcsQuery& q, double& meanRcs_m2)
        {
            (void)q;
            ++rcsCalls_;
            tryHostCallFromHook();
            meanRcs_m2 = kSmallRcs_m2;
            return true;
        };
    }
    if (has("blind"))
    {
        hooks.detection = [this](const aesaplugin::DetectionQuery& q, bool& detected)
        {
            (void)q;
            ++detectCalls_;
            tryHostCallFromHook();
            detected = false;
            return true;
        };
    }
    if (has("fog"))
    {
        hooks.propagation = [this](const aesaplugin::PropagationQuery& q, double& lossFactor)
        {
            (void)q;
            ++propCalls_;
            tryHostCallFromHook();
            lossFactor = kFogLossFactor;
            return true;
        };
    }
    if (has("prefilter"))
    {
        hooks.preFilter = [this](const aesaplugin::RadarInfo& r, const aesaplugin::TargetGeometry& g)
        {
            (void)r;
            ++preCalls_;
            tryHostCallFromHook();
            return g.range_m <= kFilterRange_km * 1000.0;
        };
    }
    if (has("filter"))
    {
        hooks.postFilter = [this](const aesaplugin::RadarInfo& r, const aesaplugin::OutputTarget& o)
        {
            (void)r;
            ++postCalls_;
            tryHostCallFromHook();
            return o.range_km <= kFilterRange_km;
        };
    }

    if (setLogicHooks)
    {
        const bool ok = setLogicHooks(hooks);
        qInfo() << "[AesaExample] setLogicHooks:" << (ok ? "accepted" : "REJECTED");
    }
    logTimer_.start();
}

// Demonstrates rule 2 of the SDK: host callbacks are refused inside hooks.
void AesaExamplePlugin::tryHostCallFromHook()
{
    if (hostCallTried_ || !listRadars) return;
    hostCallTried_ = true;
    const std::vector<aesaplugin::RadarInfo> fromHook = listRadars();
    qInfo() << "[AesaExample] host call from inside a hook returned"
            << static_cast<int>(fromHook.size())
            << "radars (expected 0: refused while a hook runs)";
}

void AesaExamplePlugin::update(float delta)
{
    (void)delta;
    if (!has("log") || !logTimer_.isValid() || logTimer_.elapsed() < kLogPeriod_ms) return;
    logTimer_.restart();

    for (const auto& r : radars_)
    {
        const aesaplugin::RadarStatus st =
            getRadarStatus ? getRadarStatus(r.radarId) : aesaplugin::RadarStatus{};
        qInfo().noquote() << "[AesaExample]" << radarLabel(r)
                          << "valid" << st.valid << "mode" << st.mode
                          << "beamAz" << st.beamAzimuth_deg
                          << "duty" << st.dutyCycle
                          << "range_km" << st.displayRange_km;
    }
    qInfo() << "[AesaExample] hook calls: rcs" << rcsCalls_.load()
            << "detect" << detectCalls_.load() << "prop" << propCalls_.load()
            << "pre" << preCalls_.load() << "post" << postCalls_.load();
}

void AesaExamplePlugin::radarAdded(const aesaplugin::RadarInfo& info)
{
    radars_.push_back(info);
    qInfo().noquote() << "[AesaExample] radar added:" << radarLabel(info)
                      << "id" << QString::fromStdString(info.radarId);
    if (has("config")) runConfigDemo(info);
}

void AesaExamplePlugin::radarRemoved(const std::string& radarId)
{
    radars_.erase(std::remove_if(radars_.begin(), radars_.end(),
                                 [&radarId](const aesaplugin::RadarInfo& r)
                                 { return r.radarId == radarId; }),
                  radars_.end());
    qInfo() << "[AesaExample] radar removed:" << QString::fromStdString(radarId);
}

void AesaExamplePlugin::runConfigDemo(const aesaplugin::RadarInfo& info)
{
    if (!getRadarConfig || !setRadarConfig) return;

    const QJsonObject before = getRadarConfig(info.radarId);
    qInfo() << "[AesaExample] config sections:" << before.keys();

    // 1. Round trip: write the config back unchanged, read it again.
    const bool wrote = setRadarConfig(info.radarId, before);
    const QJsonObject after = getRadarConfig(info.radarId);
    QStringList changed;
    for (const QString& k : before.keys())
        if (before.value(k) != after.value(k)) changed << k;
    qInfo() << "[AesaExample] config round-trip:"
            << ((wrote && changed.isEmpty()) ? "PASS" : "DIFF") << changed;

    // 2. Visible change: limit azimuth to +/-30 degrees.
    const QJsonObject patch = makeAzimuthPatch(before, kAzimuthLimit_deg);
    if (patch.isEmpty())
    {
        qInfo() << "[AesaExample] azimuth keys not found; no change made";
        return;
    }
    const bool ok = setRadarConfig(info.radarId, patch);
    qInfo() << "[AesaExample] azimuth limited to +/-" << kAzimuthLimit_deg
            << "deg:" << (ok ? "done" : "FAILED");
}

void AesaExamplePlugin::shutdown()
{
    if (clearLogicHooks) clearLogicHooks();
    qInfo() << "[AesaExample] shutdown. Totals: rcs" << rcsCalls_.load()
            << "detect" << detectCalls_.load() << "prop" << propCalls_.load()
            << "pre" << preCalls_.load() << "post" << postCalls_.load();
}
