#include "vazon_system_status.h"

#include <stddef.h>
#include <string.h>

static bool module_status_valid(vazon_system_module_status_t status)
{
    return status >= VAZON_SYSTEM_MODULE_STATUS_UNKNOWN &&
           status <= VAZON_SYSTEM_MODULE_STATUS_INACTIVE;
}

static vazon_system_affected_t affected_for_slot(vazon_system_module_slot_t slot)
{
    switch (slot) {
    case VAZON_SYSTEM_MODULE_CLIMATE:
        return VAZON_SYSTEM_AFFECTED_CLIMATE;
    case VAZON_SYSTEM_MODULE_POT_0:
    case VAZON_SYSTEM_MODULE_POT_1:
        return VAZON_SYSTEM_AFFECTED_POT;
    case VAZON_SYSTEM_MODULE_LIGHT:
        return VAZON_SYSTEM_AFFECTED_LIGHT;
    case VAZON_SYSTEM_MODULE_FAN:
        return VAZON_SYSTEM_AFFECTED_FAN;
    case VAZON_SYSTEM_MODULE_HUMIDIFIER:
        return VAZON_SYSTEM_AFFECTED_HUMIDIFIER;
    case VAZON_SYSTEM_MODULE_DOOR:
        return VAZON_SYSTEM_AFFECTED_DOOR;
    case VAZON_SYSTEM_MODULE_COUNT:
    default:
        return VAZON_SYSTEM_AFFECTED_SYSTEM;
    }
}

esp_err_t vazon_system_status_init(
    vazon_system_status_t *system_status,
    const vazon_system_status_config_t *config)
{
    if (system_status == NULL || config == NULL ||
        config->mqtt_warning_grace_sec > 60U) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(system_status, 0, sizeof(*system_status));
    system_status->mutex = xSemaphoreCreateMutex();
    if (system_status->mutex == NULL) return ESP_ERR_NO_MEM;
    system_status->config = *config;
    system_status->snapshot.status = VAZON_SYSTEM_STATUS_UNKNOWN;
    system_status->snapshot.status_reason = "startup_unknown";
    system_status->snapshot.primary_fault = VAZON_SYSTEM_AFFECTED_NONE;
    system_status->snapshot.affected_system = VAZON_SYSTEM_AFFECTED_NONE;
    system_status->last_mqtt_state = VAZON_CONNECTION_UNKNOWN;
    system_status->initialized = true;
    return ESP_OK;
}

esp_err_t vazon_system_status_update_module(
    vazon_system_status_t *system_status,
    vazon_system_module_slot_t slot,
    vazon_system_module_status_t status,
    const char *reason,
    bool sensor_missing,
    bool actuator_failed,
    uint64_t now_ms)
{
    if (system_status == NULL || !system_status->initialized ||
        slot >= VAZON_SYSTEM_MODULE_COUNT || !module_status_valid(status)) {
        return ESP_ERR_INVALID_ARG;
    }
    xSemaphoreTake(system_status->mutex, portMAX_DELAY);
    vazon_system_module_signal_t *signal = &system_status->module_signals[slot];
    if (signal->status != status) signal->status_since_ms = now_ms;
    signal->status = status;
    signal->reason = reason;
    signal->sensor_missing = sensor_missing;
    signal->actuator_failed = actuator_failed;
    xSemaphoreGive(system_status->mutex);
    return ESP_OK;
}

static int earliest_module_with_status(
    const vazon_system_status_t *system_status,
    vazon_system_module_status_t status)
{
    int selected = -1;
    uint64_t earliest = UINT64_MAX;
    for (int slot = 0; slot < (int)VAZON_SYSTEM_MODULE_COUNT; ++slot) {
        const vazon_system_module_signal_t *signal =
            &system_status->module_signals[slot];
        if (signal->status == status && signal->status_since_ms <= earliest) {
            selected = slot;
            earliest = signal->status_since_ms;
        }
    }
    return selected;
}

static void set_result(vazon_system_status_snapshot_t *snapshot,
                       vazon_system_status_value_t status,
                       const char *reason,
                       vazon_system_affected_t affected)
{
    snapshot->status = status;
    snapshot->status_reason = reason;
    snapshot->affected_system = affected;
    snapshot->primary_fault = status == VAZON_SYSTEM_STATUS_ERROR
                                  ? affected
                                  : VAZON_SYSTEM_AFFECTED_NONE;
}

esp_err_t vazon_system_status_evaluate(
    vazon_system_status_t *system_status,
    const vazon_global_context_t *global_context,
    bool door_open,
    uint64_t now_ms)
{
    if (system_status == NULL || !system_status->initialized ||
        global_context == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    xSemaphoreTake(system_status->mutex, portMAX_DELAY);

    if (!system_status->mqtt_state_initialized ||
        system_status->last_mqtt_state != global_context->connection.mqtt_state) {
        system_status->last_mqtt_state = global_context->connection.mqtt_state;
        system_status->mqtt_state_since_ms = now_ms;
        system_status->mqtt_state_initialized = true;
    }

    vazon_system_status_snapshot_t next = {
        .status = VAZON_SYSTEM_STATUS_OK,
        .status_reason = "none",
        .primary_fault = VAZON_SYSTEM_AFFECTED_NONE,
        .affected_system = VAZON_SYSTEM_AFFECTED_NONE,
    };
    bool any_unknown = false;
    for (int slot = 0; slot < (int)VAZON_SYSTEM_MODULE_COUNT; ++slot) {
        const vazon_system_module_signal_t *signal =
            &system_status->module_signals[slot];
        next.sensor_missing |= signal->sensor_missing;
        next.actuator_failed |= signal->actuator_failed;
        any_unknown |= signal->status == VAZON_SYSTEM_MODULE_STATUS_UNKNOWN;
    }

    const bool mqtt_reconnecting =
        global_context->connection.mqtt_state == VAZON_CONNECTION_CONNECTING ||
        global_context->connection.mqtt_state == VAZON_CONNECTION_DISCONNECTED;
    const uint64_t mqtt_grace_ms =
        (uint64_t)system_status->config.mqtt_warning_grace_sec * 1000ULL;
    const bool mqtt_offline = mqtt_reconnecting &&
        now_ms - system_status->mqtt_state_since_ms > mqtt_grace_ms;

    const int error_slot = earliest_module_with_status(
        system_status, VAZON_SYSTEM_MODULE_STATUS_ERROR);
    const int warning_slot = earliest_module_with_status(
        system_status, VAZON_SYSTEM_MODULE_STATUS_WARNING);

    if (mqtt_offline) {
        set_result(&next, VAZON_SYSTEM_STATUS_ERROR, "mqtt_offline",
                   VAZON_SYSTEM_AFFECTED_CONNECTION);
    } else if (error_slot >= 0) {
        const vazon_system_module_signal_t *signal =
            &system_status->module_signals[error_slot];
        set_result(&next, VAZON_SYSTEM_STATUS_ERROR,
                   signal->reason != NULL ? signal->reason : "module_error",
                   affected_for_slot((vazon_system_module_slot_t)error_slot));
    } else if (next.sensor_missing) {
        set_result(&next, VAZON_SYSTEM_STATUS_ERROR, "sensor_missing",
                   VAZON_SYSTEM_AFFECTED_SYSTEM);
    } else if (next.actuator_failed) {
        set_result(&next, VAZON_SYSTEM_STATUS_ERROR, "output_failed",
                   VAZON_SYSTEM_AFFECTED_SYSTEM);
    } else if (door_open) {
        set_result(&next, VAZON_SYSTEM_STATUS_WARNING, "door_open",
                   VAZON_SYSTEM_AFFECTED_DOOR);
    } else if (global_context->maintenance.active) {
        set_result(&next, VAZON_SYSTEM_STATUS_WARNING, "maintenance_active",
                   VAZON_SYSTEM_AFFECTED_SYSTEM);
    } else if (warning_slot >= 0) {
        const vazon_system_module_signal_t *signal =
            &system_status->module_signals[warning_slot];
        set_result(&next, VAZON_SYSTEM_STATUS_WARNING,
                   signal->reason != NULL ? signal->reason : "module_warning",
                   affected_for_slot((vazon_system_module_slot_t)warning_slot));
    } else if (global_context->connection.wifi_state ==
                   VAZON_CONNECTION_DISCONNECTED ||
               global_context->connection.wifi_state ==
                   VAZON_CONNECTION_CONNECTING) {
        set_result(&next, VAZON_SYSTEM_STATUS_WARNING, "wifi_offline",
                   VAZON_SYSTEM_AFFECTED_CONNECTION);
    } else if (mqtt_reconnecting) {
        set_result(&next, VAZON_SYSTEM_STATUS_WARNING, "mqtt_reconnecting",
                   VAZON_SYSTEM_AFFECTED_CONNECTION);
    } else if (any_unknown ||
               global_context->connection.wifi_state == VAZON_CONNECTION_UNKNOWN ||
               global_context->connection.mqtt_state == VAZON_CONNECTION_UNKNOWN) {
        set_result(&next, VAZON_SYSTEM_STATUS_UNKNOWN, "startup_unknown",
                   VAZON_SYSTEM_AFFECTED_NONE);
    }

    system_status->snapshot = next;
    xSemaphoreGive(system_status->mutex);
    return ESP_OK;
}

esp_err_t vazon_system_status_get_snapshot(
    vazon_system_status_t *system_status,
    vazon_system_status_snapshot_t *snapshot)
{
    if (system_status == NULL || !system_status->initialized || snapshot == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    xSemaphoreTake(system_status->mutex, portMAX_DELAY);
    *snapshot = system_status->snapshot;
    xSemaphoreGive(system_status->mutex);
    return ESP_OK;
}
