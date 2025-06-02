#ifndef FILE_STORAGE_H
#define FILE_STORAGE_H

#include <Arduino.h>

// Call this once in setup() to initialize the SPI bus, mount the SD card, and start the RTC.
bool initFileStorage();

// Call this in setup() (after initFileStorage) to start the RTC module.
void RTC_setup();

// Returns a timestamp string in the format "YYYY/MM/DDThh:mm:ss".
String get_timestamp();

// Returns an array of all filenames (as String) in the root of the SD card.
// The returned array is terminated by an empty String ("").  Caller must delete[] it.
String* read_file_list();

// Reads the entire contents of the given file (on SD) and returns it as a String.
// If opening fails, returns an empty String and prints an error to Serial.
String read_file(const String& fileName);

// Deletes every non‐directory file in the root of the SD card and prints each one to Serial.
void delete_file();

// Appends one JSON object (with bpm/oxy/step/isWorkingOut + timestamp + workout_id)
// into a “YYYYMMDD.json” file on the SD card.  Returns true on success, false otherwise.
bool write_file(int bpm, int oxy, int step, bool isWorkingOut);

#endif // FILE_STORAGE_H
