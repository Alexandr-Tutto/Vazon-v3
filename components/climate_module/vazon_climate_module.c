#include "vazon_climate_module.h"

#include <math.h>
#include <string.h>

static bool settings_valid(const vazon_climate_settings_t *settings)
{
    return settings != NULL && isfinite(settings->temperature_low_warn) &&
           isfinite(settings->temperature_high_warn) &&
           settings->temperature_low_warn < settings->temperature_high_warn &&
           isfinite(settings->humidity_low_warn) &&
           isfinite(settings->humidity_high_warn) &&
           settings->humidity_low_warn >= 0.0f &&
           settings->humidity_low_warn < settings->humidity_high_warn &&
           settings->humidity_high_warn <= 100.0f &&
           settings->sht31_stale_timeout_sec > 0U &&
           isfinite(settings->temperature_delta_warn) &&
           isfinite(settings->temperature_delta_error) &&
           settings->temperature_delta_warn >= 0.0f &&
           settings->temperature_delta_warn < settings->temperature_delta_error;
}

static int sensor_index(uint8_t address)
{
    if (address == VAZON_CLIMATE_SHT31_0_ADDRESS) return 0;
    if (address == VAZON_CLIMATE_SHT31_1_ADDRESS) return 1;
    return -1;
}

static vazon_climate_sensor_snapshot_t *sensor_snapshot(
    vazon_climate_module_t *module, int index)
{
    return index == 0 ? &module->snapshot.sensor_0x44
                      : &module->snapshot.sensor_0x45;
}

esp_err_t vazon_climate_module_init(vazon_climate_module_t *module,
                                    const vazon_climate_settings_t *settings)
{
    if (module == NULL || !settings_valid(settings)) return ESP_ERR_INVALID_ARG;
    memset(module, 0, sizeof(*module));
    module->snapshot.sensor_0x44.status = VAZON_CLIMATE_SENSOR_UNKNOWN;
    module->snapshot.sensor_0x45.status = VAZON_CLIMATE_SENSOR_UNKNOWN;
    module->snapshot.status = VAZON_CLIMATE_STATUS_UNKNOWN;
    module->snapshot.status_reason = VAZON_CLIMATE_REASON_NONE;
    module->snapshot.settings = *settings;
    module->initialized = true;
    return ESP_OK;
}

esp_err_t vazon_climate_module_record_sensor(vazon_climate_module_t *module,
                                             uint8_t address,
                                             bool present,
                                             esp_err_t read_result,
                                             float temperature_c,
                                             float humidity_pct,
                                             uint64_t now_ms)
{
    const int index = sensor_index(address);
    if (module == NULL || !module->initialized || index < 0) {
        return ESP_ERR_INVALID_ARG;
    }

    vazon_climate_sensor_runtime_t *runtime = &module->sensor_runtime[index];
    vazon_climate_sensor_snapshot_t *sensor = sensor_snapshot(module, index);
    runtime->present_known = true;
    sensor->present = present;
    if (!runtime->first_attempt_recorded) {
        runtime->first_attempt_recorded = true;
        runtime->first_attempt_ms = now_ms;
    }
    if (!present || read_result != ESP_OK) return ESP_OK;

    if (!isfinite(temperature_c) || temperature_c < -40.0f ||
        temperature_c > 125.0f || !isfinite(humidity_pct) ||
        humidity_pct < 0.0f || humidity_pct > 100.0f) {
        runtime->invalid_value = true;
        return ESP_OK;
    }

    runtime->invalid_value = false;
    sensor->temperature_c = temperature_c;
    sensor->humidity_pct = humidity_pct;
    sensor->has_valid_sample = true;
    sensor->last_valid_sample_ms = now_ms;
    return ESP_OK;
}

static bool sensor_is_stale(const vazon_climate_module_t *module,
                            int index,
                            uint64_t now_ms)
{
    const vazon_climate_sensor_snapshot_t *sensor =
        index == 0 ? &module->snapshot.sensor_0x44 : &module->snapshot.sensor_0x45;
    const vazon_climate_sensor_runtime_t *runtime = &module->sensor_runtime[index];
    const uint64_t timeout_ms =
        (uint64_t)module->snapshot.settings.sht31_stale_timeout_sec * 1000ULL;
    if (sensor->has_valid_sample) {
        return now_ms - sensor->last_valid_sample_ms > timeout_ms;
    }
    return runtime->first_attempt_recorded &&
           now_ms - runtime->first_attempt_ms > timeout_ms;
}

static bool sensor_warning(const vazon_climate_sensor_snapshot_t *sensor,
                           const vazon_climate_settings_t *settings,
                           vazon_climate_status_reason_t *reason)
{
    if (sensor->temperature_c < settings->temperature_low_warn ||
        sensor->temperature_c > settings->temperature_high_warn) {
        *reason = VAZON_CLIMATE_REASON_TEMPERATURE_OUT_OF_RANGE;
        return true;
    }
    if (sensor->humidity_pct < settings->humidity_low_warn ||
        sensor->humidity_pct > settings->humidity_high_warn) {
        *reason = VAZON_CLIMATE_REASON_HUMIDITY_OUT_OF_RANGE;
        return true;
    }
    return false;
}

esp_err_t vazon_climate_module_evaluate(vazon_climate_module_t *module,
                                        uint64_t now_ms)
{
    if (module == NULL || !module->initialized) return ESP_ERR_INVALID_STATE;
    if (now_ms < module->last_evaluation_ms) return ESP_ERR_INVALID_ARG;
    module->last_evaluation_ms = now_ms;
    vazon_climate_sensor_snapshot_t *sensors[2] = {
        &module->snapshot.sensor_0x44, &module->snapshot.sensor_0x45};
    bool stale[2] = {sensor_is_stale(module, 0, now_ms),
                     sensor_is_stale(module, 1, now_ms)};

    module->snapshot.status = VAZON_CLIMATE_STATUS_UNKNOWN;
    module->snapshot.status_reason = VAZON_CLIMATE_REASON_NONE;
    module->snapshot.data_valid = false;
    module->snapshot.data_stale = stale[0] || stale[1];

    for (int index = 0; index < 2; ++index) {
        if (module->sensor_runtime[index].present_known && !sensors[index]->present) {
            sensors[index]->status = VAZON_CLIMATE_SENSOR_ERROR;
        } else if (stale[index] || module->sensor_runtime[index].invalid_value) {
            sensors[index]->status = VAZON_CLIMATE_SENSOR_ERROR;
        } else if (!sensors[index]->has_valid_sample) {
            sensors[index]->status = VAZON_CLIMATE_SENSOR_UNKNOWN;
        } else {
            vazon_climate_status_reason_t ignored = VAZON_CLIMATE_REASON_NONE;
            sensors[index]->status = sensor_warning(
                                         sensors[index], &module->snapshot.settings,
                                         &ignored)
                                         ? VAZON_CLIMATE_SENSOR_WARNING
                                         : VAZON_CLIMATE_SENSOR_OK;
        }
    }

    if (module->sensor_runtime[0].present_known && !sensors[0]->present) {
        module->snapshot.status = VAZON_CLIMATE_STATUS_ERROR;
        module->snapshot.status_reason = VAZON_CLIMATE_REASON_SHT31_0X44_MISSING;
        return ESP_OK;
    }
    if (module->sensor_runtime[1].present_known && !sensors[1]->present) {
        module->snapshot.status = VAZON_CLIMATE_STATUS_ERROR;
        module->snapshot.status_reason = VAZON_CLIMATE_REASON_SHT31_0X45_MISSING;
        return ESP_OK;
    }
    if (stale[0] || stale[1]) {
        module->snapshot.status = VAZON_CLIMATE_STATUS_ERROR;
        module->snapshot.status_reason = VAZON_CLIMATE_REASON_SHT31_STALE;
        return ESP_OK;
    }
    if (module->sensor_runtime[0].invalid_value ||
        module->sensor_runtime[1].invalid_value) {
        module->snapshot.status = VAZON_CLIMATE_STATUS_ERROR;
        module->snapshot.status_reason = VAZON_CLIMATE_REASON_INVALID_VALUE;
        return ESP_OK;
    }
    if (!sensors[0]->has_valid_sample || !sensors[1]->has_valid_sample) return ESP_OK;

    module->snapshot.data_valid = true;
    module->snapshot.rh_delta_pct =
        fabsf(sensors[0]->humidity_pct - sensors[1]->humidity_pct);
    module->snapshot.temperature_delta_c =
        fabsf(sensors[0]->temperature_c - sensors[1]->temperature_c);

    if (module->snapshot.temperature_delta_c >
        module->snapshot.settings.temperature_delta_error) {
        module->snapshot.status = VAZON_CLIMATE_STATUS_ERROR;
        module->snapshot.status_reason =
            VAZON_CLIMATE_REASON_TEMPERATURE_DELTA_CRITICAL;
        return ESP_OK;
    }

    vazon_climate_status_reason_t warning_reason = VAZON_CLIMATE_REASON_NONE;
    const bool value_warning =
        sensor_warning(sensors[0], &module->snapshot.settings, &warning_reason) ||
        sensor_warning(sensors[1], &module->snapshot.settings, &warning_reason);
    if (module->snapshot.temperature_delta_c >
        module->snapshot.settings.temperature_delta_warn) {
        module->snapshot.status = VAZON_CLIMATE_STATUS_WARNING;
        module->snapshot.status_reason = VAZON_CLIMATE_REASON_TEMPERATURE_DELTA_HIGH;
    } else if (value_warning) {
        module->snapshot.status = VAZON_CLIMATE_STATUS_WARNING;
        module->snapshot.status_reason = warning_reason;
    } else {
        module->snapshot.status = VAZON_CLIMATE_STATUS_OK;
    }
    return ESP_OK;
}

esp_err_t vazon_climate_module_set_settings(
    vazon_climate_module_t *module,
    const vazon_climate_settings_patch_t *patch)
{
    if (module == NULL || !module->initialized || patch == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    vazon_climate_settings_t candidate = module->snapshot.settings;
    if (patch->has_temperature_low_warn) {
        candidate.temperature_low_warn = patch->temperature_low_warn;
    }
    if (patch->has_temperature_high_warn) {
        candidate.temperature_high_warn = patch->temperature_high_warn;
    }
    if (patch->has_humidity_low_warn) {
        candidate.humidity_low_warn = patch->humidity_low_warn;
    }
    if (patch->has_humidity_high_warn) {
        candidate.humidity_high_warn = patch->humidity_high_warn;
    }
    if (patch->has_sht31_stale_timeout_sec) {
        candidate.sht31_stale_timeout_sec = patch->sht31_stale_timeout_sec;
    }
    if (patch->has_temperature_delta_warn) {
        candidate.temperature_delta_warn = patch->temperature_delta_warn;
    }
    if (patch->has_temperature_delta_error) {
        candidate.temperature_delta_error = patch->temperature_delta_error;
    }
    if (!settings_valid(&candidate)) return ESP_ERR_INVALID_ARG;
    module->snapshot.settings = candidate;
    return vazon_climate_module_evaluate(module, module->last_evaluation_ms);
}

static vazon_command_result_t climate_command_handler(
    const vazon_command_t *command, void *context)
{
    vazon_climate_module_t *module = (vazon_climate_module_t *)context;
    if (command->args == NULL ||
        strcmp(command->cmd, VAZON_COMMAND_CLIMATE_SET_SETTINGS) != 0) {
        const vazon_command_result_t rejected = {
            .status = VAZON_COMMAND_RESULT_REJECTED,
            .error = command->args == NULL ? ESP_ERR_INVALID_ARG
                                           : ESP_ERR_NOT_SUPPORTED,
        };
        return rejected;
    }
    const esp_err_t result = vazon_climate_module_set_settings(
        module, (const vazon_climate_settings_patch_t *)command->args);
    const vazon_command_result_t command_result = {
        .status = result == ESP_OK ? VAZON_COMMAND_RESULT_ACCEPTED
                                   : VAZON_COMMAND_RESULT_REJECTED,
        .error = result,
    };
    return command_result;
}

esp_err_t vazon_climate_module_register_command_target(
    vazon_climate_module_t *module, vazon_command_router_t *router)
{
    if (module == NULL || !module->initialized || router == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return vazon_command_router_register(router, VAZON_COMMAND_TARGET_CLIMATE,
                                         climate_command_handler, module);
}

const vazon_climate_snapshot_t *vazon_climate_module_get_snapshot(
    const vazon_climate_module_t *module)
{
    return module == NULL || !module->initialized ? NULL : &module->snapshot;
}
