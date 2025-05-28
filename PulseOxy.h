#ifndef PULSE_OXY_H
#define PULSE_OXY_H

#include <Arduino.h>

void    initPulseOxy();
bool    readPulseOxy(int32_t &spo2, bool &validSpO2, int32_t &hr, bool &validHR);

#endif // PULSE_OXY_H
