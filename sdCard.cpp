#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "sdCard.h"
#include <Wire.h>
#define CS_PIN 15
#define SPI_MOSI 13
#define SPI_MISO 12
#define SPI_SCK  17
#define DS3231_ADDRESS 0x68
SPIClass spi(HSPI);
static bool wasWorkingout = false;
static String workoutID = "";

void SD_setup(){
    spi.begin(SPI_SCK, SPI_MISO, SPI_MOSI, CS_PIN);
    if (!SD.begin(CS_PIN, spi)) {
        Serial.println("SD Card Mount Failed!");
        return;
    }
    Serial.println("SD Card Initialized.");
}
byte bcdToDec(byte val) {
  return ( (val / 16 * 10) + (val % 16) );
}
String get_timestamp() {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write(0); // Set DS3231 register pointer to 00h (seconds register)
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 7);
  if (Wire.available() < 7) {
    return String("Error: No data");
  }

  byte rawSeconds = Wire.read();
  byte rawMinutes = Wire.read();
  byte rawHours = Wire.read();
  Wire.read(); // Day of week, ignore
  byte rawDay = Wire.read();
  byte rawMonth = Wire.read();
  byte rawYear = Wire.read();

  int year = 2000 + bcdToDec(rawYear);
  int month = bcdToDec(rawMonth & 0x1F); // Mask century bit
  int day = bcdToDec(rawDay);
  int hour = bcdToDec(rawHours & 0x3F); // 24-hour format mask
  int minute = bcdToDec(rawMinutes);
  int second = bcdToDec(rawSeconds);

  String timestamp = String(year) + '_' +
                     (month < 10 ? "0" : "") + String(month) + '_' +
                     (day < 10 ? "0" : "") + String(day) + 'T' +
                     (hour < 10 ? "0" : "") + String(hour) + ':' +
                     (minute < 10 ? "0" : "") + String(minute) + ':' +
                     (second < 10 ? "0" : "") + String(second);

  return timestamp;
}
void RTC_setup() {
  Wire1.begin(21, 22); // Initialize I2C bus on pins 25 (SDA) and 26 (SCL)
}

String getDate(String timestamp){
    return timestamp.substring(0, 10);
}
// function to delete all the files in the sd card
void delete_file() {
    File root = SD.open("/");
    File entry = root.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            String fileName = entry.name();
            entry.close();
            SD.remove(fileName);
            Serial.println(String("Deleted: ") + fileName);
        } else {
            entry.close();
        }
        entry = root.openNextFile();
    }
    root.close();
}
// Function to return the data in the specific file as a json string
String read_file(String fileName) {
    Serial.println("Filename: (in read_file) " + fileName);
    File file = SD.open(fileName, FILE_READ);
    String jsonStr = "";

    if (file) {
        while (file.available()) {
            jsonStr += (char)file.read();
        }
        file.close();
    } else {
        Serial.println("Failed to open file for reading.");
    }
    Serial.flush();
    return jsonStr;
}
// Function to return the all the files stored in sd card as array
String* read_file_list() {
    static String fileNames[50]; // Assuming max 50 files
    int index = 0;

    File root = SD.open("/");
    File entry = root.openNextFile();
    while (entry && index < 50) {
        if (!entry.isDirectory()) {
            fileNames[index] = String(entry.name());
            index++;
        }
        entry.close();
        entry = root.openNextFile();
    }
    root.close();

    String* result = new String[index + 1];
    for (int i = 0; i < index; i++) {
        result[i] = fileNames[i];
    }
    result[index] = "";
    return result;
}



// Function to write data (with timestamp) into the file
bool write_file(int bpm, int oxy, int step, bool isworkingout) {
    String timestamp = get_timestamp();
    Serial.println("Timestamp: " +timestamp);
    if (!wasWorkingout && isworkingout) {
        wasWorkingout = true;
        workoutID = timestamp;
    } else if (wasWorkingout && !isworkingout) {
        wasWorkingout = false;
        workoutID = "";
    }

    String fileName = "/" + getDate(timestamp) + ".json";

    // Read existing JSON content
    String jsonStr = read_file(fileName);
    StaticJsonDocument<4096> doc;
    JsonArray dataArray;

    // Parse or initialize
    if (jsonStr.length() == 0 || deserializeJson(doc, jsonStr) != DeserializationError::Ok) {
        doc["device_id"] = "12345";
        dataArray = doc.createNestedArray("data");
    } else {
        dataArray = doc["data"].as<JsonArray>();
        if (dataArray.isNull()) {
            dataArray = doc.createNestedArray("data");
        }
    }

    // Add new data
    JsonObject entry = dataArray.createNestedObject();
    if (isworkingout){
        entry["workout_id"] = workoutID;
    }
    else{
        entry["workout_id"] = NULL;
    }

    entry["timestamp"] = timestamp;
    entry["heart_rate"] = bpm;
    entry["oxygen_saturation"] = oxy;
    entry["step_count"] = step;

    // Overwrite with updated JSON
    Serial.println("Filename: (in write_file)" + fileName);
    File file = SD.open(fileName, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing.");
        return false;
    }
    serializeJson(doc, file);
    file.close();
    Serial.println("Appended data to " + fileName);
    return true;
}