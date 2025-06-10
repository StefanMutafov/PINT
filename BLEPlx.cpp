#include "BLEPlx.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <SD.h>
#include "sdCard.h"  // for read_file()


//   UUIDs for our services/characteristics
static constexpr char O2_SVC[]     = "00001822-0000-1000-8000-00805F9B34FB";
static constexpr char PLX_CHR[]    = "00002A5F-0000-1000-8000-00805F9B34FB";
static constexpr char STEP_SVC[]   = "0000FF10-0000-1000-8000-00805F9B34FB";
static constexpr char STEP_CHR[]   = "0000FF11-0000-1000-8000-00805F9B34FB";
static constexpr char FILE_SVC[]   = "12345678-1234-5678-1234-56789ABCDEF0";
static constexpr char FILE_CHR[]   = "ABCDEF01-2345-6789-ABCD-EF0123456789";

static BLEServer*         server    = nullptr;
static BLECharacteristic* plxChar   = nullptr;
static BLECharacteristic* stepChar  = nullptr;
static BLECharacteristic* fileChar  = nullptr;
static volatile bool      connected = false;


//   Convert 32‐bit int → 16‐bit IEEE‐11073 SFLOAT (12‐bit mantissa, exp=0)
static uint16_t intToSfloat(int32_t v) {
    int16_t m = constrain(v, -2048, 2047);
    return (uint16_t)(m & 0x0FFF);
}


//   BLE Server callbacks: track overall connection status
class SrvCB : public BLEServerCallbacks {
    void onConnect(BLEServer* srv) override {
        connected = true;
    }
    void onDisconnect(BLEServer* srv) override {
        connected = false;
        srv->getAdvertising()->start();
    }
};


void initBLE() {
    BLEDevice::init("ESP32 Fitness Band");
    server = BLEDevice::createServer();
    server->setCallbacks(new SrvCB());

    //PLX Service
    auto svc1 = server->createService(O2_SVC);
    plxChar = svc1->createCharacteristic(
            PLX_CHR,
            BLECharacteristic::PROPERTY_NOTIFY
    );
    plxChar->addDescriptor(new BLE2902());
    svc1->start();

    //Steps Service
    auto svc2 = server->createService(STEP_SVC);
    stepChar = svc2->createCharacteristic(
            STEP_CHR,
            BLECharacteristic::PROPERTY_NOTIFY
    );
    stepChar->addDescriptor(new BLE2902());
    svc2->start();

    //File Transfer Service
    auto svc3 = server->createService(FILE_SVC);
    fileChar = svc3->createCharacteristic(
            FILE_CHR,
            BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
    );
    fileChar->addDescriptor(new BLE2902());
    svc3->start();

    server->getAdvertising()->start();
    Serial.println("BLE Ready (PLX + Steps + File Transfer)");
}


//   isConnected(): true if any central is connected to our server
bool isConnected() {
    return connected;
}


//   notifyPlx(): pack SPO2 + HR into 5 bytes and notify
void notifyPlx(int32_t spo2, int32_t hr) {
    uint8_t buf[5];
    buf[0] = 0x00;
    uint16_t s_spo2 = intToSfloat(spo2);
    uint16_t s_hr   = intToSfloat(hr);
    buf[1] = s_spo2 & 0xFF;
    buf[2] = (s_spo2 >> 8) & 0xFF;
    buf[3] = s_hr   & 0xFF;
    buf[4] = (s_hr   >> 8) & 0xFF;
    plxChar->setValue(buf, sizeof(buf));
    plxChar->notify();
}


//   notifySteps(): pack stepCount into 2 bytes and notify
void notifySteps(int stepCount) {
    uint8_t buf[2];
    buf[0] = stepCount & 0xFF;
    buf[1] = (stepCount >> 8) & 0xFF;
    stepChar->setValue(buf, sizeof(buf));
    stepChar->notify();
}


//   sendFileOverBLE(): reads the JSON from SD (via read_file),
//   breaks it into chunks, and sends each chunk with notify().
//   At the end sends a zero‐length notify() as EOF.
void sendFileOverBLE() {
    if (!isConnected()) return;

    // 1) Get the single JSON filename
    String* files = read_file_list();
    if (files[0].isEmpty()) return;
    String filename = "/" + files[0];

    // 2) Read its contents
    String jsonStr = read_file(filename);
    if (jsonStr.isEmpty()) return;

    // 3) Stream in fixed‐size chunks directly from the String’s buffer
    const char* data = jsonStr.c_str();
    Serial.println(data);
    size_t totalLen = jsonStr.length();
    const size_t CHUNK_SIZE = 120;

    for (size_t offset = 0; offset < totalLen; offset += CHUNK_SIZE) {
        size_t len = min(CHUNK_SIZE, totalLen - offset);
        fileChar->setValue((uint8_t*)(data + offset), len);

        fileChar->notify();
        delay(10);
    }

    // 4) Send zero‐length packet as EOF
    fileChar->setValue(nullptr, 0);
    fileChar->notify();
    Serial.println("sendFileOverBLE: EOF marker sent");
}
