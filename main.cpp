#include <Arduino.h>
#include "PulseOxy.h"
#include "BLEPlx.h"
#include "MotionSensor.h"

static unsigned long lastNotify = 0;

void setup() {
    Serial.begin(115200);
    initPulseOxy();
    initMotion();
    initBLE();
}

void loop() {
    int32_t spo2, hr;
    bool    okSpO2, okHR;

    // get new step count + handle fall detection
    int steps = updateMotion();

    // read pulse-ox
    if (readPulseOxy(spo2, okSpO2, hr, okHR)) {
        Serial.printf("HR: %ld bpm, SpO₂: %ld%%, Steps: %d\n", hr, spo2, steps);
    } else {
        Serial.println("Invalid SpO₂/HR, retrying...");
    }

    // notify every 1s when connected
    unsigned long now = millis();
    if (isConnected() && now - lastNotify >= 1000) {
        lastNotify = now;
        notifyPlx(spo2, hr);
        notifySteps(steps);
        Serial.println("BLE notified all");
    }

//    delay(10);
}
