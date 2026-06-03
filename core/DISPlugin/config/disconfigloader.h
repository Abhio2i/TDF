// =============================================================================
// FILE:        DISConfigLoader.h
// MODULE:      DIS Network Plugin — Config
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Loads dis_config.json and populates DISConfig struct.
//              Falls back to safe defaults if file not found.
// =============================================================================
#ifndef DISCONFIGLOADER_H
#define DISCONFIGLOADER_H

#include "../interface/DISConfig.h"
#include <QString>

class DISConfigLoader {
public:
    // Load from JSON — creates default file next to exe if not found
    static DISConfig load(const QString& configPath);

    // Save config to JSON — resolves bare filename to exe/config/ folder
    static bool save(const DISConfig& config, const QString& configPath);

    // Returns STAGE-compatible defaults
    static DISConfig defaults();

    // Resolves path → creates config folder and default file if needed
    // Public so UI can query the resolved path if needed
    static QString resolveConfigPath(const QString& configPath);

private:
    // Resolves save target — bare filename → exe/config/filename
    // Does NOT create the file, just determines where it should go
    static QString resolveSavePath(const QString& configPath);
};

#endif // DISCONFIGLOADER_H
// #ifndef DISCONFIGLOADER_H
// #define DISCONFIGLOADER_H

// #include "../interface/DISConfig.h"
// #include <QString>

// class DISConfigLoader {
// public:
//     // Load from JSON file — returns defaults if file missing or malformed
//     static DISConfig load(const QString& configPath);

//     // Save current config back to JSON
//     static bool save(const DISConfig& config, const QString& configPath);

//     // Returns STAGE-compatible defaults
//     static DISConfig defaults();
// };

// #endif // DISCONFIGLOADER_H
