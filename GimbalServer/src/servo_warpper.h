#ifndef SERVO_WARPPER_H
#define SERVO_WARPPER_H
#include <ESP32Servo.h>
const int servoXPin = 26;
const int servoYPin = 25;

extern Servo Servo_X;
extern Servo Servo_Y;
void servo_init();
#endif