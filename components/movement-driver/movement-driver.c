#include "movement-driver.h"
#include <stdbool.h>
#include <stdio.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Pin Definitions for DOIT ESP32 DevKit V1 (30-Pin)
#define PIR_SENSOR_GPIO        GPIO_NUM_27
#define BLUE_LED_PIN           GPIO_NUM_25
#define ESP_INTR_FLAG_DEFAULT  0

static const char *TAG = "PIR_SYSTEM";
static QueueHandle_t pir_evt_queue = NULL;

// Interrupt Service Routine (ISR) - Runs instantly on physical voltage change
static void IRAM_ATTR pir_isr_handler(void* arg) {
	uint32_t gpio_num = (uint32_t)arg;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	// Send the triggered pin number to the processing task
	xQueueSendFromISR(pir_evt_queue, &gpio_num, &xHigherPriorityTaskWoken);

	if (xHigherPriorityTaskWoken) {
		portYIELD_FROM_ISR();
	}
}

// Background Task - Processes the queue items outside of the critical ISR context
static void pir_processing_task(void* arg) {
	uint32_t io_num;
	for(;;) {
		// Block indefinitely until an interrupt sends data into the queue
		if (xQueueReceive(pir_evt_queue, &io_num, portMAX_DELAY)) {
			ESP_LOGW(TAG, "Motion Detected on GPIO %lu!", io_num);

			// 1. Turn the Blue LED ON
			gpio_set_level(BLUE_LED_PIN, 1);

			// 2. Keep it shining for 2 seconds (serves as your movement window)
			vTaskDelay(pdMS_TO_TICKS(2000));

			// 3. Turn the Blue LED OFF
			gpio_set_level(BLUE_LED_PIN, 0);

			// 4. Clear out any noisy duplicate triggers that queued up while the LED was on
			xQueueReset(pir_evt_queue);
		}
	}
}

void init_movement_sensor(void) {
	ESP_LOGI(TAG, "Initializing PIR Sensor Configuration...");

	// Create the event queue
	pir_evt_queue = xQueueCreate(10, sizeof(uint32_t));
	if (pir_evt_queue == NULL) {
		ESP_LOGE(TAG, "Failed to create PIR event queue!");
		return;
	}

	// Spawn the background event processor
	xTaskCreate(pir_processing_task, "pir_proc_task", 2048, NULL, 10, NULL);

	// Step 1: Configure the Input PIR Sensor Pin
	gpio_config_t io_conf = {
		.pin_bit_mask = (1ULL << PIR_SENSOR_GPIO),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_ENABLE,  // Pulls pin to GND until PIR sends a high signal
		.intr_type = GPIO_INTR_POSEDGE         // Triggers interrupt when signal transitions LOW -> HIGH
	};
	gpio_config(&io_conf);

	// Step 2: Overwrite configuration fields for the Output LED Pin
	io_conf.pin_bit_mask = (1ULL << BLUE_LED_PIN);
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&io_conf);


	gpio_set_level(BLUE_LED_PIN, 1);
	vTaskDelay(pdMS_TO_TICKS(500));
	gpio_set_level(BLUE_LED_PIN, 0);

	// Install the global ESP32 ISR routing engine
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

	// Link our custom handler to fire when GPIO 27 experiences a rising edge
	gpio_isr_handler_add(PIR_SENSOR_GPIO, pir_isr_handler, (void*)PIR_SENSOR_GPIO);

	ESP_LOGI(TAG, "PIR Sensor initialized successfully. Awaiting movement...");
}


