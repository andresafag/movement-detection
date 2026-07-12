#ifndef FREERTOS_QUEUE_H
#define FREERTOS_QUEUE_H

#include "freertos/FreeRTOS.h"

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t item_size);
BaseType_t xQueueSendFromISR(QueueHandle_t queue, const void *item, BaseType_t *higher_priority_task_woken);
BaseType_t xQueueReceive(QueueHandle_t queue, void *item, TickType_t ticks_to_wait);
BaseType_t xQueueReset(QueueHandle_t queue);

#endif