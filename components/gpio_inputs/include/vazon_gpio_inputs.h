#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t vazon_gpio_inputs_init(void);
esp_err_t vazon_gpio_inputs_read_door_raw(int *level);
esp_err_t vazon_gpio_inputs_read_humidifier_water_raw(int *level);
esp_err_t vazon_gpio_inputs_read_provisioning_button_raw(int *level);
esp_err_t vazon_gpio_inputs_run_smoke_test(void);

#ifdef __cplusplus
}
#endif
