#include "movement-driver.h"
#include <stdbool.h>
#include <stdio.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"  // Added for FreeRTOS queues
#include "driver/gpio.h"
#include "esp_log.h"

#define PIR_SENSOR_GPIO   GPIO_NUM_27  
#define ESP_INTR_FLAG_DEFAULT 0

static const char *TAG = "PIR_SYSTEM";

// Queue handle to pass data out of the ISR
static QueueHandle_t pir_evt_queue = NULL;

// This function runs automatically inside the RAM whenever motion is triggered
static void IRAM_ATTR pir_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // Safely send the event data to the background task queue
    xQueueSendFromISR(pir_evt_queue, &gpio_num, &xHigherPriorityTaskWoken);
    
    // Force a context switch if the background task has higher priority
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

// Background task that waits for events and safely prints
static void pir_processing_task(void* arg)
{
    uint32_t io_num;
    for(;;) {
        // Wait indefinitely until the ISR sends a signal
        if (xQueueReceive(pir_evt_queue, &io_num, portMAX_DELAY)) {
            // It is now 100% safe to use normal printf and ESP_LOG macros
            printf("Motion Detected via ISR!\n");
            printf("%d\n", ESP_OK);
        }
    }
}

// RENAME THIS FROM app_main TO A COMPONENT FUNCTION
void init_movement_sensor(void)
{
    ESP_LOGI(TAG, "Initializing PIR Sensor Configuration...");

    // 1. Create a queue capable of holding 10 integer events
    pir_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    if (pir_evt_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create PIR event queue!");
        return;
    }

    // 2. Spawn the background task to handle processing
    // Allocating 2048 bytes of stack space and running at priority 10
    xTaskCreate(pir_processing_task, "pir_proc_task", 2048, NULL, 10, NULL);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIR_SENSOR_GPIO), 
        .mode = GPIO_MODE_INPUT,                   
        .pull_up_en = GPIO_PULLUP_DISABLE,         
        .pull_down_en = GPIO_PULLDOWN_ENABLE,      
        .intr_type = GPIO_INTR_POSEDGE             
    };
    
    gpio_config(&io_conf);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(PIR_SENSOR_GPIO, pir_isr_handler, (void*) PIR_SENSOR_GPIO);

    ESP_LOGI(TAG, "PIR Sensor initialized successfully. Awaiting movement...");
}
