#include "control_task.h"

#include "esp_system.h"

#include "servo_warpper.h"

TaskHandle_t Control_Task_Handle;

extern int servo_x;
extern int servo_y;

void updateServo()
{
    Servo_X.write(servo_x);
    Servo_Y.write(servo_y);
}

void Control_Task(void *pvParameters)
{

    while (1)
    {
        updateServo();
        vTaskDelay(50);
    }
}