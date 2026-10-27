// =============================================================================
// FILE:        disreferencedataloader.cpp
// MODULE:      DIS Network Plugin — Config
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "disreferencedataloader.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

// =============================================================================
// resolveDataPath
// Same directory as dis_config.json (~/TDF/DIS/), mirroring
// DISConfigLoader::resolveConfigPath()'s pattern exactly. Creates a
// minimal fallback file there on first run if the full extracted file
// (siso_reference_data.json, generated from SISO-REF-010 v35) has not
// been deployed yet.
// =============================================================================
QString DISReferenceDataLoader::resolveDataPath()
{
    QString tdfDir = QDir::homePath() + "/TDF/DIS";
    QString path   = tdfDir + "/siso_reference_data.json";

    if (QFile::exists(path))
        return path;

    QDir dir;
    if (!dir.mkpath(tdfDir)) {
        qWarning() << "[DISReferenceDataLoader] Cannot create TDF/DIS folder:" << tdfDir;
        return path;
    }

    if (writeMinimalFallback(path)) {
        qDebug() << "[DISReferenceDataLoader] ════════════════════════════════════════";
        qDebug() << "[DISReferenceDataLoader] MINIMAL FALLBACK REFERENCE DATA CREATED:";
        qDebug() << "[DISReferenceDataLoader]" << path;
        qDebug() << "[DISReferenceDataLoader] Only Kind and Domain(Platform) are populated.";
        qDebug() << "[DISReferenceDataLoader] Replace this file with the full";
        qDebug() << "[DISReferenceDataLoader] siso_reference_data.json (extracted from";
        qDebug() << "[DISReferenceDataLoader] SISO-REF-010 v35) to get complete Country";
        qDebug() << "[DISReferenceDataLoader] and Category tables.";
        qDebug() << "[DISReferenceDataLoader] ════════════════════════════════════════";
    } else {
        qWarning() << "[DISReferenceDataLoader] Failed to write fallback file to:" << path;
    }

    return path;
}

// =============================================================================
// writeMinimalFallback
// Written only if siso_reference_data.json is missing entirely. Keeps the
// DIS Type panel usable (Kind/Domain still populate correctly) even
// before the full extracted file has been placed alongside dis_config.json.
// =============================================================================
bool DISReferenceDataLoader::writeMinimalFallback(const QString& path)
{
    QJsonObject root;

    QJsonObject meta;
    meta["source"] = "Minimal fallback — replace with the full SISO-REF-010 v35 extract";
    meta["note"]   = "Only Kind and Domain(Platform) are populated here. "
                   "Country/Category/SubCategory/Specific/Extra are empty "
                   "until the full siso_reference_data.json is deployed "
                   "to this same folder.";
    root["_meta"] = meta;

    QJsonArray kindArr;
    auto addKind = [&](int v, const char* n) {
        QJsonObject o; o["value"] = v; o["name"] = n; kindArr.append(o);
    };
    addKind(0, "Other");          addKind(1, "Platform");
    addKind(2, "Munition");       addKind(3, "Life Form");
    addKind(4, "Environmental");  addKind(5, "Cultural Feature");
    addKind(6, "Supply");         addKind(7, "Radio");
    addKind(8, "Expendable");     addKind(9, "Sensor/Emitter");
    root["kind"] = kindArr;

    QJsonArray platformDomain;
    auto addDom = [&](int v, const char* n) {
        QJsonObject o; o["value"] = v; o["name"] = n; platformDomain.append(o);
    };
    addDom(0, "Other"); addDom(1, "Land");       addDom(2, "Air");
    addDom(3, "Surface"); addDom(4, "Subsurface"); addDom(5, "Space");
    QJsonObject domainObj;
    domainObj["1"] = platformDomain;   // "1" = Kind::Platform
    root["domain"] = domainObj;

    root["country"]     = QJsonArray();
    root["category"]    = QJsonObject();
    root["subcategory"] = QJsonObject();
    root["specific"]    = QJsonObject();
    root["extra"]       = QJsonObject();

    QJsonDocument doc(root);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

// =============================================================================
// parseEntryArray
// Converts a JSON array of {"value":N,"name":"..."} objects into
// std::vector<DISRefEntry>.
// =============================================================================
std::vector<DISRefEntry> DISReferenceDataLoader::parseEntryArray(const QJsonArray& arr)
{
    std::vector<DISRefEntry> out;
    out.reserve(static_cast<size_t>(arr.size()));
    for (const QJsonValue& v : arr) {
        QJsonObject o = v.toObject();
        DISRefEntry e;
        e.value = static_cast<uint16_t>(o.value("value").toInt(0));
        e.name  = o.value("name").toString().toStdString();
        out.push_back(e);
    }
    return out;
}

// =============================================================================
// load
// Parses the resolved JSON file into a DISReferenceData struct. Any
// missing or malformed section is simply left empty — DISTypeProfile
// falls back to showing the bare numeric value when a lookup misses,
// exactly as it already does for unpopulated branches.
// =============================================================================
DISReferenceData DISReferenceDataLoader::load(const QString& path)
{
    DISReferenceData data;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[DISReferenceDataLoader] Cannot open:" << path;
        return data;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[DISReferenceDataLoader] JSON parse error:"
                   << parseError.errorString();
        return data;
    }

    QJsonObject root = doc.object();

    if (root.contains("kind"))
        data.kind = parseEntryArray(root["kind"].toArray());

    if (root.contains("country"))
        data.country = parseEntryArray(root["country"].toArray());

    if (root.contains("domain") && root["domain"].isObject()) {
        QJsonObject domainObj = root["domain"].toObject();
        for (auto it = domainObj.begin(); it != domainObj.end(); ++it) {
            uint8_t kindVal = static_cast<uint8_t>(it.key().toUInt());
            data.domain[kindVal] = parseEntryArray(it.value().toArray());
        }
    }

    if (root.contains("category") && root["category"].isObject()) {
        QJsonObject catObj = root["category"].toObject();
        for (auto kIt = catObj.begin(); kIt != catObj.end(); ++kIt) {
            uint8_t kindVal = static_cast<uint8_t>(kIt.key().toUInt());
            QJsonObject domObj = kIt.value().toObject();
            for (auto dIt = domObj.begin(); dIt != domObj.end(); ++dIt) {
                uint8_t domVal = static_cast<uint8_t>(dIt.key().toUInt());
                data.category[kindVal][domVal] = parseEntryArray(dIt.value().toArray());
            }
        }
    }

    if (root.contains("subcategory") && root["subcategory"].isObject()) {
        QJsonObject scKind = root["subcategory"].toObject();
        for (auto kIt = scKind.begin(); kIt != scKind.end(); ++kIt) {
            uint8_t kindVal = static_cast<uint8_t>(kIt.key().toUInt());
            QJsonObject scDom = kIt.value().toObject();
            for (auto dIt = scDom.begin(); dIt != scDom.end(); ++dIt) {
                uint8_t domVal = static_cast<uint8_t>(dIt.key().toUInt());
                QJsonObject scCat = dIt.value().toObject();
                for (auto cIt = scCat.begin(); cIt != scCat.end(); ++cIt) {
                    uint8_t catVal = static_cast<uint8_t>(cIt.key().toUInt());
                    data.subcategory[kindVal][domVal][catVal] =
                        parseEntryArray(cIt.value().toArray());
                }
            }
        }
    }

    if (root.contains("specific") && root["specific"].isObject()) {
        QJsonObject spKind = root["specific"].toObject();
        for (auto kIt = spKind.begin(); kIt != spKind.end(); ++kIt) {
            uint8_t kindVal = static_cast<uint8_t>(kIt.key().toUInt());
            QJsonObject spDom = kIt.value().toObject();
            for (auto dIt = spDom.begin(); dIt != spDom.end(); ++dIt) {
                uint8_t domVal = static_cast<uint8_t>(dIt.key().toUInt());
                QJsonObject spCat = dIt.value().toObject();
                for (auto cIt = spCat.begin(); cIt != spCat.end(); ++cIt) {
                    uint8_t catVal = static_cast<uint8_t>(cIt.key().toUInt());
                    QJsonObject spSub = cIt.value().toObject();
                    for (auto sIt = spSub.begin(); sIt != spSub.end(); ++sIt) {
                        uint8_t subVal = static_cast<uint8_t>(sIt.key().toUInt());
                        data.specific[kindVal][domVal][catVal][subVal] =
                            parseEntryArray(sIt.value().toArray());
                    }
                }
            }
        }
    }

    if (root.contains("extra") && root["extra"].isObject()) {
        QJsonObject exKind = root["extra"].toObject();
        for (auto kIt = exKind.begin(); kIt != exKind.end(); ++kIt) {
            uint8_t kindVal = static_cast<uint8_t>(kIt.key().toUInt());
            QJsonObject exDom = kIt.value().toObject();
            for (auto dIt = exDom.begin(); dIt != exDom.end(); ++dIt) {
                uint8_t domVal = static_cast<uint8_t>(dIt.key().toUInt());
                QJsonObject exCat = dIt.value().toObject();
                for (auto cIt = exCat.begin(); cIt != exCat.end(); ++cIt) {
                    uint8_t catVal = static_cast<uint8_t>(cIt.key().toUInt());
                    QJsonObject exSub = cIt.value().toObject();
                    for (auto sIt = exSub.begin(); sIt != exSub.end(); ++sIt) {
                        uint8_t subVal = static_cast<uint8_t>(sIt.key().toUInt());
                        data.extra[kindVal][domVal][catVal][subVal] =
                            parseEntryArray(sIt.value().toArray());
                    }
                }
            }
        }
    }

    data.loaded = true;

    qDebug() << "[DISReferenceDataLoader] Loaded from:" << path;
    qDebug() << "[DISReferenceDataLoader]   kind    =" << data.kind.size() << "entries";
    qDebug() << "[DISReferenceDataLoader]   country =" << data.country.size() << "entries";
    qDebug() << "[DISReferenceDataLoader]   domain groups   =" << data.domain.size();
    qDebug() << "[DISReferenceDataLoader]   category groups =" << data.category.size();

    return data;
}

// =============================================================================
// instance
// Lazy singleton — the file is read from disk exactly once per process.
// =============================================================================
const DISReferenceData& DISReferenceDataLoader::instance()
{
    static DISReferenceData s_data = load(resolveDataPath());
    return s_data;
}
