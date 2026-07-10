#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt-aws.h"
#include "movement-driver.h"

void app_main(void)
{
    // Initialize PIR sensor and create the rfid_trigger_sem semaphore
    init_movement_sensor();
}
