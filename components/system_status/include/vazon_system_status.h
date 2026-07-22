#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "vazon_runtime_core.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VAZON_SYSTEM_STATUS_UNKNOWN = 0,
    VAZON_SYSTEM_STATUS_OK,
    VAZON_SYSTEM_STATUS_WARNING,
    VAZON_SYSTEM_STATUS_ERROR,
} vazon_system_status_value_t;

typedef enum {
    VAZON_SYSTEM_AFFECTED_NONE = 0,
    VAZON_SYSTEM_AFFECTED_CLIMATE,
    VAZON_SYSTEM_AFFECTED_POT,
    VAZON_SYSTEM_AFFECTED_LIGHT,
    VAZON_SYSTEM_AFFECTED_FAN,
    VAZON_SYSTEM_AFFECTED_HUMIDIFIER,
    VAZON_SYSTEM_AFFECTED_DOOR,
    VAZON_SYSTEM_AFFECTED_CONNECTION,
    VAZON_SYSTEM_AFFECTED_SYSTEM,
} vazon_system_affected_t;

typedef enum {
    VAZON_SYSTEM_MODULE_CLIMATE = 0,
    VAZON_SYSTEM_MODULE_POT_0,
    VAZON_SYSTEM_MODULE_POT_1,
    VAZON_SYSTEM_MODULE_LIGHT,
    VAZON_SYSTEM_MODULE_FAN,
    VAZON_SYSTEM_MODULE_HUMIDIFIER,
    VAZON_SYSTEM_MODULE_DOOR,
    VAZON_SYSTEM_MODULE_COUNT,
} vazon_system_module_slot_t;

typedef enum {
    VAZON_SYSTEM_MODULE_STATUS_UNKNOWN = 0,
    VAZON_SYSTEM_MODULE_STATUS_OK,
    VAZON_SYSTEM_MODULE_STATUS_WARNING,
    VAZON_SYSTEM_MODULE_STATUS_ERROR,
    VAZON_SYSTEM_MODULE_STATUS_INACTIVE,
} vazon_system_module_status_t;

typedef struct {
    uint32_t mqtt_warning_grace_sec;
} vazon_system_status_config_t;

typedef struct {
    vazon_system_status_value_t status;
    const char *status_reason;
    vazon_system_affected_t primary_fault;
    vazon_system_affected_t affected_system;
    bool sensor_missing;
    bool actuator_failed;
} vazon_system_status_snapshot_t;

typedef struct {
    vazon_system_module_status_t status;
    const char *reason;
    bool sensor_missing;
    bool actuator_failed;
    uint64_t status_since_ms;
} vazon_system_module_signal_t;

typedef struct {
    vazon_system_status_config_t config;
    vazon_system_status_snapshot_t snapshot;
    vazon_system_module_signal_t module_signals[VAZON_SYSTEM_MODULE_COUNT];
    SemaphoreHandle_t mutex;
    vazon_connection_state_t last_mqtt_state;
    uint64_t mqtt_state_since_ms;
    bool mqtt_state_initialized;
    bool initialized;
} vazon_system_status_t;

esp_err_t vazon_system_status_init(
    vazon_system_status_t *system_status,
    const vazon_system_status_config_t *config);
esp_err_t vazon_system_status_update_module(
    vazon_system_status_t *system_status,
    vazon_system_module_slot_t slot,
    vazon_system_module_status_t status,
    const char *reason,
    bool sensor_missing,
    bool actuator_failed,
    uint64_t now_ms);
esp_err_t vazon_system_status_evaluate(
    vazon_system_status_t *system_status,
    const vazon_global_context_t *global_context,
    bool door_open,
    uint64_t now_ms);
esp_err_t vazon_system_status_get_snapshot(
    vazon_system_status_t *system_status,
    vazon_system_status_snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif
