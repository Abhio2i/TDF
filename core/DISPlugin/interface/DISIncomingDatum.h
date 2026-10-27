#ifndef DISINCOMINGDATUM_H
#define DISINCOMINGDATUM_H
#include <string>
#include <cstdint>
#include <vector>
#include <QByteArray>
#include "DISDatumSnapshot.h"

struct DISIncomingDatum {
    std::string originatingEntityID;
    std::string receivingEntityID;
    uint32_t requestID    = 0;
    uint32_t timeInterval = 0;
    std::vector<DISFixedDatumEntry>    fixedDatums;
    std::vector<DISVariableDatumEntry> variableDatums;
};
#endif
