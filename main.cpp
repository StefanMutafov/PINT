#include <Arduino.h>
#include "PulseOxy.h"
#include "BLEPlx.h"
#include "MotionSensor.h"

#define BLE_TIMEOUT 5000

//   Shared globals for SpO2/HR (written by pulseTask)
//   g_spo2/g_hr hold whatever the last valid reading was.
//   g_validPulse tells us if that reading was valid.
//   g_pulseUpdated flips to true exactly when pulseTask finishes a read.
volatile int32_t  g_spo2         = 0;
volatile int32_t  g_hr           = 0;
volatile bool     g_validPulse   = false;
volatile bool     g_pulseUpdated = false;


TaskHandle_t motionTaskHandle = nullptr;
TaskHandle_t pulseTaskHandle  = nullptr;

//motionTask
void motionTask(void* pv) {
    initMotion();
    while (true) {
        updateMotion();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

//pulseTask
void pulseTask(void* pv) {
    initPulseOxy();
    while (true) {
        int32_t spo2, hr;
        bool    okSpO2, okHR;

        if (readPulseOxy(spo2, okSpO2, hr, okHR)) {
            g_spo2       = spo2;
            g_hr         = hr;
            g_validPulse = true;
        } else {
            g_validPulse = false;
        }

        // New data is ready (whether valid or not)
        g_pulseUpdated = true;

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void setup() {
    Serial.begin(115200);

    // Initialize sensors & BLE
    initPulseOxy();
    initMotion();
    initBLE();

    // Create motionTask on Core 1
    xTaskCreatePinnedToCore(
            motionTask,
            "MotionTask",
            4096,
            nullptr,
            1,                // priority 1
            &motionTaskHandle,
            1                 // run on core 1
    );

    // Create pulseTask on Core 1
    xTaskCreatePinnedToCore(
            pulseTask,
            "PulseTask",
            4096,
            nullptr,
            1,                // priority 1
            &pulseTaskHandle,
            1                 // run on core 1
    );
}


void loop() {
    static unsigned long lastNotifyTime = 0;
    //static unsigned long lastSDRecord = 0;

   //Get current step count
    int steps = getStepCount();

    //If pulseTask just finished a read, print
    if (g_pulseUpdated) {
        g_pulseUpdated = false;  // clear the flag

        if (g_validPulse) {

            Serial.printf("HR: %ld bpm, SpO2: %ld%%, Steps: %d\n",
                          (long)g_hr, (long)g_spo2, steps);
        } else {
            Serial.println("Invalid SpO2/HR, retrying...");
        }
    }

    // 3) BLE‐notify every BLE_TIMEOUT ms, but only if the last pulse was valid
    unsigned long now = millis();
    if (g_validPulse && (now - lastNotifyTime >= BLE_TIMEOUT)) {
        lastNotifyTime = now;
        if (isConnected()) {
            notifyPlx(g_spo2, g_hr);
            notifySteps(steps);
            Serial.println("BLE notified all");
        }
    }


    //    now = millis();
    //    if(now - lastSDRecord >= SD_TIMEOUT){
    //        //HERE SAVE DATA TO SD CARD, ONCE EVERY SD_TIMEOUT
    //    }



    delay(10);
}
