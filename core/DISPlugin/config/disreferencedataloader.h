// =============================================================================
// FILE:        disreferencedataloader.h
// MODULE:      DIS Network Plugin — Config
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Loads SISO-REF-010 reference data (Kind/Domain/Country/
//              Category/SubCategory/Specific/Extra) from
//              ~/TDF/DIS/siso_reference_data.json, mirroring
//              DISConfigLoader's resolve/load/create-default pattern.
//
//              instance() is a lazy singleton — the file is read from disk
//              exactly once, the first time any caller needs it, and the
//              parsed result is reused for the lifetime of the process.
//
//              Editing siso_reference_data.json by hand (adding
//              SubCategory/Specific/Extra entries, correcting a name,
//              etc.) takes effect on the next application launch — no
//              rebuild needed. This is the extensibility mechanism: fields
//              not yet populated simply fall back to showing their bare
//              numeric value (see DISTypeProfile::formatEntry), and
//              become real dropdown entries the moment someone adds them
//              to the JSON file.
// =============================================================================
#ifndef DISREFERENCEDATALOADER_H
#define DISREFERENCEDATALOADER_H

#include "../interface/DISReferenceData.h"
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

class DISReferenceDataLoader
{
public:
    // Lazy singleton accessor — loads from disk on first call only.
    static const DISReferenceData& instance();

    // Resolves the on-disk path (~/TDF/DIS/siso_reference_data.json),
    // creating a minimal fallback file there if none exists yet.
    // Public so UI/tools can query or point users at the resolved path.
    static QString resolveDataPath();

private:
    static DISReferenceData load(const QString& path);
    static bool writeMinimalFallback(const QString& path);
    static std::vector<DISRefEntry> parseEntryArray(const QJsonArray& arr);
};

#endif // DISREFERENCEDATALOADER_H
