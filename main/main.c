#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt-aws.h"
#include "movement-driver.h"

void app_main(void) {
	init_movement_sensor();

	// Loop forever to keep the FreeRTOS scheduler from discarding this thread
	while (1) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
