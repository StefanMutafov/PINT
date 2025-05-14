#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "MPU6050.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer   *pServer;
BLECharacteristic *pPLXCharacteristic;  // PLX Continuous Measurement Characteristic
bool          deviceConnected = false;

#define O2_SERVICE_UUID         "00001822-0000-1000-8000-00805F9B34FB"
#define PLX_CHARACTERISTIC_UUID "00002A5F-0000-1000-8000-00805F9B34FB"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        deviceConnected = true;
    }
    void onDisconnect(BLEServer* pServer) override {
        deviceConnected = false;
        pServer->getAdvertising()->start();  // restart advertising
    }
};

MAX30105 particleSensor;

uint32_t irBuffer[100];
uint32_t redBuffer[100];
int32_t  bufferLength = 100;
int32_t  spo2;
int8_t   validSPO2;
int32_t  heartRate;
int8_t   validHeartRate;
unsigned long prevTime = 0;

// Convert a 32-bit int into a 16-bit IEEE-11073 SFLOAT (exponent=0)
static uint16_t intToSfloat(int32_t v) {
    int16_t mant = constrain(v, -2048, 2047);
    return (uint16_t)(mant & 0x0FFF);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);

    // --- MAX30105 init (unchanged) ---
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println(F("MAX30102 not found. Check wiring."));
        while (1) delay(1000);
    }
    particleSensor.setup(
            /*ledBrightness=*/65,
            /*sampleAverage=*/32,
            /*ledMode=*/2,
            /*sampleRate=*/400,
            /*pulseWidth=*/411,
            /*adcRange=*/8192
    );

    // --- BLE init/replacement ---
    BLEDevice::init("ESP32 Fitness Band");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pO2Service = pServer->createService(O2_SERVICE_UUID);

    pPLXCharacteristic = pO2Service->createCharacteristic(
            PLX_CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_NOTIFY
    );
    pPLXCharacteristic->addDescriptor(new BLE2902());

    pO2Service->start();
    pServer->getAdvertising()->start();
    Serial.println("BLE Ready!");
    bufferLength = 100;
    for (byte i = 0; i < bufferLength; i++) {
        while (!particleSensor.available())
            particleSensor.check();
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i]  = particleSensor.getIR();
        particleSensor.nextSample();
    }
    maxim_heart_rate_and_oxygen_saturation(
            irBuffer, bufferLength,
            redBuffer, &spo2, &validSPO2,
            &heartRate, &validHeartRate
    );
}

void loop() {
        // shift buffer
        for (byte i = 25; i < 100; i++) {
            redBuffer[i - 25] = redBuffer[i];
            irBuffer[i - 25]  = irBuffer[i];
        }
        // refill
        for (byte i = 75; i < 100; i++) {
            while (!particleSensor.available())
                particleSensor.check();
            redBuffer[i] = particleSensor.getRed();
            irBuffer[i]  = particleSensor.getIR();
            particleSensor.nextSample();
        }
        maxim_heart_rate_and_oxygen_saturation(
                irBuffer, bufferLength,
                redBuffer, &spo2, &validSPO2,
                &heartRate, &validHeartRate
        );

        Serial.print("Heart Rate: ");
        Serial.print(heartRate);
        Serial.print(" bpm, SPO2: ");
        Serial.print(spo2);
        Serial.println("%");

        if (validSPO2 != 1 || validHeartRate != 1) {
            Serial.println("Invalid values, retrying...");
        }else {

            // --- BLE notify logic (modified) ---
            unsigned long cTime = millis();
            if (deviceConnected && cTime - prevTime >= 1000) {
                prevTime = cTime;

                uint8_t plxData[5];
                plxData[0] = 0x00;  // Flags = 0

                uint16_t s_spo2 = intToSfloat(spo2);
                uint16_t s_hr = intToSfloat(heartRate);

                plxData[1] = s_spo2 & 0xFF;
                plxData[2] = (s_spo2 >> 8) & 0xFF;
                plxData[3] = s_hr & 0xFF;
                plxData[4] = (s_hr >> 8) & 0xFF;

                pPLXCharacteristic->setValue(plxData, sizeof(plxData));
                pPLXCharacteristic->notify();
                Serial.println("Notified new measurement");
            }
        }
}
