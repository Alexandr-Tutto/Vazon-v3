#pragma once

#include "esp_err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t vazon_adc_service_init(void);
esp_err_t vazon_adc_service_read_raw(uint8_t pot_id, int *raw_value);
esp_err_t vazon_adc_service_read_raw_average(uint8_t pot_id,
                                             size_t sample_count,
                                             int *raw_value);
esp_err_t vazon_adc_service_read_raw_soil_moisture(void);

#ifdef __cplusplus
}
#endif
