#include "FileStorage.h"
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <uRTCLib.h>


#define CS_PIN    15
#define SPI_MOSI  13
#define SPI_MISO  12
#define SPI_SCK   17


SPIClass spi(HSPI);
uRTCLib rtc(0x68);

// Keeps track of whether we were “in a workout” on the last write.
static bool wasWorkingout = false;
static String workoutID   = "";


// initFileStorage()
//   Initializes the SPI bus & SD card.  Returns true if SD mounted OK, false otherwise.
bool initFileStorage() {
    spi.begin(SPI_SCK, SPI_MISO, SPI_MOSI, CS_PIN);
    if (!SD.begin(CS_PIN, spi)) {
        Serial.println("SD Card Mount Failed!");
        return false;
    }
    Serial.println("SD Card Initialized.");
    return true;
}


//   Starts the I2C bus for the RTC.  Must be called once before using get_timestamp().
void RTC_setup() {
    URTCLIB_WIRE.begin();
}


//   Returns “YYYY/MM/DDThh:mm:ss” from the RTC.
String get_timestamp() {
    rtc.refresh();
    String timestamp = String(rtc.year()) + '/' +
                       ((rtc.month() < 10) ? "0" : "") + String(rtc.month()) + '/' +
                       ((rtc.day()   < 10) ? "0" : "") + String(rtc.day())   + 'T' +
                       ((rtc.hour()  < 10) ? "0" : "") + String(rtc.hour())  + ':' +
                       ((rtc.minute()< 10) ? "0" : "") + String(rtc.minute())+ ':' +
                       ((rtc.second()< 10) ? "0" : "") + String(rtc.second());
    return timestamp;
}


//   (Unused in write_file, but provided in case you want to look up filenames.)
int isFileExisting(String fileName, String* fileList) {
    int i = 0;
    while (fileList[i] != "") {  // Stop at sentinel
        if (fileName == fileList[i]) {
            return i;  // Found at index i
        }
        i++;
    }
    return -1;  // Not found
}


//   Given a timestamp “YYYY/MM/DDThh:mm:ss”, returns “YYYYMMDD”.
String getDate(const String& timestamp) {
    // timestamp[0..3] = year, [5..6]=month, [8..9]=day
    return timestamp.substring(0,4)
           + timestamp.substring(5,7)
           + timestamp.substring(8,10);
}


//   Deletes every non‐directory entry in the root of the SD card.
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


//   Reads the entire file contents into a String.  If open fails, returns "".
String read_file(const String& fileName) {
    File file = SD.open(fileName, FILE_READ);
    String jsonStr = "";
    if (file) {
        while (file.available()) {
            jsonStr += (char)file.read();
        }
        file.close();
    } else {
        Serial.println("Failed to open file for reading: " + fileName);
    }
    return jsonStr;
}


//   Returns a dynamically allocated String array of all filenames (max 50).
//   Last element is "" as sentinel.  Caller must delete[] the returned pointer.
String* read_file_list() {
    static String fileNames[50]; // temp storage
    int index = 0;

    File root = SD.open("/");
    File entry = root.openNextFile();
    while (entry && index < 50) {
        if (!entry.isDirectory()) {
            fileNames[index++] = String(entry.name());
        }
        entry.close();
        entry = root.openNextFile();
    }
    root.close();

    String* result = new String[index + 1];
    for (int i = 0; i < index; i++) {
        result[i] = fileNames[i];
    }
    result[index] = "";  // sentinel
    return result;
}


//   Appends one JSON object into "<YYYYMMDD>.json" on SD.  Creates file if needed.
//   Each entry has: workout_id ("" if not in workout), timestamp, heart_rate, oxygen_saturation, step_count.
//   Returns true on success, false on failure.
bool write_file(int bpm, int oxy, int step, bool isWorkingOut) {
    String timestamp = get_timestamp();

    // Manage workoutID state
    if (!wasWorkingout && isWorkingOut) {
        wasWorkingout = true;
        workoutID = timestamp;
    } else if (wasWorkingout && !isWorkingOut) {
        wasWorkingout = false;
        workoutID = "";
    }

    String fileName = "/" + getDate(timestamp) + ".json";

    // Read existing JSON content (if any)
    String jsonStr = read_file(fileName);
    StaticJsonDocument<4096> doc;
    JsonArray dataArray;

    // If file is empty or invalid JSON, start a new document
    if (jsonStr.length() == 0 || deserializeJson(doc, jsonStr) != DeserializationError::Ok) {
        doc["device_id"] = "12345";  // ← change if you want a dynamic device_id
        dataArray = doc.createNestedArray("data");
    } else {
        dataArray = doc["data"].as<JsonArray>();
        if (dataArray.isNull()) {
            dataArray = doc.createNestedArray("data");
        }
    }

    // Append the new entry
    JsonObject entry = dataArray.createNestedObject();
    entry["workout_id"]       = workoutID;
    entry["timestamp"]        = timestamp;
    entry["heart_rate"]       = bpm;
    entry["oxygen_saturation"] = oxy;
    entry["step_count"]       = step;

    // Overwrite the file with updated JSON
    File file = SD.open(fileName, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing: " + fileName);
        return false;
    }
    serializeJson(doc, file);
    file.close();

    Serial.println("Appended data to " + fileName);
    return true;
}
