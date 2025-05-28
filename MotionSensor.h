#ifndef MOTION_SENSOR_H
#define MOTION_SENSOR_H

#include <Arduino.h>

// must call once in setup()
void initMotion();

// call each loop; returns current total steps
int updateMotion();

// after your other declarations
int  getStepCount();


#endif // MOTION_SENSOR_H
