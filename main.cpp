#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "MPU6050.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer;
BLECharacteristic *pPLXCharacteristic;  // PLX Continuous Measurement Characteristic

bool deviceConnected = false;

#define O2_SERVICE_UUID        "00001822-0000-1000-8000-00805F9B34FB" // Pulse Oximeter Service (SPO2 + HR)
#define PLX_CHARACTERISTIC_UUID "00002A5F-0000-1000-8000-00805F9B34FB" // PLX Continuous Measurement

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

MAX30105 particleSensor;
MPU6050 mpu;

#define MAX_BRIGHTNESS 255

uint32_t irBuffer[100];
uint32_t redBuffer[100];

int32_t bufferLength = 100;
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

//int32_t lastSPO2 = -1;
//int32_t lastHeartRate = -1;
unsigned long prevTime = 0;

void setup() {
    Serial.begin(115200);

    Wire.begin(21, 22);

    // Initialize MAX30102
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println(F("MAX30102 not found. Check wiring."));
        return;
    }

    // MAX30102 Sensor settings
    byte ledBrightness = 65;
    byte sampleAverage = 32;
    byte ledMode = 2;
    int sampleRate = 400;
    int pulseWidth = 411;
    int adcRange = 8192;

    particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

    BLEDevice::init("ESP32 Fitness Band");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Pulse Oximeter Service for SPO2 + HR
    BLEService *pO2Service = pServer->createService(O2_SERVICE_UUID);

    // PLX Continuous Measurement Characteristic (for both SPO2 and HR)
    pPLXCharacteristic = pO2Service->createCharacteristic(
            PLX_CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pPLXCharacteristic->addDescriptor(new BLE2902());

    pO2Service->start();

    pServer->getAdvertising()->start();
    Serial.println("BLE Ready!");
}

void loop() {
    bufferLength = 100;

    for (byte i = 0; i < bufferLength; i++) {
        while (!particleSensor.available())
            particleSensor.check();

        redBuffer[i] = particleSensor.getRed();
        irBuffer[i] = particleSensor.getIR();
        particleSensor.nextSample();
    }

    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

    while (true) {
        for (byte i = 25; i < 100; i++) {
            redBuffer[i - 25] = redBuffer[i];
            irBuffer[i - 25] = irBuffer[i];
        }

        for (byte i = 75; i < 100; i++) {
            while (!particleSensor.available())
                particleSensor.check();

            redBuffer[i] = particleSensor.getRed();
            irBuffer[i] = particleSensor.getIR();
            particleSensor.nextSample();
        }

        maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

        Serial.print("Heart Rate: ");
        Serial.print(heartRate);
        Serial.print(" bpm, SPO2: ");
        Serial.print(spo2);
        Serial.println("%");

        if (validSPO2 != 1 || validHeartRate != 1) {
            Serial.println("Invalid values, retrying...");
            continue;
        }

        unsigned long cTime = millis();

        if (deviceConnected && cTime-prevTime >= 1000) {
            prevTime = cTime;

            // Prepare data for PLX Continuous Measurement (SPO2 + HR)
            uint8_t plxData[5];  // 1 byte for flags, 2 bytes for SPO2 value, 2 bytes for Heart Rate
            plxData[0] = 0b00000000;  // Flags

            // Use the lower 16 bits of the 32-bit int32_t spo2 value
            uint16_t spo2Value = (uint16_t)(spo2 & 0xFFFF); // Extract lower 16 bits of spo2

            // Fill the 4-byte data structure (2 bytes for SpO₂, 2 bytes for HR)
            plxData[1] = (uint8_t)(spo2Value & 0xFF);         // Low byte of uint16_t for SPO2 (little-endian)
            plxData[2] = (uint8_t)((spo2Value >> 8) & 0xFF);  // High byte of uint16_t for SPO2 (little-endian)

            // Heart rate data (same format as SpO₂)
            uint16_t heartRateValue = (uint16_t)(heartRate & 0xFFFF);
            plxData[3] = (uint8_t)(heartRateValue & 0xFF);     // Low byte of uint16_t for HR
            plxData[4] = (uint8_t)((heartRateValue >> 8) & 0xFF);  // High byte of uint16_t for HR

            pPLXCharacteristic->setValue(plxData, sizeof(plxData));  // Set combined data
            pPLXCharacteristic->notify();  // Send data
        }

//        lastSPO2 = spo2;
//        lastHeartRate = heartRate;
    }
}
