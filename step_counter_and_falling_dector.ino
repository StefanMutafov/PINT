#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

int16_t ax, ay, az;
int16_t gx, gy, gz;
int16_t gx_offset = 0, gy_offset = 0, gz_offset = 0;
unsigned long lastStepTime = 0;
unsigned long lastPrintTime = 0;
int stepCount = 0;
float lastMagnitude = 0;
const float stepThreshold = 1.2;
const int stepDelay = 350;
int printInterval = 2000;       
bool freeFallDetected = false;
bool stillnessTimerRunning =false;
int stillnessDuration = 2000;
unsigned long freeFallTime = 0;
unsigned long stillnessStartTime = 0;
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // SDA = GPIO21, SCL = GPIO22

  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU6050 successfully connected!");
  } else {
    Serial.println("MPU6050 connection failed!");
    return;
  }

  calibrateGyro();
  Serial.println("Starting to read sensor data...");
}

void loop() {
  
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  gx -= gx_offset;
  gy -= gy_offset;
  gz -= gz_offset;

  // Convert raw accelerometer values to Gs
  float ax_g = ax / 16384.0;
  float ay_g = ay / 16384.0;
  float az_g = az / 16384.0;

  // Convert raw gyroscope values to degrees per second
  float gx_dps = gx / 131.0;
  float gy_dps = gy / 131.0;
  float gz_dps = gz / 131.0;

  // Compute magnitude of acceleration vector
  float magnitude = sqrt(ax_g * ax_g + ay_g * ay_g + az_g * az_g);

/*  //debounce
  bool onVehicle = (magnitude > stepThreshold && gx_dps <20 && gy_dps <20&&gz_dps<20);
  unsigned long now = millis();
  if ((magnitude - lastMagnitude > 0.3) && magnitude > stepThreshold && (now - lastStepTime > stepDelay)&&(!onVehicle)){
    stepCount++;
    lastStepTime = now;
    Serial.print("Step detected! Total steps: ");
    Serial.println(stepCount);
  }
*/
  lastMagnitude = magnitude;
  checkFall(magnitude, gx_dps, gy_dps, gz_dps);
  /*// Print accelerometer and gyroscope data
  if(now - lastPrintTime >= printInterval){
    lastPrintTime = now;
    Serial.print("\nAccel (G): X=");
    Serial.print(ax_g, 2);
    Serial.print(" Y=");
    Serial.print(ay_g, 2);
    Serial.print(" Z=");
    Serial.print(az_g, 2);
    Serial.print(" | Mag=");
    Serial.print(magnitude, 2);
    

    Serial.print(" | Gyro (°/s): X=");
    Serial.print(gx_dps, 2);
    Serial.print(" Y=");
    Serial.print(gy_dps, 2);
    Serial.print(" Z=");
    Serial.println(gz_dps, 2);
  }
*/
  delay(100); //
}

void calibrateGyro() {
  int readings = 100;
  int32_t temp_gx = 0, temp_gy = 0, temp_gz = 0;

  for (int i = 0; i < readings; i++) {
    mpu.getRotation(&gx, &gy, &gz);
    temp_gx += gx;
    temp_gy += gy;
    temp_gz += gz;
    delay(10);
  }

  gx_offset = temp_gx / readings;
  gy_offset = temp_gy / readings;
  gz_offset = temp_gz / readings;

  Serial.print("Calibration complete! Offsets: ");
  Serial.print("X: ");
  Serial.print(gx_offset);
  Serial.print(" Y: ");
  Serial.print(gy_offset);
  Serial.print(" Z: ");
  Serial.println(gz_offset);
}

void checkFall(float magnitude, float gyroX, float gyroY, float gyroZ) {
  unsigned long now = millis();
  float angularVelocity = sqrt(gyroX * gyroX + gyroY * gyroY + gyroZ * gyroZ);
  /*
  Serial.print("agularVelocity: ");
  Serial.println(angularVelocity);
  Serial.print("Magnitude: ");
  Serial.println(magnitude);
  // Step 1: Detect Free Fall
  if (!freeFallDetected && magnitude < 0.5 && angularVelocity > 250) {
    freeFallDetected = true;
    freeFallTime = now;
    Serial.println("Free fall detected!");
  }
*/
  // Step 1: Detect free fall
  if (magnitude > 3.2 && angularVelocity > 300) {
    Serial.println("Potential fall detected!");
    stillnessTimerRunning = true;
    stillnessStartTime = now;
  }

  // Step 3: Monitor Stillness
  if (stillnessTimerRunning) {
    if (magnitude >= 0.8 && magnitude <= 1.2) {
      if (now - stillnessStartTime >= stillnessDuration) {
        Serial.println("Fall confirmed due to prolonged stillness!");
        freeFallDetected = false;
        stillnessTimerRunning = false;
      }
    } else {
      // Movement detected, reset stillness timer
      stillnessStartTime = now;
    }
  }

  // Reset if timeout
  if (freeFallDetected && now - freeFallTime > 3000) {
    stillnessTimerRunning = false;
  }
}


