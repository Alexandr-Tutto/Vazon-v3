#include "vazon_fan_module.h"

#include <math.h>
#include <string.h>

#define FAN_PWM_TRANSITION_MS 1000ULL
#define FAN_MAX_DURATION_SEC 86400U

static bool power_level_valid(uint8_t power_level_pct)
{
    return power_level_pct == 20U || power_level_pct == 40U ||
           power_level_pct == 60U || power_level_pct == 80U ||
           power_level_pct == 100U;
}

esp_err_t vazon_fan_module_validate_settings(const vazon_fan_settings_t *settings)
{
    if (settings == NULL ||
        (settings->mode != VAZON_FAN_MODE_AUTO &&
         settings->mode != VAZON_FAN_MODE_MANUAL) ||
        (settings->runtime != VAZON_FAN_RUNTIME_DAY &&
         settings->runtime != VAZON_FAN_RUNTIME_ALWAYS) ||
        (settings->auto_strategy != VAZON_FAN_AUTO_STRATEGY_DELTA &&
         settings->auto_strategy != VAZON_FAN_AUTO_STRATEGY_TIMER) ||
        settings->manual_duration_sec < 10U ||
        settings->manual_duration_sec > FAN_MAX_DURATION_SEC ||
        settings->auto_timer_on_sec == 0U ||
        settings->auto_timer_on_sec > FAN_MAX_DURATION_SEC ||
        settings->auto_timer_off_sec == 0U ||
        settings->auto_timer_off_sec > FAN_MAX_DURATION_SEC ||
        !isfinite(settings->auto_delta_on_pct) ||
        !isfinite(settings->auto_delta_off_pct) ||
        settings->auto_delta_off_pct < 0.0f ||
        settings->auto_delta_on_pct > 100.0f ||
        settings->auto_delta_on_pct - settings->auto_delta_off_pct < 5.0f ||
        !power_level_valid(settings->power_level_pct)) {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

esp_err_t vazon_fan_module_init(vazon_fan_module_t *module,
                                const vazon_fan_settings_t *settings)
{
    if (module == NULL || vazon_fan_module_validate_settings(settings) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(module, 0, sizeof(*module));
    module->snapshot.output = VAZON_FAN_OUTPUT_UNKNOWN;
    module->snapshot.output_request = VAZON_FAN_OUTPUT_OFF;
    module->snapshot.requested_power_pct = 0;
    module->snapshot.applied_power_pct = VAZON_FAN_POWER_UNKNOWN;
    module->snapshot.auto_state = VAZON_FAN_AUTO_STATE_UNKNOWN;
    module->snapshot.status = VAZON_FAN_STATUS_UNKNOWN;
    module->snapshot.status_reason = VAZON_FAN_STATUS_REASON_NONE;
    module->snapshot.last_command_result = VAZON_COMMAND_RESULT_UNKNOWN;
    module->snapshot.last_output_confirmed = VAZON_TRISTATE_UNKNOWN;
    module->snapshot.settings = *settings;
    module->pwm_phase = VAZON_FAN_PWM_PHASE_OFF;
    module->pwm_target_pct = settings->power_level_pct;
    module->initialized = true;
    return ESP_OK;
}

static uint8_t ramp_value(uint8_t start, uint8_t target, uint64_t elapsed_ms)
{
    if (elapsed_ms >= FAN_PWM_TRANSITION_MS) return target;

    const int32_t difference = (int32_t)target - (int32_t)start;
    const int32_t scaled = difference * (int32_t)elapsed_ms;
    const int32_t value = (int32_t)start + scaled / (int32_t)FAN_PWM_TRANSITION_MS;
    return (uint8_t)value;
}

static uint8_t current_pwm_request(vazon_fan_module_t *module, uint64_t now_ms)
{
    uint64_t elapsed = now_ms - module->pwm_phase_started_ms;
    if (module->pwm_phase == VAZON_FAN_PWM_PHASE_BOOST) {
        if (elapsed < FAN_PWM_TRANSITION_MS) return 100U;
        if (module->pwm_target_pct == 100U) {
            module->pwm_phase = VAZON_FAN_PWM_PHASE_STEADY;
            return 100U;
        }
        module->pwm_phase = VAZON_FAN_PWM_PHASE_RAMP;
        module->pwm_phase_started_ms += FAN_PWM_TRANSITION_MS;
        module->pwm_ramp_start_pct = 100U;
        elapsed = now_ms - module->pwm_phase_started_ms;
    }

    if (module->pwm_phase == VAZON_FAN_PWM_PHASE_RAMP) {
        if (elapsed >= FAN_PWM_TRANSITION_MS) {
            module->pwm_phase = VAZON_FAN_PWM_PHASE_STEADY;
            return module->pwm_target_pct;
        }
        return ramp_value(module->pwm_ramp_start_pct,
                          module->pwm_target_pct, elapsed);
    }

    return module->pwm_target_pct;
}

static void update_pwm_request(vazon_fan_module_t *module,
                               bool demand_on,
                               uint64_t now_ms)
{
    if (!demand_on) {
        module->requested_on = false;
        module->pwm_phase = VAZON_FAN_PWM_PHASE_OFF;
        module->snapshot.output_request = VAZON_FAN_OUTPUT_OFF;
        module->snapshot.requested_power_pct = 0;
        return;
    }

    if (!module->requested_on) {
        module->requested_on = true;
        module->pwm_phase = VAZON_FAN_PWM_PHASE_BOOST;
        module->pwm_phase_started_ms = now_ms;
        module->pwm_target_pct = module->snapshot.settings.power_level_pct;
        module->snapshot.output_request = VAZON_FAN_OUTPUT_ON;
        module->snapshot.requested_power_pct = 100;
        return;
    }

    uint8_t current = current_pwm_request(module, now_ms);
    if (module->pwm_target_pct != module->snapshot.settings.power_level_pct) {
        module->pwm_phase = VAZON_FAN_PWM_PHASE_RAMP;
        module->pwm_phase_started_ms = now_ms;
        module->pwm_ramp_start_pct = current;
        module->pwm_target_pct = module->snapshot.settings.power_level_pct;
        current = module->pwm_ramp_start_pct;
    }

    module->snapshot.output_request = VAZON_FAN_OUTPUT_ON;
    module->snapshot.requested_power_pct = current_pwm_request(module, now_ms);
}

static void reset_timer_cycle(vazon_fan_module_t *module)
{
    module->timer_cycle_valid = false;
}

static bool timer_demand(vazon_fan_module_t *module, uint64_t now_ms)
{
    if (!module->timer_cycle_valid) {
        module->timer_cycle_valid = true;
        module->timer_cycle_started_ms = now_ms;
        return true;
    }

    const uint64_t on_ms = (uint64_t)module->snapshot.settings.auto_timer_on_sec * 1000ULL;
    const uint64_t off_ms = (uint64_t)module->snapshot.settings.auto_timer_off_sec * 1000ULL;
    const uint64_t cycle_ms = on_ms + off_ms;
    const uint64_t position = (now_ms - module->timer_cycle_started_ms) % cycle_ms;
    return position < on_ms;
}

esp_err_t vazon_fan_module_evaluate(vazon_fan_module_t *module,
                                    const vazon_fan_inputs_t *inputs)
{
    if (module == NULL || !module->initialized || inputs == NULL ||
        inputs->global_context == NULL ||
        (inputs->door_state != VAZON_DOOR_STATE_UNKNOWN &&
         inputs->door_state != VAZON_DOOR_STATE_CLOSED &&
         inputs->door_state != VAZON_DOOR_STATE_OPEN)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (module->has_last_evaluation && inputs->now_ms < module->last_evaluation_ms) {
        return ESP_ERR_INVALID_ARG;
    }
    module->has_last_evaluation = true;
    module->last_evaluation_ms = inputs->now_ms;

    bool demand_on = false;
    module->snapshot.status = VAZON_FAN_STATUS_OK;
    module->snapshot.status_reason = VAZON_FAN_STATUS_REASON_NONE;

    if (inputs->global_context->maintenance.manual_active ||
        inputs->global_context->maintenance.external_active) {
        module->snapshot.auto_state = VAZON_FAN_AUTO_STATE_BLOCKED;
        reset_timer_cycle(module);
    } else if (inputs->door_state == VAZON_DOOR_STATE_OPEN) {
        module->snapshot.auto_state = VAZON_FAN_AUTO_STATE_BLOCKED;
        module->snapshot.status = VAZON_FAN_STATUS_WARNING;
        module->snapshot.status_reason = VAZON_FAN_STATUS_REASON_DOOR_OPEN;
        reset_timer_cycle(module);
    } else if (inputs->door_state == VAZON_DOOR_STATE_UNKNOWN) {
        module->snapshot.auto_state = VAZON_FAN_AUTO_STATE_BLOCKED;
        module->snapshot.status = VAZON_FAN_STATUS_WARNING;
        module->snapshot.status_reason = VAZON_FAN_STATUS_REASON_DOOR_UNKNOWN;
        reset_timer_cycle(module);
    } else if (module->snapshot.settings.mode == VAZON_FAN_MODE_MANUAL) {
        if (module->manual_run_active && inputs->now_ms >= module->manual_deadline_ms) {
            module->manual_run_active = false;
        }
        demand_on = module->manual_run_active;
        module->snapshot.auto_state = demand_on ? VAZON_FAN_AUTO_STATE_RUNNING
                                                : VAZON_FAN_AUTO_STATE_OFF;
        reset_timer_cycle(module);
    } else if (module->snapshot.settings.runtime == VAZON_FAN_RUNTIME_DAY &&
               inputs->global_context->day_window.active != VAZON_TRISTATE_TRUE) {
        module->snapshot.auto_state =
            inputs->global_context->day_window.active == VAZON_TRISTATE_UNKNOWN
                ? VAZON_FAN_AUTO_STATE_ALERT
                : VAZON_FAN_AUTO_STATE_OFF;
        if (inputs->global_context->day_window.active == VAZON_TRISTATE_UNKNOWN) {
            module->snapshot.status = VAZON_FAN_STATUS_WARNING;
            module->snapshot.status_reason = VAZON_FAN_STATUS_REASON_DAY_WINDOW_UNKNOWN;
        }
        reset_timer_cycle(module);
    } else if (module->snapshot.settings.auto_strategy ==
               VAZON_FAN_AUTO_STRATEGY_DELTA) {
        reset_timer_cycle(module);
        if (!inputs->climate_delta_valid || inputs->climate_delta_stale ||
            !isfinite(inputs->rh_delta_pct) || inputs->rh_delta_pct < 0.0f ||
            inputs->rh_delta_pct > 100.0f) {
            module->snapshot.auto_state = VAZON_FAN_AUTO_STATE_ALERT;
            module->snapshot.status = VAZON_FAN_STATUS_WARNING;
            module->snapshot.status_reason = VAZON_FAN_STATUS_REASON_CLIMATE_INVALID;
        } else {
            if (inputs->rh_delta_pct >= module->snapshot.settings.auto_delta_on_pct) {
                module->delta_demand = true;
            } else if (inputs->rh_delta_pct <=
                       module->snapshot.settings.auto_delta_off_pct) {
                module->delta_demand = false;
            }
            demand_on = module->delta_demand;
            module->snapshot.auto_state = demand_on ? VAZON_FAN_AUTO_STATE_RUNNING
                                                    : VAZON_FAN_AUTO_STATE_OFF;
        }
    } else {
        demand_on = timer_demand(module, inputs->now_ms);
        module->snapshot.auto_state = demand_on ? VAZON_FAN_AUTO_STATE_RUNNING
                                                : VAZON_FAN_AUTO_STATE_PAUSE;
    }

    update_pwm_request(module, demand_on, inputs->now_ms);
    if (module->output_failed) {
        module->snapshot.status = VAZON_FAN_STATUS_ERROR;
        module->snapshot.status_reason = VAZON_FAN_STATUS_REASON_OUTPUT_FAILED;
    }
    return ESP_OK;
}

esp_err_t vazon_fan_module_set_settings(vazon_fan_module_t *module,
                                        const vazon_fan_settings_patch_t *patch)
{
    if (module == NULL || !module->initialized || patch == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    vazon_fan_settings_t candidate = module->snapshot.settings;
    if (patch->has_mode) candidate.mode = patch->mode;
    if (patch->has_runtime) candidate.runtime = patch->runtime;
    if (patch->has_auto_strategy) candidate.auto_strategy = patch->auto_strategy;
    if (patch->has_manual_duration_sec) {
        candidate.manual_duration_sec = patch->manual_duration_sec;
    }
    if (patch->has_auto_delta_on_pct) candidate.auto_delta_on_pct = patch->auto_delta_on_pct;
    if (patch->has_auto_delta_off_pct) candidate.auto_delta_off_pct = patch->auto_delta_off_pct;
    if (patch->has_auto_timer_on_sec) candidate.auto_timer_on_sec = patch->auto_timer_on_sec;
    if (patch->has_auto_timer_off_sec) candidate.auto_timer_off_sec = patch->auto_timer_off_sec;
    if (patch->has_power_level_pct) candidate.power_level_pct = patch->power_level_pct;

    if (vazon_fan_module_validate_settings(&candidate) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }

    if (candidate.mode != module->snapshot.settings.mode) {
        module->manual_run_active = false;
        module->delta_demand = false;
        reset_timer_cycle(module);
    }
    if (candidate.runtime != module->snapshot.settings.runtime ||
        candidate.auto_strategy != module->snapshot.settings.auto_strategy ||
        candidate.auto_timer_on_sec != module->snapshot.settings.auto_timer_on_sec ||
        candidate.auto_timer_off_sec != module->snapshot.settings.auto_timer_off_sec) {
        reset_timer_cycle(module);
    }
    if (candidate.auto_strategy != module->snapshot.settings.auto_strategy) {
        module->delta_demand = false;
    }

    module->snapshot.settings = candidate;
    return ESP_OK;
}

esp_err_t vazon_fan_module_manual_run(vazon_fan_module_t *module, uint64_t now_ms)
{
    if (module == NULL || !module->initialized ||
        module->snapshot.settings.mode != VAZON_FAN_MODE_MANUAL) {
        return ESP_ERR_INVALID_STATE;
    }
    const uint64_t duration_ms =
        (uint64_t)module->snapshot.settings.manual_duration_sec * 1000ULL;
    if (UINT64_MAX - now_ms < duration_ms) {
        return ESP_ERR_INVALID_ARG;
    }

    module->manual_run_active = true;
    module->manual_deadline_ms = now_ms + duration_ms;
    return ESP_OK;
}

esp_err_t vazon_fan_module_stop(vazon_fan_module_t *module)
{
    if (module == NULL || !module->initialized) return ESP_ERR_INVALID_STATE;
    module->manual_run_active = false;
    return ESP_OK;
}

esp_err_t vazon_fan_module_report_output_result(vazon_fan_module_t *module,
                                                uint8_t applied_power_pct,
                                                esp_err_t driver_result)
{
    if (module == NULL || !module->initialized || applied_power_pct > 100U) {
        return ESP_ERR_INVALID_ARG;
    }
    if (driver_result != ESP_OK) {
        module->output_failed = true;
        module->snapshot.status = VAZON_FAN_STATUS_ERROR;
        module->snapshot.status_reason = VAZON_FAN_STATUS_REASON_OUTPUT_FAILED;
        module->snapshot.last_command_result = VAZON_COMMAND_RESULT_FAILED;
        return driver_result;
    }

    module->output_failed = false;
    module->snapshot.applied_power_pct = applied_power_pct;
    module->snapshot.output = applied_power_pct == 0U ? VAZON_FAN_OUTPUT_OFF
                                                      : VAZON_FAN_OUTPUT_ON;
    module->snapshot.last_command_result = VAZON_COMMAND_RESULT_ACCEPTED;
    module->snapshot.last_output_confirmed = VAZON_TRISTATE_UNKNOWN;
    return ESP_OK;
}

static vazon_command_result_t command_result(vazon_command_result_status_t status,
                                             esp_err_t error)
{
    const vazon_command_result_t result = {.status = status, .error = error};
    return result;
}

static vazon_command_result_t fan_command_handler(const vazon_command_t *command,
                                                  void *context)
{
    vazon_fan_module_t *module = (vazon_fan_module_t *)context;
    esp_err_t result = ESP_ERR_NOT_SUPPORTED;
    if (strcmp(command->cmd, VAZON_COMMAND_FAN_MANUAL_RUN) == 0) {
        result = vazon_fan_module_manual_run(module, module->last_evaluation_ms);
    } else if (strcmp(command->cmd, VAZON_COMMAND_FAN_STOP) == 0) {
        result = vazon_fan_module_stop(module);
    } else if (command->args == NULL) {
        result = ESP_ERR_INVALID_ARG;
    } else if (strcmp(command->cmd, VAZON_COMMAND_FAN_SET_MODE) == 0) {
        const vazon_fan_set_mode_args_t *args = command->args;
        const vazon_fan_settings_patch_t patch = {
            .has_mode = true,
            .mode = args->mode,
        };
        result = vazon_fan_module_set_settings(module, &patch);
    } else if (strcmp(command->cmd, VAZON_COMMAND_FAN_SET_RUNTIME) == 0) {
        const vazon_fan_set_runtime_args_t *args = command->args;
        const vazon_fan_settings_patch_t patch = {
            .has_runtime = true,
            .runtime = args->runtime,
        };
        result = vazon_fan_module_set_settings(module, &patch);
    } else if (strcmp(command->cmd, VAZON_COMMAND_FAN_SET_AUTO_STRATEGY) == 0) {
        const vazon_fan_set_auto_strategy_args_t *args = command->args;
        const vazon_fan_settings_patch_t patch = {
            .has_auto_strategy = true,
            .auto_strategy = args->auto_strategy,
        };
        result = vazon_fan_module_set_settings(module, &patch);
    } else if (strcmp(command->cmd, VAZON_COMMAND_FAN_SET_POWER_LEVEL) == 0) {
        const vazon_fan_set_power_level_args_t *args = command->args;
        const vazon_fan_settings_patch_t patch = {
            .has_power_level_pct = true,
            .power_level_pct = args->power_level_pct,
        };
        result = vazon_fan_module_set_settings(module, &patch);
    } else if (strcmp(command->cmd, VAZON_COMMAND_FAN_SET_SETTINGS) == 0) {
        result = vazon_fan_module_set_settings(
            module, (const vazon_fan_settings_patch_t *)command->args);
    }

    module->snapshot.last_command_result =
        result == ESP_OK ? VAZON_COMMAND_RESULT_ACCEPTED
                         : VAZON_COMMAND_RESULT_REJECTED;
    return command_result(module->snapshot.last_command_result, result);
}

esp_err_t vazon_fan_module_register_command_target(
    vazon_fan_module_t *module, vazon_command_router_t *router)
{
    if (module == NULL || !module->initialized || router == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return vazon_command_router_register(router, VAZON_COMMAND_TARGET_FAN,
                                         fan_command_handler, module);
}

const vazon_fan_snapshot_t *vazon_fan_module_get_snapshot(
    const vazon_fan_module_t *module)
{
    return module == NULL || !module->initialized ? NULL : &module->snapshot;
}
