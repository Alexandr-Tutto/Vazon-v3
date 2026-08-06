#pragma once

#include "esp_err.h"
#include "vazon_climate_module.h"
#include "vazon_fan_module.h"
#include "vazon_humidifier_module.h"
#include "vazon_light_module.h"
#include "vazon_pot_module.h"
#include "vazon_runtime_core.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool schedule_enabled;
    char time_on[VAZON_RUNTIME_TIME_TEXT_SIZE];
    char time_off[VAZON_RUNTIME_TIME_TEXT_SIZE];
    vazon_climate_settings_t climate;
    vazon_light_settings_t light;
    vazon_fan_settings_t fan;
    vazon_humidifier_settings_t humidifier;
    vazon_pot_settings_t pots[2];
    vazon_pot_moisture_calibration_t pot_calibrations[2];
} vazon_persisted_settings_t;

esp_err_t vazon_settings_store_load(vazon_persisted_settings_t *settings);
esp_err_t vazon_settings_store_save(
    const vazon_persisted_settings_t *settings);
bool vazon_settings_store_equal(
    const vazon_persisted_settings_t *left,
    const vazon_persisted_settings_t *right);

#ifdef __cplusplus
}
#endif
