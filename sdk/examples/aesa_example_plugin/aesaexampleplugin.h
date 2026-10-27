// =============================================================================
// FILE:         aesaexampleplugin.h
// MODULE:       AESA plugin SDK — example plugin
// DESCRIPTION:  Demonstrates every part of AesaRadarPluginInterface: radar
//               list and status, config read/write, the three replaceable
//               logic stages (RCS, detection, propagation) and the pre/post
//               filters. Behaviour is chosen with the environment variable
//               AESA_EXAMPLE_MODE (comma-separated):
//                 log, rcs, blind, fog, prefilter, filter, config
//               Default: log.
// =============================================================================
#ifndef AESAEXAMPLEPLUGIN_H
#define AESAEXAMPLEPLUGIN_H

#include <QObject>
#include <QElapsedTimer>
#include <QSet>
#include <QString>
#include <atomic>
#include <string>
#include <vector>
#include "core/Plugins/Interfaces/aesaRadarInterface.h"

class AesaExamplePlugin : public QObject, public AesaRadarPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AesaRadarPluginInterface_iid)
    Q_INTERFACES(AesaRadarPluginInterface)

public:
    void init() override;
    void update(float delta) override;
    void radarAdded(const aesaplugin::RadarInfo& info) override;
    void radarRemoved(const std::string& radarId) override;
    void shutdown() override;

private:
    bool has(const char* mode) const;
    void tryHostCallFromHook();
    void runConfigDemo(const aesaplugin::RadarInfo& info);

    QSet<QString>                      modes_;
    std::vector<aesaplugin::RadarInfo> radars_;
    QElapsedTimer                      logTimer_;
    bool                               hostCallTried_ = false;

    // Hook call counters (hooks and update() run on the same thread today;
    // atomics keep the example correct if that changes).
    std::atomic<long> rcsCalls_{0};
    std::atomic<long> detectCalls_{0};
    std::atomic<long> propCalls_{0};
    std::atomic<long> preCalls_{0};
    std::atomic<long> postCalls_{0};
};

#endif // AESAEXAMPLEPLUGIN_H
