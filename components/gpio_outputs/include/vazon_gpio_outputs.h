#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t vazon_gpio_outputs_init_safe_off(void);
esp_err_t vazon_gpio_outputs_run_status_led_test(void);
bool vazon_gpio_outputs_is_actuator_gpio(int gpio);
esp_err_t vazon_gpio_outputs_set_raw(gpio_num_t gpio, int level);
esp_err_t vazon_gpio_outputs_set_all_off(void);

#ifdef __cplusplus
}
#endif
