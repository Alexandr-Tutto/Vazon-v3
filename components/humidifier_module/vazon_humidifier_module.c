#include "vazon_humidifier_module.h"

#include "vazon_gpio_levels.h"

#include <math.h>
#include <string.h>

static bool settings_valid(const vazon_humidifier_settings_t *settings)
{
    return settings != NULL &&
           (settings->mode == VAZON_HUMIDIFIER_MODE_AUTO ||
            settings->mode == VAZON_HUMIDIFIER_MODE_MANUAL) &&
           (settings->runtime == VAZON_HUMIDIFIER_RUNTIME_DAY ||
            settings->runtime == VAZON_HUMIDIFIER_RUNTIME_ALWAYS) &&
           isfinite(settings->rh_start) && isfinite(settings->rh_stop) &&
           isfinite(settings->rh_delta) && settings->rh_start >= 0.0f &&
           settings->rh_stop <= 90.0f && settings->rh_delta >= 5.0f &&
           settings->rh_stop - settings->rh_start >= settings->rh_delta &&
           settings->mist_power_level >= VAZON_HUMIDIFIER_MIST_POWER_LOW &&
           settings->mist_power_level <= VAZON_HUMIDIFIER_MIST_POWER_HIGH &&
           settings->manual_mist_duration_sec > 0U && settings->post_fan_sec <= 600U;
}

esp_err_t vazon_humidifier_module_init(
    vazon_humidifier_module_t *module,
    const vazon_humidifier_config_t *config,
    const vazon_humidifier_settings_t *settings)
{
    if (module == NULL || config == NULL || config->water_debounce_ms == 0U ||
        config->water_unstable_timeout_ms < config->water_debounce_ms ||
        !settings_valid(settings)) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(module, 0, sizeof(*module));
    module->config = *config;
    module->snapshot.water_status = VAZON_HUMIDIFIER_WATER_UNKNOWN;
    module->snapshot.mist_output = VAZON_HUMIDIFIER_OUTPUT_UNKNOWN;
    module->snapshot.mist_output_request = VAZON_HUMIDIFIER_OUTPUT_OFF;
    module->snapshot.fan_output = VAZON_HUMIDIFIER_FAN_UNKNOWN;
    module->snapshot.fan_output_request = VAZON_HUMIDIFIER_FAN_OFF;
    module->snapshot.status = VAZON_HUMIDIFIER_STATUS_UNKNOWN;
    module->snapshot.status_reason = VAZON_HUMIDIFIER_STATUS_REASON_NONE;
    module->snapshot.last_command_result = VAZON_COMMAND_RESULT_UNKNOWN;
    module->snapshot.last_mist_output_confirmed = VAZON_TRISTATE_UNKNOWN;
    module->snapshot.last_fan_output_confirmed = VAZON_TRISTATE_UNKNOWN;
    module->snapshot.settings = *settings;
    module->initialized = true;
    return ESP_OK;
}

static void accept_water_candidate(vazon_humidifier_module_t *module)
{
    module->water_accepted_level = module->water_candidate_level;
    module->water_has_accepted_level = true;
    module->water_transition_active = false;
    module->water_unstable = false;
    module->snapshot.water_status =
        module->water_accepted_level == VAZON_WATER_PRESENT_LEVEL
            ? VAZON_HUMIDIFIER_WATER_PRESENT
            : VAZON_HUMIDIFIER_WATER_EMPTY;
}

static void update_water(vazon_humidifier_module_t *module,
                         bool raw_valid,
                         int raw_level,
                         uint64_t now_ms)
{
    if (!raw_valid || (raw_level != VAZON_WATER_EMPTY_LEVEL &&
                       raw_level != VAZON_WATER_PRESENT_LEVEL)) {
        module->water_unstable = true;
        module->water_transition_active = false;
        module->snapshot.water_status = VAZON_HUMIDIFIER_WATER_UNKNOWN;
        return;
    }

    if (!module->water_transition_active) {
        if (module->water_has_accepted_level && raw_level == module->water_accepted_level &&
            !module->water_unstable) {
            return;
        }
        module->water_transition_active = true;
        module->water_transition_since_ms = now_ms;
        module->water_candidate_since_ms = now_ms;
        module->water_candidate_level = raw_level;
    } else if (raw_level != module->water_candidate_level) {
        module->water_candidate_level = raw_level;
        module->water_candidate_since_ms = now_ms;
    }

    if (now_ms - module->water_candidate_since_ms >= module->config.water_debounce_ms) {
        accept_water_candidate(module);
    } else if (now_ms - module->water_transition_since_ms >=
               module->config.water_unstable_timeout_ms) {
        module->water_unstable = true;
        module->snapshot.water_status = VAZON_HUMIDIFIER_WATER_UNKNOWN;
    }
}

static bool climate_values_valid(const vazon_humidifier_inputs_t *inputs)
{
    return inputs->climate_valid && !inputs->climate_stale &&
           isfinite(inputs->zone_0_rh_pct) && isfinite(inputs->zone_1_rh_pct) &&
           inputs->zone_0_rh_pct >= 0.0f && inputs->zone_0_rh_pct <= 100.0f &&
           inputs->zone_1_rh_pct >= 0.0f && inputs->zone_1_rh_pct <= 100.0f;
}

static void update_output_requests(vazon_humidifier_module_t *module,
                                   bool mist_demand,
                                   bool allow_post_fan,
                                   uint64_t now_ms)
{
    if (!allow_post_fan) {
        module->post_fan_active = false;
    } else if (module->mist_requested_on && !mist_demand &&
               module->snapshot.settings.post_fan_sec > 0U) {
        module->post_fan_active = true;
        module->post_fan_deadline_ms =
            now_ms + (uint64_t)module->snapshot.settings.post_fan_sec * 1000ULL;
    }

    if (mist_demand) module->post_fan_active = false;
    if (module->post_fan_active && now_ms >= module->post_fan_deadline_ms) {
        module->post_fan_active = false;
    }

    module->mist_requested_on = mist_demand;
    module->snapshot.mist_output_request = mist_demand
                                               ? VAZON_HUMIDIFIER_OUTPUT_ON
                                               : VAZON_HUMIDIFIER_OUTPUT_OFF;
    module->snapshot.fan_output_request =
        mist_demand ? VAZON_HUMIDIFIER_FAN_ON
                    : (module->post_fan_active ? VAZON_HUMIDIFIER_FAN_POST_RUN
                                               : VAZON_HUMIDIFIER_FAN_OFF);
}

esp_err_t vazon_humidifier_module_evaluate(
    vazon_humidifier_module_t *module,
    const vazon_humidifier_inputs_t *inputs)
{
    if (module == NULL || !module->initialized || inputs == NULL ||
        inputs->global_context == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (module->has_last_evaluation && inputs->now_ms < module->last_evaluation_ms) {
        return ESP_ERR_INVALID_ARG;
    }
    module->has_last_evaluation = true;
    module->last_evaluation_ms = inputs->now_ms;
    update_water(module, inputs->water_raw_valid, inputs->water_raw_level,
                 inputs->now_ms);

    bool mist_demand = false;
    bool allow_post_fan = true;
    module->snapshot.status = VAZON_HUMIDIFIER_STATUS_OK;
    module->snapshot.status_reason = VAZON_HUMIDIFIER_STATUS_REASON_NONE;

    if (inputs->global_context->maintenance.manual_active ||
        inputs->global_context->maintenance.external_active) {
        allow_post_fan = false;
    } else if (inputs->door_state == VAZON_DOOR_STATE_OPEN) {
        allow_post_fan = false;
        module->snapshot.status = VAZON_HUMIDIFIER_STATUS_WARNING;
        module->snapshot.status_reason = VAZON_HUMIDIFIER_STATUS_REASON_DOOR_OPEN;
    } else if (inputs->door_state == VAZON_DOOR_STATE_UNKNOWN) {
        allow_post_fan = false;
        module->snapshot.status = VAZON_HUMIDIFIER_STATUS_WARNING;
        module->snapshot.status_reason = VAZON_HUMIDIFIER_STATUS_REASON_DOOR_UNKNOWN;
    } else if (module->snapshot.water_status == VAZON_HUMIDIFIER_WATER_UNKNOWN) {
        module->snapshot.status = module->water_unstable
                                      ? VAZON_HUMIDIFIER_STATUS_ERROR
                                      : VAZON_HUMIDIFIER_STATUS_UNKNOWN;
        module->snapshot.status_reason = module->water_unstable
                                             ? VAZON_HUMIDIFIER_STATUS_REASON_WATER_UNKNOWN
                                             : VAZON_HUMIDIFIER_STATUS_REASON_NONE;
    } else if (module->snapshot.water_status == VAZON_HUMIDIFIER_WATER_EMPTY) {
        module->snapshot.status = VAZON_HUMIDIFIER_STATUS_ERROR;
        module->snapshot.status_reason = VAZON_HUMIDIFIER_STATUS_REASON_NO_WATER;
    } else if (module->snapshot.settings.mode == VAZON_HUMIDIFIER_MODE_MANUAL) {
        if (module->manual_mist_active &&
            inputs->now_ms >= module->manual_mist_deadline_ms) {
            module->manual_mist_active = false;
        }
        mist_demand = module->manual_mist_active;
    } else if (!climate_values_valid(inputs)) {
        module->snapshot.status = VAZON_HUMIDIFIER_STATUS_ERROR;
        module->snapshot.status_reason = VAZON_HUMIDIFIER_STATUS_REASON_CLIMATE_INVALID;
    } else if (module->snapshot.settings.runtime == VAZON_HUMIDIFIER_RUNTIME_DAY &&
               inputs->global_context->day_window.active != VAZON_TRISTATE_TRUE) {
        if (inputs->global_context->day_window.active == VAZON_TRISTATE_UNKNOWN) {
            module->snapshot.status = VAZON_HUMIDIFIER_STATUS_WARNING;
            module->snapshot.status_reason =
                VAZON_HUMIDIFIER_STATUS_REASON_DAY_WINDOW_UNKNOWN;
        }
    } else {
        const float minimum_rh = fminf(inputs->zone_0_rh_pct, inputs->zone_1_rh_pct);
        const float maximum_rh = fmaxf(inputs->zone_0_rh_pct, inputs->zone_1_rh_pct);
        if (minimum_rh <= module->snapshot.settings.rh_start) {
            module->automatic_demand = true;
        }
        if (maximum_rh >= module->snapshot.settings.rh_stop) {
            module->automatic_demand = false;
        }
        mist_demand = module->automatic_demand;
    }

    update_output_requests(module, mist_demand, allow_post_fan, inputs->now_ms);
    if (module->mist_output_failed || module->fan_output_failed) {
        module->snapshot.status = VAZON_HUMIDIFIER_STATUS_ERROR;
        module->snapshot.status_reason = VAZON_HUMIDIFIER_STATUS_REASON_OUTPUT_FAILED;
    }
    return ESP_OK;
}

esp_err_t vazon_humidifier_module_manual_mist(
    vazon_humidifier_module_t *module, uint64_t now_ms)
{
    if (module == NULL || !module->initialized ||
        module->snapshot.settings.mode != VAZON_HUMIDIFIER_MODE_MANUAL) {
        return ESP_ERR_INVALID_STATE;
    }
    const uint64_t duration_ms =
        (uint64_t)module->snapshot.settings.manual_mist_duration_sec * 1000ULL;
    if (UINT64_MAX - now_ms < duration_ms) return ESP_ERR_INVALID_ARG;
    module->manual_mist_active = true;
    module->manual_mist_deadline_ms = now_ms + duration_ms;
    return ESP_OK;
}

esp_err_t vazon_humidifier_module_stop(vazon_humidifier_module_t *module)
{
    if (module == NULL || !module->initialized) return ESP_ERR_INVALID_STATE;
    module->manual_mist_active = false;
    module->automatic_demand = false;
    return ESP_OK;
}

esp_err_t vazon_humidifier_module_set_settings(
    vazon_humidifier_module_t *module,
    const vazon_humidifier_settings_patch_t *patch)
{
    if (module == NULL || !module->initialized || patch == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    vazon_humidifier_settings_t candidate = module->snapshot.settings;
    if (patch->has_mode) candidate.mode = patch->mode;
    if (patch->has_runtime) candidate.runtime = patch->runtime;
    if (patch->has_rh_start) candidate.rh_start = patch->rh_start;
    if (patch->has_rh_stop) candidate.rh_stop = patch->rh_stop;
    if (patch->has_rh_delta) candidate.rh_delta = patch->rh_delta;
    if (patch->has_mist_power_level) {
        candidate.mist_power_level = patch->mist_power_level;
    }
    if (patch->has_manual_mist_duration_sec) {
        candidate.manual_mist_duration_sec = patch->manual_mist_duration_sec;
    }
    if (patch->has_post_fan_sec) candidate.post_fan_sec = patch->post_fan_sec;
    if (!settings_valid(&candidate)) return ESP_ERR_INVALID_ARG;

    if (candidate.mode != module->snapshot.settings.mode) {
        module->manual_mist_active = false;
        module->automatic_demand = false;
    }
    module->snapshot.settings = candidate;
    return ESP_OK;
}

esp_err_t vazon_humidifier_module_report_mist_output(
    vazon_humidifier_module_t *module, bool on, esp_err_t driver_result)
{
    if (module == NULL || !module->initialized) return ESP_ERR_INVALID_STATE;
    module->mist_output_failed = driver_result != ESP_OK;
    if (driver_result != ESP_OK) return driver_result;
    module->snapshot.mist_output = on ? VAZON_HUMIDIFIER_OUTPUT_ON
                                      : VAZON_HUMIDIFIER_OUTPUT_OFF;
    module->snapshot.last_mist_output_confirmed = VAZON_TRISTATE_UNKNOWN;
    return ESP_OK;
}

esp_err_t vazon_humidifier_module_report_fan_output(
    vazon_humidifier_module_t *module, bool on, esp_err_t driver_result)
{
    if (module == NULL || !module->initialized) return ESP_ERR_INVALID_STATE;
    module->fan_output_failed = driver_result != ESP_OK;
    if (driver_result != ESP_OK) return driver_result;
    module->snapshot.fan_output = on
                                      ? (module->post_fan_active
                                             ? VAZON_HUMIDIFIER_FAN_POST_RUN
                                             : VAZON_HUMIDIFIER_FAN_ON)
                                      : VAZON_HUMIDIFIER_FAN_OFF;
    module->snapshot.last_fan_output_confirmed = VAZON_TRISTATE_UNKNOWN;
    return ESP_OK;
}

static vazon_command_result_t command_result(vazon_command_result_status_t status,
                                             esp_err_t error)
{
    const vazon_command_result_t result = {.status = status, .error = error};
    return result;
}

static vazon_command_result_t humidifier_command_handler(
    const vazon_command_t *command, void *context)
{
    vazon_humidifier_module_t *module =
        (vazon_humidifier_module_t *)context;
    esp_err_t result = ESP_ERR_NOT_SUPPORTED;
    if (strcmp(command->cmd, VAZON_COMMAND_HUMIDIFIER_MANUAL_MIST) == 0) {
        result = vazon_humidifier_module_manual_mist(
            module, module->last_evaluation_ms);
    } else if (strcmp(command->cmd, VAZON_COMMAND_HUMIDIFIER_STOP) == 0) {
        result = vazon_humidifier_module_stop(module);
    } else if (command->args == NULL) {
        result = ESP_ERR_INVALID_ARG;
    } else if (strcmp(command->cmd, VAZON_COMMAND_HUMIDIFIER_SET_MODE) == 0) {
        const vazon_humidifier_set_mode_args_t *args = command->args;
        const vazon_humidifier_settings_patch_t patch = {
            .has_mode = true,
            .mode = args->mode,
        };
        result = vazon_humidifier_module_set_settings(module, &patch);
    } else if (strcmp(command->cmd,
                      VAZON_COMMAND_HUMIDIFIER_SET_RUNTIME) == 0) {
        const vazon_humidifier_set_runtime_args_t *args = command->args;
        const vazon_humidifier_settings_patch_t patch = {
            .has_runtime = true,
            .runtime = args->runtime,
        };
        result = vazon_humidifier_module_set_settings(module, &patch);
    } else if (strcmp(command->cmd,
                      VAZON_COMMAND_HUMIDIFIER_SET_MIST_POWER_LEVEL) == 0) {
        const vazon_humidifier_set_mist_power_args_t *args = command->args;
        const vazon_humidifier_settings_patch_t patch = {
            .has_mist_power_level = true,
            .mist_power_level = args->mist_power_level,
        };
        result = vazon_humidifier_module_set_settings(module, &patch);
    } else if (strcmp(command->cmd,
                      VAZON_COMMAND_HUMIDIFIER_SET_SETTINGS) == 0) {
        result = vazon_humidifier_module_set_settings(
            module, (const vazon_humidifier_settings_patch_t *)command->args);
    }

    module->snapshot.last_command_result =
        result == ESP_OK ? VAZON_COMMAND_RESULT_ACCEPTED
                         : VAZON_COMMAND_RESULT_REJECTED;
    return command_result(module->snapshot.last_command_result, result);
}

esp_err_t vazon_humidifier_module_register_command_target(
    vazon_humidifier_module_t *module, vazon_command_router_t *router)
{
    if (module == NULL || !module->initialized || router == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return vazon_command_router_register(
        router, VAZON_COMMAND_TARGET_HUMIDIFIER,
        humidifier_command_handler, module);
}

const vazon_humidifier_snapshot_t *vazon_humidifier_module_get_snapshot(
    const vazon_humidifier_module_t *module)
{
    return module == NULL || !module->initialized ? NULL : &module->snapshot;
}
