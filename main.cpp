#include <Arduino.h>
#include "PulseOxy.h"
#include "BLEPlx.h"
#include "MotionSensor.h"
#include "sdCard.h"
#include <Wire.h>

#define BLE_TIMEOUT 5000
#define SD_TIMEOUT 10000
#define BT_SEND_TIMEOUT 60000

#define BUTTON_PIN 0 // Button for starting and stopping session

//   Shared     globals for SpO2/HR (written by pulseTask)
//   g_spo2/g_hr hold whatever the last valid reading was.
//   g_validPulse tells us if that reading was valid.
//   g_pulseUpdated flips to true exactly when pulseTask finishes a read.
volatile int32_t g_spo2 = 0;
volatile int32_t g_hr = 0;
volatile bool g_validPulse = false;
volatile bool g_pulseUpdated = false;
volatile bool g_sendInitiated = false;

bool inSession = false; // True is the user is in a training session
String lastDate = "";  // holds YYYY_MM_DD of the file we last saw

TaskHandle_t motionTaskHandle = nullptr;
TaskHandle_t pulseTaskHandle = nullptr;

// Detect button presses
bool buttonPressed() {
    static int lastRaw = HIGH;   // last raw reading
    static int debounced = HIGH;   // last accepted state
    static unsigned long lastDebounce = 0;      // when raw last changed
    const unsigned long DEBOUNCE_DELAY = 50;

    int raw = digitalRead(BUTTON_PIN);

    if (raw != lastRaw) {
        lastDebounce = millis();
    }

    if (millis() - lastDebounce > DEBOUNCE_DELAY) {
        if (raw != debounced) {
            debounced = raw;

            if (debounced == LOW) {
                lastRaw = raw;
                return true;
            }
        }
    }

    lastRaw = raw;
    return false;
}

//motionTask
void motionTask(void *pv) {
    while (true) {
        updateMotion();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

//pulseTask
void pulseTask(void *pv) {
    while (true) {
        int32_t spo2, hr;
        bool okSpO2, okHR;

        if (readPulseOxy(spo2, okSpO2, hr, okHR)) {
            g_spo2 = spo2;
            g_hr = hr;
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

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    SD_setup();
    delay(1000);
    Wire.begin(21, 22);
    delay(1000);
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
    static unsigned long lastSDRecord = 0;
    static unsigned long lastSendTry = 0;
    String today = getDate(get_timestamp());
    unsigned long now = millis();

    if (g_sendInitiated && now - lastSendTry >= BT_SEND_TIMEOUT) {
        if (isConnected()) {
           // sendFileOverBLE();
            g_sendInitiated = false;
        } else {
            Serial.println("No BLE client connected—cannot send file.");
        }
    }




    String* files = read_file_list();
    int count = 0;
    while (files[count].length() > 0) {
        count++;
    }
    if (count > 1 && isConnected()) {
        String filepath = "/" + files[0];
       // sendFileOverBLE();
        delete_file();
    }
    delete[] files;



    //Get current step count
    int steps = getStepCount();


    if (buttonPressed()) {
        if (inSession) {
            inSession = false;
            Serial.printf("Session ended!");
            g_sendInitiated = true;
        } else {
            inSession = true;
            Serial.println("Session started!");
        }
    }

    //If pulseTask just finished a read, print
    if (g_pulseUpdated) {
        g_pulseUpdated = false;  // clear the flag

        if (g_validPulse) {

            Serial.printf("HR: %ld bpm, SpO2: %ld%%, Steps: %d\n",
                          (long) g_hr, (long) g_spo2, steps);
        } else {
            Serial.println("Invalid SpO2/HR, retrying...");
        }
    }

    //BLE‐notify every BLE_TIMEOUT ms, but only if the last pulse was valid

    if (g_validPulse && (now - lastNotifyTime >= BLE_TIMEOUT)) {
        lastNotifyTime = now;
        if (isConnected()) {
            notifyPlx(g_spo2, g_hr);
            notifySteps(steps);
            Serial.println("BLE notified all");
        }
    }

    now = millis();
    if (now - lastSDRecord >= SD_TIMEOUT) {
        lastSDRecord = now;
        write_file(g_hr, g_spo2, steps, inSession);
    }

    delay(10);
}
