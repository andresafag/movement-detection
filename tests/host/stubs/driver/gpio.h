#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include "esp_err.h"

typedef enum {
    GPIO_NUM_25 = 25,
    GPIO_NUM_27 = 27
} gpio_num_t;

typedef struct {
    uint64_t pin_bit_mask;
    int mode;
    int pull_up_en;
    int pull_down_en;
    int intr_type;
} gpio_config_t;

#define GPIO_MODE_INPUT 0
#define GPIO_MODE_OUTPUT 1
#define GPIO_PULLUP_DISABLE 0
#define GPIO_PULLDOWN_ENABLE 1
#define GPIO_PULLDOWN_DISABLE 0
#define GPIO_INTR_POSEDGE 1
#define GPIO_INTR_DISABLE 0

#define IRAM_ATTR

typedef void (*gpio_isr_t)(void *);

esp_err_t gpio_config(const gpio_config_t *conf);
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);
esp_err_t gpio_install_isr_service(uint32_t flags);
esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args);

#endif