#include "servo_warpper.h"

Servo Servo_X;
Servo Servo_Y;

void servo_init()
{
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    Servo_X.setPeriodHertz(50); // Standard 50hz servo
    Servo_Y.setPeriodHertz(50); // Standard 50hz servo
    Servo_X.attach(servoXPin, 500, 2500);
    Servo_Y.attach(servoYPin, 500, 2500);
}