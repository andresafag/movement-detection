/*
 * Stubs for esp_log.h — discard everything.
 */
#ifndef HOST_STUB_ESP_LOG_H
#define HOST_STUB_ESP_LOG_H

typedef enum {
    ESP_LOG_NONE   = 0,
    ESP_LOG_ERROR  = 1,
    ESP_LOG_WARN   = 2,
    ESP_LOG_INFO   = 3,
    ESP_LOG_DEBUG  = 4,
    ESP_LOG_VERBOSE = 5,
} esp_log_level_t;

#define ESP_LOGE(tag, fmt, ...) ((void)0)
#define ESP_LOGW(tag, fmt, ...) ((void)0)
#define ESP_LOGI(tag, fmt, ...) ((void)0)
#define ESP_LOGD(tag, fmt, ...) ((void)0)
#define ESP_LOGV(tag, fmt, ...) ((void)0)

#endif /* HOST_STUB_ESP_LOG_H */
