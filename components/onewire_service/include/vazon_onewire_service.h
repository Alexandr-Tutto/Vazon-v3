#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t vazon_onewire_service_scan_pot_temperature_bus(void);
esp_err_t vazon_onewire_service_sensor_present(uint8_t pot_id, bool *present);
esp_err_t vazon_onewire_service_read_pot_temperatures(
    float temperatures_c[2], esp_err_t read_results[2]);

#ifdef __cplusplus
}
#endif
