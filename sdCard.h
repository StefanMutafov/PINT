
#ifndef SDCARD_H
#define SDCARD_H



void RTC_setup();
void SD_setup();
String get_timestamp();

String getDate(const String& timestamp);
//function to delete the oldest file on the SD card
void delete_file();
// Function to return the data in the specific file as a json string
String read_file(const String& fileName);
// Function to return the all the files stored in sd card as array
String* read_file_list();
// Function to write data (with timestamp) into the file
bool write_file(int bpm, int oxy, int step, bool isworkingout);


#endif //SDCARD_H
