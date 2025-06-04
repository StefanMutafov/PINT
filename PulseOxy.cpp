#include "PulseOxy.h"
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

// sensor & buffers
static MAX30105 sensor;
static uint32_t irBuf[100], redBuf[100];
static const uint8_t BUF_LEN = 100;

void initPulseOxy() {
   // Wire.begin(21, 22);
    if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println(F("MAX30102 not found!"));
        while (1) delay(1000);
    }
    sensor.setup(
            /*ledBrightness=*/65,
            /*sampleAverage=*/32,
            /*ledMode=*/2,
            /*sampleRate=*/400,
            /*pulseWidth=*/411,
            /*adcRange=*/8192
    );
    // prefill buffer once
    for (uint8_t i = 0; i < BUF_LEN; i++) {
        while (!sensor.available()) sensor.check();
        redBuf[i] = sensor.getRed();
        irBuf[i]  = sensor.getIR();
        sensor.nextSample();
    }
}

bool readPulseOxy(int32_t &spo2, bool &validSpO2, int32_t &hr, bool &validHR) {
    // shift 25 samples out, read 25 new ones
    for (uint8_t i = 25; i < BUF_LEN; i++) {
        redBuf[i - 25] = redBuf[i];
        irBuf[i - 25]  = irBuf[i];
    }
    for (uint8_t i = 75; i < BUF_LEN; i++) {
        while (!sensor.available()) sensor.check();
        redBuf[i] = sensor.getRed();
        irBuf[i]  = sensor.getIR();
        sensor.nextSample();
    }

    int32_t  iSpo2, iHr;
    int8_t   vSpo2, vHr;
    maxim_heart_rate_and_oxygen_saturation(
            irBuf, BUF_LEN,
            redBuf, &iSpo2, &vSpo2,
            &iHr,   &vHr
    );
    spo2       = iSpo2;
    validSpO2  = (vSpo2 == 1);
    hr         = iHr;
    validHR    = (vHr   == 1);
    return validSpO2 && validHR;
}
