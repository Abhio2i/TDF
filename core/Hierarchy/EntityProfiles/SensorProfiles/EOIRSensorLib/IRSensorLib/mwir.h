#ifndef MWIR_H
#define MWIR_H

#include "irsensorlib.h"

class MWIR : public IRSensorLib
{
public:
    MWIR();
    bool Range_Detection(float Φ, float SNR);
};

#endif // MWIR_H
