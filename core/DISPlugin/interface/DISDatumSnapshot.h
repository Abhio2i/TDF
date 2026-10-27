#ifndef DISDATUMSNAPSHOT_H
#define DISDATUMSNAPSHOT_H
#include <string>
#include <cstdint>
#include <vector>
#include <QByteArray>

// Shared payload shape for DataQuery(18)/SetData(19)/Data(20) — all three
// are wire-identical apart from DataQuery's extra timeInterval field
// (IEEE 1278.1 §7.5.9-7.5.11). timeInterval is ignored when this struct
// is used for SetData or Data.
struct DISFixedDatumEntry {
    uint32_t datumID    = 0;   // SISO-REF-010 datum ID enumeration
    uint32_t datumValue = 0;
};

struct DISVariableDatumEntry {
    uint32_t datumID = 0;      // SISO-REF-010 datum ID enumeration
    QByteArray data;           // arbitrary-length payload
};

struct DISDatumSnapshot {
    std::string originatingEntityID;
    std::string receivingEntityID;  // empty = not set on wire (mapper lookup skipped)
    uint32_t requestID    = 0;
    uint32_t timeInterval = 0;      // DataQuery only; 0 = send once
    std::vector<DISFixedDatumEntry>    fixedDatums;
    std::vector<DISVariableDatumEntry> variableDatums;
};
#endif
