#include "vazon_pot_module.h"

#include <math.h>
#include <string.h>

static bool settings_valid(const vazon_pot_settings_t *settings)
{
    return settings != NULL && settings->moisture_stale_timeout_sec > 0U &&
           settings->temperature_stale_timeout_sec > 0U &&
           isfinite(settings->temperature_low_warn_c) &&
           isfinite(settings->temperature_high_warn_c) &&
           settings->temperature_low_warn_c < settings->temperature_high_warn_c;
}

static bool calibration_complete(const vazon_pot_moisture_calibration_t *calibration)
{
    return calibration->dry_valid && calibration->normal_valid &&
           calibration->wet_valid;
}

static bool calibration_order_valid(
    const vazon_pot_moisture_calibration_t *calibration)
{
    if (!calibration_complete(calibration)) return false;
    return (calibration->dry_calibration_mv < calibration->normal_calibration_mv &&
            calibration->normal_calibration_mv < calibration->wet_calibration_mv) ||
           (calibration->dry_calibration_mv > calibration->normal_calibration_mv &&
            calibration->normal_calibration_mv > calibration->wet_calibration_mv);
}

static int16_t clamp_pct(float value)
{
    if (value < 0.0f) return 0;
    if (value > 100.0f) return 100;
    return (int16_t)lroundf(value);
}

static int16_t moisture_pct(const vazon_pot_moisture_calibration_t *calibration,
                            int millivolts)
{
    float value = 0.0f;
    if ((calibration->dry_calibration_mv < calibration->normal_calibration_mv &&
         millivolts <= calibration->normal_calibration_mv) ||
        (calibration->dry_calibration_mv > calibration->normal_calibration_mv &&
         millivolts >= calibration->normal_calibration_mv)) {
        value = 50.0f * (float)(millivolts - calibration->dry_calibration_mv) /
                (float)(calibration->normal_calibration_mv -
                        calibration->dry_calibration_mv);
    } else {
        value = 50.0f +
                50.0f * (float)(millivolts - calibration->normal_calibration_mv) /
                    (float)(calibration->wet_calibration_mv -
                            calibration->normal_calibration_mv);
    }
    return clamp_pct(value);
}

static vazon_pot_moisture_class_t moisture_class(int16_t value_pct)
{
    if (value_pct < 30) return VAZON_POT_MOISTURE_CLASS_DRY;
    if (value_pct < 70) return VAZON_POT_MOISTURE_CLASS_NORMAL;
    if (value_pct < 90) return VAZON_POT_MOISTURE_CLASS_WET;
    return VAZON_POT_MOISTURE_CLASS_OVERWET;
}

esp_err_t vazon_pot_module_init(vazon_pot_module_t *module,
                                uint8_t pot_id,
                                const vazon_pot_settings_t *settings)
{
    if (module == NULL || pot_id > 1U || !settings_valid(settings)) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(module, 0, sizeof(*module));
    module->snapshot.pot_id = pot_id;
    module->snapshot.soil_moisture.value_pct = VAZON_POT_VALUE_UNKNOWN;
    module->snapshot.soil_moisture.moisture_class =
        VAZON_POT_MOISTURE_CLASS_UNKNOWN;
    module->snapshot.soil_moisture.status = VAZON_POT_SENSOR_UNKNOWN;
    module->snapshot.soil_temperature.status = VAZON_POT_SENSOR_UNKNOWN;
    module->snapshot.status = VAZON_POT_STATUS_UNKNOWN;
    module->snapshot.status_reason = VAZON_POT_REASON_NONE;
    module->snapshot.settings = *settings;
    module->initialized = true;
    return ESP_OK;
}

esp_err_t vazon_pot_module_record_moisture(vazon_pot_module_t *module,
                                           esp_err_t read_result,
                                           int raw_adc_value,
                                           int raw_mv,
                                           uint64_t now_ms)
{
    if (module == NULL || !module->initialized) return ESP_ERR_INVALID_STATE;
    if (!module->moisture_attempted) module->moisture_first_attempt_ms = now_ms;
    module->moisture_attempted = true;
    module->snapshot.soil_moisture.raw_adc_value = raw_adc_value;
    module->moisture_last_read_failed = read_result != ESP_OK;
    if (read_result != ESP_OK) return ESP_OK;
    if (raw_adc_value < 0 || raw_mv < 0) {
        module->moisture_last_read_failed = true;
        return ESP_OK;
    }
    module->snapshot.soil_moisture.raw_mv = raw_mv;
    module->moisture_has_valid_read = true;
    module->moisture_last_valid_ms = now_ms;
    return ESP_OK;
}

esp_err_t vazon_pot_module_record_temperature(vazon_pot_module_t *module,
                                              bool present,
                                              esp_err_t read_result,
                                              float temperature_c,
                                              uint64_t now_ms)
{
    if (module == NULL || !module->initialized) return ESP_ERR_INVALID_STATE;
    if (!module->temperature_attempted) module->temperature_first_attempt_ms = now_ms;
    module->temperature_attempted = true;
    module->snapshot.soil_temperature.present = present;
    module->temperature_last_read_failed = read_result != ESP_OK;
    if (!present || read_result != ESP_OK) return ESP_OK;
    if (!isfinite(temperature_c) || temperature_c < -55.0f ||
        temperature_c > 125.0f) {
        module->temperature_invalid = true;
        return ESP_OK;
    }
    module->temperature_invalid = false;
    module->snapshot.soil_temperature.temperature_c = temperature_c;
    module->temperature_has_valid_read = true;
    module->temperature_last_valid_ms = now_ms;
    return ESP_OK;
}

static bool is_stale(bool attempted,
                     bool has_valid,
                     uint64_t first_attempt_ms,
                     uint64_t last_valid_ms,
                     uint32_t timeout_sec,
                     uint64_t now_ms)
{
    if (!attempted) return false;
    const uint64_t reference_ms = has_valid ? last_valid_ms : first_attempt_ms;
    return now_ms - reference_ms > (uint64_t)timeout_sec * 1000ULL;
}

esp_err_t vazon_pot_module_evaluate(vazon_pot_module_t *module,
                                    uint64_t now_ms)
{
    if (module == NULL || !module->initialized) return ESP_ERR_INVALID_STATE;
    if (now_ms < module->last_evaluation_ms) return ESP_ERR_INVALID_ARG;
    module->last_evaluation_ms = now_ms;
    const vazon_pot_settings_t *settings = &module->snapshot.settings;
    const bool moisture_stale = is_stale(
        module->moisture_attempted, module->moisture_has_valid_read,
        module->moisture_first_attempt_ms, module->moisture_last_valid_ms,
        settings->moisture_stale_timeout_sec, now_ms);
    const bool temperature_stale = is_stale(
        module->temperature_attempted, module->temperature_has_valid_read,
        module->temperature_first_attempt_ms, module->temperature_last_valid_ms,
        settings->temperature_stale_timeout_sec, now_ms);

    vazon_pot_status_reason_t moisture_reason = VAZON_POT_REASON_NONE;
    if (!settings->soil_moisture_enabled) {
        module->snapshot.soil_moisture.status = VAZON_POT_SENSOR_INACTIVE;
        module->snapshot.soil_moisture.value_pct = VAZON_POT_VALUE_UNKNOWN;
        module->snapshot.soil_moisture.moisture_class =
            VAZON_POT_MOISTURE_CLASS_UNKNOWN;
    } else if (!calibration_order_valid(&module->snapshot.calibration)) {
        module->snapshot.soil_moisture.status = VAZON_POT_SENSOR_ERROR;
        module->snapshot.soil_moisture.value_pct = VAZON_POT_VALUE_UNKNOWN;
        module->snapshot.soil_moisture.moisture_class =
            VAZON_POT_MOISTURE_CLASS_UNKNOWN;
        moisture_reason = VAZON_POT_REASON_CALIBRATION_INVALID;
    } else if (moisture_stale) {
        module->snapshot.soil_moisture.status = VAZON_POT_SENSOR_ERROR;
        module->snapshot.soil_moisture.value_pct = VAZON_POT_VALUE_UNKNOWN;
        module->snapshot.soil_moisture.moisture_class =
            VAZON_POT_MOISTURE_CLASS_UNKNOWN;
        moisture_reason = VAZON_POT_REASON_MOISTURE_STALE;
    } else if (!module->moisture_has_valid_read) {
        module->snapshot.soil_moisture.status = VAZON_POT_SENSOR_ERROR;
        moisture_reason = VAZON_POT_REASON_MOISTURE_READ_FAILED;
    } else {
        module->snapshot.soil_moisture.value_pct = moisture_pct(
            &module->snapshot.calibration, module->snapshot.soil_moisture.raw_mv);
        module->snapshot.soil_moisture.moisture_class = moisture_class(
            module->snapshot.soil_moisture.value_pct);
        module->snapshot.soil_moisture.status =
            module->moisture_last_read_failed ? VAZON_POT_SENSOR_WARNING
                                              : VAZON_POT_SENSOR_OK;
        if (module->moisture_last_read_failed) {
            moisture_reason = VAZON_POT_REASON_MOISTURE_READ_FAILED;
        }
    }

    vazon_pot_status_reason_t temperature_reason = VAZON_POT_REASON_NONE;
    if (!settings->soil_temperature_enabled) {
        module->snapshot.soil_temperature.status = VAZON_POT_SENSOR_INACTIVE;
    } else if (!module->snapshot.soil_temperature.present) {
        module->snapshot.soil_temperature.status = VAZON_POT_SENSOR_ERROR;
        temperature_reason = VAZON_POT_REASON_TEMPERATURE_MISSING;
    } else if (module->temperature_invalid) {
        module->snapshot.soil_temperature.status = VAZON_POT_SENSOR_ERROR;
        temperature_reason = VAZON_POT_REASON_TEMPERATURE_INVALID;
    } else if (temperature_stale) {
        module->snapshot.soil_temperature.status = VAZON_POT_SENSOR_ERROR;
        temperature_reason = VAZON_POT_REASON_TEMPERATURE_STALE;
    } else if (!module->temperature_has_valid_read) {
        module->snapshot.soil_temperature.status = VAZON_POT_SENSOR_ERROR;
        temperature_reason = VAZON_POT_REASON_TEMPERATURE_READ_FAILED;
    } else if (module->snapshot.soil_temperature.temperature_c <
                   settings->temperature_low_warn_c ||
               module->snapshot.soil_temperature.temperature_c >
                   settings->temperature_high_warn_c) {
        module->snapshot.soil_temperature.status = VAZON_POT_SENSOR_WARNING;
        temperature_reason = VAZON_POT_REASON_TEMPERATURE_OUT_OF_RANGE;
    } else {
        module->snapshot.soil_temperature.status =
            module->temperature_last_read_failed ? VAZON_POT_SENSOR_WARNING
                                                 : VAZON_POT_SENSOR_OK;
        if (module->temperature_last_read_failed) {
            temperature_reason = VAZON_POT_REASON_TEMPERATURE_READ_FAILED;
        }
    }

    const vazon_pot_sensor_status_t moisture_status =
        module->snapshot.soil_moisture.status;
    const vazon_pot_sensor_status_t temperature_status =
        module->snapshot.soil_temperature.status;
    if (moisture_status == VAZON_POT_SENSOR_INACTIVE &&
        temperature_status == VAZON_POT_SENSOR_INACTIVE) {
        module->snapshot.status = VAZON_POT_STATUS_INACTIVE;
        module->snapshot.status_reason = VAZON_POT_REASON_NONE;
    } else if (moisture_status == VAZON_POT_SENSOR_ERROR ||
               temperature_status == VAZON_POT_SENSOR_ERROR) {
        module->snapshot.status = VAZON_POT_STATUS_ERROR;
        module->snapshot.status_reason = moisture_status == VAZON_POT_SENSOR_ERROR
                                             ? moisture_reason
                                             : temperature_reason;
    } else if (moisture_status == VAZON_POT_SENSOR_WARNING ||
               temperature_status == VAZON_POT_SENSOR_WARNING) {
        module->snapshot.status = VAZON_POT_STATUS_WARNING;
        module->snapshot.status_reason =
            moisture_status == VAZON_POT_SENSOR_WARNING ? moisture_reason
                                                        : temperature_reason;
    } else {
        module->snapshot.status = VAZON_POT_STATUS_OK;
        module->snapshot.status_reason = VAZON_POT_REASON_NONE;
    }
    return ESP_OK;
}

esp_err_t vazon_pot_module_set_settings(
    vazon_pot_module_t *module, const vazon_pot_settings_patch_t *patch)
{
    if (module == NULL || !module->initialized || patch == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    vazon_pot_settings_t candidate = module->snapshot.settings;
    if (patch->has_soil_moisture_enabled) {
        candidate.soil_moisture_enabled = patch->soil_moisture_enabled;
    }
    if (patch->has_soil_temperature_enabled) {
        candidate.soil_temperature_enabled = patch->soil_temperature_enabled;
    }
    if (patch->has_moisture_stale_timeout_sec) {
        candidate.moisture_stale_timeout_sec = patch->moisture_stale_timeout_sec;
    }
    if (patch->has_temperature_stale_timeout_sec) {
        candidate.temperature_stale_timeout_sec =
            patch->temperature_stale_timeout_sec;
    }
    if (patch->has_temperature_low_warn_c) {
        candidate.temperature_low_warn_c = patch->temperature_low_warn_c;
    }
    if (patch->has_temperature_high_warn_c) {
        candidate.temperature_high_warn_c = patch->temperature_high_warn_c;
    }
    if (!settings_valid(&candidate)) return ESP_ERR_INVALID_ARG;
    module->snapshot.settings = candidate;
    return vazon_pot_module_evaluate(module, module->last_evaluation_ms);
}

esp_err_t vazon_pot_module_calibrate_moisture(
    vazon_pot_module_t *module, vazon_pot_calibration_point_t point)
{
    if (module == NULL || !module->initialized) return ESP_ERR_INVALID_STATE;
    vazon_pot_moisture_calibration_t *calibration =
        &module->snapshot.calibration;
    if (point == VAZON_POT_CALIBRATION_RESET) {
        memset(calibration, 0, sizeof(*calibration));
        return ESP_OK;
    }
    if (!module->moisture_has_valid_read) return ESP_ERR_INVALID_STATE;
    const int current_mv = module->snapshot.soil_moisture.raw_mv;
    if (point == VAZON_POT_CALIBRATION_DRY) {
        calibration->dry_calibration_mv = current_mv;
        calibration->dry_valid = true;
        calibration->normal_valid = false;
        calibration->wet_valid = false;
        return ESP_OK;
    }
    if (point == VAZON_POT_CALIBRATION_NORMAL && calibration->dry_valid &&
        current_mv != calibration->dry_calibration_mv) {
        calibration->normal_calibration_mv = current_mv;
        calibration->normal_valid = true;
        calibration->wet_valid = false;
        return ESP_OK;
    }
    if (point == VAZON_POT_CALIBRATION_WET && calibration->normal_valid) {
        vazon_pot_moisture_calibration_t candidate = *calibration;
        candidate.wet_calibration_mv = current_mv;
        candidate.wet_valid = true;
        if (!calibration_order_valid(&candidate)) return ESP_ERR_INVALID_ARG;
        *calibration = candidate;
        return ESP_OK;
    }
    return ESP_ERR_INVALID_STATE;
}

static vazon_command_result_t command_result(vazon_command_result_status_t status,
                                             esp_err_t error)
{
    const vazon_command_result_t result = {.status = status, .error = error};
    return result;
}

static vazon_command_result_t pot_command_handler(const vazon_command_t *command,
                                                  void *context)
{
    vazon_pot_module_t *module = (vazon_pot_module_t *)context;
    if (command->args == NULL) {
        return command_result(VAZON_COMMAND_RESULT_REJECTED, ESP_ERR_INVALID_ARG);
    }

    esp_err_t result = ESP_ERR_NOT_SUPPORTED;
    if (strcmp(command->cmd, VAZON_COMMAND_POT_SET_SOIL_MOISTURE_ENABLED) == 0) {
        const vazon_pot_set_enabled_args_t *args = command->args;
        const vazon_pot_settings_patch_t patch = {
            .has_soil_moisture_enabled = true,
            .soil_moisture_enabled = args->enabled,
        };
        result = vazon_pot_module_set_settings(module, &patch);
    } else if (strcmp(command->cmd,
                      VAZON_COMMAND_POT_SET_SOIL_TEMPERATURE_ENABLED) == 0) {
        const vazon_pot_set_enabled_args_t *args = command->args;
        const vazon_pot_settings_patch_t patch = {
            .has_soil_temperature_enabled = true,
            .soil_temperature_enabled = args->enabled,
        };
        result = vazon_pot_module_set_settings(module, &patch);
    } else if (strcmp(command->cmd, VAZON_COMMAND_POT_SET_SETTINGS) == 0) {
        result = vazon_pot_module_set_settings(
            module, (const vazon_pot_settings_patch_t *)command->args);
    } else if (strcmp(command->cmd,
                      VAZON_COMMAND_POT_CALIBRATE_SOIL_MOISTURE) == 0) {
        const vazon_pot_calibration_args_t *args = command->args;
        result = vazon_pot_module_calibrate_moisture(module, args->point);
        if (result == ESP_OK) {
            result = vazon_pot_module_evaluate(module,
                                               module->last_evaluation_ms);
        }
    }

    return command_result(result == ESP_OK ? VAZON_COMMAND_RESULT_ACCEPTED
                                           : VAZON_COMMAND_RESULT_REJECTED,
                          result);
}

esp_err_t vazon_pot_module_register_command_target(
    vazon_pot_module_t *module, vazon_command_router_t *router)
{
    if (module == NULL || !module->initialized || router == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    const char *target = module->snapshot.pot_id == 0U
                             ? VAZON_COMMAND_TARGET_POT_0
                             : VAZON_COMMAND_TARGET_POT_1;
    return vazon_command_router_register(router, target, pot_command_handler,
                                         module);
}

const vazon_pot_snapshot_t *vazon_pot_module_get_snapshot(
    const vazon_pot_module_t *module)
{
    return module == NULL || !module->initialized ? NULL : &module->snapshot;
}
