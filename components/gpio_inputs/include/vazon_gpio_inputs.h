#pragma once

#include "esp_err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t vazon_gpio_inputs_init(void);
esp_err_t vazon_gpio_inputs_read_door_raw(int *level);
esp_err_t vazon_gpio_inputs_read_humidifier_water_raw(int *level);
esp_err_t vazon_gpio_inputs_read_provisioning_button_raw(int *level);
esp_err_t vazon_gpio_inputs_take_main_fan_tach_pulses(uint32_t *pulse_count);
esp_err_t vazon_gpio_inputs_run_smoke_test(void);

#ifdef __cplusplus
}
#endif
