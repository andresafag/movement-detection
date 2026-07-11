/*
 * Fake FreeRTOS implementation for host-based unit tests.
 *
 * Implements just enough of the FreeRTOS API surface that movement-driver.c
 * can be compiled and run on a host (Linux/macOS/Windows) without an
 * RTOS scheduler.  The intent is to verify the *logic* of the driver
 * (state transitions, queue usage, debounce timing) — not real-time
 * behaviour.
 */

#ifndef HOST_STUB_FREERTOS_H
#define HOST_STUB_FREERTOS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Tick type — milliseconds work fine for our needs. */
typedef uint32_t TickType_t;
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#define portMAX_DELAY    ((TickType_t)0xFFFFFFFFu)

/* Return values for queue operations. */
typedef enum {
    pdPASS = 0,
    pdFAIL = 1,
} BaseType_t;

/* Opaque handle types — we tag them with the kind we use. */
typedef struct HostQueue HostQueue_t;
typedef HostQueue_t       *QueueHandle_t;
typedef void             *TaskHandle_t;
typedef uint32_t          UBaseType_t;

/* Queue capacity & element size used by the driver. */
#define HOST_QUEUE_CAPACITY  16
#define HOST_QUEUE_ELEM_SIZE sizeof(uint32_t)

struct HostQueue {
    uint8_t  storage[HOST_QUEUE_CAPACITY * HOST_QUEUE_ELEM_SIZE];
    size_t   head;
    size_t   tail;
    size_t   count;
    size_t   capacity;
    size_t   elem_size;
    bool     full;
};

/* Counters exposed to tests so we can assert on side effects. */
typedef struct {
    uint32_t tasks_created;
    uint32_t task_create_failures;
    uint32_t isrs_installed;
    uint32_t isr_handlers_added;
    uint32_t queues_created;
    uint32_t queue_send_count;
    uint32_t queue_receive_count;
    uint32_t queue_reset_count;
    uint32_t delays_requested_ms;
    uint32_t yields_from_isr;
} HostRtosCounters;

extern HostRtosCounters g_host_rtos;

/* Task function signature (mirrors the FreeRTOS TaskFunction_t). */
typedef void (*TaskFunction_t)(void *);

/* Test hooks — let tests inject events into the ISR-to-task path. */
void       host_rtos_reset(void);
QueueHandle_t host_queue_create(size_t length, size_t item_size);
BaseType_t host_queue_send_from_isr(QueueHandle_t q, const void *item);
BaseType_t host_queue_receive(QueueHandle_t q, void *buf, TickType_t wait);
BaseType_t host_queue_reset(QueueHandle_t q);

/* The driver creates a task and a queue.  We capture both so the test
 * can drive the task's body directly. */
TaskFunction_t host_task_capture_last_body(void);
void          host_task_set_create_return(BaseType_t rv);

/* ---------------------------------------------------------------------------
 * FreeRTOS API surface — the names the driver calls.
 * ---------------------------------------------------------------------------*/

/* xQueueCreate is a macro in FreeRTOS that picks the correct backend
 * (queue.c) — we just route it to our host implementation. */
static inline QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize) {
    return host_queue_create(uxQueueLength, uxItemSize);
}

static inline BaseType_t xQueueSendFromISR(QueueHandle_t q, const void *pvItemToQueue, BaseType_t *pxHigherPriorityTaskWoken) {
    if (pxHigherPriorityTaskWoken) {
        *pxHigherPriorityTaskWoken = pdTRUE;
        g_host_rtos.yields_from_isr++;
    }
    return host_queue_send_from_isr(q, pvItemToQueue);
}

static inline BaseType_t xQueueReceive(QueueHandle_t q, void *pvBuffer, TickType_t xTicksToWait) {
    return host_queue_receive(q, pvBuffer, xTicksToWait);
}

static inline BaseType_t xQueueReset(QueueHandle_t q) {
    return host_queue_reset(q);
}

static inline void vTaskDelay(TickType_t xTicksToDelay) {
    g_host_rtos.delays_requested_ms += (uint32_t)xTicksToDelay;
}

static inline void portYIELD_FROM_ISR(void) {
    g_host_rtos.yields_from_isr++;
}

static inline BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char *const pcName,
                                     uint32_t usStackDepth, void *pvParameters,
                                     UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask) {
    (void)pcName;
    (void)usStackDepth;
    (void)pvParameters;
    (void)uxPriority;
    (void)pxCreatedTask;
    s_last_task_body = pxTaskCode;
    if (s_task_create_return == pdPASS) {
        g_host_rtos.tasks_created++;
        return pdPASS;
    } else {
        g_host_rtos.task_create_failures++;
        return pdFAIL;
    }
}

#endif /* HOST_STUB_FREERTOS_H */
