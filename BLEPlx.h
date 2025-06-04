#ifndef BLEPLX_H
#define BLEPLX_H

#include <Arduino.h>

// Initialize BLE
void initBLE();

// Returns true if at least one client is connected (to any of your services)
bool isConnected();

// Notify real‐time PLX (SpO2+HR)
void notifyPlx(int32_t spo2, int32_t hr);

// Notify real‐time step count
void notifySteps(int stepCount);

// Stream the SD file over BLE, chunked.
void sendFileOverBLE();

#endif // BLEPLX_H
