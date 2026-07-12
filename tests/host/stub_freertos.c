#include "stub_freertos.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

HostRtosCounters g_host_rtos;

static TaskFunction_t s_last_task_body       = NULL;
static BaseType_t     s_task_create_return   = pdPASS;

void host_rtos_reset(void) {
	memset(&g_host_rtos, 0, sizeof(g_host_rtos));
	s_last_task_body     = NULL;
	s_task_create_return = pdPASS;
}

TaskFunction_t host_task_capture_last_body(void) {
	return s_last_task_body;
}

void host_task_set_create_return(BaseType_t rv) {
	s_task_create_return = rv;
}

QueueHandle_t host_queue_create(size_t length, size_t item_size) {
	(void)length;
	(void)item_size;
	HostQueue_t *q = (HostQueue_t *)calloc(1, sizeof(HostQueue_t));
	if (q == NULL) {
		return NULL;
	}
	q->capacity  = HOST_QUEUE_CAPACITY;
	q->elem_size = HOST_QUEUE_ELEM_SIZE;
	g_host_rtos.queues_created++;
	return (QueueHandle_t)q;
}

BaseType_t host_queue_send_from_isr(QueueHandle_t q, const void *item) {
	if (q == NULL || item == NULL) {
		return pdFAIL;
	}
	if (q->count >= q->capacity || q->full) {
		return pdFAIL;
	}
	memcpy(&q->storage[q->tail * q->elem_size], item, q->elem_size);
	q->tail = (q->tail + 1) % q->capacity;
	q->count++;
	if (q->tail == q->head) {
		q->full = true;
	}
	g_host_rtos.queue_send_count++;
	/* The real driver uses this to decide whether to yield; we always
	 * claim the task should be woken because the test exercises that path. */
	return pdPASS;
}

BaseType_t host_queue_receive(QueueHandle_t q, void *buf, TickType_t wait) {
	(void)wait;
	if (q == NULL || buf == NULL) {
		return pdFAIL;
	}
	if (q->count == 0 && !q->full) {
		/* In a real RTOS this would block; for tests we treat empty queue
		 * as "not yet" so the test loop terminates predictably. */
		return pdFAIL;
	}
	memcpy(buf, &q->storage[q->head * q->elem_size], q->elem_size);
	q->head = (q->head + 1) % q->capacity;
	q->count--;
	q->full = false;
	g_host_rtos.queue_receive_count++;
	return pdPASS;
}

BaseType_t host_queue_reset(QueueHandle_t q) {
	if (q == NULL) {
		return pdFAIL;
	}
	q->head = q->tail = q->count = 0;
	q->full = false;
	g_host_rtos.queue_reset_count++;
	return pdPASS;
}
