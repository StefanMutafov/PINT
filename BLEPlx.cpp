#include "BLEPlx.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUIDs
static constexpr char O2_SVC[]    = "00001822-0000-1000-8000-00805F9B34FB";
static constexpr char PLX_CHR[]   = "00002A5F-0000-1000-8000-00805F9B34FB";
static constexpr char STEP_SVC[]  = "0000FF10-0000-1000-8000-00805F9B34FB";
static constexpr char STEP_CHR[]  = "0000FF11-0000-1000-8000-00805F9B34FB";

static BLEServer*        server;
static BLECharacteristic* plxChar;
static BLECharacteristic* stepChar;
static volatile bool     connected = false;

// pack int → IEEE-11073 SFLOAT (12-bit mantissa, exp=0)
static uint16_t intToSfloat(int32_t v) {
    int16_t m = constrain(v, -2048, 2047);
    return (uint16_t)(m & 0x0FFF);
}

class SrvCB : public BLEServerCallbacks {
    void onConnect(BLEServer* srv)    override { connected = true;  }
    void onDisconnect(BLEServer* srv) override {
        connected = false;
        srv->getAdvertising()->start();
    }
};

void initBLE() {
    BLEDevice::init("ESP32 Fitness Band");
    server = BLEDevice::createServer();
    server->setCallbacks(new SrvCB());

    // PLX service
    auto svc1 = server->createService(O2_SVC);
    plxChar = svc1->createCharacteristic(
            PLX_CHR, BLECharacteristic::PROPERTY_NOTIFY
    );
    plxChar->addDescriptor(new BLE2902());
    svc1->start();

    // Step-count service
    auto svc2 = server->createService(STEP_SVC);
    stepChar = svc2->createCharacteristic(
            STEP_CHR,
            BLECharacteristic::PROPERTY_NOTIFY
    );
    stepChar->addDescriptor(new BLE2902());
    svc2->start();

    server->getAdvertising()->start();
    Serial.println("BLE Ready (PLX + Steps)");
}

bool isConnected() {
    return connected;
}

void notifyPlx(int32_t spo2, int32_t hr) {
    uint8_t buf[5];
    buf[0] = 0x00;
    uint16_t s_spo2 = intToSfloat(spo2);
    uint16_t s_hr   = intToSfloat(hr);
    buf[1] = s_spo2 & 0xFF;
    buf[2] = (s_spo2>>8) & 0xFF;
    buf[3] = s_hr   & 0xFF;
    buf[4] = (s_hr  >>8) & 0xFF;
    plxChar->setValue(buf, sizeof(buf));
    plxChar->notify();
}

void notifySteps(int stepCount) {
    uint8_t buf[2];
    buf[0] = stepCount & 0xFF;
    buf[1] = (stepCount>>8) & 0xFF;
    stepChar->setValue(buf, sizeof(buf));
    stepChar->notify();
}
