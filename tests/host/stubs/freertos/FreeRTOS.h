#ifndef FREERTOS_H
#define FREERTOS_H

#include <stdint.h>

typedef int32_t BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint32_t TickType_t;
typedef void * QueueHandle_t;
typedef void * TaskHandle_t;

#define pdFALSE 0
#define pdTRUE 1
#define pdMS_TO_TICKS(x) (x)
#define portMAX_DELAY 0xffffffffUL
#define portYIELD_FROM_ISR() ((void)0)

#endif