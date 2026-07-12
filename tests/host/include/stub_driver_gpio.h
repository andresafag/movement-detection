/*
 * Stubs for driver/gpio.h — sufficient to compile and exercise the
 * movement-driver logic on a host.
 */

#ifndef HOST_STUB_DRIVER_GPIO_H
#define HOST_STUB_DRIVER_GPIO_H

#include <stdbool.h>
#include <stdint.h>

/* Enum mirroring the ESP-IDF GPIO types. */
typedef enum {
	GPIO_NUM_NC = -1,
	GPIO_NUM_0  = 0,
	GPIO_NUM_25 = 25,
	GPIO_NUM_27 = 27,
	GPIO_NUM_MAX,
} gpio_num_t;

typedef enum {
	GPIO_MODE_DISABLE = 0,
	GPIO_MODE_INPUT  = 1,
	GPIO_MODE_OUTPUT = 2,
} gpio_mode_t;

typedef enum {
	GPIO_PULLUP_DISABLE = 0,
	GPIO_PULLUP_ENABLE  = 1,
} gpio_pullup_t;

typedef enum {
	GPIO_PULLDOWN_DISABLE = 0,
	GPIO_PULLDOWN_ENABLE  = 1,
} gpio_pulldown_t;

typedef enum {
	GPIO_INTR_DISABLE = 0,
	GPIO_INTR_POSEDGE = 1,
	GPIO_INTR_NEGEDGE = 2,
	GPIO_INTR_ANYEDGE = 3,
	GPIO_INTR_LOW_LEVEL = 4,
	GPIO_INTR_HIGH_LEVEL = 5,
} gpio_int_type_t;

typedef struct {
	uint64_t       pin_bit_mask;
	gpio_mode_t    mode;
	gpio_pullup_t  pull_up_en;
	gpio_pulldown_t pull_down_en;
	gpio_int_type_t intr_type;
} gpio_config_t;

#define ESP_INTR_FLAG_DEFAULT 0

/* Counters + last-config tracker for tests. */
typedef struct {
	uint32_t configs_set;
	uint32_t set_level_calls;
	uint32_t isr_service_installs;
	uint32_t isr_handler_adds;
	uint32_t isr_handler_removes;
	int      last_level;
	uint32_t last_pin_bit_mask;
	gpio_mode_t last_mode;
	gpio_int_type_t last_intr_type;
	gpio_pullup_t last_pullup;
	gpio_pulldown_t last_pulldown;
} HostGpioCounters;

extern HostGpioCounters g_host_gpio;

/* Test hooks */
void host_gpio_reset(void);
int  host_gpio_get_level(uint32_t pin);

/* Public APIs that map to ESP-IDF. */
int  gpio_config(const gpio_config_t *cfg);
int  gpio_set_level(gpio_num_t pin, uint32_t level);
int  gpio_install_isr_service(int flags);
int  gpio_isr_handler_add(gpio_num_t pin, void (*handler)(void *), void *arg);
int  gpio_isr_handler_remove(gpio_num_t pin);

#endif /* HOST_STUB_DRIVER_GPIO_H */
