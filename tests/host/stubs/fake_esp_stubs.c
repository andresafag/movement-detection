#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t item_size)
{
    (void)length;
    (void)item_size;
    return (QueueHandle_t)1;
}

BaseType_t xQueueSendFromISR(QueueHandle_t queue, const void *item, BaseType_t *higher_priority_task_woken)
{
    (void)queue;
    (void)item;
    (void)higher_priority_task_woken;
    return pdTRUE;
}

BaseType_t xQueueReceive(QueueHandle_t queue, void *item, TickType_t ticks_to_wait)
{
    (void)queue;
    (void)item;
    (void)ticks_to_wait;
    return pdTRUE;
}

BaseType_t xQueueReset(QueueHandle_t queue)
{
    (void)queue;
    return pdTRUE;
}

void vTaskDelay(TickType_t ticks)
{
    (void)ticks;
}

BaseType_t xTaskCreate(TaskFunction_t pvTaskCode,
                       const char *pcName,
                       uint32_t usStackDepth,
                       void *pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *pxCreatedTask)
{
    (void)pvTaskCode;
    (void)pcName;
    (void)usStackDepth;
    (void)pvParameters;
    (void)uxPriority;
    (void)pxCreatedTask;
    return pdTRUE;
}

esp_err_t gpio_config(const gpio_config_t *conf)
{
    (void)conf;
    return ESP_OK;
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    (void)gpio_num;
    (void)level;
    return ESP_OK;
}

esp_err_t gpio_install_isr_service(uint32_t flags)
{
    (void)flags;
    return ESP_OK;
}

esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args)
{
    (void)gpio_num;
    (void)isr_handler;
    (void)args;
    return ESP_OK;
}