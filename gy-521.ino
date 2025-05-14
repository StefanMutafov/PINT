#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"


MAX30105 particleSensor;


long lastBeat = 0;
float beatsPerMinute = 0;
int beatAvg = 0;


void setup() {
 Serial.begin(115200);
 delay(1000);


 Wire.begin(21, 22); // SDA, SCL


 Serial.println("Initializing MAX30102...");


 if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
   Serial.println("MAX30102 not found. Check wiring!");
   while (1);
 }


 // Sensor configuration
 particleSensor.setup();
 particleSensor.setPulseAmplitudeRed(0x0A);   // Turn Red LED to low brightness
 particleSensor.setPulseAmplitudeIR(0x0A);    // Turn IR LED to low brightness
 particleSensor.setPulseAmplitudeGreen(0);    // Turn off Green LED




}


void loop() {
 long irValue = particleSensor.getIR();
 long redValue = particleSensor.getRed();


 // Heartbeat detection
 if (checkForBeat(irValue)) {
   long delta = millis() - lastBeat;
   lastBeat = millis();


   float bpm = 60 / (delta / 1000.0);
   if (bpm < 255 && bpm > 20) {
     beatsPerMinute = bpm;
     beatAvg = (beatAvg * 3 + (int)beatsPerMinute) / 4;
   }
 }


 // Always print values
 Serial.print("IR: ");
 Serial.print(irValue);
 Serial.print("  Red: ");
 Serial.print(redValue);
 Serial.print("  BPM: ");
 Serial.print(beatsPerMinute, 1);  // 1 decimal place
 Serial.print("  Avg BPM: ");
 Serial.println(beatAvg);


 delay(50);
}
