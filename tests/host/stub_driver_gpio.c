#include "stub_driver_gpio.h"

#include <string.h>

HostGpioCounters g_host_gpio;

/* Per-pin level state — tests can read this via host_gpio_get_level. */
static int s_pin_levels[GPIO_NUM_MAX] = {0};

void host_gpio_reset(void) {
	memset(&g_host_gpio, 0, sizeof(g_host_gpio));
	memset(s_pin_levels, 0, sizeof(s_pin_levels));
}

int host_gpio_get_level(uint32_t pin) {
	if (pin >= GPIO_NUM_MAX) {
		return 0;
	}
	return s_pin_levels[pin];
}

int gpio_config(const gpio_config_t *cfg) {
	if (cfg == NULL) {
		return -1;
	}
	g_host_gpio.configs_set++;
	g_host_gpio.last_pin_bit_mask = (uint32_t)cfg->pin_bit_mask;
	g_host_gpio.last_mode         = cfg->mode;
	g_host_gpio.last_intr_type    = cfg->intr_type;
	g_host_gpio.last_pullup       = cfg->pull_up_en;
	g_host_gpio.last_pulldown     = cfg->pull_down_en;
	return 0;
}

int gpio_set_level(gpio_num_t pin, uint32_t level) {
	if (pin < 0 || pin >= GPIO_NUM_MAX) {
		return -1;
	}
	s_pin_levels[pin] = (int)(level ? 1 : 0);
	g_host_gpio.set_level_calls++;
	g_host_gpio.last_level = (int)(level ? 1 : 0);
	return 0;
}

int gpio_install_isr_service(int flags) {
	(void)flags;
	g_host_gpio.isr_service_installs++;
	return 0;
}

int gpio_isr_handler_add(gpio_num_t pin, void (*handler)(void *), void *arg) {
	(void)handler;
	(void)arg;
	if (pin < 0 || pin >= GPIO_NUM_MAX) {
		return -1;
	}
	g_host_gpio.isr_handler_adds++;
	return 0;
}

int gpio_isr_handler_remove(gpio_num_t pin) {
	if (pin < 0 || pin >= GPIO_NUM_MAX) {
		return -1;
	}
	g_host_gpio.isr_handler_adds--;
	g_host_gpio.isr_handler_removes++;
	return 0;
}
