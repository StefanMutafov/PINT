#include <Arduino.h>
#include "PulseOxy.h"
#include "BLEPlx.h"

#include "MotionSensor.h"
#include "sdCard.h"
#include <Wire.h>

#include <Arduino.h>
#include "PulseOxy.h"
#include "BLEPlx.h"
#include "MotionSensor.h"
#include "sdCard.h"
#include "GUI_Controller.h"
#include "InputController.h"
#include <Wire.h>

#define BLE_TIMEOUT 5000 //Timeout for sending live data through BLE
#define SD_TIMEOUT 10000 //Timeout for saving data on SD
#define BT_SEND_TIMEOUT 60000 //Timeout for sending files through BLE


#define BLE_TIMEOUT 5000 //Timeout for sending live data through BLE
#define SD_TIMEOUT 10000 //Timeout for saving data on SD
#define BT_SEND_TIMEOUT 60000 //Timeout for sending files through BLE

#define BUTTON_PIN 0 // Button for starting and stopping session

//   Shared globals for SpO2/HR (written by pulseTask)
//   g_spo2/g_hr hold whatever the last valid reading was.
//   g_validPulse tells us if that reading was valid.
//   g_pulseUpdated flips to true exactly when pulseTask finishes a read.
volatile int32_t g_spo2 = 0;
volatile int32_t g_hr = 0;
volatile bool g_validPulse = false;
volatile bool g_pulseUpdated = false;
volatile bool g_sendInitiated = false; //Flag for sending files through BLE

bool inSession = false; // True is the user is in a training session

TaskHandle_t motionTaskHandle = nullptr;
TaskHandle_t pulseTaskHandle = nullptr;

// global GUI controller and ninput controller
GUI_Controller GUI{};
InputController input{};

// Detect button presses
//Used for staring and ending workout
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
    GUI.init();
    GUI.draw_startup();
    input.init();
    Wire.begin(21, 22);
    initPulseOxy();
    initMotion();
    initBLE();

    // Create motionTask on Core 1(Main loop on Core 0)
    xTaskCreatePinnedToCore(
            motionTask,
            "MotionTask",
            4096,
            nullptr,
            1,                // priority 1
            &motionTaskHandle,
            1                 // run on core 1
    );

    // Create pulseTask on Core 1(Main loop on core 0)
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
    unsigned long now = millis();

    ///
    ///TODO:
    ///Add fall_detected variable and change it in update_motion(), so that instead of a print, the variable is set to true
    /// If fall_detected is true, play sound untill button pressed
    /// Add GUI
    ///
    
    // handle the inputs for the GUI
    static unsigned long idle_timer = 0;
    if (input.L_pressed())
    {
        idle_timer = millis();
        GUI.select_button();
    }
    else if (input.R_pressed())
    {
        idle_timer = millis();
        GUI.execute_button();
    }
    if (now - idle_timer > 5000)
    {
        GUI.go_idle();
    }

    GUI.draw();

    static unsigned long lastNotifyTime = 0; //Last time BLE notify
    static unsigned long lastSDRecord = 0; //Last SD-card write
    static unsigned long lastSendTry = 0; //Last BLE file transfer attempt
    String timestamp = get_timestamp();
    String today = getDate(timestamp);
    

    //If g_sendInitiated flag is true, try to send the oldest file in SD
    if (g_sendInitiated && now - lastSendTry >= BT_SEND_TIMEOUT) {
        lastSendTry = now;
        if (isConnected()) {
            sendFileOverBLE();
            g_sendInitiated = false;
        } else {
            //Test print, can be removed
            Serial.println("No BLE client connected—cannot send file.");
        }
    }



    //Files on the sd card
    String* files = read_file_list();
    int count = 0;
    while (files[count].length() > 0) {
        count++;
    }
    if (count > 1 && isConnected()) {
        String filepath = "/" + files[0];
        sendFileOverBLE();
        delete_file();
    }



    //Get current step count
    int steps = getStepCount();

    //If button is presses, start or stop session and set send flag true if needed
    /*if (buttonPressed()) {
        if (inSession) {
            inSession = false;
            Serial.printf("Session ended!");
            g_sendInitiated = true;
        } else {
            inSession = true;
            Serial.println("Session started!");
        }
    }*/

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
    if (now - lastNotifyTime >= BLE_TIMEOUT) {
        lastNotifyTime = now;
        if (isConnected()) {
            notifyPlx(g_spo2, g_hr);
            notifySteps(steps);
            Serial.println("BLE notified all");
        }
    }

    now = millis();
    //Write data to SD card
    if (now - lastSDRecord >= SD_TIMEOUT) {
        lastSDRecord = now;
        Serial.printf("Writing to file: HR: %ld bpm, SpO2: %ld%%, Steps: %d\n",
                      (long) g_hr, (long) g_spo2, steps);
        write_file(g_hr, g_spo2, steps, inSession);
    }

    String clock = getClock(timestamp);
    GUI.update_values(clock, g_hr, g_spo2, steps);

    delay(10);
}
