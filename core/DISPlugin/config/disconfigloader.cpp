// =============================================================================
// FILE:        DISConfigLoader.cpp
// MODULE:      DIS Network Plugin — Config
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================
// =============================================================================
// FILE:        DISConfigLoader.cpp
// MODULE:      DIS Network Plugin — Config
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "disconfigloader.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QCoreApplication>   // ADD — applicationDirPath()
#include <QDir>               // ADD — mkpath()

// =============================================================================
// defaults
// =============================================================================
DISConfig DISConfigLoader::defaults()
{
    return DISDefaultConfig();
}

// =============================================================================
// resolveSavePath  (private)
// If configPath is a bare filename with no directory (e.g. "dis_config.json"),
// redirect it to <exe>/config/<filename> so load and save always use the
// same location. Absolute or relative paths with directories pass through
// unchanged.
// =============================================================================
QString DISConfigLoader::resolveSavePath(const QString& configPath)
{
    // If path contains a directory separator, use it as-is
    if (configPath.contains('/') || configPath.contains('\\'))
        return configPath;

    // Bare filename — map to <exe>/config/<filename>
    // Bare filename — map to ~/TDF/DIS/<filename>
    QString tdfDir = QDir::homePath() + "/TDF/DIS";
    QDir().mkpath(tdfDir);
    return tdfDir + "/" + configPath;
    // QString exeDir    = QCoreApplication::applicationDirPath();
    // QString configDir = exeDir + "/config";

    // // Ensure the folder exists (may already exist from resolveConfigPath)
    // QDir dir;
    // dir.mkpath(configDir);

    // return configDir + "/" + configPath;
}

// =============================================================================
// resolveConfigPath  (public)
// Called by load() before opening any file.
//
// Resolution order:
//   1. File exists exactly at configPath             → use it
//   2. File exists at <exe>/config/dis_config.json   → use it
//   3. Neither exists → create <exe>/config/<file> with defaults → use it
//
// This means:
//   Dev machine with build folder   → existing file found at step 1, used as-is
//   Deployed executable first run   → default file created at step 3
//   Subsequent runs                 → file found at step 2
// =============================================================================
QString DISConfigLoader::resolveConfigPath(const QString& configPath)
{
    QString tdfDir  = QDir::homePath() + "/TDF/DIS";
    QString tdfPath = tdfDir + "/dis_config.json";

    // Step 1 — already in ~/TDF/DIS/ → use it (normal case after first run)
    if (QFile::exists(tdfPath))
        return tdfPath;

    // Ensure ~/TDF/DIS/ folder exists
    QDir dir;
    if (!dir.mkpath(tdfDir)) {
        qWarning() << "[DISConfigLoader] Cannot create TDF/DIS folder:" << tdfDir;
        return configPath;
    }

    // Step 2 — auto-migrate from old build/config/ location (one time only)
    QString exeDir  = QCoreApplication::applicationDirPath();
    QString oldPath = exeDir + "/config/" + QFileInfo(configPath).fileName();
    if (QFile::exists(oldPath)) {
        QFile::copy(oldPath, tdfPath);
        QFile::setPermissions(tdfPath,
                              QFile::ReadOwner | QFile::WriteOwner |
                                  QFile::ReadGroup | QFile::ReadOther);
        qDebug() << "[DISConfigLoader] Config migrated from:" << oldPath;
        qDebug() << "[DISConfigLoader] Config now at:" << tdfPath;
        return tdfPath;
    }

    // Step 3 — first run, no existing config anywhere → create default
    DISConfig def = defaults();
    if (save(def, tdfPath)) {
        qDebug() << "[DISConfigLoader] ════════════════════════════════════════";
        qDebug() << "[DISConfigLoader] DEFAULT CONFIG CREATED:";
        qDebug() << "[DISConfigLoader]" << tdfPath;
        qDebug() << "[DISConfigLoader] Edit before starting DIS:";
        qDebug() << "[DISConfigLoader]   dis > applicationID"
                 << "— unique per machine (1, 2, 3, 4)";
        qDebug() << "[DISConfigLoader]   dis > siteID"
                 << "— same on all machines (1)";
        qDebug() << "[DISConfigLoader]   dis > exerciseID"
                 << "— same on all machines (1)";
        qDebug() << "[DISConfigLoader]   network > networkInterface"
                 << "— this machine's LAN IP";
        qDebug() << "[DISConfigLoader]   network > connectionMode"
                 << "— Multicast or Unicast";
        qDebug() << "[DISConfigLoader]   peers > overrides"
                 << "— peer IPs if Unicast mode";
        qDebug() << "[DISConfigLoader] ════════════════════════════════════════";
    } else {
        qWarning() << "[DISConfigLoader] Failed to write default config to:"
                   << tdfPath;
        return configPath;
    }

    return tdfPath;
}
// QString DISConfigLoader::resolveConfigPath(const QString& configPath)
// {
//     // Step 1 — file exists exactly where specified (dev/build workflow)
//     if (QFile::exists(configPath))
//         return configPath;

//     // Step 2 — look next to executable in config/ subfolder
//     QString exeDir     = QCoreApplication::applicationDirPath();
//     QString configDir  = exeDir + "/config";
//     QString targetPath = configDir + "/" +
//                          QFileInfo(configPath).fileName();

//     if (QFile::exists(targetPath))
//         return targetPath;

//     // Step 3 — neither location has the file → create folder + default file
//     QDir dir;
//     if (!dir.exists(configDir)) {
//         if (!dir.mkpath(configDir)) {
//             qWarning() << "[DISConfigLoader] Cannot create config folder:"
//                        << configDir;
//             return configPath;  // fall back — load() will return defaults
//         }
//         qDebug() << "[DISConfigLoader] Created config folder:" << configDir;
//     }

//     // Write default config so user has a file to edit
//     DISConfig def = defaults();
//     if (save(def, targetPath)) {
//         qDebug() << "[DISConfigLoader] ════════════════════════════════════════";
//         qDebug() << "[DISConfigLoader] DEFAULT CONFIG CREATED:";
//         qDebug() << "[DISConfigLoader]" << targetPath;
//         qDebug() << "[DISConfigLoader] Edit before starting DIS:";
//         qDebug() << "[DISConfigLoader]   dis > applicationID"
//                  << "— unique per machine (1, 2, 3, 4)";
//         qDebug() << "[DISConfigLoader]   dis > siteID"
//                  << "— same on all machines (1)";
//         qDebug() << "[DISConfigLoader]   dis > exerciseID"
//                  << "— same on all machines (1)";
//         qDebug() << "[DISConfigLoader]   network > networkInterface"
//                  << "— this machine's LAN IP";
//         qDebug() << "[DISConfigLoader]   network > connectionMode"
//                  << "— Multicast or Unicast";
//         qDebug() << "[DISConfigLoader]   peers > overrides"
//                  << "— peer IPs if Unicast mode";
//         qDebug() << "[DISConfigLoader] ════════════════════════════════════════";
//     } else {
//         qWarning() << "[DISConfigLoader] Failed to write default config to:"
//                    << targetPath;
//         return configPath;
//     }

//     return targetPath;
// }

// =============================================================================
// load
// =============================================================================
DISConfig DISConfigLoader::load(const QString& configPath)
{
    DISConfig config = defaults();

    // Resolve to correct path — creates folder and default file if needed
    QString resolvedPath = resolveConfigPath(configPath);

    QFile file(resolvedPath);
    if (!file.exists()) {
        qWarning() << "[DISConfigLoader] Config not found:"
                   << resolvedPath << "— using defaults";
        return config;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[DISConfigLoader] Cannot open:"
                   << resolvedPath << "— using defaults";
        return config;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[DISConfigLoader] JSON parse error:"
                   << parseError.errorString() << "— using defaults";
        return config;
    }

    QJsonObject root = doc.object();

    // ── Network ───────────────────────────────────────────────────────────────
    if (root.contains("network")) {
        QJsonObject net = root["network"].toObject();

        if (net.contains("multicastGroup"))
            config.multicastGroup =
                net["multicastGroup"].toString().toStdString();

        if (net.contains("port"))
            config.port =
                static_cast<uint16_t>(net["port"].toInt(3000));

        if (net.contains("ttl"))
            config.ttl =
                static_cast<uint8_t>(net["ttl"].toInt(1));

        if (net.contains("receiveBufferSize"))
            config.receiveBufferSize =
                net["receiveBufferSize"].toInt(65536);

        if (net.contains("loopbackEnabled"))
            config.loopbackEnabled =
                net["loopbackEnabled"].toBool(true);

        if (net.contains("connectionMode"))
            config.connectionMode =
                net["connectionMode"].toString("Multicast").toStdString();

        if (net.contains("networkInterface"))
            config.networkInterface =
                net["networkInterface"].toString().toStdString();

        qDebug() << "[DISConfigLoader]   interface ="
                 << QString::fromStdString(config.networkInterface);
    }

    // ── DIS ───────────────────────────────────────────────────────────────────
    if (root.contains("dis")) {
        QJsonObject dis = root["dis"].toObject();

        if (dis.contains("defaultVersion"))
            config.defaultVersion =
                static_cast<uint8_t>(dis["defaultVersion"].toInt(7));

        if (dis.contains("exerciseID"))
            config.exerciseID =
                static_cast<uint8_t>(dis["exerciseID"].toInt(1));

        if (dis.contains("siteID"))
            config.siteID =
                static_cast<uint16_t>(dis["siteID"].toInt(1));

        if (dis.contains("applicationID"))
            config.applicationID =
                static_cast<uint16_t>(dis["applicationID"].toInt(1));

        if (dis.contains("countryCode"))
            config.countryCode =
                static_cast<uint16_t>(dis["countryCode"].toInt(164));

        if (dis.contains("ignoreOtherVersions"))
            config.ignoreOtherVersions =
                dis["ignoreOtherVersions"].toBool(false);
        if (dis.contains("globalTraceLevel"))
            config.globalTraceLevel =
                dis["globalTraceLevel"].toString("Basic").toStdString();
    }

    // ── Dead Reckoning ────────────────────────────────────────────────────────
    if (root.contains("deadReckoning")) {
        QJsonObject dr = root["deadReckoning"].toObject();

        if (dr.contains("positionThresholdMeters"))
            config.positionThresholdMeters =
                static_cast<float>(
                    dr["positionThresholdMeters"].toDouble(1.0));

        if (dr.contains("heartbeatIntervalSeconds"))
            config.heartbeatIntervalSeconds =
                static_cast<float>(
                    dr["heartbeatIntervalSeconds"].toDouble(5.0));
        if (dr.contains("defaultDRAlgorithm"))
            config.defaultDRAlgorithm =
                static_cast<uint8_t>(dr["defaultDRAlgorithm"].toInt(2));
    }

    // ── Peers ─────────────────────────────────────────────────────────────────
    if (root.contains("peers")) {
        QJsonObject peers = root["peers"].toObject();
        if (peers.contains("overrides")) {
            QJsonArray overrides = peers["overrides"].toArray();
            for (const QJsonValue& val : overrides) {
                QJsonObject obj = val.toObject();
                DISConfig::UnicastPeer p;
                p.ip   = obj["ip"].toString().toStdString();
                p.port = static_cast<uint16_t>(obj["port"].toInt(3000));
                if (!p.ip.empty())
                    config.unicastPeers.push_back(p);
            }
        }
    }
    // ── PDU configs ───────────────────────────────────────────────────────────
    // If present in JSON — use saved settings (user customised)
    // If absent — keep DISDefaultConfig() values loaded at start (backward compat)
    if (root.contains("pduConfigs")) {
        QJsonArray arr = root["pduConfigs"].toArray();
        if (!arr.isEmpty()) {
            config.pduConfigs.clear();
            for (const QJsonValue& val : arr) {
                QJsonObject obj = val.toObject();
                DISPDUConfig pdu;
                pdu.pduType    = static_cast<uint8_t>(obj["pduType"].toInt(0));
                pdu.pduName    = obj["pduName"].toString().toStdString();
                pdu.send       = obj["send"].toBool(true);
                pdu.receive    = obj["receive"].toBool(true);
                pdu.traceLevel = obj["trace"].toString("Error").toStdString();
                config.pduConfigs.push_back(pdu);
            }
            qDebug() << "[DISConfigLoader]   pduConfigs loaded:"
                     << config.pduConfigs.size() << "entries";
        }
    }

    qDebug() << "[DISConfigLoader] Loaded from:" << resolvedPath;
    qDebug() << "[DISConfigLoader] Loaded from:" << resolvedPath;
    qDebug() << "[DISConfigLoader]   multicast =" << QString::fromStdString(config.multicastGroup);
    qDebug() << "[DISConfigLoader]   port      =" << config.port;
    qDebug() << "[DISConfigLoader]   exerciseID=" << config.exerciseID;
    qDebug() << "[DISConfigLoader]   siteID    =" << config.siteID;
    qDebug() << "[DISConfigLoader]   appID     =" << config.applicationID;
    qDebug() << "[DISConfigLoader]   version   =" << config.defaultVersion;
    qDebug() << "[DISConfigLoader]   interface =" << QString::fromStdString(config.networkInterface);

    return config;
}

// =============================================================================
// save
// =============================================================================
bool DISConfigLoader::save(const DISConfig& config, const QString& configPath)
{
    // Resolve bare filename to exe/config/ folder
    // Absolute paths and relative paths with directories pass through unchanged
    QString targetPath = resolveSavePath(configPath);

    QJsonObject net;
    net["multicastGroup"]    = QString::fromStdString(config.multicastGroup);
    net["port"]              = config.port;
    net["ttl"]               = config.ttl;
    net["receiveBufferSize"] = config.receiveBufferSize;
    net["loopbackEnabled"]   = config.loopbackEnabled;
    net["connectionMode"]    = QString::fromStdString(config.connectionMode);
    net["networkInterface"]  = QString::fromStdString(config.networkInterface);

    QJsonObject dis;
    dis["defaultVersion"]      = config.defaultVersion;
    dis["exerciseID"]          = config.exerciseID;
    dis["siteID"]              = config.siteID;
    dis["applicationID"]       = config.applicationID;
    dis["countryCode"]         = config.countryCode;
    dis["ignoreOtherVersions"] = config.ignoreOtherVersions;
    dis["globalTraceLevel"] = QString::fromStdString(config.globalTraceLevel);
    QJsonObject dr;
    dr["positionThresholdMeters"]  =
        static_cast<double>(config.positionThresholdMeters);
    dr["heartbeatIntervalSeconds"] =
        static_cast<double>(config.heartbeatIntervalSeconds);
    dr["defaultDRAlgorithm"]       = static_cast<int>(config.defaultDRAlgorithm);

    QJsonArray overrides;
    for (const DISConfig::UnicastPeer& p : config.unicastPeers) {
        QJsonObject peer;
        peer["ip"]   = QString::fromStdString(p.ip);
        peer["port"] = p.port;
        overrides.append(peer);
    }
    QJsonObject peersObj;
    peersObj["overrides"] = overrides;

    // QJsonObject root;
    // root["network"]       = net;
    // root["dis"]           = dis;
    // root["deadReckoning"] = dr;
    // root["peers"]         = peersObj;
    // Serialize per-PDU send/receive/trace settings
    QJsonArray pduConfigsArray;
    for (const DISPDUConfig& pdu : config.pduConfigs) {
        QJsonObject pduObj;
        pduObj["pduType"] = static_cast<int>(pdu.pduType);
        pduObj["pduName"] = QString::fromStdString(pdu.pduName);
        pduObj["send"]    = pdu.send;
        pduObj["receive"] = pdu.receive;
        pduObj["trace"]   = QString::fromStdString(pdu.traceLevel);
        pduConfigsArray.append(pduObj);
    }

    QJsonObject root;
    root["network"]       = net;
    root["dis"]           = dis;
    root["deadReckoning"] = dr;
    root["peers"]         = peersObj;
    root["pduConfigs"]    = pduConfigsArray;   // ADD

    QJsonDocument doc(root);

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[DISConfigLoader] Cannot write:" << targetPath;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "[DISConfigLoader] Saved to:" << targetPath;
    return true;
}
// #include "disconfigloader.h"

// #include <QFile>
// #include <QJsonDocument>
// #include <QJsonObject>
// #include <QJsonArray>
// #include <QDebug>

// // =============================================================================
// // defaults
// // STAGE 22.0 compatible defaults
// // Safe to use even without a config file
// // =============================================================================
// DISConfig DISConfigLoader::defaults()
// {
//     return DISDefaultConfig();
// }

// // =============================================================================
// // load
// // Reads dis_config.json and fills DISConfig
// // Falls back to defaults on any error
// // =============================================================================
// DISConfig DISConfigLoader::load(const QString& configPath)
// {
//     DISConfig config = defaults();

//     QFile file(configPath);
//     if (!file.exists()) {
//         qWarning() << "[DISConfigLoader] Config file not found:"
//                    << configPath
//                    << "— using defaults";
//         return config;
//     }

//     if (!file.open(QIODevice::ReadOnly)) {
//         qWarning() << "[DISConfigLoader] Cannot open:" << configPath
//                    << "— using defaults";
//         return config;
//     }

//     QJsonParseError parseError;
//     QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
//     file.close();

//     if (parseError.error != QJsonParseError::NoError) {
//         qWarning() << "[DISConfigLoader] JSON parse error:" << parseError.errorString()
//         << "— using defaults";
//         return config;
//     }

//     QJsonObject root = doc.object();

//     // ── Network section ───────────────────────────────────────────────────────
//     if (root.contains("network")) {
//         QJsonObject net = root["network"].toObject();

//         if (net.contains("multicastGroup"))
//             config.multicastGroup = net["multicastGroup"].toString().toStdString();

//         if (net.contains("port"))
//             config.port = static_cast<uint16_t>(net["port"].toInt(3000));

//         if (net.contains("ttl"))
//             config.ttl = static_cast<uint8_t>(net["ttl"].toInt(1));

//         if (net.contains("receiveBufferSize"))
//             config.receiveBufferSize = net["receiveBufferSize"].toInt(65536);

//         if (net.contains("loopbackEnabled"))
//             config.loopbackEnabled = net["loopbackEnabled"].toBool(true);

//         if (net.contains("connectionMode"))
//             config.connectionMode =
//                 net["connectionMode"].toString("Multicast").toStdString();
//         if (net.contains("networkInterface"))
//             config.networkInterface = net["networkInterface"].toString().toStdString();
//         qDebug() << "[DISConfigLoader]   interface =" << QString::fromStdString(config.networkInterface);
//     }

//     // ── DIS section ───────────────────────────────────────────────────────────
//     if (root.contains("dis")) {
//         QJsonObject dis = root["dis"].toObject();

//         if (dis.contains("defaultVersion"))
//             config.defaultVersion = static_cast<uint8_t>(dis["defaultVersion"].toInt(7));

//         if (dis.contains("exerciseID"))
//             config.exerciseID = static_cast<uint8_t>(dis["exerciseID"].toInt(1));

//         if (dis.contains("siteID"))
//             config.siteID = static_cast<uint16_t>(dis["siteID"].toInt(1));

//         if (dis.contains("applicationID"))
//             config.applicationID = static_cast<uint16_t>(dis["applicationID"].toInt(1));

//         if (dis.contains("countryCode"))
//             config.countryCode = static_cast<uint16_t>(dis["countryCode"].toInt(164));

//         if (dis.contains("ignoreOtherVersions"))
//             config.ignoreOtherVersions = dis["ignoreOtherVersions"].toBool(false);
//     }

//     // ── Dead reckoning section ────────────────────────────────────────────────
//     if (root.contains("deadReckoning")) {
//         QJsonObject dr = root["deadReckoning"].toObject();

//         if (dr.contains("positionThresholdMeters"))
//             config.positionThresholdMeters =
//                 static_cast<float>(dr["positionThresholdMeters"].toDouble(1.0));

//         if (dr.contains("heartbeatIntervalSeconds"))
//             config.heartbeatIntervalSeconds =
//                 static_cast<float>(dr["heartbeatIntervalSeconds"].toDouble(5.0));
//     }
//     // ── Peers section ─────────────────────────────────────────────────────────
//     if (root.contains("peers")) {
//         QJsonObject peers = root["peers"].toObject();
//         if (peers.contains("overrides")) {
//             QJsonArray overrides = peers["overrides"].toArray();
//             for (const QJsonValue& val : overrides) {
//                 QJsonObject obj = val.toObject();
//                 DISConfig::UnicastPeer p;
//                 p.ip   = obj["ip"].toString().toStdString();
//                 p.port = static_cast<uint16_t>(obj["port"].toInt(3000));
//                 if (!p.ip.empty())
//                     config.unicastPeers.push_back(p);
//             }
//         }
//     }
//     qDebug() << "[DISConfigLoader] Loaded config from" << configPath;
//     qDebug() << "[DISConfigLoader]   multicast =" << QString::fromStdString(config.multicastGroup);
//     qDebug() << "[DISConfigLoader]   port      =" << config.port;
//     qDebug() << "[DISConfigLoader]   exerciseID=" << config.exerciseID;
//     qDebug() << "[DISConfigLoader]   siteID    =" << config.siteID;
//     qDebug() << "[DISConfigLoader]   appID     =" << config.applicationID;
//     qDebug() << "[DISConfigLoader]   version   =" << config.defaultVersion;

//     return config;
// }

// // =============================================================================
// // save
// // Writes current config back to JSON file
// // =============================================================================
// bool DISConfigLoader::save(const DISConfig& config, const QString& configPath)
// {
//     QJsonObject net;
//     net["multicastGroup"]   = QString::fromStdString(config.multicastGroup);
//     net["port"]             = config.port;
//     net["ttl"]              = config.ttl;
//     net["receiveBufferSize"]= config.receiveBufferSize;
//     net["loopbackEnabled"]  = config.loopbackEnabled;

//     net["connectionMode"] = QString::fromStdString(config.connectionMode);

//     QJsonObject dis;
//     dis["defaultVersion"]      = config.defaultVersion;
//     dis["exerciseID"]          = config.exerciseID;
//     dis["siteID"]              = config.siteID;
//     dis["applicationID"]       = config.applicationID;
//     dis["countryCode"]         = config.countryCode;
//     dis["ignoreOtherVersions"] = config.ignoreOtherVersions;

//     QJsonObject dr;
//     dr["positionThresholdMeters"]  = static_cast<double>(config.positionThresholdMeters);
//     dr["heartbeatIntervalSeconds"] = static_cast<double>(config.heartbeatIntervalSeconds);

//     QJsonObject root;
//     QJsonArray overrides;
//     for (const DISConfig::UnicastPeer& p : config.unicastPeers) {
//         QJsonObject peer;
//         peer["ip"]   = QString::fromStdString(p.ip);
//         peer["port"] = p.port;
//         overrides.append(peer);
//     }
//     QJsonObject peersObj;
//     peersObj["overrides"] = overrides;
//     root["peers"] = peersObj;
//     root["network"]       = net;
//     root["dis"]           = dis;
//     root["deadReckoning"] = dr;

//     QJsonDocument doc(root);

//     QFile file(configPath);
//     if (!file.open(QIODevice::WriteOnly)) {
//         qWarning() << "[DISConfigLoader] Cannot write:" << configPath;
//         return false;
//     }

//     file.write(doc.toJson(QJsonDocument::Indented));
//     file.close();

//     qDebug() << "[DISConfigLoader] Config saved to" << configPath;
//     return true;
// }
