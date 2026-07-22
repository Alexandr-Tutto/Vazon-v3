#include "vazon_runtime_core.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

static bool maintenance_reason_valid(vazon_maintenance_reason_t reason)
{
    return reason == VAZON_MAINTENANCE_REASON_MANUAL ||
           reason == VAZON_MAINTENANCE_REASON_EXTERNAL;
}

static void update_maintenance(vazon_runtime_core_t *core)
{
    core->context.maintenance.manual_active = core->manual_maintenance_requested;
    core->context.maintenance.external_active = core->external_maintenance_requested;
    core->context.maintenance.door_active = core->door_service_required;

    if (core->manual_maintenance_requested) {
        core->context.maintenance.active = true;
        core->context.maintenance.reason = VAZON_MAINTENANCE_REASON_MANUAL;
    } else if (core->external_maintenance_requested) {
        core->context.maintenance.active = true;
        core->context.maintenance.reason = VAZON_MAINTENANCE_REASON_EXTERNAL;
    } else if (core->door_service_required) {
        core->context.maintenance.active = true;
        core->context.maintenance.reason = VAZON_MAINTENANCE_REASON_DOOR;
    } else {
        core->context.maintenance.active = false;
        core->context.maintenance.reason = VAZON_MAINTENANCE_REASON_UNKNOWN;
    }

    core->context.global_interlocks.maintenance_active =
        core->context.maintenance.active;
}

static bool connection_state_valid(vazon_connection_state_t state)
{
    return (unsigned int)state <= (unsigned int)VAZON_CONNECTION_CONNECTED;
}

static esp_err_t parse_time(const char *text, uint16_t *minute_of_day)
{
    if (text == NULL || minute_of_day == NULL || strlen(text) != 5 || text[2] != ':' ||
        !isdigit((unsigned char)text[0]) || !isdigit((unsigned char)text[1]) ||
        !isdigit((unsigned char)text[3]) || !isdigit((unsigned char)text[4])) {
        return ESP_ERR_INVALID_ARG;
    }

    const unsigned int hour = (unsigned int)(text[0] - '0') * 10U +
                              (unsigned int)(text[1] - '0');
    const unsigned int minute = (unsigned int)(text[3] - '0') * 10U +
                                (unsigned int)(text[4] - '0');
    if (hour > 23U || minute > 59U) {
        return ESP_ERR_INVALID_ARG;
    }

    *minute_of_day = (uint16_t)(hour * 60U + minute);
    return ESP_OK;
}

void vazon_runtime_core_init(vazon_runtime_core_t *core)
{
    if (core == NULL) {
        return;
    }

    memset(core, 0, sizeof(*core));
    core->context.maintenance.reason = VAZON_MAINTENANCE_REASON_UNKNOWN;
    core->context.day_window.active = VAZON_TRISTATE_UNKNOWN;
    core->context.day_window.schedule_enabled = true;
    memcpy(core->context.day_window.time_on, "08:00", VAZON_RUNTIME_TIME_TEXT_SIZE);
    memcpy(core->context.day_window.time_off, "21:00", VAZON_RUNTIME_TIME_TEXT_SIZE);
    core->context.connection.wifi_state = VAZON_CONNECTION_UNKNOWN;
    core->context.connection.mqtt_state = VAZON_CONNECTION_UNKNOWN;
}

const vazon_global_context_t *vazon_runtime_core_get_context(const vazon_runtime_core_t *core)
{
    return core == NULL ? NULL : &core->context;
}

esp_err_t vazon_runtime_core_set_maintenance(vazon_runtime_core_t *core,
                                             bool active,
                                             vazon_maintenance_reason_t reason)
{
    if (core == NULL || !maintenance_reason_valid(reason)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (reason == VAZON_MAINTENANCE_REASON_MANUAL) {
        core->manual_maintenance_requested = active;
    } else {
        core->external_maintenance_requested = active;
    }
    update_maintenance(core);
    return ESP_OK;
}

esp_err_t vazon_runtime_core_set_door_service_required(vazon_runtime_core_t *core,
                                                       bool required)
{
    if (core == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    core->door_service_required = required;
    update_maintenance(core);
    return ESP_OK;
}

esp_err_t vazon_runtime_core_set_day_window(vazon_runtime_core_t *core,
                                            const vazon_day_window_patch_t *patch)
{
    if (core == NULL || patch == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    vazon_day_window_t candidate = core->context.day_window;
    uint16_t ignored = 0;

    if (patch->has_schedule_enabled) {
        candidate.schedule_enabled = patch->schedule_enabled;
    }
    if (patch->time_on != NULL) {
        if (parse_time(patch->time_on, &ignored) != ESP_OK) {
            return ESP_ERR_INVALID_ARG;
        }
        memcpy(candidate.time_on, patch->time_on, VAZON_RUNTIME_TIME_TEXT_SIZE);
    }
    if (patch->time_off != NULL) {
        if (parse_time(patch->time_off, &ignored) != ESP_OK) {
            return ESP_ERR_INVALID_ARG;
        }
        memcpy(candidate.time_off, patch->time_off, VAZON_RUNTIME_TIME_TEXT_SIZE);
    }

    core->context.day_window = candidate;
    if (!candidate.schedule_enabled) {
        core->context.day_window.active = VAZON_TRISTATE_TRUE;
    } else {
        core->context.day_window.active = VAZON_TRISTATE_UNKNOWN;
    }
    return ESP_OK;
}

esp_err_t vazon_runtime_core_evaluate_day_window(vazon_runtime_core_t *core,
                                                 bool clock_valid,
                                                 uint8_t hour,
                                                 uint8_t minute)
{
    if (core == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!core->context.day_window.schedule_enabled) {
        core->context.day_window.active = VAZON_TRISTATE_TRUE;
        return ESP_OK;
    }
    if (!clock_valid) {
        core->context.day_window.active = VAZON_TRISTATE_UNKNOWN;
        return ESP_OK;
    }
    if (hour > 23U || minute > 59U) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t time_on = 0;
    uint16_t time_off = 0;
    esp_err_t result = parse_time(core->context.day_window.time_on, &time_on);
    if (result != ESP_OK) return result;
    result = parse_time(core->context.day_window.time_off, &time_off);
    if (result != ESP_OK) return result;

    const uint16_t now = (uint16_t)((unsigned int)hour * 60U + minute);
    bool active = true;
    if (time_on < time_off) {
        active = now >= time_on && now < time_off;
    } else if (time_on > time_off) {
        active = now >= time_on || now < time_off;
    }

    core->context.day_window.active = active ? VAZON_TRISTATE_TRUE : VAZON_TRISTATE_FALSE;
    return ESP_OK;
}

esp_err_t vazon_runtime_core_set_connection(vazon_runtime_core_t *core,
                                            vazon_connection_state_t wifi_state,
                                            vazon_connection_state_t mqtt_state)
{
    if (core == NULL || !connection_state_valid(wifi_state) ||
        !connection_state_valid(mqtt_state)) {
        return ESP_ERR_INVALID_ARG;
    }

    core->context.connection.wifi_state = wifi_state;
    core->context.connection.mqtt_state = mqtt_state;
    return ESP_OK;
}

esp_err_t vazon_runtime_core_update_interlocks(vazon_runtime_core_t *core,
                                               bool sensor_missing,
                                               bool actuator_failed)
{
    if (core == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    core->context.global_interlocks.maintenance_active = core->context.maintenance.active;
    core->context.global_interlocks.sensor_missing = sensor_missing;
    core->context.global_interlocks.actuator_failed = actuator_failed;
    return ESP_OK;
}

static vazon_command_result_t system_command_handler(const vazon_command_t *command,
                                                     void *context)
{
    vazon_runtime_core_t *core = (vazon_runtime_core_t *)context;
    if (command->args == NULL) {
        const vazon_command_result_t missing_args = {
            .status = VAZON_COMMAND_RESULT_REJECTED,
            .error = ESP_ERR_INVALID_ARG,
        };
        return missing_args;
    }

    esp_err_t update_result = ESP_ERR_NOT_SUPPORTED;
    if (strcmp(command->cmd, VAZON_COMMAND_SYSTEM_SET_DAY_WINDOW) == 0) {
        update_result = vazon_runtime_core_set_day_window(
            core, (const vazon_day_window_patch_t *)command->args);
    } else if (strcmp(command->cmd, VAZON_COMMAND_SYSTEM_SET_MAINTENANCE) == 0) {
        const vazon_maintenance_command_args_t *args = command->args;
        update_result = vazon_runtime_core_set_maintenance(
            core, args->active, VAZON_MAINTENANCE_REASON_MANUAL);
    }

    const vazon_command_result_t result = {
        .status = update_result == ESP_OK ? VAZON_COMMAND_RESULT_ACCEPTED
                                         : VAZON_COMMAND_RESULT_REJECTED,
        .error = update_result,
    };
    return result;
}

esp_err_t vazon_runtime_core_register_command_target(vazon_runtime_core_t *core,
                                                     vazon_command_router_t *router)
{
    if (core == NULL || router == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return vazon_command_router_register(router, VAZON_COMMAND_TARGET_SYSTEM,
                                         system_command_handler, core);
}
