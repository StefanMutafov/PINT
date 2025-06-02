#include "MotionSensor.h"
#include <Wire.h>
#include <MPU6050.h>
#include <math.h>

static MPU6050 mpu;
static int16_t ax, ay, az, gx, gy, gz;
static int16_t gx_offset, gy_offset, gz_offset;
static unsigned long lastStepTime = 0;
static float lastMagnitude = 0;
static const float stepThreshold = 1.2;
static const int   stepDelay     = 350;
volatile int         stepCount     = 0;
//volitale int workoutStepCount = 0;

// fall detection state
static bool     stillnessTimerRunning = false;
static const int stillnessDuration     = 2000;
static unsigned long stillnessStartTime=0;


// at bottom, add:
int getStepCount() {
    return stepCount;
}

//int getWorkoutStepCount(){
//    return workoutStepCount;
//}

//void setWorkoutStepCount(int step){
//    workoutStepCount = step;
//}


static void calibrateGyro() {
    const int readings = 100;
    int32_t tx=0, ty=0, tz=0;
    for(int i=0; i<readings; i++){
        mpu.getRotation(&gx, &gy, &gz);
        tx += gx; ty += gy; tz += gz;
        delay(10);
    }
    gx_offset = tx/readings;
    gy_offset = ty/readings;
    gz_offset = tz/readings;
    Serial.print("Gyro offsets X,Y,Z: ");
    Serial.print(gx_offset); Serial.print(", ");
    Serial.print(gy_offset); Serial.print(", ");
    Serial.println(gz_offset);
}

void initMotion() {
    Wire.begin(21,22);
    mpu.initialize();
    if (mpu.testConnection()) {
        Serial.println("MPU6050 connected");
    } else {
        Serial.println("MPU6050 FAIL");
        while(1) delay(1000);
    }
    calibrateGyro();
}

int updateMotion() {
    unsigned long now = millis();
    mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);
    gx -= gx_offset; gy -= gy_offset; gz -= gz_offset;

    float axg = ax/16384.0f;
    float ayg = ay/16384.0f;
    float azg = az/16384.0f;
    float magnitude = sqrt(axg*axg + ayg*ayg + azg*azg);

    // step detection
    if ( (magnitude - lastMagnitude > 0.3) &&
         (magnitude > stepThreshold) &&
         (now - lastStepTime > stepDelay) )
    {
        stepCount++;
        //workoutStepCount++
        lastStepTime = now;
        Serial.print("Step: "); Serial.println(stepCount);
    }
    lastMagnitude = magnitude;

    // fall detection: large accel + angular velocity
    float angVel = sqrt( (gx/131.0f)*(gx/131.0f)
                         + (gy/131.0f)*(gy/131.0f)
                         + (gz/131.0f)*(gz/131.0f) );
    if (!stillnessTimerRunning && magnitude > 3.2 && angVel > 300) {
        Serial.println("Potential fall detected!");
        stillnessTimerRunning = true;
        stillnessStartTime = now;
    }
    if (stillnessTimerRunning) {
        if (magnitude >= 0.8 && magnitude <= 1.2) {
            if (now - stillnessStartTime >= stillnessDuration) {
                Serial.println("Fall confirmed (stillness)!");
                stillnessTimerRunning = false;
            }
        } else {
            stillnessStartTime = now;
        }
    }

    return stepCount;
}
