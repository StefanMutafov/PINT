#include <Arduino.h>
#include "PulseOxy.h"
#include "BLEPlx.h"
#include "MotionSensor.h"
#include "sdCard.h"
#define BLE_TIMEOUT 5000

//Shared globals for SpO2/HR (updated by pulseTask)
volatile int32_t g_spo2        = 0;
volatile int32_t g_hr          = 0;
volatile bool    g_validPulse  = false;

// New flag: becomes true when pulseTask finishes a read:
volatile bool    g_pulseUpdated = false;


//Task handles
TaskHandle_t motionTaskHandle = nullptr;
TaskHandle_t pulseTaskHandle  = nullptr;

//motionTask
void motionTask(void* pv) {
    while (1) {
        updateMotion();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

//pulseTask (runs ~once per second)
void pulseTask(void* pv) {
    while (1) {
        int32_t spo2, hr;
        bool    okSpO2, okHR;

        if (readPulseOxy(spo2, okSpO2, hr, okHR)) {
            g_spo2        = spo2;
            g_hr          = hr;
            g_validPulse  = true;
        } else {
            g_validPulse  = false;
        }

        // Signal that new data is ready
        g_pulseUpdated = true;

        // Wait ~1 s before next read:
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize sensors and BLE:
    initPulseOxy();
    initMotion();
    initBLE();
    SD_setup();
    RTC_setup();
    // Create motionTask pinned to Core 1:
    xTaskCreatePinnedToCore(
            motionTask,
            "MotionTask",
            4096,
            nullptr,
            1,                // priority
            &motionTaskHandle,
            1                 // run on core 1
    );

    // Create pulseTask pinned to Core 1:
    xTaskCreatePinnedToCore(
            pulseTask,
            "PulseTask",
            4096,
            nullptr,
            1,                //priority
            &pulseTaskHandle,
            1                 // run on core 1
    );
}



void loop() {
    static int32_t   lastSpo2         = 0;
    static int32_t   lastHr           = 0;
    static unsigned long lastNotify = 0;
    //static unsigned long lastSDRecord = 0;

    //Get current step count (motionTask updates this in background)
    int steps = getStepCount();


    //If pulseTask just finished a read, print exactly once:
    if (g_pulseUpdated) {
        g_pulseUpdated = false;  // clear immediately

        if (g_validPulse) {
            // Grab the new values and print:
            lastSpo2 = g_spo2;
            lastHr   = g_hr;
            Serial.printf("HR: %ld bpm, SpO: %ld%%, Steps: %d\n",
                          lastHr, lastSpo2, steps);
        } else {
            Serial.println("Invalid SpO/HR, retrying...");
        }
    }

    // 3) BLE‐notify
    unsigned long now = millis();
    if(isConnected() && now - lastNotify >= BLE_TIMEOUT){
        lastNotify = now;
        notifyPlx(lastSpo2, lastHr);
        notifySteps(steps);
        Serial.println("BLE notified all");
    }

//    now = millis();
//    if(now - lastSDRecord >= SD_TIMEOUT){
//        //HERE SAVE DATA TO SD CARD, ONCE EVERY SD_TIMEOUT
//    }



    delay(10);
}
