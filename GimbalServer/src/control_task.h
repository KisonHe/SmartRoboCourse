#ifndef CONTROL_TASK_H
#define CONTROL_TASK_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern TaskHandle_t Control_Task_Handle;
void Control_Task(void *pvParameters);
#endif