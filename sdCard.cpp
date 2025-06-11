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
SPIClass hspi(HSPI);


void SD_setup(){
    hspi.begin(SPI_SCK, SPI_MISO, SPI_MOSI, CS_PIN);
    if (!SD.begin(CS_PIN, hspi)) {
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
        return {"Error: No data"};
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

String getDate(const String& timestamp){
    return timestamp.substring(0, 10);
}

String getClock(const String& timestamp)
{
    auto pos = timestamp.indexOf('T') + 1;
    return timestamp.substring(pos, pos+5);
}

// function to delete the oldest file on the SD card
void delete_file() {
    String* files = read_file_list();
    if (files[0].length() > 0) {
        String path = "/" + files[0];
        SD.remove(path);
    }

}

// Function to return the data in the specific file as a json string
String read_file(const String& fileName) {
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

//return a list of the files by chronological order
String* read_file_list() {
    static String fileNames[51];  // up to 50 names + sentinel
    int count = 0;

    File root = SD.open("/");
    if (!root || !root.isDirectory()) {
        fileNames[0] = "";
        return fileNames;
    }

    File entry;
    while ((entry = root.openNextFile()) && count < 50) {
        if (!entry.isDirectory()) {
            String name = entry.name();
            if (!name.startsWith("._") && !name.endsWith(".jpg")) {
                fileNames[count++] = name;
            }
        }
        entry.close();
    }
    root.close();

    // In‐place sort ascending
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (fileNames[j] < fileNames[i]) {
                String tmp = fileNames[i];
                fileNames[i] = fileNames[j];
                fileNames[j] = tmp;
            }
        }
    }

    fileNames[count] = "";  // sentinel
    return fileNames;
}




bool appendToJsonArrayFile(const String& fileName, JsonObject& newEntry) {
    // Try to open existing file for reading
    File file = SD.open(fileName, FILE_READ);
    size_t fileSize = file ? file.size() : 0;

    // If no file or too small to be a JSON array, create a new array with one entry
    if (!file || fileSize < 2) {
        if (file) {
            file.close();
        }
        DynamicJsonDocument doc(1024);
        JsonArray data = doc.to<JsonArray>();
        data.add(newEntry);

        File out = SD.open(fileName, FILE_WRITE);
        if (!out) {
            return false;
        }
        serializeJson(doc, out);
        out.close();
        return true;
    }

    // File exists and is at least "[ ]"
    // Read the character before the closing ']' to see if array is empty
    file.seek(fileSize - 2);
    char prevChar = file.read();
    file.close();

    // Open file for writing, seek to overwrite the closing ']'
    File out = SD.open(fileName, FILE_WRITE);
    if (!out) {
        return false;
    }
    out.seek(fileSize - 1); // position before ']'

    if (prevChar == '[') {
        // Empty array: write the new entry then closing bracket
        serializeJson(newEntry, out);
        out.print("]");
    } else {
        // Non-empty array: prepend comma, write entry, then closing bracket
        out.print(",");
        serializeJson(newEntry, out);
        out.print("]");
    }

    out.close();
    return true;
}


bool write_file(int bpm, int oxy, int step, bool isworkingout) {
    String timestamp = get_timestamp();
    Serial.println("Timestamp: " + timestamp);

    static bool wasWorkingout = false;
    static String workoutID = "";

    if (!wasWorkingout && isworkingout) {
        wasWorkingout = true;
        workoutID = timestamp;
    } else if (wasWorkingout && !isworkingout) {
        wasWorkingout = false;
        workoutID = "";
    }

    String fileName = "/" + getDate(timestamp) + ".json";
    DynamicJsonDocument entryDoc(256);
    JsonObject entry = entryDoc.to<JsonObject>();

    if (isworkingout) {
        entry["workout_id"] = workoutID;
    } else {
        entry["workout_id"] = NULL;
    }
    entry["timestamp"] = timestamp;
    entry["heart_rate"] = bpm;
    entry["oxygen_saturation"] = oxy;
    entry["step_count"] = step;

    bool success = appendToJsonArrayFile(fileName, entry);

    if (success) {
        Serial.println("Appended data to " + fileName);
    } else {
        Serial.println("Failed to append data.");
    }

    return success;
}