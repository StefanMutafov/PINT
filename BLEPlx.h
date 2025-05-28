#ifndef BLE_PLX_H
#define BLE_PLX_H

#include <Arduino.h>

// call once in setup()
void initBLE();

// true if a central is connected
bool isConnected();

// notify HB + O₂
void notifyPlx(int32_t spo2, int32_t hr);

// notify step count
void notifySteps(int stepCount);

#endif // BLE_PLX_H
