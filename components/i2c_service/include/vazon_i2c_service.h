#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t vazon_i2c_service_scan_climate_bus(void);
esp_err_t vazon_i2c_service_sht31_is_present(uint8_t address, bool *present);
esp_err_t vazon_i2c_service_read_sht31(uint8_t address,
                                      float *temperature_c,
                                      float *humidity_pct);

#ifdef __cplusplus
}
#endif
